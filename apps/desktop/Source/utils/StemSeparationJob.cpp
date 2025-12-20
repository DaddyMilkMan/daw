/*
  ==============================================================================

    StemSeparationJob.cpp
    Created: 2025-12-09
    Author:  Zenith DAW AI Team

  ==============================================================================
*/

#include "StemSeparationJob.h"
#include <juce_events/juce_events.h>

namespace zenith {
namespace utils {

StemSeparationJob::StemSeparationJob(const juce::File &inputFile,
                                     const juce::File &outputDirectory,
                                     CompletionCallback callback)
    : juce::ThreadPoolJob("Stem Separation: " + inputFile.getFileName()),
      inputFile_(inputFile), outputDirectory_(outputDirectory),
      callback_(callback) {}

StemSeparationJob::~StemSeparationJob() {}

juce::ThreadPoolJob::JobStatus StemSeparationJob::runJob() {
  // 1. Initialize result
  StemFiles result;

  if (!inputFile_.existsAsFile()) {
    result.error = "Input file does not exist";
    if (callback_) {
      juce::MessageManager::callAsync(
          [cb = callback_, res = result]() { cb(res); });
    }
    return juce::ThreadPoolJob::jobHasFinished;
  }

  // 2. Load Audio File
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(inputFile_));

  if (!reader) {
    result.error = "Could not read input file";
    if (callback_) {
      juce::MessageManager::callAsync(
          [cb = callback_, res = result]() { cb(res); });
    }
    return juce::ThreadPoolJob::jobHasFinished;
  }

  juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                                  static_cast<int>(reader->lengthInSamples));
  reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true,
               true);
  double sampleRate = reader->sampleRate;

  // 3. Run Separation
  ONNXStemSeparator separator;

  // Check for model file availability
  juce::File modelFile;

  // 1. Check AppData
  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
  auto appDataModel = appDataDir.getChildFile("ZenithDAW/Models/htdemucs.onnx");

  // 2. Check App Directory (Portable)
  auto appDir =
      juce::File::getSpecialLocation(juce::File::currentApplicationFile)
          .getParentDirectory();
  auto localModel = appDir.getChildFile("Models/htdemucs.onnx");

  if (appDataModel.existsAsFile()) {
    modelFile = appDataModel;
  } else if (localModel.existsAsFile()) {
    modelFile = localModel;
  }

  // Initialize separator
  bool onnxReady = false;
  if (modelFile.existsAsFile()) {
    onnxReady = separator.initialize(modelFile);
  }

  // If initialization failed but we expected it to work (model exists), log
  // warning
  if (modelFile.existsAsFile() && !onnxReady) {
    DBG("StemSeparationJob: Model found but failed to initialize ONNX session");
  }

  if (!separator.isAvailable() && !onnxReady) {
    // If we can't use ONNX (runtime missing OR model missing/failed),
    // the separator might have a DSP fallback.
    // However, if the user explicitly requested AI separation, this might be
    // disappointing. For now, we proceed to separate(), which handles fallback
    // internally and sets the 'usedONNX' flag in the result.

    // Optional: Abort if stricter requirements needed
    // result.error = "ONNX Runtime not available or model missing.";
    // ...
  }

  auto separationResult = separator.separate(buffer, sampleRate);

  if (!separationResult.success) {
    result.error = separationResult.error;
    if (callback_) {
      juce::MessageManager::callAsync(
          [cb = callback_, res = result]() { cb(res); });
    }
    return juce::ThreadPoolJob::jobHasFinished;
  }

  // 4. Write Outputs
  outputDirectory_.createDirectory();
  juce::String baseName = inputFile_.getFileNameWithoutExtension();

  result.vocals = writeStemToFile(separationResult.vocals, baseName + "_Vocals",
                                  sampleRate);
  result.drums =
      writeStemToFile(separationResult.drums, baseName + "_Drums", sampleRate);
  result.bass =
      writeStemToFile(separationResult.bass, baseName + "_Bass", sampleRate);
  result.other =
      writeStemToFile(separationResult.other, baseName + "_Other", sampleRate);

  if (result.vocals.exists() && result.drums.exists() && result.bass.exists() &&
      result.other.exists()) {
    result.success = true;
  } else {
    result.error = "Failed to write stem files";
  }

  // 5. Callback on Message Thread
  if (callback_) {
    // Capture by value to ensure safety
    juce::MessageManager::callAsync(
        [cb = callback_, res = result]() { cb(res); });
  }

  return juce::ThreadPoolJob::jobHasFinished;
}

juce::File
StemSeparationJob::writeStemToFile(const juce::AudioBuffer<float> &buffer,
                                   const juce::String &stemName,
                                   double sampleRate) {
  juce::File outFile = outputDirectory_.getChildFile(stemName + ".wav");

  if (outFile.exists()) {
    outFile.deleteFile();
  }

  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
      new juce::FileOutputStream(outFile), sampleRate,
      static_cast<unsigned int>(buffer.getNumChannels()), 24, {}, 0));

  if (writer) {
    writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
  }

  return outFile;
}

} // namespace utils
} // namespace zenith
