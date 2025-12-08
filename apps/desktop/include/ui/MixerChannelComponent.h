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

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../../Source/ui/skia/SkiaSlider.h"
#include "../../Source/ui/skia/SkiaKnob.h"
#include "../../Source/ui/skia/SkiaButton.h"
#include "../../Source/ui/skia/SkiaComponent.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"

// Forward declarations
namespace zenith {
    class Track;


//==============================================================================
/**
 * @class MixerChannelComponent
 * @brief Single channel strip in the mixer
 *
 * Represents one track in the mixer view with all its controls.
 * Communicates directly with the Track object via thread-safe atomics.
 */
class MixerChannelComponent : public zenith::SkiaComponent
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

    void drawSkia(SkCanvas* canvas) override;
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

    // GPU-accelerated Skia components with spring physics
    zenith::SkiaSlider faderSlider_;      // Vertical fader for volume
    zenith::SkiaKnob panKnob_;            // Rotary knob for pan
    zenith::SkiaButton muteButton_;
    zenith::SkiaButton soloButton_;

    // Beautiful custom level meter component with smooth animations
    class LevelMeter : public zenith::SkiaComponent
    {
    public:
        LevelMeter();
        ~LevelMeter() override;

        void drawSkia(SkCanvas* canvas) override;
        void setLevel(float level) [[maybe_unused]];  // 0.0 to 1.0
        void timerCallback() override;

    private:
        std::atomic<float> targetLevel_{0.0f};
        float currentLevel_{0.0f};  // Animated level with smooth fall-off
        float peakLevel_{0.0f};     // Peak hold value
        int peakHoldCounter_{0};    // Frames to hold peak
    };

    LevelMeter meter_; // Integrated Skia Level Meter

    // State tracking
    bool updatingControls_ = false;  // Prevent feedback loops

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelComponent)
};

} // namespace zenith

