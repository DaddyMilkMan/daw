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

// Conditional Skia components (GPU-accelerated with spring physics)
#ifdef ZENITH_USE_SKIA
    #include "../Source/ui/skia/SkiaSliderComponent.h"
    #include "../Source/ui/skia/SkiaKnobComponent.h"
    #include "../Source/ui/skia/SkiaButtonComponent.h"
#else
    // Fallback custom JUCE components
    #include "ZenithSlider.h"
    #include "ZenithKnob.h"
    #include "ZenithButton.h"
#endif

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

#ifdef ZENITH_USE_SKIA
    // GPU-accelerated Skia components with spring physics
    zenith::SkiaSliderComponent faderSlider_;      // Vertical fader for volume
    zenith::SkiaKnobComponent panKnob_;            // Rotary knob for pan
    zenith::SkiaButtonComponent muteButton_;
    zenith::SkiaButtonComponent soloButton_;
#else
    // Fallback custom JUCE components
    zenith::ZenithSlider faderSlider_;      // Vertical fader for volume
    zenith::ZenithKnob panKnob_;            // Rotary knob for pan
    zenith::ZenithButton muteButton_;
    zenith::ZenithButton soloButton_;
#endif

    // Beautiful custom level meter component with smooth animations
    class LevelMeter : public juce::Component,
                       public juce::Timer
    {
    public:
        LevelMeter();
        ~LevelMeter() override;

        void paint(juce::Graphics& g) override;
        void setLevel(float level) [[maybe_unused]];  // 0.0 to 1.0
        void timerCallback() override;

    private:
        std::atomic<float> targetLevel_{0.0f};
        float currentLevel_{0.0f};  // Animated level with smooth fall-off
        float peakLevel_{0.0f};     // Peak hold value
        int peakHoldCounter_{0};    // Frames to hold peak
    };

    LevelMeter meter_;

    // State tracking
    bool updatingControls_ = false;  // Prevent feedback loops

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelComponent)
};

