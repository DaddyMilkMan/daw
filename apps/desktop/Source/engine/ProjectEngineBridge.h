/*
  ==============================================================================

    ProjectEngineBridge.h
    Created: 2025-12-11
    Author:  Zenith DAW

    Consolidated synchronization between ProjectState (ValueTree) and Engine.
    
    Replaces the following classes:
    - ClipSynchronizer
    - TrackStateSynchronizer
    - TrackAutomationSynchronizer
    - TempoMapSynchronizer

    Thread Safety:
    - All methods are MESSAGE THREAD ONLY
    - Uses batched updates for efficiency
    - Listens to ValueTree changes and dispatches to Engine

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <memory>

namespace zenith {

// Forward declarations
class ProjectState;
class Engine;
class TempoMap;

//==============================================================================
/**
    Consolidated bridge between ProjectState (ValueTree) and Engine.
    
    Features:
    - Single listener for all ValueTree changes
    - Batched updates for efficiency
    - Timer-based automation sampling during playback
    - Explicit commit calls for Engine → ProjectState direction
*/
class ProjectEngineBridge : public juce::ValueTree::Listener,
                             public juce::Timer {
public:
    //==========================================================================
    ProjectEngineBridge(ProjectState& projectState, Engine& engine);
    ~ProjectEngineBridge() override;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /**
     * @brief Start listening to ProjectState changes
     * @param automationUpdateRateHz Rate for automation sampling during playback
     * @note MESSAGE THREAD ONLY
     */
    void start(int automationUpdateRateHz = 60);

    /**
     * @brief Stop listening and cleanup
     * @note MESSAGE THREAD ONLY
     */
    void stop();

    /**
     * @brief Force full synchronization of all state
     * @note MESSAGE THREAD ONLY
     */
    void forceFullSync();

    //==========================================================================
    // Explicit Commits (Engine → ProjectState)
    //==========================================================================

    /**
     * @brief Commit transport position to ProjectState
     * @note Call after recording stops to save final position
     */
    void commitTransportPosition();

    /**
     * @brief Commit recording results to ProjectState (create clips)
     * @note Called by Engine::stopRecording()
     */
    void commitRecordingResults();

    //==========================================================================
    // ValueTree::Listener Overrides
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, 
                                   const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, 
                              juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, 
                                juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, 
                                     int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

protected:
    //==========================================================================
    // Timer Override
    //==========================================================================

    void timerCallback() override;

private:
    //==========================================================================
    // Sync Helpers
    //==========================================================================

    /** Sync a track property change to Engine */
    void syncTrackProperty(const juce::ValueTree& track, 
                          const juce::Identifier& property);

    /** Sync a clip property change to Engine */
    void syncClipProperty(const juce::ValueTree& clip,
                         const juce::Identifier& property);

    /** Sync tempo map changes */
    void syncTempoMap();

    /** Update Engine automation during playback */
    void updatePlaybackAutomation();

    /** Get engine track index for a ProjectState track ID */
    int getEngineTrackIndex(const juce::String& trackId) const;

    /** Sample automation envelope at a given beat position */
    double sampleEnvelope(const juce::ValueTree& envelope, double timeBeats) const;

    //==========================================================================
    // Re-entrancy Protection
    //==========================================================================

    /** Prevent infinite recursion when modifying state */
    bool isModifyingState_{false};

    //==========================================================================
    // References
    //==========================================================================

    ProjectState& projectState_;
    Engine& engine_;

    //==========================================================================
    // State
    //==========================================================================

    bool isActive_{false};
    
    /** Cache of track ID to engine index for fast lookup */
    std::map<juce::String, int> trackIdToIndex_;

    //==========================================================================
    // Pending Changes (for batching)
    //==========================================================================

    struct PendingChange {
        enum class Type { TrackProperty, ClipProperty, TempoMap, TrackAdded, TrackRemoved };
        Type type;
        juce::String trackId;
        juce::String clipId;
        juce::Identifier property;
    };

    std::vector<PendingChange> pendingChanges_;
    juce::CriticalSection pendingLock_;

    /** Process pending changes (called from timer) */
    void processPendingChanges();

    /** Rebuild track ID to index mapping */
    void rebuildTrackMapping();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectEngineBridge)
};

} // namespace zenith
