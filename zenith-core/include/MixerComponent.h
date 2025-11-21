/**
 * @file MixerComponent.h
 * @brief Mixer panel component for Zenith DAW
 *
 * Displays track mixer controls:
 * - Volume fader
 * - Pan knob
 * - Mute/Solo/Arm buttons
 * - Track name label
 *
 * Phase 10: Mixer MVP
 * - Basic mixer UI with track strips
 * - Integration with ProjectState
 * - Undo/redo support
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include <memory>
#include <vector>

//==============================================================================
/**
 * @class MixerComponent
 * @brief Mixer panel with vertical track strips
 *
 * Shows one vertical strip per track with:
 * - Track name label
 * - Volume fader (vertical)
 * - Pan knob (rotary)
 * - Mute/Solo/Arm buttons
 *
 * All changes are routed through ProjectState and are undoable.
 */
class MixerComponent : public juce::Component,
                       private juce::ValueTree::Listener
{
public:
    //==========================================================================
    explicit MixerComponent(ProjectState& projectState);
    ~MixerComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    // Track Strip
    //==========================================================================

    /**
     * @struct TrackStrip
     * @brief UI components for a single track
     */
    struct TrackStrip
    {
        juce::String trackId;
        juce::String trackName;

        std::unique_ptr<juce::Label>        nameLabel;
        std::unique_ptr<juce::Slider>       volumeSlider;
        std::unique_ptr<juce::Slider>       panSlider;
        std::unique_ptr<juce::ToggleButton> muteButton;
        std::unique_ptr<juce::ToggleButton> soloButton;
        std::unique_ptr<juce::ToggleButton> armButton;

        juce::Rectangle<int> bounds;

        TrackStrip() = default;
        ~TrackStrip() = default;

        // Delete copy and move to prevent issues with unique_ptr
        TrackStrip(const TrackStrip&) = delete;
        TrackStrip& operator=(const TrackStrip&) = delete;
        TrackStrip(TrackStrip&&) = default;
        TrackStrip& operator=(TrackStrip&&) = default;
    };

    //==========================================================================
    // ValueTree::Listener interface
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                  const juce::Identifier& property) override;

    void valueTreeChildAdded(juce::ValueTree& parentTree,
                            juce::ValueTree& childWhichHasBeenAdded) override;

    void valueTreeChildRemoved(juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenRemoved,
                              int indexFromWhichChildWasRemoved) override;

    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
                                   int oldIndex,
                                   int newIndex) override;

    void valueTreeParentChanged(juce::ValueTree& treeWhoseParentHasChanged) override {}
    void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override {}

    //==========================================================================
    // Helper methods
    //==========================================================================

    /**
     * @brief Rebuild all track strips from current ProjectState
     */
    void rebuildTrackStrips();

    /**
     * @brief Create a new track strip for a track
     */
    std::unique_ptr<TrackStrip> createTrackStrip(const juce::ValueTree& trackNode);

    /**
     * @brief Update a track strip from ProjectState
     */
    void updateTrackStripFromState(TrackStrip& strip, const juce::ValueTree& trackNode);

    /**
     * @brief Find track strip by track ID
     */
    TrackStrip* findTrackStrip(const juce::String& trackId);

    //==========================================================================
    // Control callbacks
    //==========================================================================

    void onVolumeChanged(const juce::String& trackId, float value) [[maybe_unused]];
    void onPanChanged(const juce::String& trackId, float value) [[maybe_unused]];
    void onMuteClicked(const juce::String& trackId, bool state) [[maybe_unused]];
    void onSoloClicked(const juce::String& trackId, bool state) [[maybe_unused]];
    void onArmClicked(const juce::String& trackId, bool state) [[maybe_unused]];

    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;

    std::vector<std::unique_ptr<TrackStrip>> trackStrips;

    // UI constants
    static constexpr int stripWidth = 80;
    static constexpr int stripSpacing = 4;
    static constexpr int topMargin = 10;
    static constexpr int bottomMargin = 10;
    static constexpr int sideMargin = 10;

    // Flag to prevent feedback loops
    bool updatingFromState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};

