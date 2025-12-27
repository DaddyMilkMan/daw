/*
  ==============================================================================

    AudioExporter.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

    Professional audio export with:
    - Two-pass normalization
    - TPDF dithering with noise shaping
    - Asynchronous multi-file stem export

  ==============================================================================
*/

#include "AudioExporter.h"
#include "../dsp/Dither.h"
#include "AudioRenderer.h"
#include "Engine.h"
#include "Track.h"
#include "TempoMap.h"

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

AudioExporter::AudioExporter(Engine &engine) : engine_(engine) {
  registerFormats();
}

AudioExporter::~AudioExporter() {
  cancelExport();
}

void AudioExporter::registerFormats() {
  formatManager_.registerBasicFormats();
  formatManager_.registerFormat(new juce::FlacAudioFormat(), false);
  formatManager_.registerFormat(new juce::OggVorbisAudioFormat(), false);
}

juce::AudioFormat* AudioExporter::getFormatForType(ExportFormat format) {
  switch (format) {
  case ExportFormat::WAV:  return formatManager_.findFormatForFileExtension("wav");
  case ExportFormat::FLAC: return formatManager_.findFormatForFileExtension("flac");
  case ExportFormat::OGG:  return formatManager_.findFormatForFileExtension("ogg");
  case ExportFormat::AIFF: return formatManager_.findFormatForFileExtension("aiff");
  }
  return nullptr;
}

//==============================================================================
// Main Export Entry Point
//==============================================================================

bool AudioExporter::exportProject(const ExportOptions &options) {
  DBG("AudioExporter: Starting export...");

  if (options.sampleRate <= 0)
    return false;

  isExporting_.store(true);
  shouldCancel_.store(false);

  // Safety: Suspend engine to preventing race conditions with VSTs
  engine_.suspendProcessing(true);

  // RAII helper to ensure resume
  struct ScopedResume {
      Engine& e;
      ~ScopedResume() { e.suspendProcessing(false); }
  } resumer{engine_};

  bool result = false;

  // Stem export path
  if (options.exportStems) {
    if (options.exportStemsAsync) {
      result = exportStemsAsync(options);
    } else {
      result = exportStems(options);
    }
    isExporting_.store(false);
    return result;
  }

  // Setup dither with requested type
  zenith::dsp::Dither dither;
  if (options.enableDither) {
    dither.prepare(2);
    dither.setType(options.ditherType);
  }

  // Determine duration
  double duration = options.duration;
  if (duration <= 0.0) {
    duration = engine_.autoDetectProjectDuration();
  }
  DBG("AudioExporter: Duration: " + juce::String(duration, 2) + "s");

  // Two-pass normalization path
  if (options.normalize) {
    DBG("AudioExporter: 2-Pass Normalization Enabled");

    if (options.progressCallback) {
      options.progressCallback(0.0f, "Pass 1: Analyzing peaks...");
    }

    auto tempFile = options.outputFile.getParentDirectory().getChildFile(
        "zenith_export_temp_" + juce::String(juce::Time::currentTimeMillis()) + ".wav");

    float maxPeak = 0.0f;
    if (!renderToTempFile(tempFile, duration, options.sampleRate, options.startTime, maxPeak)) {
      tempFile.deleteFile();
      isExporting_.store(false);
      return false;
    }

    if (shouldCancel_.load()) {
      tempFile.deleteFile();
      isExporting_.store(false);
      return false;
    }

    DBG("AudioExporter: Peak found: " +
        juce::String(juce::Decibels::gainToDecibels(maxPeak)) + " dB");

    if (options.progressCallback) {
      options.progressCallback(0.5f, "Pass 2: Writing normalized audio...");
    }

    result = writeFinalFile(tempFile, options, maxPeak);
    tempFile.deleteFile();
    isExporting_.store(false);
    return result;
  }

  // Single-pass direct export
  DBG("AudioExporter: Direct Export (No Normalization)");

  juce::AudioFormat *format = getFormatForType(options.format);
  if (!format) {
    isExporting_.store(false);
    return false;
  }

  juce::File outputFile = options.outputFile;
  outputFile.deleteFile();

  std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());
  if (!fileStream) {
    isExporting_.store(false);
    return false;
  }

  auto writerOptions = juce::AudioFormatWriterOptions()
      .withSampleRate(options.sampleRate)
      .withNumChannels(2)
      .withBitsPerSample(options.bitDepth);

  std::unique_ptr<juce::OutputStream> streamPtr(std::move(fileStream));
  std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(streamPtr, writerOptions));

  if (!writer) {
    isExporting_.store(false);
    return false;
  }

  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);

  // Creates context for Direct Export
  AudioRenderContext offlineContext;
  offlineContext.prepare(options.sampleRate, blockSize, engine_.getNumTracks(),
                         engine_.getNumAuxBuses());

  juce::int64 startSample = static_cast<juce::int64>(options.startTime * options.sampleRate);
  juce::int64 totalSamples = static_cast<juce::int64>(options.sampleRate * duration);
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(blockSize), 
                                                  totalSamples - samplesWritten));

    engine_.renderOfflineBlock(offlineContext, buffer, numSamples, startSample + samplesWritten);

    if (options.enableDither && options.bitDepth < 32) {
      dither.process(buffer, options.bitDepth);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples)) {
      isExporting_.store(false);
      return false;
    }

    samplesWritten += numSamples;

    if (options.progressCallback) {
      float progress = static_cast<float>(samplesWritten) / static_cast<float>(totalSamples);
      options.progressCallback(progress, "Exporting audio...");
    }
  }

  if (shouldCancel_.load()) {
    outputFile.deleteFile();
    isExporting_.store(false);
    return false;
  }

  if (options.progressCallback) {
    options.progressCallback(1.0f, "Export complete!");
  }

  isExporting_.store(false);
  return true;
}

