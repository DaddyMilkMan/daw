/*
  ==============================================================================

    AudioExporter.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AudioExporter.h"
#include "../dsp/Dither.h"
#include "AudioRenderer.h"
#include "Engine.h"
#include "Track.h"
#include "TempoMap.h"

namespace zenith {

AudioExporter::AudioExporter(Engine &engine) : engine_(engine) {
  registerFormats();
}

void AudioExporter::registerFormats() {
  formatManager.registerBasicFormats();
  formatManager.registerFormat(new juce::FlacAudioFormat(), false);
  formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
  // AIFF is already included in registerBasicFormats()
}

bool AudioExporter::exportProject(const ExportOptions &options) {
  DBG("AudioExporter: Starting export...");

  if (options.sampleRate <= 0)
    return false;

  // CRITICAL: Check if stem export is requested
  if (options.exportStems) {
    return exportStems(options);
  }

  // 1. Setup Dither
  zenith::dsp::Dither dither;
  if (options.enableDither) {
    dither.prepare(2); // Stereo
  }

  // 2. Determine Duration
  double duration = options.duration;
  if (duration <= 0.0) {
    duration = engine_.autoDetectProjectDuration();
  }
  DBG("AudioExporter: Duration: " + juce::String(duration, 2) + "s");

  // 3. Normalization Check
  if (options.normalize) {
    DBG("AudioExporter: 2-Pass Normalization Enabled");

    // Create Temp File
    auto tempFile = options.outputFile.getParentDirectory().getChildFile(
        "zenith_export_temp.wav");

    // Pass 1: Render to Float32 Temp
    float maxPeak = 0.0f;
    if (!renderToTempFile(tempFile, duration, options.sampleRate, maxPeak)) {
      tempFile.deleteFile();
      return false;
    }

    DBG("AudioExporter: Peak found: " +
        juce::String(juce::Decibels::gainToDecibels(maxPeak)) + " dB");

    // Pass 2: Write Final
    bool success = writeFinalFile(tempFile, options, maxPeak);

    // Cleanup
    tempFile.deleteFile();
    return success;
  }

  // 4. Single Pass (Direct Render)
  // Reuse writeFinalFile logic but with direct rendering ideally,
  // but to adhere to DRY, let's just use the temp file approach for consistency
  // OR strictly implement single pass to save disk I/O.
  // Given "Strict No-Stubbing" and "High Effort", I should implement a direct
  // path for non-normalized to be efficient.

  // Actually, for simplicity and robustness, doing direct render here is
  // better.

  DBG("AudioExporter: Direct Export (No Normalization)");

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

  juce::File outputFile = options.outputFile;
  outputFile.deleteFile(); // Overwrite

  std::unique_ptr<juce::FileOutputStream> fileStream(
      outputFile.createOutputStream());
  if (!fileStream)
    return false;

  std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(
      fileStream.release(), options.sampleRate, 2, options.bitDepth, {}, 0));

  if (!writer)
    return false;

  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);

  // Prepare Engine
  if (engine_.audioRenderer_) {
    engine_.audioRenderer_->prepare(options.sampleRate, blockSize,
                                    engine_.getNumTracks(),
                                    engine_.getNumAuxBuses());
  }

  juce::int64 totalSamples =
      static_cast<juce::int64>(options.sampleRate * duration);
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples) {
    int numSamples =
        (int)juce::jmin((juce::int64)blockSize, totalSamples - samplesWritten);

    // Render
    engine_.renderOfflineBlock(buffer, numSamples, samplesWritten);

    // Dither
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(buffer, options.bitDepth);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
      return false;

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

bool AudioExporter::renderToTempFile(const juce::File &tempFile,
                                     double duration, double sampleRate,
                                     float &outMaxPeak) {
  tempFile.deleteFile();

  // WAV Float 32 for high fidelity temp
  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::FileOutputStream> stream(tempFile.createOutputStream());
  if (!stream)
    return false;

  std::unique_ptr<juce::AudioFormatWriter> writer(
      wavFormat.createWriterFor(stream.release(), sampleRate, 2, 32, {}, 0));

  if (!writer)
    return false;

  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);

  // Prepare Engine
  if (engine_.audioRenderer_) {
    engine_.audioRenderer_->prepare(
        sampleRate, blockSize, engine_.getNumTracks(),
        engine_.getNumAuxBuses()); // Important to reset state
  }

  juce::int64 totalSamples = static_cast<juce::int64>(sampleRate * duration);
  juce::int64 samplesProcessed = 0;
  outMaxPeak = 0.0f;

  while (samplesProcessed < totalSamples) {
    int numSamples = (int)juce::jmin((juce::int64)blockSize,
                                     totalSamples - samplesProcessed);

    engine_.renderOfflineBlock(buffer, numSamples, samplesProcessed);

    // Find Peak
    float currentPeak = buffer.getMagnitude(0, numSamples);
    if (currentPeak > outMaxPeak)
      outMaxPeak = currentPeak;

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
      return false;

    samplesProcessed += numSamples;
  }

  return true;
}

bool AudioExporter::writeFinalFile(const juce::File &tempFile,
                                   const ExportOptions &options,
                                   float maxPeak) {
  // Determine gain
  float targetLinear =
      juce::Decibels::decibelsToGain((float)options.normalizeDb);
  float gain = 1.0f;

  if (maxPeak > 0.0001f) {
    gain = targetLinear / maxPeak;
  }

  // Clamp gain (safety) logic could go here, but normalization implies trust.

  DBG("AudioExporter: Applying Gain: " + juce::String(gain));

  // Setup Reader for Temp
  juce::AudioFormatManager tempMgr;
  tempMgr.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(
      tempMgr.createReaderFor(tempFile));
  if (!reader)
    return false;

  // Setup Writer for Final
  juce::AudioFormat *targetFormat = nullptr;
  switch (options.format) {
  case ExportFormat::WAV:
    targetFormat = formatManager.findFormatForFileExtension("wav");
    break;
  case ExportFormat::FLAC:
    targetFormat = formatManager.findFormatForFileExtension("flac");
    break;
  case ExportFormat::OGG:
    targetFormat = formatManager.findFormatForFileExtension("ogg");
    break;
  }
  if (!targetFormat)
    return false;

  juce::File outputFile = options.outputFile;
  outputFile.deleteFile();

  std::unique_ptr<juce::FileOutputStream> outStream(
      outputFile.createOutputStream());
  if (!outStream)
    return false;

  std::unique_ptr<juce::AudioFormatWriter> writer(targetFormat->createWriterFor(
      outStream.release(), options.sampleRate, 2, options.bitDepth, {}, 0));
  if (!writer)
    return false;

  // Process
  const int blockSize = 4096;
  juce::AudioBuffer<float> buffer(2, blockSize);
  zenith::dsp::Dither dither;
  if (options.enableDither)
    dither.prepare(2);

  juce::int64 totalSamples = reader->lengthInSamples;
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples) {
    int numSamples =
        (int)juce::jmin((juce::int64)blockSize, totalSamples - samplesWritten);

    reader->read(&buffer, 0, numSamples, samplesWritten, true, true);

    // Apply Gain
    buffer.applyGain(gain);

    // Dither
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(buffer, options.bitDepth);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
      return false;

    samplesWritten += numSamples;
  }

  return true;
}

bool AudioExporter::exportStems(const ExportOptions &options) {
  DBG("AudioExporter: Starting stem export...");

  const int numTracks = engine_.getNumTracks();
  if (numTracks == 0) {
    DBG("AudioExporter: No tracks to export");
    return false;
  }

  // Determine which tracks to export
  std::vector<int> tracksToExport;
  if (options.stemTrackIndices.empty()) {
    // Export all tracks
    for (int i = 0; i < numTracks; ++i) {
      tracksToExport.push_back(i);
    }
  } else {
    tracksToExport = options.stemTrackIndices;
  }

  // Get base filename and extension
  juce::String extension;
  switch (options.format) {
  case ExportFormat::WAV:  extension = ".wav";  break;
  case ExportFormat::FLAC: extension = ".flac"; break;
  case ExportFormat::OGG:  extension = ".ogg";  break;
  case ExportFormat::AIFF: extension = ".aiff"; break;
  }

  juce::File outputDir = options.outputFile.getParentDirectory();
  juce::String baseName = options.outputFile.getFileNameWithoutExtension();

  int successCount = 0;
  for (size_t i = 0; i < tracksToExport.size(); ++i) {
    int trackIndex = tracksToExport[i];
    
    // Get track name for filename
    juce::String trackName = "Track_" + juce::String(trackIndex + 1);
    const auto& tracks = engine_.tracks();
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size())) {
      if (tracks[trackIndex]) {
        trackName = tracks[trackIndex]->getName();
        // Sanitize for filename
        trackName = trackName.replaceCharacters("/:*?\"<>|\\", "_________");
      }
    }

    // Create stem output file
    juce::File stemFile = outputDir.getChildFile(baseName + "_" + trackName + extension);

    // Create single-track export options
    ExportOptions stemOptions = options;
    stemOptions.outputFile = stemFile;
    stemOptions.exportStems = false; // Prevent recursion

    if (options.progressCallback) {
      float overallProgress = static_cast<float>(i) / static_cast<float>(tracksToExport.size());
      options.progressCallback(overallProgress, "Exporting stem: " + trackName);
    }

    if (exportSingleStem(trackIndex, stemOptions)) {
      successCount++;
    }
  }

  if (options.progressCallback) {
    options.progressCallback(1.0f, "Stem export complete!");
  }

  DBG("AudioExporter: Exported " << successCount << " of " << tracksToExport.size() << " stems");
  return successCount == static_cast<int>(tracksToExport.size());
}

bool AudioExporter::exportSingleStem(int trackIndex, const ExportOptions &options) {
  DBG("AudioExporter: Exporting stem for track " << trackIndex);

  const auto& tracks = engine_.tracks();
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

  // Get audio format
  juce::AudioFormat *format = nullptr;
  switch (options.format) {
  case ExportFormat::WAV:  format = formatManager.findFormatForFileExtension("wav");  break;
  case ExportFormat::FLAC: format = formatManager.findFormatForFileExtension("flac"); break;
  case ExportFormat::OGG:  format = formatManager.findFormatForFileExtension("ogg");  break;
  case ExportFormat::AIFF: format = formatManager.findFormatForFileExtension("aiff"); break;
  }

  if (!format) {
    DBG("AudioExporter: Format not found");
    return false;
  }

  // Create output file
  juce::File outputFile = options.outputFile;
  outputFile.deleteFile();

  std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());
  if (!fileStream) {
    DBG("AudioExporter: Could not create output stream");
    return false;
  }

  std::unique_ptr<juce::AudioFormatWriter> writer(format->createWriterFor(
      fileStream.release(), options.sampleRate, 2, options.bitDepth, {}, 0));

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
  }

  // Prepare track for offline rendering
  track->prepareToPlay(blockSize, options.sampleRate);

  juce::int64 totalSamples = static_cast<juce::int64>(options.sampleRate * duration);
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(blockSize), 
                                                  totalSamples - samplesWritten));

    buffer.clear();
    trackBuffer.clear();

    // Render single track
    juce::AudioSourceChannelInfo info(&trackBuffer, 0, numSamples);
    juce::MidiBuffer midiBuffer;
    juce::AudioBuffer<float> sidechainBuffer; // Empty sidechain for stems
    std::vector<juce::AudioBuffer<float>*> auxBuffers; // Empty aux for stems
    
    track->getNextAudioBlock(info, (int64_t)samplesWritten, &midiBuffer, auxBuffers, 
                             (const zenith::TempoMap*)&engine_.getTempoMap(), &sidechainBuffer);

    // Copy track output to main buffer
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), trackBuffer.getNumChannels()); ++ch) {
      buffer.copyFrom(ch, 0, trackBuffer, ch, 0, numSamples);
    }

    // Apply dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(buffer, options.bitDepth);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples)) {
      return false;
    }

    samplesWritten += numSamples;

    // Report progress for this stem
    if (options.progressCallback) {
      float progress = static_cast<float>(samplesWritten) / static_cast<float>(totalSamples);
      options.progressCallback(progress, "Exporting stem...");
    }
  }

  return true;
}

} // namespace zenith

