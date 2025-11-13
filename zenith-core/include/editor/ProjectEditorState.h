/**
 * @file ProjectEditorState.h
 * @brief High-level coordinator for project editing, playback, and file I/O
 *
 * This class sits between the UI and the lower-level components:
 * - Owns the ProjectModel (in-memory project data)
 * - Owns the ArrangementPlaybackController (model → engine bridge)
 * - Manages project file association (save/load)
 * - Provides unified API for UI layer
 *
 * Thread Safety:
 * - MESSAGE THREAD ONLY
 * - All methods assume message thread
 */

#pragma once

#include <model/ProjectModel.h>
#include <playback/ArrangementPlaybackController.h>
#include <JuceHeader.h>

namespace zenith
{
    // Forward declarations
    class Engine;

    /**
     * @class ProjectEditorState
     * @brief High-level project state coordinator
     *
     * Responsibilities:
     * - Own and manage ProjectModel
     * - Own ArrangementPlaybackController for playback
     * - Handle save/load operations
     * - Track current project file
     * - Provide unified API for UI
     *
     * Usage:
     * ```
     * ProjectEditorState state(engine);
     * state.loadFromFile(File("project.zenithproj"));
     * state.playFromStart();
     * state.save();
     * ```
     */
    class ProjectEditorState
    {
    public:
        using SamplePos = juce::int64;

        /**
         * @brief Constructor
         * @param engine Reference to audio engine
         */
        explicit ProjectEditorState(Engine& engine);

        //==========================================================================
        // Project Management
        //==========================================================================

        /**
         * @brief Set the current project (replaces model and reloads playback)
         * @param project New project to load
         * @note Stops playback, resets transport to 0
         */
        void setProject(const ProjectModel& project);

        /**
         * @brief Get current project model
         * @return Const reference to internal project
         */
        const ProjectModel& getProject() const noexcept { return model_; }

        /**
         * @brief Get mutable project model (for editing)
         * @return Reference to internal project
         * @note After editing, call reloadPlayback() to apply changes to engine
         */
        ProjectModel& getProjectMutable() noexcept { return model_; }

        /**
         * @brief Reload playback from current model
         * @note Call this after editing the project model
         */
        void reloadPlayback();

        /**
         * @brief Get project sample rate
         * @return Sample rate in Hz
         */
        double getSampleRate() const noexcept { return model_.sampleRate; }

        //==========================================================================
        // Clip Editing Operations
        //==========================================================================

        /**
         * @brief Delete a clip from a track
         * @param trackIndex Track index
         * @param clipIndex Clip index within track
         * @return true if successful, false if indices invalid
         */
        bool deleteClip(int trackIndex, int clipIndex);

        /**
         * @brief Duplicate a clip on the same track
         * @param trackIndex Track index
         * @param clipIndex Clip index within track
         * @param offsetSamples Offset for duplicated clip (added to original startSample)
         * @return true if successful, false if indices invalid
         */
        bool duplicateClip(int trackIndex, int clipIndex, juce::int64 offsetSamples);

        /**
         * @brief Insert a new clip from an audio file
         * @param trackIndex Target track (creates track 0 if no tracks exist)
         * @param startSample Start position in timeline
         * @param audioFile Audio file to import
         * @return true if successful, false if file invalid or other error
         */
        bool insertClipFromFile(int trackIndex, juce::int64 startSample, const juce::File& audioFile);

        /**
         * @brief Trim left edge of clip (non-destructive)
         * @param trackIndex Track index
         * @param clipIndex Clip index within track
         * @param newStartSample New start position (increases srcOffset and startSample)
         * @return true if successful, false if indices invalid
         * @note For v0.1: cannot extend left beyond original start
         */
        bool trimClipLeft(int trackIndex, int clipIndex, juce::int64 newStartSample);

        /**
         * @brief Trim right edge of clip (non-destructive)
         * @param trackIndex Track index
         * @param clipIndex Clip index within track
         * @param newEndSample New end position (changes lengthSamples)
         * @return true if successful, false if indices invalid
         */
        bool trimClipRight(int trackIndex, int clipIndex, juce::int64 newEndSample);

        //==========================================================================
        // Constants
        //==========================================================================

        static constexpr juce::int64 kMinClipLengthSamples = 128;  // ~3ms @ 44.1kHz

        //==========================================================================
        // File I/O
        //==========================================================================

        /**
         * @brief Get current project file
         * @return File (may be invalid if no file associated yet)
         */
        const juce::File& getCurrentProjectFile() const noexcept { return currentProjectFile_; }

        /**
         * @brief Check if project has an associated file
         * @return true if project file exists on disk
         */
        bool hasProjectFile() const noexcept;

        /**
         * @brief Save to specific file (Save As)
         * @param file Target file path
         * @return Result indicating success or failure
         * @note On success, this becomes the current project file
         */
        juce::Result saveToFile(const juce::File& file);

        /**
         * @brief Save to current project file
         * @return Result indicating success or failure
         * @note Fails if no project file associated (use saveToFile first)
         */
        juce::Result save();

        /**
         * @brief Load project from file
         * @param file Project file to load
         * @return Result indicating success or failure
         * @note Stops playback, replaces model, reloads playback
         */
        juce::Result loadFromFile(const juce::File& file);

        //==========================================================================
        // Offline Export
        //==========================================================================

        /**
         * @brief Render current project to WAV file (offline export)
         * @param outputFile Target WAV file path
         * @param blockSize Processing block size (default: 1024)
         * @param tailSeconds Extra time after last clip for FX tails (default: 0.0)
         * @return Result indicating success or failure
         * @note Stops playback during export, uses throwaway engine
         */
        juce::Result renderCurrentProjectToWav(const juce::File& outputFile,
                                               int blockSize = 1024,
                                               double tailSeconds = 0.0);

        //==========================================================================
        // Transport Controls (delegates to playback controller)
        //==========================================================================

        /**
         * @brief Start playback from sample 0
         */
        void playFromStart();

        /**
         * @brief Start playback from specific sample
         * @param startSample Sample position to start from
         */
        void playFromSamples(SamplePos startSample);

        /**
         * @brief Stop playback and reset to 0
         */
        void stop();

        /**
         * @brief Pause playback (keep position)
         */
        void pause();

        /**
         * @brief Get current transport position
         * @return Transport position in samples
         */
        SamplePos getTransportSamples() const;

        /**
         * @brief Check if playing
         * @return true if transport is playing
         */
        bool isPlaying() const;

        /**
         * @brief Set playhead position without changing play state
         * @param sample Target sample position
         * @note Just seeks the transport, doesn't start/stop playback
         */
        void setPlayheadSamples(SamplePos sample);

        /**
         * @brief Start playback from current playhead position
         * @note Convenience method for UI (e.g., spacebar play/pause)
         */
        void playFromPlayhead();

        //==========================================================================
        // Playback Controller Access (for advanced use)
        //==========================================================================

        /**
         * @brief Get playback controller
         * @return Reference to internal playback controller
         */
        ArrangementPlaybackController& getPlaybackController() noexcept { return playback_; }

        /**
         * @brief Get playback controller (const)
         * @return Const reference to internal playback controller
         */
        const ArrangementPlaybackController& getPlaybackController() const noexcept { return playback_; }

    private:
        //==========================================================================
        // Member Variables
        //==========================================================================

        ProjectModel model_;                        // In-memory project data
        ArrangementPlaybackController playback_;    // Playback controller (owns engine ref)
        juce::File currentProjectFile_;             // Current project file (empty = no file)

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectEditorState)
    };

} // namespace zenith
