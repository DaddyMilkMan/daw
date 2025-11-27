/**
 * @file TransportControlComponent.h
 * @brief Transport controls (play/stop/record) with Apple-like design
 *
 * Features:
 * - Circular play/stop/record buttons (iOS Media Player style)
 * - Smooth press animations and feedback
 * - Real-time playback position display (HH:MM:SS.ms)
 * - Recording indicator with pulse animation
 * - Keyboard shortcut hints
 * - BPM sync visualization
 * - Apple-inspired flat design with gradients
 * - JUCE 8 animation support
 *
 * @phase Phase 2: Transport UI
 */

#pragma once

#ifdef ZENITH_USE_SKIA
    #include "../Source/ui/skia/SkiaComponent.h"
    class SkCanvas;
    struct SkRect;
#endif

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

class Engine;

namespace zenith {

/**
 * @class TransportControlComponent
 * @brief Advanced transport control UI with modern design
 */
class TransportControlComponent : public juce::Component,
                       , public zenith::SkiaComponent
                                  public juce::Timer
{
public:
    //==========================================================================
    explicit TransportControlComponent(Engine& eng);
    ~TransportControlComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Get play button bounds for hit testing
     */
    juce::Rectangle<int> getPlayButtonBounds() const;

    /**
     * @brief Get stop button bounds
     */
    juce::Rectangle<int> getStopButtonBounds() const;

    /**
     * @brief Get record button bounds
     */
    juce::Rectangle<int> getRecordButtonBounds() const;

#ifdef ZENITH_USE_SKIA
    void paintToSkia(SkCanvas* canvas, SkRect bounds) override;
    bool supportsSkiaRendering() const override { return true; }
#endif

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Draw a circular button with animations
     */
    void drawButton(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                   const juce::String& label, const juce::Colour& color,
                   bool isPressed, bool isHovered, bool isActive);

    /**
     * @brief Format time display (HH:MM:SS.ms)
     */
    juce::String formatTime(juce::int64 samples);

    /**
     * @brief Update animation state
     */
    void updateAnimationState();

    /**
     * @brief Handle play button click
     */
    void handlePlayButtonClick();

    /**
     * @brief Handle stop button click
     */
    void handleStopButtonClick();

    /**
     * @brief Handle record button click
     */
    void handleRecordButtonClick();

    //==========================================================================
    // Members
    //==========================================================================

    Engine& engine_;

    // Button states
    bool playPressed_ = false;
    bool stopPressed_ = false;
    bool recordPressed_ = false;

    bool playHovered_ = false;
    bool stopHovered_ = false;
    bool recordHovered_ = false;

    // Animation state for pulse effect (recording indicator)
    float recordPulseAnimation_ = 0.0f;

    // Button press animation (spring effect)
    float playButtonScale_ = 1.0f;
    float stopButtonScale_ = 1.0f;
    float recordButtonScale_ = 1.0f;

    // Layout constants
    static constexpr int BUTTON_SIZE = 56;
    static constexpr int BUTTON_SPACING = 24;
    static constexpr int SPACING = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportControlComponent)
};

}  // namespace zenith

