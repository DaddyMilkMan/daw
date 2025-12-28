#include "StemSeparationJob.h"
#include "../ai/AIStatusManager.h"
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
  // Start AI Status Tracking
  auto &statusMgr = ai::AIStatusManager::getInstance();
  juce::String opId = statusMgr.beginOperation(
      "NeuralEngine", "Separating stems for " + inputFile_.getFileName());

  // 1. Initialize result
  StemFiles result;

  auto abortWithError = [&](const juce::String &err) {
    result.error = err;
    statusMgr.completeOperation(opId, false, err);
    if (callback_) {
      juce::MessageManager::callAsync(
          [cb = callback_, res = result]() { cb(res); });
    }
  };

  if (!inputFile_.existsAsFile()) {
    abortWithError("Input file does not exist");
    return juce::ThreadPoolJob::jobHasFinished;
  }

  // 2. Load Audio File
  statusMgr.updateProgress(opId, 0.1f, "Loading audio file...");
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(inputFile_));

  if (!reader) {
    abortWithError("Could not read input file");
    return juce::ThreadPoolJob::jobHasFinished;
  }

  juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                                  static_cast<int>(reader->lengthInSamples));
  reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true,
               true);
  double sampleRate = reader->sampleRate;

  // 3. Run Separation
  statusMgr.updateProgress(opId, 0.3f, "Initializing Neural Engine...");
  ONNXStemSeparator separator;

  // Initialize with default model path
  juce::File modelFile = ONNXStemSeparator::findDefaultModel();
  if (!modelFile.existsAsFile() || !separator.initialize(modelFile)) {
    // If initialization fails, we might still proceed with DSP fallback
    // but we record the warning.
    DBG("StemSeparationJob: Model initialization failed, attempting DSP fallback");
  }

  statusMgr.updateProgress(opId, 0.4f, "Processing neural inference...");
  auto separationResult = separator.separate(buffer, sampleRate);

  if (!separationResult.success) {
    abortWithError(separationResult.error);
    return juce::ThreadPoolJob::jobHasFinished;
  }

  // 4. Write Outputs
  statusMgr.updateProgress(opId, 0.8f, "Writing stems to disk...");
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
    result.usedNeuralEngine = separationResult.usedONNX;
    statusMgr.completeOperation(opId, true, "Stem separation completed successfully");
  } else {
    abortWithError("Failed to write stem files");
  }

  // 5. Callback on Message Thread (Single point of contact)
  if (callback_) {
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
  auto writerOptions = juce::AudioFormatWriterOptions()
                           .withSampleRate(sampleRate)
                           .withNumChannels((int)buffer.getNumChannels())
                           .withBitsPerSample(24);

  std::unique_ptr<juce::OutputStream> fileStream(new juce::FileOutputStream(outFile));
  if (static_cast<juce::FileOutputStream*>(fileStream.get())->failedToOpen()) {
      return juce::File(); 
  }

  std::unique_ptr<juce::AudioFormatWriter> writer = wavFormat.createWriterFor(fileStream, writerOptions);

  if (writer) {
    writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
  }

  return outFile;
}

} // namespace utils
} // namespace zenith
