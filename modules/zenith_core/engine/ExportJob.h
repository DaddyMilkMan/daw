/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

 // File: ExportJob.h
 // Brief: Asynchronous export job for background audio rendering
 *
 * This class enables non-blocking audio export by running the render loop
 * on a background thread with proper progress reporting and cancellation.
 */


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
 // Brief: Thread pool job for asynchronous audio export
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
     // Brief: Create an export job
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
     // Brief: Main job entry point (runs on thread pool thread)
     * @return jobHasFinished when done, or jobNeedsRunningAgain if more work needed
     */
    JobStatus runJob() override;

    /**
     // Brief: Request cancellation of the export
     // Note: Thread-safe. Export stops at next block boundary (~100ms worst case)
     */
    void cancel();

    /**
     // Brief: Check if cancellation was requested
     */
    bool isCancelled() const { return shouldCancel_.load(std::memory_order_acquire); }

    /**
     // Brief: Get current progress (0.0 - 1.0)
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
     // Brief: Internal render implementation
     * @return Result indicating success or error with specific message
     */
    juce::Result performExport();

    /**
     // Brief: Create an audio format writer for the given options
     * @param outputFile Target file
     * @param format Audio format to use
     * @return Writer or nullptr with error set
     */
    std::unique_ptr<juce::AudioFormatWriter> createWriter(
        const juce::File& outputFile,
        juce::AudioFormat* format,
        juce::String& outErrorMessage);

    /**
     // Brief: Report progress to callback if sufficient time has elapsed
     * @param progress Current progress (0.0 - 1.0)
     * @param status Status message
     * @param lastReportTime Time of last report (updated if reported)
     */
    void reportProgress(float progress, const juce::String& status, 
                        juce::int64& lastReportTime);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportJob)
};

} // namespace zenith
