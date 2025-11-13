/**
 * @file ProjectEditorState.h
 * @brief Editor state layer between UI and Engine (v0.1)
 *
 * Responsibilities:
 * - Own current ProjectModel (in-memory copy)
 * - Own ProjectPlaybackContext (bridge to Engine)
 * - Track UI state (playhead position, selection)
 * - Provide editing operations (moveClipStart, etc.)
 *
 * Thread model: MESSAGE THREAD only
 */

#pragma once

#include <JuceHeader.h>
#include "../model/ProjectModel.h"
#include "../model/ProjectPlaybackContext.h"

// Forward declarations
class Engine;

namespace zenith {

/**
 * @brief Editor state for a loaded project
 *
 * Central hub for UI ↔ Model ↔ Engine communication.
 * Owns the ProjectModel and delegates playback to ProjectPlaybackContext.
 *
 * MESSAGE THREAD only - all methods
 */
class ProjectEditorState
{
public:
    using SamplePos = int64_t;

    /**
     * @brief Construct editor state
     * @param engine Reference to audio engine
     */
    explicit ProjectEditorState(Engine& engine);
    ~ProjectEditorState();

    //==========================================================================
    // Project Model Access
    //==========================================================================

    /**
     * @brief Get current project (read-only)
     * @return Reference to current ProjectModel
     */
    const ProjectModel& getProject() const noexcept { return model_; }

    /**
     * @brief Get current project (mutable)
     * @return Reference to current ProjectModel
     *
     * For direct in-place edits. Changes take effect on next play.
     * MESSAGE THREAD only
     */
    ProjectModel& getProject() noexcept { return model_; }

    /**
     * @brief Replace entire project
     * @param newProject New project data
     *
     * Stops playback, replaces model, reloads playback context.
     * MESSAGE THREAD only
     */
    void setProject(const ProjectModel& newProject);

    /**
     * @brief Set project root directory
     * @param root Root directory for resolving relative clip paths
     *
     * MESSAGE THREAD only
     */
    void setProjectRoot(const juce::File& root) { projectRoot_ = root; }

    /**
     * @brief Get project root directory
     * @return Root directory for clip path resolution
     */
    const juce::File& getProjectRoot() const noexcept { return projectRoot_; }

    //==========================================================================
    // Playhead Control
    //==========================================================================

    /**
     * @brief Set playhead position
     * @param pos Absolute timeline position (samples)
     *
     * MESSAGE THREAD only
     */
    void setPlayheadSamples(SamplePos pos) noexcept;

    /**
     * @brief Get playhead position
     * @return Absolute timeline position (samples)
     *
     * Safe to call from any thread
     */
    SamplePos getPlayheadSamples() const noexcept { return playhead_; }

    //==========================================================================
    // Transport Control
    //==========================================================================

    /**
     * @brief Start playback
     *
     * Reloads playback context from model, starts from current playhead.
     * MESSAGE THREAD only
     */
    void play();

    /**
     * @brief Stop playback
     *
     * MESSAGE THREAD only
     */
    void stop();

    /**
     * @brief Play from current playhead position
     *
     * Convenience method: reloads playback, plays from cursor.
     * MESSAGE THREAD only
     */
    void playFromCursor();

    /**
     * @brief Check if playback is active
     * @return true if playing
     */
    bool isPlaying() const;

    //==========================================================================
    // Editing Operations (MESSAGE THREAD ONLY)
    //==========================================================================

    /**
     * @brief Move clip start position
     * @param trackIndex Track index (0-based)
     * @param clipIndex Clip index within track (0-based, positional)
     * @param newStart New start position (samples)
     *
     * Updates model in-place. Changes take effect on next play.
     * v0.1: Only allowed while NOT playing.
     *
     * MESSAGE THREAD only
     */
    void moveClipStart(int trackIndex, int clipIndex, SamplePos newStart);

    // v0.2+: Add more editing operations (resize, delete, duplicate, etc.)

private:
    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /**
     * @brief Reload playback context from current model
     *
     * Stops playback, clears existing arrangement, loads from model_.
     * Called before starting playback to sync model → engine.
     *
     * MESSAGE THREAD only
     */
    void reloadPlayback_();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectModel model_;                 ///< Current project data (in-memory copy)
    ProjectPlaybackContext playback_;    ///< Playback bridge to Engine
    juce::File projectRoot_;             ///< Root directory for clip path resolution

    SamplePos playhead_ = 0;             ///< Current playhead position (samples)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectEditorState)
};

} // namespace zenith
