/*
  ==============================================================================

    SessionController.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Bridge between SkiaSessionView and the engine.
    
    Connects the view to:
    - ProjectState (tracks, clips)
    - TransportController (playhead, quantization)
    - Engine (clip launching, meters)

  ==============================================================================
*/

#pragma once

#include "../../../engine/ProjectState.h"
#include "../../../engine/TransportController.h"
#include "../../../engine/Engine.h"
#include "../session/SkiaSessionView.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

namespace zenith::ui {

/**
 * @brief Launch quantization options
 */
enum class LaunchQuantize {
    None,       ///< Immediate
    Bar,        ///< Next bar
    HalfBar,    ///< Next half bar
    Beat,       ///< Next beat
    HalfBeat,   ///< Next eighth note
    Quarter     ///< Next sixteenth note
};

/**
 * @class SessionController
 * @brief Connects SkiaSessionView to engine state
 */
class SessionController : public juce::ValueTree::Listener,
                           public juce::Timer {
public:
    SessionController(SkiaSessionView& view,
                      zenith::Engine& engine,
                      zenith::ProjectState& projectState);
    ~SessionController() override;

    //==========================================================================
    // Quantization
    //==========================================================================

    void setLaunchQuantize(LaunchQuantize q) { launchQuantize_ = q; }
    LaunchQuantize getLaunchQuantize() const { return launchQuantize_; }

    //==========================================================================
    // Clip Launching
    //==========================================================================

    /**
     * @brief Launch a clip at the specified slot
     */
    void launchClip(int trackIndex, int sceneIndex);

    /**
     * @brief Stop clip on a track
     */
    void stopClip(int trackIndex);

    /**
     * @brief Launch entire scene
     */
    void launchScene(int sceneIndex);

    /**
     * @brief Stop all clips
     */
    void stopAllClips();

    //==========================================================================
    // Recording
    //==========================================================================

    /**
     * @brief Arm a slot for recording
     */
    void armSlot(int trackIndex, int sceneIndex);

    /**
     * @brief Start recording into armed slots
     */
    void startRecording();

    /**
     * @brief Stop recording
     */
    void stopRecording();

    //==========================================================================
    // Transport
    //==========================================================================

    void play();
    void stop();
    void togglePlayback();

    //==========================================================================
    // Track Operations
    //==========================================================================

    void setTrackMute(int trackIndex, bool muted);
    void setTrackSolo(int trackIndex, bool soloed);
    void setTrackVolume(int trackIndex, float volume);
    void setTrackPan(int trackIndex, float pan);

    //==========================================================================
    // Sync
    //==========================================================================

    /**
     * @brief Full sync from project state
     */
    void fullSync();

    /**
     * @brief Sync meter levels from engine
     */
    void syncMeters();

    /**
     * @brief Sync clip playback states
     */
    void syncClipStates();

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Timer (for meters and clip state updates)
    //==========================================================================

    void timerCallback() override;

private:
    SkiaSessionView& view_;
    zenith::Engine& engine_;
    zenith::ProjectState& projectState_;

    LaunchQuantize launchQuantize_ = LaunchQuantize::Bar;

    // Track ID to index mapping
    std::vector<juce::String> trackIds_;

    // Scene data
    std::vector<SceneData> scenes_;

    // Clip states for each track/scene
    std::map<std::pair<int, int>, juce::String> slotToClipId_;
    
    // Clip metadata for view updates
    std::map<juce::String, juce::String> clipNames_;
    std::map<juce::String, juce::Colour> clipColors_;
    
    // Currently playing clips (trackIndex -> slot)
    std::map<int, std::pair<int, int>> activeClips_;

    // Selected clip slot
    std::pair<int, int> selectedSlot_ = {-1, -1};

    double tempo_ = 120.0;
    double sampleRate_ = 44100.0;

    //==========================================================================
    // Re-entrancy Guard
    //==========================================================================

    // Guard against re-entrant ValueTree callbacks that could cause infinite loops
    bool isProcessingValueTreeChange_ = false;

    //==========================================================================
    // Internal Methods
    //==========================================================================

    void rebuildTrackData();
    void rebuildSceneData();
    void rebuildClipSlots();
    void updateViewData();

    SessionTrackData extractTrackData(const juce::ValueTree& trackTree, int index) const;
    ClipSlotData extractClipSlotData(const juce::ValueTree& clipTree) const;

    juce::int64 getNextQuantizedPosition() const;
    double samplesToBeats(juce::int64 samples) const;
    juce::int64 beatsToSamples(double beats) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionController)
};

} // namespace zenith::ui
