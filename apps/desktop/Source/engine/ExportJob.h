/**
 * @file ExportJob.h
 * @brief Asynchronous export job for background audio rendering
 *
 * This class enables non-blocking audio export by running the render loop
 * on a background thread with proper progress reporting and cancellation.
 */

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <functional>
#include "AudioExporter.h"

namespace zenith {

class Engine;
struct ExportOptions;

/**
 * @class ExportJob
 * @brief Thread pool job for asynchronous audio export
 *
 * Usage:
 * @code
 * auto job = std::make_unique<ExportJob>(
 *     engine, options,
 *     [](float progress, const juce::String& status) {
 *         // Update UI progress bar
 *     },
 *     [](juce::Result result) {
 *         if (result.wasOk())
 *             // Export succeeded
 *         else
 *             // Show error: result.getErrorMessage()
 *     }
 * );
 * threadPool.addJob(job.release(), true); // Pool takes ownership
 * @endcode
 *
 * To cancel:
 * @code
 * job->cancel();
 * // Export stops within ~1 second (one block iteration)
 * @endcode
 */
class ExportJob : public juce::ThreadPoolJob {
public:
    /// Progress callback: (progress 0.0-1.0, status message)
    using ProgressCallback = std::function<void(float, const juce::String&)>;
    
    /// Completion callback: Result indicates success or error with message
    using CompletionCallback = std::function<void(juce::Result)>;

    /**
     * @brief Create an export job
     * @param engine Reference to the audio engine
     * @param options Export options (format, sample rate, etc.)
     * @param progress Callback for progress updates (~every 100ms)
     * @param completion Callback when export finishes (success or error)
     */
    ExportJob(Engine& engine, 
              const ExportOptions& options,
              ProgressCallback progress = nullptr,
              CompletionCallback completion = nullptr);

    ~ExportJob() override = default;

    /**
     * @brief Main job entry point (runs on thread pool thread)
     * @return jobHasFinished when done, or jobNeedsRunningAgain if more work needed
     */
    JobStatus runJob() override;

    /**
     * @brief Request cancellation of the export
     * @note Thread-safe. Export stops at next block boundary (~100ms worst case)
     */
    void cancel();

    /**
     * @brief Check if cancellation was requested
     */
    bool isCancelled() const { return shouldCancel_.load(std::memory_order_acquire); }

    /**
     * @brief Get current progress (0.0 - 1.0)
     */
    float getProgress() const { return currentProgress_.load(std::memory_order_relaxed); }

private:
    // Constants for progress reporting
    static constexpr int kExportBlockSize = 4096;
    static constexpr int kProgressReportIntervalMs = 100;

    Engine& engine_;
    ExportOptions options_;
    ProgressCallback progressCallback_;
    CompletionCallback completionCallback_;
    
    std::atomic<bool> shouldCancel_{false};
    std::atomic<float> currentProgress_{0.0f};

    /**
     * @brief Internal render implementation
     * @return Result indicating success or error with specific message
     */
    juce::Result performExport();

    /**
     * @brief Create an audio format writer for the given options
     * @param outputFile Target file
     * @param format Audio format to use
     * @return Writer or nullptr with error set
     */
    std::unique_ptr<juce::AudioFormatWriter> createWriter(
        const juce::File& outputFile,
        juce::AudioFormat* format,
        juce::String& outErrorMessage);

    /**
     * @brief Report progress to callback if sufficient time has elapsed
     * @param progress Current progress (0.0 - 1.0)
     * @param status Status message
     * @param lastReportTime Time of last report (updated if reported)
     */
    void reportProgress(float progress, const juce::String& status, 
                        juce::int64& lastReportTime);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportJob)
};

} // namespace zenith
