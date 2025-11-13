/**
 * @file ProjectPlaybackContext.h
 * @brief Bridge between ProjectModel and Engine (v0.1)
 *
 * Loads arrangement data from ProjectModel and schedules playback events
 * to the Engine's W10.2 scheduler + W10.3 Track/Clip mixer.
 *
 * Thread model: MESSAGE THREAD only (all methods)
 * No real-time operations - delegates to Engine for RT scheduling.
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectModel.h"

// Forward declarations
class Engine;

namespace zenith {

/**
 * @brief Playback context for a loaded project
 *
 * Responsibilities:
 * - Load ProjectModel into Engine (tracks, clips, FX)
 * - Schedule clip playback events from current transport position
 * - Handle seek operations (mid-clip offset handling)
 * - Apply track-level mute/gain/pan settings
 *
 * MESSAGE THREAD only - no RT operations
 * All RT work delegated to Engine's scheduler + mixer
 */
class ProjectPlaybackContext
{
public:
    /**
     * @brief Construct playback context
     * @param engine Reference to audio engine
     */
    explicit ProjectPlaybackContext(Engine& engine);
    ~ProjectPlaybackContext();

    //==========================================================================
    // Project Loading (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Load arrangement from ProjectModel
     * @param model Project data to load
     * @param projectRoot Root directory for resolving relative clip paths
     *
     * Performs:
     * 1. Stops playback if running
     * 2. Clears existing arrangement
     * 3. Creates tracks (Mixer::setNumTracks)
     * 4. Loads clips into Track voice pool
     * 5. Configures track FX (gain/pan only in v0.1)
     *
     * MESSAGE THREAD only
     * MUST be called when playback is stopped
     */
    void loadFromModel(const ProjectModel& model,
                       const juce::File& projectRoot);

    /**
     * @brief Clear loaded arrangement
     *
     * Stops playback, clears all tracks/clips.
     * MESSAGE THREAD only
     */
    void clear();

    //==========================================================================
    // Transport Control (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Start playback from timeline position
     * @param startSample Absolute timeline position (samples)
     *
     * Schedules all active clips from startSample onwards.
     * Handles mid-clip start offsets correctly.
     *
     * MESSAGE THREAD only
     */
    void playFrom(SamplePos startSample);

    /**
     * @brief Stop playback
     *
     * MESSAGE THREAD only
     */
    void stop();

    /**
     * @brief Seek to timeline position
     * @param targetSample Absolute timeline position (samples)
     *
     * Updates transport position.
     * If playing, reschedules events from new position.
     *
     * MESSAGE THREAD only
     */
    void seek(SamplePos targetSample);

    //==========================================================================
    // State Query (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Get current transport position
     * @return Absolute timeline position (samples)
     */
    SamplePos getTransportSamples() const;

    /**
     * @brief Check if playback is active
     * @return true if playing
     */
    bool isPlaying() const;

    /**
     * @brief Get loaded project data
     * @return Reference to current ProjectModel (empty if not loaded)
     */
    const ProjectModel& getModel() const noexcept { return model_; }

private:
    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /**
     * @brief Schedule all clips from timeline position onwards
     * @param startSample Absolute timeline position
     *
     * Iterates all tracks/clips, schedules ClipStart events.
     * Handles mid-clip offsets for clips that overlap startSample.
     */
    void scheduleEventsFrom(SamplePos startSample);

    /**
     * @brief Load single clip into track voice pool
     * @param trackIndex Target track index
     * @param clip Clip data to load
     * @param projectRoot Root directory for path resolution
     * @return true if clip loaded successfully
     */
    bool loadClipIntoTrack(int trackIndex,
                           const ClipModel& clip,
                           const juce::File& projectRoot);

    //==========================================================================
    // Member Variables
    //==========================================================================

    Engine& engine_;                   ///< Reference to audio engine
    ProjectModel model_;               ///< Loaded project data (empty if not loaded)
    juce::File projectRoot_;           ///< Root directory for clip path resolution

    bool isLoaded_ = false;            ///< true if project loaded successfully

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectPlaybackContext)
};

} // namespace zenith
