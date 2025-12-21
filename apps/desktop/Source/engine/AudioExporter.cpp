/*
  ==============================================================================

    AudioExporter.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AudioExporter.h"
#include "../dsp/Dither.h"
#include "Engine.h"

namespace zenith {

AudioExporter::AudioExporter(Engine &engine) : engine_(engine) {
  registerFormats();
}

void AudioExporter::registerFormats() {
  formatManager.registerBasicFormats();
  formatManager.registerFormat(new juce::FlacAudioFormat(), false);
  formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);
}

bool AudioExporter::exportProject(const ExportOptions &options) {
  DBG("AudioExporter: Starting export...");

  if (options.sampleRate <= 0)
    return false;

  // 1. Setup Dither
  Dither dither;
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
  Dither dither;
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

} // namespace zenith
