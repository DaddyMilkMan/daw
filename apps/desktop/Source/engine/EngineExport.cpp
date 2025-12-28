/**
 * @file EngineExport.cpp
 * @brief Offline project export and rendering implementation
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include "ExportJob.h"
#include "../engine/AudioRenderer.h"
#include "../engine/Track.h"
#include "../engine/Clip.h"
#include "../engine/AuxBus.h"
#include "../engine/PluginHost.h"
#include "../engine/RoutingGraph.h"
#include "../engine/TempoMap.h"
#include "../Source/dsp/Dither.h"

namespace zenith {

//==============================================================================
// Export Job Class (Async)
//==============================================================================

//==============================================================================
// Asynchronous Export Implementation
//==============================================================================

void Engine::cancelExport() {
  auto* job = currentExportJob_.load();
  if (job) {
    job->cancel();
  }
}

bool Engine::exportProjectToWav(const juce::File& outputFile, 
                                double sampleRate,
                                int bitDepth, 
                                double durationInSeconds,
                                double startTimeSeconds,
                                ExportProgressCallback progressCallback) {
  if (currentExportJob_.load() != nullptr) {
      DBG("Engine: Export already in progress");
      return false; 
  }

  DBG("Engine: Starting Async WAV export to " + outputFile.getFullPathName());

  if (sampleRate <= 0.0) return false;
  if (bitDepth != 16 && bitDepth != 24 && bitDepth != 32) return false;

  // Auto-detect duration
  if (durationInSeconds <= 0.0) {
    double projectDuration = autoDetectProjectDuration();
    durationInSeconds = std::max(0.0, projectDuration - startTimeSeconds);
  }
  
  // Create Options
  ExportOptions options;
  options.outputFile = outputFile;
  options.sampleRate = sampleRate;
  options.bitDepth = bitDepth;
  options.duration = durationInSeconds;
  options.startTime = startTimeSeconds;
  options.format = ExportFormat::WAV; // Default/Legacy is WAV
  options.progressCallback = progressCallback;

  // Adapt completion callback to legacy progress callback format
  auto completionCallback = [progressCallback](juce::Result result) {
      if (progressCallback) {
          if (result.wasOk()) {
              progressCallback(1.0f, "Export complete!");
          } else {
              progressCallback(0.0f, "Error: " + result.getErrorMessage());
          }
      }
  };

  // Launch Background Job
  auto job = std::make_unique<ExportJob>(*this, options, progressCallback, completionCallback);
  
  // Store pointer for cancellation (job will clear it when done)
  ExportJob* jobPtr = job.get();
  ExportJob* expected = nullptr;
  if (currentExportJob_.compare_exchange_strong(expected, jobPtr)) {
      getThreadPool().addJob(job.release(), true); // Pool takes ownership
      return true;
  } else {
      return false; // Race condition
  }
}

bool Engine::exportProject(const ExportOptions &options) {
  DBG("Engine: Starting Advanced Export...");
  registerFormats();

  if (options.sampleRate <= 0)
    return false;

  if (options.bitDepth == 8 && options.format != ExportFormat::WAV) {
    DBG("Engine: 8-bit export is only supported for WAV format.");
    return false;
  }

  juce::AudioFormat *format = nullptr;
  switch (options.format) {
  case ExportFormat::WAV:
    format = formatManager.findFormatForFileExtension("wav");
    break;
  case ExportFormat::FLAC:
    format = formatManager.findFormatForFileExtension("flac");
    break;
  case ExportFormat::OGG:
    format = formatManager.findFormatForFileExtension("ogg");
    break;
  case ExportFormat::AIFF:
    format = formatManager.findFormatForFileExtension("aiff");
    break;
  }

  if (!format)
    return false;

  // Create file stream
  std::unique_ptr<juce::OutputStream> fileStream = std::make_unique<juce::FileOutputStream>(options.outputFile);
  if (fileStream == nullptr || static_cast<juce::FileOutputStream*>(fileStream.get())->failedToOpen())
    return false;

  auto writerOptions = juce::AudioFormatWriterOptions()
                           .withSampleRate(options.sampleRate)
                           .withNumChannels(2)
                           .withBitsPerSample(options.bitDepth);

  std::unique_ptr<juce::AudioFormatWriter> writer = format->createWriterFor(fileStream, writerOptions);

  if (!writer)
    return false;

  const int blockSize = 4096;
  juce::AudioBuffer<float> renderBuffer(2, blockSize);
  if (audioRenderer_) {
    renderContext_.prepare(options.sampleRate, blockSize, tracks_.size(),
                            auxBuses_.size());
  }

  if (options.enableDither)
    dither.prepare(2);

  // Use auto-detect duration if not specified
  double duration =
      options.duration > 0 ? options.duration : autoDetectProjectDuration();
  DBG("Engine: Export duration: " + juce::String(duration, 2) + " seconds");
  juce::int64 totalSamples =
      static_cast<juce::int64>(options.sampleRate * duration);
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples) {
    int numSamples =
        (int)juce::jmin((juce::int64)blockSize, totalSamples - samplesWritten);

    // Use Engine's wrapper which handles graph rendering
    renderOfflineBlock(renderBuffer, numSamples, samplesWritten);

    // Apply Dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(renderBuffer, options.bitDepth);
    }

    // Normalization (2-Pass: Find Peak -> Apply Gain)
    // Normalization (Offline Render Refactor required for full track)
    // NOTE: Per-block normalization is WRONG for full track export.
    // Correct implementation requires render-to-temp-file -> scan -> write-to-final
    // This is disabled pending a full offline-render refactor.
    // See: applyNormalization() for when this gets properly implemented.
    (void)options.normalize; // Suppress unused warning


    if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples)) {
      return false;
    }

    samplesWritten += numSamples;
    
    // Report progress
    if (options.progressCallback) {
      float progress = static_cast<float>(samplesWritten) / static_cast<float>(totalSamples);
      options.progressCallback(progress, "Exporting audio...");
    }
  }

  if (options.progressCallback) {
    options.progressCallback(1.0f, "Export complete!");
  }

  return true;
}

void Engine::applyNormalization(juce::AudioBuffer<float> &buffer, float maxPeak,
                                float targetDb) {
  if (maxPeak <= 0.00001f) return;
  
  float targetLinear = juce::Decibels::decibelsToGain(targetDb);
  float gain = targetLinear / maxPeak;
  buffer.applyGain(gain);
}

double Engine::autoDetectProjectDuration() const {
  double maxDuration = 0.0;
  const double sampleRate = currentSampleRate.load();

  if (sampleRate <= 0.0)
    return 10.0; // Fallback

  // Scan all tracks for the latest clip end position
  for (const std::shared_ptr<zenith::Track> &track : tracks_) {
    if (!track)
      continue;

    for (int i = 0; i < track->getNumClips(); ++i) {
      const auto *clip = track->getClip(i);
      if (clip != nullptr) {
        // Get clip end position in samples and convert to seconds
        juce::int64 clipEnd = clip->getStartPosition() + clip->getLength();
        double endSeconds = static_cast<double>(clipEnd) / sampleRate;

        if (endSeconds > maxDuration) {
          maxDuration = endSeconds;
        }
      }
    }
  }

  // Add a small tail (2 seconds) for reverb/delay tails
  if (maxDuration > 0.0) {
    maxDuration += 2.0;
  } else {
    maxDuration = 10.0; // Default if no clips
  }

  return maxDuration;
}

bool Engine::exportProjectToWavSync(const juce::File &outputFile, double sampleRate,
                                    int bitDepth, double durationInSeconds,
                                    double startTimeSeconds) {
    if (sampleRate <= 0.0) return false;
    
    if (durationInSeconds <= 0.0) {
        durationInSeconds = autoDetectProjectDuration() - startTimeSeconds;
    }
    
    const juce::int64 startSample = static_cast<juce::int64>(startTimeSeconds * sampleRate);
    const juce::int64 totalSamples = static_cast<juce::int64>(durationInSeconds * sampleRate);
    constexpr int offlineBlockSize = 4096;
    const int numChannels = 2;

    // We must use a separate render context to avoid clashing with live playback
    AudioRenderContext aiContext;
    aiContext.prepare(sampleRate, offlineBlockSize, tracks_.size(), auxBuses_.size());

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::OutputStream> fileStream = outputFile.createOutputStream();
    if (!fileStream || static_cast<juce::FileOutputStream*>(fileStream.get())->failedToOpen()) return false;

    auto writerOptions = juce::AudioFormatWriterOptions()
                           .withSampleRate(sampleRate)
                           .withNumChannels(numChannels)
                           .withBitsPerSample(bitDepth);

    std::unique_ptr<juce::AudioFormatWriter> writer = wavFormat.createWriterFor(fileStream, writerOptions);
    if (!writer) return false;

    juce::AudioBuffer<float> renderBuffer(numChannels, offlineBlockSize);
    juce::int64 samplesRendered = 0;

    // Snapshot state for thread safety
    auto tracksSnapshot = getTracksSnapshot();
    std::vector<Track*> trackPtrs;
    for (const auto& t : tracksSnapshot) if (t) trackPtrs.push_back(t.get());
    
    std::vector<AuxBus*> auxPtrs;
    for (const auto& a : auxBuses_) if (a) auxPtrs.push_back(a.get());

    while (samplesRendered < totalSamples) {
        const int samplesToRender = static_cast<int>(juce::jmin(static_cast<juce::int64>(offlineBlockSize), totalSamples - samplesRendered));
        const juce::int64 currentPosition = startSample + samplesRendered;

        if (audioRenderer_) {
            juce::MidiBuffer dummyMidi;
            std::vector<std::shared_ptr<juce::AudioPluginInstance>> tmpPlugins;
            for (const auto& p : masterPlugins_) tmpPlugins.push_back(p);

            audioRenderer_->renderAudioGraph(aiContext, renderBuffer, samplesToRender, currentPosition, 
                                           trackPtrs, auxPtrs, routingGraph_, masterLimiter_, tmpPlugins, 
                                           tempoMap_.get(), &dummyMidi, nullptr, 0);
        } else {
            renderBuffer.clear();
        }

        if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender)) break;
        samplesRendered += samplesToRender;
    }

    return true;
}

void Engine::renderOfflineBlock(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 position) {
  if (audioRenderer_) {
      // Get master plugins snapshot
      auto *masterSnapshot = activeMasterPluginsSnapshot_.load();
      std::span<const std::shared_ptr<juce::AudioPluginInstance>> masterPlugins;
      if (masterSnapshot) masterPlugins = masterSnapshot->plugins;

      // Convert Tracks to span (pointers from vector)
      auto tracksSnapshot = getTracksSnapshot();
      std::vector<Track*> trackPtrs; 
      trackPtrs.reserve(tracksSnapshot.size());
      for(auto& t : tracksSnapshot) trackPtrs.push_back(t.get());

      std::vector<AuxBus*> auxPtrs;
      auxPtrs.reserve(auxBuses_.size());
      for(auto& b : auxBuses_) auxPtrs.push_back(b.get());

      // Use renderContext_ (internal member)
      audioRenderer_->renderAudioGraph(
          renderContext_,
          buffer, numSamples, position,
          trackPtrs, auxPtrs, routingGraph_,
          masterLimiter_, masterPlugins,
          tempoMap_.get(), nullptr, nullptr, 0);
  } else {
    buffer.clear();
  }
}

} // namespace zenith