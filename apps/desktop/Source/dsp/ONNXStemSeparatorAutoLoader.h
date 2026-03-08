#pragma once

#include "ONNXStemSeparator.h"
#include "ModelManager.h"
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @brief Automatic model loading and management for stem separation
 *
 * Provides a simple interface that automatically handles:
 * - Checking if model is installed
 * - Downloading model if needed
 * - Loading model for inference
 * - Fallback to DSP if model unavailable
 */
class StemSeparationWorker {
public:
    enum class Status {
        NotInitialized,
        LoadingModel,
        ModelReady,
        Processing,
        Completed,
        Error,
        ModelNotAvailable
    };

    struct Progress {
        Status status = Status::NotInitialized;
        double progress = 0.0;  // 0.0 to 1.0
        juce::String message;
        int64_t bytesDownloaded = 0;
        int64_t totalBytes = 0;
    };

    using ProgressCallback = std::function<void(const Progress&)>;
    using CompletionCallback = std::function<void(const ONNXStemSeparator::SeparationResult&)>;

    StemSeparationWorker();
    ~StemSeparationWorker();

    /**
     * @brief Prepare model (download if needed)
     *
     * If model is not installed, will automatically download it.
     * Use withProgressCallback to monitor download progress.
     */
    void prepareModel();

    /**
     * @brief Separate audio into stems
     *
     * Will automatically prepare model if needed.
     */
    void separate(const juce::AudioBuffer<float>& input, double sampleRate,
                  CompletionCallback completion);

    /**
     * @brief Set progress callback for model download and processing
     */
    void setProgressCallback(ProgressCallback callback) {
        progressCallback = std::move(callback);
    }

    /**
     * @brief Cancel current operation
     */
    void cancel();

    /**
     * @brief Check if model is ready
     */
    bool isModelReady() const {
        return currentStatus == Status::ModelReady;
    }

    /**
     * @brief Check if using ONNX or DSP fallback
     */
    bool isUsingONNX() const {
#ifdef ZENITH_USE_ONNX_RUNTIME
        return stemSeparator.isAvailable() && modelLoaded;
#else
        return false;
#endif
    }

    /**
     * @brief Get current status
     */
    Status getStatus() const { return currentStatus; }

private:
    ONNXStemSeparator stemSeparator;
    ModelManager modelManager;
    std::unique_ptr<juce::Thread> processingThread;

    Status currentStatus = Status::NotInitialized;
    ProgressCallback progressCallback;
    bool modelLoaded = false;
    bool shouldCancel = false;

    struct SeparationTask {
        juce::AudioBuffer<float> input;
        double sampleRate;
        CompletionCallback completion;
    };

    std::optional<SeparationTask> pendingTask;

    void updateProgress(Status status, double progress, const juce::String& message = {});
    void loadModel();
    void processSeparation();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparationWorker)
};

} // namespace zenith
