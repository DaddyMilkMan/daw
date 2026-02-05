/**
 * @file EngineExport.cpp
 * @brief Offline project export and rendering implementation
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include "../engine/AudioRenderer.h"
#include "../engine/AudioExporter.h"
#include "../engine/Track.h"
#include "../engine/Clip.h"
#include "../engine/AuxBus.h"
#include "../engine/PluginHost.h"
#include "../engine/RoutingGraph.h"
#include "../engine/TempoMap.h"
#include "../Source/dsp/Dither.h"

namespace zenith {

//==============================================================================
// Offline Export Implementation
//==============================================================================

bool Engine::exportProjectToWav(const juce::File &outputFile, double sampleRate,
                                int bitDepth, double durationInSeconds) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("Engine: Starting WAV export to " + outputFile.getFullPathName());

  if (sampleRate <= 0.0)
    return false;
  if (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)
    return false;

  // Auto-detect duration
  if (durationInSeconds <= 0.0) {
    double maxEndTime = 0.0;
    for (const auto &track : tracks_) {
      if (track) {
        for (int i = 0; i < track->getNumClips(); ++i) {
          auto *clip = track->getClip(i);
          if (clip) {
            double rate = currentSampleRate.load() > 0
                              ? currentSampleRate.load()
                              : 44100.0;
            double clipEnd = static_cast<double>(clip->getStartPosition() +
                                                 clip->getLength()) /
                             rate;
            maxEndTime = std::max<double>(maxEndTime, clipEnd);
          }
        }
      }
    }
    durationInSeconds = std::max(10.0, maxEndTime + 1.0);
  }

  const juce::int64 totalSamples =
      static_cast<juce::int64>(durationInSeconds * sampleRate);
  constexpr int offlineBlockSize = 4096;
  const int numChannels = 2;

  // Prepare for export - message thread safe here
  prepareTracks(offlineBlockSize,
                sampleRate); // Prepare tracks for new rate/size
  if (audioRenderer_) {
    // AudioRenderer is stateless, prepare context instead
    renderContext_.prepare(sampleRate, offlineBlockSize, tracks_.size(),
                            auxBuses_.size());
  }

  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::OutputStream> fileStream = std::make_unique<juce::FileOutputStream>(outputFile);
  if (fileStream == nullptr || static_cast<juce::FileOutputStream*>(fileStream.get())->failedToOpen()) return false;

  auto writerOptions = juce::AudioFormatWriterOptions()
      .withSampleRate(sampleRate)
      .withNumChannels(numChannels)
      .withBitsPerSample(bitDepth);
      
  std::unique_ptr<juce::AudioFormatWriter> writer = wavFormat.createWriterFor(fileStream, writerOptions);

  if (!writer)
    return false;

  juce::AudioBuffer<float> renderBuffer(numChannels, offlineBlockSize);
  juce::int64 samplesRendered = 0;

  while (samplesRendered < totalSamples) {
    const int samplesToRender =
        static_cast<int>(std::min(static_cast<juce::int64>(offlineBlockSize),
                                    totalSamples - samplesRendered));

    // Render using AudioRenderer
    if (audioRenderer_) {
      // Note: Empty MIDI buffer is correct for offline export
      // Clip MIDI data is processed internally by MIDITrack/InstrumentTrack
      // This buffer is only for live/incoming MIDI (e.g., from controllers)
      juce::MidiBuffer dummyMidi;
      
      // Build raw pointer vectors for AudioRenderer
      std::vector<Track *> trackPtrs;
      trackPtrs.reserve(tracks_.size());
      for (const auto &t : tracks_)
        if (t)
          trackPtrs.push_back(t.get());

      std::vector<AuxBus *> auxPtrs;
      auxPtrs.reserve(auxBuses_.size());
      for (const auto &a : auxBuses_)
        if (a)
          auxPtrs.push_back(a.get());

      // Create temp shared_ptr vector for AudioRenderer compatibility
      std::vector<std::shared_ptr<juce::AudioPluginInstance>> tmpPlugins;
      for (const auto &p : masterPlugins_)
        tmpPlugins.push_back(p);

      audioRenderer_->renderAudioGraph(
          renderContext_, // Pass context
          renderBuffer, samplesToRender, samplesRendered, trackPtrs, auxPtrs,
          routingGraph_, masterLimiter_, tmpPlugins, tempoMap_.get(),
          &dummyMidi, nullptr, 0);
    } else {
      renderBuffer.clear();
    }

    if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesToRender)) {
      break; // Error
    }

    samplesRendered += samplesToRender;
  }

  writer.reset();

  // Restore state
  double originalRate = currentSampleRate.load();
  int originalSize = currentBufferSize.load();

  prepareTracks(originalSize, originalRate);
  if (audioRenderer_) {
    renderContext_.prepare(originalRate, originalSize, tracks_.size(),
                            auxBuses_.size());
  }

  return true;
}

bool Engine::exportProjectToWavSync(const juce::File &outputFile, double sampleRate,
                                    int bitDepth, double duration, double startTime) {
    if (!audioExporter_)
        return false;

    zenith::ExportOptions options; // Explicitly use AudioExporter's options struct
    options.outputFile = outputFile;
    options.sampleRate = sampleRate;
    options.bitDepth = bitDepth;
    options.duration = duration;
    options.startTime = startTime;
    options.format = zenith::ExportFormat::WAV; // Explicitly use zenith::ExportFormat
    options.enableDither = true;
    options.normalize = false;

    return audioExporter_->exportProject(options);
}

bool Engine::exportProject(const ExportOptions &options) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
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

  // Use auto-detect duration if not specified
  double duration =
      options.duration > 0 ? options.duration : autoDetectProjectDuration();
  DBG("Engine: Export duration: " + juce::String(duration, 2) + " seconds");
  juce::int64 totalSamples =
      static_cast<juce::int64>(options.sampleRate * duration);

  const int blockSize = 4096;
  juce::AudioBuffer<float> renderBuffer(2, blockSize);
  
  // Prepare Engine for rendering
  if (audioRenderer_) {
    renderContext_.prepare(options.sampleRate, blockSize, tracks_.size(),
                            auxBuses_.size());
  }

  //==============================================================================
  // NORMALIZATION PATH (2-PASS)
  //==============================================================================
  if (options.normalize) {
      DBG("Engine: Exporting with Normalization (2-Pass)...");

      // Pass 1: Render to Temp File (32-bit Float)
      juce::File tempFile = options.outputFile.getSiblingFile("temp_render_" + juce::Uuid().toString() + ".wav");
      
      juce::WavAudioFormat tempFormat;
      // Always write temp file as 32-bit float to preserve headroom
      std::unique_ptr<juce::OutputStream> tempStream(tempFile.createOutputStream());
      std::unique_ptr<juce::AudioFormatWriter> tempWriter(tempFormat.createWriterFor(
          tempStream, 
          juce::AudioFormatWriterOptions()
            .withSampleRate(options.sampleRate)
            .withNumChannels(2)
            .withBitsPerSample(32)));
          
      if (!tempWriter) {
         DBG("Engine: Failed to create temp file for normalization pass 1");
         return false;
      }
      
      float maxPeak = 0.0f;
      juce::int64 samplesRendered = 0;
      
      while (samplesRendered < totalSamples) {
          int numSamples = (int)std::min((juce::int64)blockSize, totalSamples - samplesRendered);
          
          // Render graph
          renderOfflineBlock(renderBuffer, numSamples, samplesRendered);
          
          // Analysis: Update Peak
          float peak = renderBuffer.getMagnitude(0, numSamples);
          maxPeak = std::max(maxPeak, peak);
          
          if (!tempWriter->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples)) {
             tempFile.deleteFile();
             return false;
          }
          samplesRendered += numSamples;
          
           if (options.progressCallback) {
              options.progressCallback(0.5f * (float)samplesRendered / totalSamples, "Pass 1: Analysis...");
          }
      }
      tempWriter.reset(); // Flush and close temp file
      
      // Calculate Gain
      float targetDb = -0.1f; // Standard normalization target
      float targetLinear = juce::Decibels::decibelsToGain(targetDb);
      float gain = (maxPeak > 0.00001f) ? (targetLinear / maxPeak) : 1.0f;
      DBG("Engine: Normalize Analysis - Max Peak: " + juce::String(juce::Decibels::gainToDecibels(maxPeak)) + " dB");
      DBG("Engine: Applying Gain: " + juce::String(juce::Decibels::gainToDecibels(gain)) + " dB");

      // Pass 2: Transfer to Final Format
      std::unique_ptr<juce::AudioFormatReader> reader(tempFormat.createReaderFor(tempFile.createInputStream().release(), true));
      if (!reader) { 
          tempFile.deleteFile(); 
          return false; 
      }
      
      std::unique_ptr<juce::OutputStream> outStream(options.outputFile.createOutputStream());
      std::unique_ptr<juce::AudioFormatWriter> writer = format->createWriterFor(
          outStream, 
          juce::AudioFormatWriterOptions()
            .withSampleRate(options.sampleRate)
            .withNumChannels(2)
            .withBitsPerSample(options.bitDepth));
          
      if (!writer) { 
          tempFile.deleteFile(); 
          return false; 
      }
      
      if (options.enableDither && options.bitDepth < 32) dither.prepare(2);
      
      samplesRendered = 0;
      juce::int64 readerPos = 0;
      
      while (readerPos < totalSamples) {
          int numSamples = (int)std::min((juce::int64)blockSize, totalSamples - readerPos);
          reader->read(&renderBuffer, 0, numSamples, readerPos, true, true);
          
          // Apply Normalization Gain
          renderBuffer.applyGain(gain);
          
          // Dither (Final Stage)
          if (options.enableDither && options.bitDepth < 32) {
              dither.process(renderBuffer, options.bitDepth);
          }
          
          if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples)) break;
          
          readerPos += numSamples;
           if (options.progressCallback) {
              options.progressCallback(0.5f + 0.5f * (float)readerPos / totalSamples, "Pass 2: Encoding...");
          }
      }
      
      writer.reset();
      tempFile.deleteFile();
      
      if (options.progressCallback) {
        options.progressCallback(1.0f, "Export complete!");
      }
      return true;
  }

  //==============================================================================
  // DIRECT RENDER PATH (SINGLE PASS)
  //==============================================================================
  
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

  if (options.enableDither)
    dither.prepare(2);
    
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples) {
    int numSamples =
        (int)std::min((juce::int64)blockSize, totalSamples - samplesWritten);

    // Use Engine's wrapper which handles graph rendering
    renderOfflineBlock(renderBuffer, numSamples, samplesWritten);

    // Apply Dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(renderBuffer, options.bitDepth);
    }

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

// function removed (moved to Engine.cpp with updated signature)

//==============================================================================
// Offline Block Rendering
//==============================================================================

void Engine::renderOfflineBlock(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 position) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  // Clear the buffer first
  buffer.clear();

  if (!audioRenderer_) {
    return;
  }

  // Create a span for tracks
  std::vector<Track*> trackPtrs;
  trackPtrs.reserve(tracks_.size());
  for (auto& track : tracks_) {
    if (track) {
      trackPtrs.push_back(track.get());
    }
  }

  // Create a span for aux buses
  std::vector<AuxBus*> auxBusPtrs;
  auxBusPtrs.reserve(auxBuses_.size());
  for (auto& bus : auxBuses_) {
    if (bus) {
      auxBusPtrs.push_back(bus.get());
    }
  }

  // Prepare the render context if needed
  if (renderContext_.trackBuffers.empty()) {
    double rate = currentSampleRate.load();
    if (rate <= 0) rate = 44100.0;
    renderContext_.prepare(rate, numSamples, tracks_.size(), auxBuses_.size());
  }

  // Render the audio graph
  juce::MidiBuffer emptyMidi;
  audioRenderer_->renderAudioGraph(
      renderContext_,
      buffer,
      numSamples,
      position,
      std::span<Track* const>(trackPtrs.data(), trackPtrs.size()),
      std::span<AuxBus* const>(auxBusPtrs.data(), auxBusPtrs.size()),
      routingGraph_,
      masterLimiter_,
      std::span<const std::shared_ptr<juce::AudioPluginInstance>>(),
      tempoMap_.get(),
      &emptyMidi,
      nullptr,
      0
  );
}

} // namespace zenith
