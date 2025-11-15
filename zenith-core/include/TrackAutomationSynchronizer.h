/**
 * @file TrackAutomationSynchronizer.h
 * @brief RT-safe automation synchronization between ProjectState and Engine
 *
 * Phase 13: Track Automation MVP
 *
 * This class runs on the message thread and samples automation curves from
 * ProjectState, writing the results to Track atomics that the audio thread
 * can read safely.
 *
 * Design:
 * - Runs on a Timer (60Hz on message thread)
 * - Reads ValueTree automation data (message thread only)
 * - Samples curves based on current playback position
 * - Writes to Track atomics (volume, pan, mute)
 * - Audio thread only reads atomics - no locks, no allocations
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

class ProjectState;
class Engine;

namespace zenith {
    class Track;
}

//==============================================================================
/**
 * @class TrackAutomationSynchronizer
 * @brief Samples automation curves and updates track atomics (RT-safe)
 *
 * This class bridges the gap between:
 * - ProjectState (ValueTree, message thread only)
 * - Track atomics (read by audio thread, written by message thread)
 *
 * Thread safety:
 * - All methods run on MESSAGE THREAD only
 * - Updates std::atomic values in Track objects
 * - Audio thread reads those atomics lock-free
 */
class TrackAutomationSynchronizer : public juce::Timer
{
public:
    //==========================================================================
    TrackAutomationSynchronizer(ProjectState& projectState, Engine& engine);
    ~TrackAutomationSynchronizer() override;

    //==========================================================================
    // Control
    //==========================================================================

    /**
     * @brief Start automation synchronization
     * @param updateRateHz Update rate in Hz (default 60)
     */
    void start(int updateRateHz = 60);

    /**
     * @brief Stop automation synchronization
     */
    void stop();

    /**
     * @brief Check if synchronizer is running
     */
    bool isRunning() const { return isTimerRunning(); }

    //==========================================================================
    // Timer callback (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Called periodically to sample automation and update tracks
     * @note Runs on MESSAGE THREAD only
     */
    void timerCallback() override;

private:
    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Sample automation for a single track
     * @param track Track to update
     * @param trackId Track ID in ProjectState
     * @param timeBeats Current time in beats
     */
    void sampleTrackAutomation(zenith::Track* track, const juce::String& trackId, double timeBeats);

    /**
     * @brief Sample a single automation parameter
     * @param trackId Track ID
     * @param paramId Parameter ID ("volume", "pan", "mute")
     * @param timeBeats Current time in beats
     * @return Sampled value (interpolated between points)
     */
    double sampleAutomationValue(const juce::String& trackId, const juce::String& paramId, double timeBeats);

    /**
     * @brief Linear interpolation between two automation points
     */
    double interpolate(double time1, double value1, double time2, double value2, double currentTime);

    /**
     * @brief Build track ID map from ProjectState
     * @note Called when synchronizer starts or when tracks change
     */
    void rebuildTrackIdMap();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    Engine& engine;

    // Map of track indices to track IDs (for fast lookup)
    std::vector<juce::String> trackIdMap;

    // Last known track count (to detect changes)
    int lastTrackCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackAutomationSynchronizer)
};