//==============================================================================
// Cancellation
//==============================================================================

void AudioExporter::cancelExport() {
  shouldCancel_.store(true);
}

//==============================================================================
// Progress Tracking
//==============================================================================

std::vector<StemExportProgress> AudioExporter::getStemProgress() const {
  juce::ScopedLock lock(stemProgressLock_);
  return stemProgress_;
}

void AudioExporter::updateStemProgress(int trackIndex, float progress) {
  juce::ScopedLock lock(stemProgressLock_);
  for (auto& stem : stemProgress_) {
    if (stem.trackIndex == trackIndex) {
      stem.progress = progress;
      break;
    }
  }
}

void AudioExporter::markStemComplete(int trackIndex, bool success) {
  juce::ScopedLock lock(stemProgressLock_);
  for (auto& stem : stemProgress_) {
    if (stem.trackIndex == trackIndex) {
      stem.completed = true;
      stem.success = success;
      stem.progress = 1.0f;
      completedStemCount_.fetch_add(1);
      break;
    }
  }
}

void AudioExporter::reportAggregateProgress(const ExportOptions& options) {
  if (!options.progressCallback) return;
  
  float totalProgress = 0.0f;
  int count = 0;
  
  {
    juce::ScopedLock lock(stemProgressLock_);
    for (const auto& stem : stemProgress_) {
      totalProgress += stem.progress;
      count++;
    }
  }
  
  if (count > 0) {
    float avgProgress = totalProgress / static_cast<float>(count);
    int completed = completedStemCount_.load();
    options.progressCallback(avgProgress, 
        "Exporting stems (" + juce::String(completed) + "/" + juce::String(count) + ")...");
  }
}

//==============================================================================
// Two-Pass Normalization Helpers
//==============================================================================

bool AudioExporter::analyzeProjectPeak(double duration, double sampleRate,
                                       double startTime, float &outMaxPeak) {
  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);
  AudioRenderContext offlineContext;
  offlineContext.prepare(sampleRate, blockSize, engine_.getNumTracks(), engine_.getNumAuxBuses());

  juce::int64 startSample = static_cast<juce::int64>(startTime * sampleRate);
  juce::int64 totalSamples = static_cast<juce::int64>(sampleRate * duration);
  juce::int64 samplesProcessed = 0;
  outMaxPeak = 0.0f;

  while (samplesProcessed < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(blockSize),
                                                  totalSamples - samplesProcessed));
    engine_.renderOfflineBlock(offlineContext, buffer, numSamples, startSample + samplesProcessed);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      float channelPeak = buffer.getMagnitude(ch, 0, numSamples);
      if (channelPeak > outMaxPeak) outMaxPeak = channelPeak;
    }
    samplesProcessed += numSamples;
  }
  return !shouldCancel_.load();
}

