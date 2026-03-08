#include "ONNXStemSeparatorAutoLoader.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

//==============================================================================
// StemSeparationWorker Implementation
//==============================================================================

class SeparationThread : public juce::Thread {
public:
    SeparationThread(ONNXStemSeparator& separator,
                    const juce::AudioBuffer<float>& input,
                    double sampleRate,
                    StemSeparationWorker::CompletionCallback completion)
        : juce::Thread("Stem Separation"),
          separator(separator),
          input(input),
          sampleRate(sampleRate),
          completion(std::move(completion)) {}

    void run() override {
        ONNXStemSeparator::SeparationResult result = separator.separate(input, sampleRate);
        completion(result);
    }

private:
    ONNXStemSeparator& separator;
    juce::AudioBuffer<float> input;
    double sampleRate;
    StemSeparationWorker::CompletionCallback completion;
};

StemSeparationWorker::StemSeparationWorker() {
    // Check if model is already available
    if (modelManager.isModelInstalled("htdemucs")) {
        modelLoaded = false;  // Need to load
        currentStatus = Status::ModelReady;
    }
}

StemSeparationWorker::~StemSeparationWorker() {
    cancel();
    if (processingThread) {
        processingThread->stopThread(1000);
    }
}

void StemSeparationWorker::prepareModel() {
    if (currentStatus == Status::ModelReady || currentStatus == Status::LoadingModel) {
        return;  // Already ready or loading
    }

    if (!modelManager.isModelInstalled("htdemucs")) {
        currentStatus = Status::ModelNotAvailable;
        updateProgress(Status::ModelNotAvailable, 0.0, "Model not installed");

        // Start download
        modelManager.downloadModel("htdemucs",
            [this](int64_t downloaded, int64_t total) {
                double progress = total > 0 ? static_cast<double>(downloaded) / total : 0.0;
                updateProgress(Status::LoadingModel, progress,
                             "Downloading model: " + juce::String(progress * 100, 1) + "%");
            },
            [this](bool success, const juce::String& message) {
                if (success) {
                    currentStatus = Status::ModelReady;
                    updateProgress(Status::ModelReady, 1.0, "Model downloaded successfully");
                    loadModel();
                } else {
                    currentStatus = Status::Error;
                    updateProgress(Status::Error, 0.0, "Download failed: " + message);
                }
            }
        );
    } else {
        loadModel();
    }
}

void StemSeparationWorker::loadModel() {
#ifdef ZENITH_USE_ONNX_RUNTIME
    juce::File modelPath = modelManager.getModelInfo("htdemucs").localPath;
    if (stemSeparator.initialize(modelPath)) {
        modelLoaded = true;
        currentStatus = Status::ModelReady;
        updateProgress(Status::ModelReady, 1.0, "Model loaded successfully");
    } else {
        currentStatus = Status::Error;
        updateProgress(Status::Error, 0.0, "Failed to load model");
    }
#else
    currentStatus = Status::ModelReady;  // DSP fallback always ready
    updateProgress(Status::ModelReady, 1.0, "Using DSP fallback (ONNX not available)");
#endif
}

void StemSeparationWorker::separate(const juce::AudioBuffer<float>& input,
                                    double sampleRate,
                                    CompletionCallback completion) {
    pendingTask = { input, sampleRate, std::move(completion) };

    // Ensure model is ready first
    if (currentStatus != Status::ModelReady) {
        prepareModel();
        // Will process after model loads (in the completion callback)
        return;
    }

    processSeparation();
}

void StemSeparationWorker::processSeparation() {
    if (!pendingTask.has_value()) {
        return;
    }

    currentStatus = Status::Processing;
    updateProgress(Status::Processing, 0.0, "Separating stems...");

    // Run separation in background thread
    processingThread = std::make_unique<SeparationThread>(
        stemSeparator,
        pendingTask->input,
        pendingTask->sampleRate,
        [this](const ONNXStemSeparator::SeparationResult& result) {
            currentStatus = Status::Completed;
            updateProgress(Status::Completed, 1.0,
                         result.success ? "Separation complete" : "Separation failed");
            pendingTask->completion(result);
            pendingTask = std::nullopt;
        }
    );

    processingThread->startThread();
}

void StemSeparationWorker::cancel() {
    shouldCancel = true;
    modelManager.cancelDownload();
    if (processingThread && processingThread->isThreadRunning()) {
        processingThread->stopThread(1000);
    }
    pendingTask = std::nullopt;
}

void StemSeparationWorker::updateProgress(Status status, double progress,
                                        const juce::String& message) {
    currentStatus = status;

    if (progressCallback) {
        Progress p;
        p.status = status;
        p.progress = progress;
        p.message = message;
        progressCallback(p);
    }
}

} // namespace zenith
