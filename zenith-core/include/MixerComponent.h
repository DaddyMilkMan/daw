/**
 * @file MixerComponent.h
 * @brief Mixer UI component for Zenith DAW
 *
 * Phase 10+11: Mixer MVP + Engine Wiring
 *
 * Displays a horizontal row of track strips, each showing:
 * - Track name
 * - Vertical volume fader
 * - Pan knob
 * - Mute/Solo/Arm buttons
 * - Level meter
 *
 * All controls are bound to ProjectState (for undo/redo) and
 * displayed levels are read from Engine (for RT-safe metering).
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "Engine.h"

//==============================================================================
/**
 * @class TrackStrip
 * @brief Single vertical track strip in the mixer
 *
 * Shows all mixer controls for one track.
 */
class TrackStrip : public juce::Component
{
public:
    TrackStrip(ProjectState& projectState, Engine& engine, int trackIndex);
    ~TrackStrip() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Update meter value from engine
     * @param level Current level (0.0 - 1.0+)
     */
    void setMeterLevel(float level);

    /**
     * @brief Refresh controls from ProjectState
     */
    void refreshFromState();

    int getTrackIndex() const { return trackIndex_; }

private:
    ProjectState& projectState_;
    Engine& engine_;
    int trackIndex_;

    juce::String trackId_;  // ProjectState track ID

    // UI Components
    juce::Label nameLabel_;
    juce::Slider volumeSlider_;
    juce::Slider panSlider_;
    juce::TextButton muteButton_;
    juce::TextButton soloButton_;
    juce::TextButton armButton_;

    // Meter display
    float meterLevel_{0.0f};

    // Callbacks
    void onVolumeChanged();
    void onPanChanged();
    void onMuteToggled();
    void onSoloToggled();
    void onArmToggled();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackStrip)
};

//==============================================================================
/**
 * @class MixerComponent
 * @brief Main mixer component showing all tracks
 *
 * Layout:
 * - Horizontal row of TrackStrip components
 * - Master strip on the right
 * - Scrollable if needed
 *
 * Updates:
 * - Timer polls engine for meter levels (30-60 Hz)
 * - Listens to ProjectState for track count changes
 */
class MixerComponent : public juce::Component,
                       private juce::Timer
{
public:
    MixerComponent(ProjectState& projectState, Engine& engine);
    ~MixerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Rebuild all track strips (call when tracks added/removed)
     */
    void rebuildTracks();

private:
    void timerCallback() override;

    ProjectState& projectState_;
    Engine& engine_;

    // Track strips
    juce::OwnedArray<TrackStrip> trackStrips_;

    // Master strip (simple for now - just level display)
    juce::Label masterLabel_;
    float masterMeterLevel_{0.0f};

    static constexpr int STRIP_WIDTH = 80;
    static constexpr int MASTER_WIDTH = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