bool AudioExporter::renderToTempFile(const juce::File &tempFile,
                                     double duration, double sampleRate,
                                     double startTime, float &outMaxPeak) {
  tempFile.deleteFile();

  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::FileOutputStream> stream(tempFile.createOutputStream());
  if (!stream)
    return false;

  auto writerOptions = juce::AudioFormatWriterOptions()
      .withSampleRate(sampleRate)
      .withNumChannels(2)
      .withBitsPerSample(32);

  std::unique_ptr<juce::OutputStream> streamPtr(std::move(stream));
  std::unique_ptr<juce::AudioFormatWriter> writer(
      wavFormat.createWriterFor(streamPtr, writerOptions));

  if (!writer)
    return false;

  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);

  // Create local render context for this thread (Thread-Safe!)
  AudioRenderContext offlineContext;
  offlineContext.prepare(sampleRate, blockSize, engine_.getNumTracks(), engine_.getNumAuxBuses());

  // Note: Engine playback should already be suspended here by wrapper

  juce::int64 startSample = static_cast<juce::int64>(startTime * sampleRate);
  juce::int64 totalSamples = static_cast<juce::int64>(sampleRate * duration);
  juce::int64 samplesProcessed = 0;
  outMaxPeak = 0.0f;

  while (samplesProcessed < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(blockSize),
                                                  totalSamples - samplesProcessed));

    engine_.renderOfflineBlock(offlineContext, buffer, numSamples, startSample + samplesProcessed);

    // Find peak across both channels
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      float channelPeak = buffer.getMagnitude(ch, 0, numSamples);
      if (channelPeak > outMaxPeak)
        outMaxPeak = channelPeak;
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
      return false;

    samplesProcessed += numSamples;
  }

  return !shouldCancel_.load();
}

bool AudioExporter::writeFinalFile(const juce::File &tempFile,
                                   const ExportOptions &options,
                                   float maxPeak) {
  // Calculate normalization gain
  float targetLinear = juce::Decibels::decibelsToGain(static_cast<float>(options.normalizeDb));
  float gain = 1.0f;

  if (maxPeak > 0.0001f) {
    gain = targetLinear / maxPeak;
  }

  DBG("AudioExporter: Applying Gain: " + juce::String(gain) + 
      " (" + juce::String(juce::Decibels::gainToDecibels(gain)) + " dB)");

  // Read temporary file
  juce::AudioFormatManager tempMgr;
  tempMgr.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(tempMgr.createReaderFor(tempFile));
  if (!reader)
    return false;

  // Prepare output format
  juce::AudioFormat *targetFormat = getFormatForType(options.format);
  if (!targetFormat)
    return false;

  juce::File outputFile = options.outputFile;
  outputFile.deleteFile();

  std::unique_ptr<juce::FileOutputStream> outStream(outputFile.createOutputStream());
  if (!outStream)
    return false;

  auto writerOptions = juce::AudioFormatWriterOptions()
      .withSampleRate(options.sampleRate)
      .withNumChannels(2)
      .withBitsPerSample(options.bitDepth);

  std::unique_ptr<juce::OutputStream> streamPtr(std::move(outStream));
  std::unique_ptr<juce::AudioFormatWriter> writer(
      targetFormat->createWriterFor(streamPtr, writerOptions));
  if (!writer)
    return false;

  // Process with gain and dithering
  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);
  
  zenith::dsp::Dither dither;
  if (options.enableDither) {
    dither.prepare(2);
    dither.setType(options.ditherType);
  }

  juce::int64 totalSamples = reader->lengthInSamples;
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(blockSize), 
                                                  totalSamples - samplesWritten));

    reader->read(&buffer, 0, numSamples, samplesWritten, true, true);

    // Apply normalization gain
    buffer.applyGain(gain);

    // Apply dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(buffer, options.bitDepth);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
      return false;

    samplesWritten += numSamples;

    if (options.progressCallback) {
      float progress = 0.5f + (0.5f * static_cast<float>(samplesWritten) / 
                               static_cast<float>(totalSamples));
      options.progressCallback(progress, "Writing normalized audio...");
    }
  }

  return !shouldCancel_.load();
}

