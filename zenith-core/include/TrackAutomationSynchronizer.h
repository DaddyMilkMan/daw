/**
 * @file TrackAutomationSynchronizer.h
 * @brief Synchronizes automation data from ProjectState to Track atomics
 *
 * Phase 13: Track Automation MVP
 *
 * This class bridges the message-thread ProjectState (ValueTree) with the
 * audio-thread Track objects (atomics). It:
 * - Listens to ProjectState automation changes
 * - Samples automation curves at the current playback position
 * - Updates Track volume/pan/mute atomics in an RT-safe manner
 *
 * Thread Safety:
 * - Listens to ValueTree on MESSAGE THREAD
 * - Writes to atomics from MESSAGE THREAD (via timer)
 * - Audio thread reads atomics (lock-free, safe)
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "Engine.h"
#include <map>
#include <memory>

//==============================================================================
/**
 * @class TrackAutomationSynchronizer
 * @brief Syncs automation from ProjectState to Engine tracks
 *
 * This class runs a timer on the message thread that:
 * 1. Gets the current playback position from Engine (in samples)
 * 2. Converts to beats using tempo
 * 3. Samples automation envelopes at that beat position
 * 4. Writes sampled values to Track atomics
 *
 * The audio thread then reads these atomics without any locking.
 */
class TrackAutomationSynchronizer : public juce::Timer,
                                     private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state (must outlive this object)
     * @param engine Reference to audio engine (must outlive this object)
     */
    TrackAutomationSynchronizer(ProjectState& projectState, Engine& engine);

    /**
     * @brief Destructor
     */
    ~TrackAutomationSynchronizer() override;

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

private:
    //==========================================================================
    // Timer callback (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Timer callback - samples automation and updates tracks
     * @note Called on MESSAGE THREAD at regular intervals
     */
    void timerCallback() override;

    //==========================================================================
    // ValueTree::Listener (MESSAGE THREAD)
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Sample automation envelope at a specific time
     * @param envelope Envelope ValueTree
     * @param timeBeats Time in beats
     * @return Interpolated value
     */
    double sampleEnvelope(const juce::ValueTree& envelope, double timeBeats) const;

    /**
     * @brief Update automation for a specific track
     * @param trackId Track ID from ProjectState
     * @param track Track object to update
     * @param playbackBeats Current playback position in beats
     */
    void updateTrackAutomation(const juce::String& trackId, zenith::Track* track, double playbackBeats);

    /**
     * @brief Rebuild automation listeners
     * @note Call when tracks change or automation structure changes
     */
    void rebuildListeners();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    Engine& engine;

    // Track ID mapping (ProjectState ID -> Engine track index)
    std::map<juce::String, int> trackIdToIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackAutomationSynchronizer)
};
