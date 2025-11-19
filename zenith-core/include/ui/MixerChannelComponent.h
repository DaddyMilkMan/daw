/**
 * @file MixerChannelComponent.h
 * @brief Mixer channel strip UI component
 *
 * Displays a single mixer channel with:
 * - Track name label
 * - Vertical fader (volume)
 * - Pan control (rotary knob)
 * - Mute/Solo buttons
 * - Level meter
 *
 * Thread Safety:
 * - All UI updates on MESSAGE THREAD
 * - Reads from Track via atomics (thread-safe)
 * - Writes to Track via setters (thread-safe)
 */

#pragma once

#include <JuceHeader.h>

// Forward declarations
namespace zenith {
    class Track;
}

//==============================================================================
/**
 * @class MixerChannelComponent
 * @brief Single channel strip in the mixer
 *
 * Represents one track in the mixer view with all its controls.
 * Communicates directly with the Track object via thread-safe atomics.
 */
class MixerChannelComponent : public juce::Component,
                               private juce::Timer
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param track Pointer to the track this channel represents (NOT owned)
     */
    explicit MixerChannelComponent(zenith::Track* track);
    ~MixerChannelComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Channel operations
    //==========================================================================

    /**
     * @brief Get the track this channel represents
     */
    zenith::Track* getTrack() const { return track_; }

    /**
     * @brief Update UI from track state (called periodically via timer)
     */
    void updateFromTrack();

private:
    //==========================================================================
    // Timer interface (for meter updates)
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // UI callbacks
    //==========================================================================

    void onFaderChanged();
    void onPanChanged();
    void onMuteClicked();
    void onSoloClicked();

    //==========================================================================
    // Member variables
    //==========================================================================

    zenith::Track* track_;  // NOT owned

    // UI components
    juce::Label nameLabel_;
    juce::Slider faderSlider_;      // Vertical fader for volume
    juce::Slider panSlider_;        // Rotary knob for pan
    juce::TextButton muteButton_;
    juce::TextButton soloButton_;

    // Simple level meter component
    class LevelMeter : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override;
        void setLevel(float level);  // 0.0 to 1.0

    private:
        std::atomic<float> level_{0.0f};
    };

    LevelMeter meter_;

    // State tracking
    bool updatingControls_ = false;  // Prevent feedback loops

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelComponent)
};