//==============================================================================
// Stem Export - Synchronous
//==============================================================================

bool AudioExporter::exportStems(const ExportOptions &options) {
  DBG("AudioExporter: Starting synchronous stem export...");

  const int numTracks = engine_.getNumTracks();
  if (numTracks == 0) {
    DBG("AudioExporter: No tracks to export");
    return false;
  }

  // Determine which tracks to export
  std::vector<int> tracksToExport;
  if (options.stemTrackIndices.empty()) {
    for (int i = 0; i < numTracks; ++i) {
      tracksToExport.push_back(i);
    }
  } else {
    tracksToExport = options.stemTrackIndices;
  }

  // Get file extension
  juce::String extension;
  switch (options.format) {
  case ExportFormat::WAV:  extension = ".wav";  break;
  case ExportFormat::FLAC: extension = ".flac"; break;
  case ExportFormat::OGG:  extension = ".ogg";  break;
  case ExportFormat::AIFF: extension = ".aiff"; break;
  }

  juce::File outputDir = options.outputFile.getParentDirectory();
  juce::String baseName = options.outputFile.getFileNameWithoutExtension();

    // Handle normalization gain (2-pass)
    float normalizationGain = 1.0f;
    if (options.normalize) {
        float maxPeak = 0.0f;
        if (analyzeProjectPeak(options.duration > 0 ? options.duration : engine_.autoDetectProjectDuration(),
                               options.sampleRate, options.startTime, maxPeak)) {
            float targetLinear = juce::Decibels::decibelsToGain(static_cast<float>(options.normalizeDb));
            if (maxPeak > 0.0001f) normalizationGain = targetLinear / maxPeak;
        }
    }

    int successCount = 0;
    for (size_t i = 0; i < tracksToExport.size() && !shouldCancel_.load(); ++i) {
        int trackIndex = tracksToExport[i];

        // Get track name for filename
        juce::String trackName = "Track_" + juce::String(trackIndex + 1);
        auto tracks = engine_.getTracksSnapshot();
        if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()) && tracks[trackIndex]) {
            trackName = tracks[trackIndex]->getName();
            trackName = trackName.replaceCharacters("/:*?\"<>|\\", "_________");
        }

        juce::File stemFile = outputDir.getChildFile(baseName + "_" + trackName + extension);

        if (options.progressCallback) {
            float overallProgress = static_cast<float>(i) / static_cast<float>(tracksToExport.size());
            options.progressCallback(overallProgress, "Exporting: " + trackName);
        }

        if (exportSingleStemInternal(trackIndex, options, stemFile, normalizationGain)) {
            successCount++;
        }
    }

  if (options.progressCallback && !shouldCancel_.load()) {
    options.progressCallback(1.0f, "Stem export complete!");
  }

  DBG("AudioExporter: Exported " << successCount << " of " << tracksToExport.size() << " stems");
  return successCount == static_cast<int>(tracksToExport.size());
}

//==============================================================================
// Stem Export - Asynchronous (Thread Pool)
//==============================================================================

