/**
 * @file ExportWavJob.h
 * @brief Background worker thread for WAV export
 *
 * Runs ProjectEditorState::renderCurrentProjectToWav() on a background thread
 * to avoid blocking the UI during export. Notifies completion via polling.
 *
 * Thread Safety:
 * - run() executes on WORKER THREAD
 * - Uses SafePointer to editor state (UI thread owned)
 * - Results stored in atomics for safe cross-thread access
 */

#pragma once

#include <JuceHeader.h>

// Forward declaration
namespace zenith { class ProjectEditorState; }

/**
 * @class ExportWavJob
 * @brief Background thread for offline WAV export
 *
 * Usage:
 * ```
 * auto job = std::make_unique<ExportWavJob>(editorState, outputFile);
 * job->startThread();
 *
 * // Later (poll from timer):
 * if (!job->isThreadRunning()) {
 *     bool ok = job->wasSuccessful();
 *     // ... handle completion
 * }
 * ```
 */
class ExportWavJob : public juce::Thread
{
public:
    /**
     * @brief Constructor
     * @param editor Reference to ProjectEditorState (must outlive job)
     * @param outputFile Target WAV file path
     * @param blockSize Processing block size (default: 1024)
     * @param tailSeconds Extra time after clips for FX tails (default: 0.5)
     */
    ExportWavJob(zenith::ProjectEditorState& editor,
                 const juce::File& outputFile,
                 int blockSize = 1024,
                 double tailSeconds = 0.5);

    /**
     * @brief Destructor - ensures thread is stopped
     */
    ~ExportWavJob() override;

    /**
     * @brief Worker thread entry point
     * @note Executes on WORKER THREAD
     */
    void run() override;

    /**
     * @brief Check if export succeeded
     * @return true if export completed successfully
     */
    bool wasSuccessful() const noexcept { return success_.load(); }

    /**
     * @brief Get error message if export failed
     * @return Error message (empty if successful)
     */
    juce::String getErrorMessage() const noexcept { return errorMessage_; }

    /**
     * @brief Get output file path
     * @return Target file that was (or will be) exported
     */
    juce::File getOutputFile() const noexcept { return outputFile_; }

private:
    juce::Component::SafePointer<zenith::ProjectEditorState> editorState_;
    juce::File outputFile_;
    int blockSize_;
    double tailSeconds_;

    std::atomic<bool> success_{ false };
    juce::String errorMessage_;  // Only written from worker thread before completion

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportWavJob)
};