bool AudioExporter::exportStemsAsync(const ExportOptions &options) {
  DBG("AudioExporter: Starting asynchronous stem export...");

  const int numTracks = engine_.getNumTracks();
  if (numTracks == 0) {
    DBG("AudioExporter: No tracks to export");
    return false;
  }

  // Determine tracks to export
  std::vector<int> tracksToExport;
  if (options.stemTrackIndices.empty()) {
    for (int i = 0; i < numTracks; ++i) {
      tracksToExport.push_back(i);
    }
  } else {
    tracksToExport = options.stemTrackIndices;
  }

  // Get file extension
  juce::String extension;
  switch (options.format) {
  case ExportFormat::WAV:  extension = ".wav";  break;
  case ExportFormat::FLAC: extension = ".flac"; break;
  case ExportFormat::OGG:  extension = ".ogg";  break;
  case ExportFormat::AIFF: extension = ".aiff"; break;
  }

  juce::File outputDir = options.outputFile.getParentDirectory();
  juce::String baseName = options.outputFile.getFileNameWithoutExtension();

  // Initialize progress tracking
  {
    juce::ScopedLock lock(stemProgressLock_);
    stemProgress_.clear();
    completedStemCount_.store(0);
    
    for (int trackIndex : tracksToExport) {
      StemExportProgress progress;
      progress.trackIndex = trackIndex;
      progress.trackName = "Track_" + juce::String(trackIndex + 1);
      
      auto tracks = engine_.getTracksSnapshot();
      if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()) && tracks[trackIndex]) {
        progress.trackName = tracks[trackIndex]->getName();
      }
      
      stemProgress_.push_back(progress);
    }
  }

  // Get thread pool from engine
  juce::ThreadPool& threadPool = engine_.getThreadPool();
  
  // Create jobs for each stem
  std::atomic<int> failureCount{0};
  juce::WaitableEvent allComplete;
  std::atomic<int> pendingJobs{static_cast<int>(tracksToExport.size())};

  // Handle normalization for async stems (2-pass)
  float normalizationGain = 1.0f;
  if (options.normalize) {
    float maxPeak = 0.0f;
    if (options.progressCallback) options.progressCallback(0.0f, "Analyzing project peaks for stems...");
    if (analyzeProjectPeak(options.duration > 0 ? options.duration : engine_.autoDetectProjectDuration(), 
                           options.sampleRate, options.startTime, maxPeak)) {
      float targetLinear = juce::Decibels::decibelsToGain(static_cast<float>(options.normalizeDb));
      if (maxPeak > 0.0001f) normalizationGain = targetLinear / maxPeak;
    }
  }

  for (int trackIndex : tracksToExport) {
    juce::String trackName;
    {
      juce::ScopedLock lock(stemProgressLock_);
      for (const auto& stem : stemProgress_) {
        if (stem.trackIndex == trackIndex) {
          trackName = stem.trackName;
          break;
        }
      }
    }
    
    juce::String safeTrackName = trackName.replaceCharacters("/:*?\"<>|\\", "_________");
    juce::File stemFile = outputDir.getChildFile(baseName + "_" + safeTrackName + extension);

    // Capture by value for thread safety
    threadPool.addJob([this, trackIndex, options, stemFile, normalizationGain, &failureCount, &pendingJobs, &allComplete]() {
      if (shouldCancel_.load()) {
        markStemComplete(trackIndex, false);
        if (--pendingJobs == 0) allComplete.signal();
        return;
      }

      // Create per-stem options with progress callback
      ExportOptions stemOptions = options;
      stemOptions.exportStems = false;
      stemOptions.progressCallback = [this, trackIndex](float progress, const juce::String&) {
        updateStemProgress(trackIndex, progress);
      };

      bool success = exportSingleStemInternal(trackIndex, stemOptions, stemFile, normalizationGain);
      
      if (!success) {
        failureCount.fetch_add(1);
      }
      
      markStemComplete(trackIndex, success);
      reportAggregateProgress(options);
      
      if (--pendingJobs == 0) {
        allComplete.signal();
      }
    });
  }

  // Wait for all jobs to complete (with timeout for cancellation checks)
  while (pendingJobs.load() > 0) {
    if (allComplete.wait(100)) break;
    
    if (shouldCancel_.load()) {
      // Signal remaining jobs to stop
      break;
    }
    
    reportAggregateProgress(options);
  }

  // Wait a bit more for jobs to finish cleanup
  allComplete.wait(1000);

  if (options.progressCallback) {
    int completed = completedStemCount_.load();
    int total = static_cast<int>(tracksToExport.size());
    if (shouldCancel_.load()) {
      options.progressCallback(1.0f, "Export cancelled");
    } else if (failureCount.load() == 0) {
      options.progressCallback(1.0f, "Stem export complete! (" + 
                               juce::String(completed) + " files)");
    } else {
      options.progressCallback(1.0f, "Stem export finished with errors");
    }
  }

  DBG("AudioExporter: Async export complete. Failures: " << failureCount.load());
  return failureCount.load() == 0 && !shouldCancel_.load();
}

//==============================================================================
// Single Stem Export
//==============================================================================

bool AudioExporter::exportSingleStem(int trackIndex, const ExportOptions &options) {
  // Public wrapper uses outputFile from options
  return exportSingleStemInternal(trackIndex, options, options.outputFile);
}

bool AudioExporter::exportSingleStemInternal(int trackIndex, const ExportOptions &options,
                                             const juce::File& stemOutputFile, float normalizationGain) {
  DBG("AudioExporter: Exporting stem for track " << trackIndex);

  auto tracks = engine_.getTracksSnapshot();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
    DBG("AudioExporter: Invalid track index");
    return false;
  }

  auto* track = tracks[trackIndex].get();
  if (!track) {
    DBG("AudioExporter: Track is null");
    return false;
  }

  // Determine duration
  double duration = options.duration;
  if (duration <= 0.0) {
    duration = engine_.autoDetectProjectDuration();
  }

  // Get format
  juce::AudioFormat *format = getFormatForType(options.format);
  if (!format) {
    DBG("AudioExporter: Format not found");
    return false;
  }

  // Create output file
  juce::File outputFile = stemOutputFile;
  outputFile.deleteFile();

  std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());
  if (!fileStream) {
    DBG("AudioExporter: Could not create output stream");
    return false;
  }

  auto writerOptions = juce::AudioFormatWriterOptions()
      .withSampleRate(options.sampleRate)
      .withNumChannels(2)
      .withBitsPerSample(options.bitDepth);

  std::unique_ptr<juce::OutputStream> streamPtr(std::move(fileStream));
  std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(streamPtr, writerOptions));

  if (!writer) {
    DBG("AudioExporter: Could not create writer");
    return false;
  }

  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);
  juce::AudioBuffer<float> trackBuffer(2, blockSize);

  zenith::dsp::Dither dither;
  if (options.enableDither) {
    dither.prepare(2);
    dither.setType(options.ditherType);
  }

  // Prepare track for offline rendering
  // track->prepareToPlay(blockSize, options.sampleRate); // Track buffers are now in Context!

  // Create local render context
  AudioRenderContext offlineContext;
  offlineContext.prepare(options.sampleRate, blockSize, engine_.getNumTracks(), engine_.getNumAuxBuses());
  juce::int64 startSample = static_cast<juce::int64>(options.startTime * options.sampleRate);
  juce::int64 totalSamples = static_cast<juce::int64>(options.sampleRate * duration);
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(blockSize),
                                                  totalSamples - samplesWritten));

    buffer.clear();
    trackBuffer.clear();

    // Render single track
    juce::AudioSourceChannelInfo info(&trackBuffer, 0, numSamples);
    juce::MidiBuffer midiBuffer;
    juce::AudioBuffer<float> sidechainBuffer;
    std::vector<juce::AudioBuffer<float>*> auxBuffers;

    track->getNextAudioBlock(info, startSample + static_cast<int64_t>(samplesWritten), 
                             &midiBuffer, auxBuffers,
                             static_cast<const zenith::TempoMap*>(&engine_.getTempoMap()), 
                             &sidechainBuffer);

    // Copy track output to main buffer
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
      buffer.copyFrom(ch, 0, trackBuffer, ch, 0, numSamples);
    }

    // Apply normalization gain
    buffer.applyGain(normalizationGain);

    // Apply dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(buffer, options.bitDepth);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples)) {
      return false;
    }

    samplesWritten += numSamples;

    // Report progress
    if (options.progressCallback) {
      float progress = static_cast<float>(samplesWritten) / static_cast<float>(totalSamples);
      options.progressCallback(progress, "Exporting stem...");
    }
  }

  if (shouldCancel_.load()) {
    outputFile.deleteFile();
    return false;
  }

  return true;
}

} // namespace zenith
