/**
 * @file ZenithKnob.h
 * @brief Beautiful custom rotary knob with gradients and animations
 *
 * Features:
 * - Circular gradient design (lighter at top, darker at bottom)
 * - Animated rotation with smooth easing
 * - Value arc indicator with glow
 * - Center indicator line
 * - Hover effects with scale and brightness
 * - Click and drag interaction
 * - Double-click to reset
 * - Tooltip with current value
 * - 60 Hz smooth animations
 * - NO JUCE default components - fully custom drawn
 *
 * Inspired by: Ableton Live, Bitwig Studio rotary controls
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

namespace zenith {

/**
 * @class ZenithKnob
 * @brief Stunning custom rotary knob with smooth animations
 */
class ZenithKnob : public juce::Component,
                   public juce::Timer
{
public:
    //==========================================================================
    ZenithKnob();
    ~ZenithKnob() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // Mouse interaction
    //==========================================================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    //==========================================================================
    // Value control
    //==========================================================================

    /**
     * @brief Set value (0.0 to 1.0)
     */
    void setValue(float newValue, bool sendNotification = true);

    /**
     * @brief Get current value (0.0 to 1.0)
     */
    float getValue() const { return value_; }

    /**
     * @brief Set value range (min, max, default)
     */
    void setRange(float min, float max, float defaultValue) [[maybe_unused]];

    /**
     * @brief Get display value (mapped to range)
     */
    float getDisplayValue() const;

    /**
     * @brief Set label text
     */
    void setLabel(const juce::String& label) { label_ = label; }

    /**
     * @brief Set value suffix (e.g., "Hz", "dB", "%")
     */
    void setSuffix(const juce::String& suffix) { suffix_ = suffix; }

    //==========================================================================
    // Callback
    //==========================================================================
    std::function<void(float)> onValueChange;

private:
    //==========================================================================
    // Drawing helpers
    //==========================================================================
    void drawKnobBody(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawValueArc(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawIndicator(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawLabel(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    //==========================================================================
    // Members
    //==========================================================================
    float value_ = 0.5f;           // Current value (0.0 to 1.0)
    float targetValue_ = 0.5f;     // Target for smooth animation
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    float defaultValue_ = 0.5f;

    juce::String label_;
    juce::String suffix_;

    // Animation state
    bool isHovered_ = false;
    bool isDragging_ = false;
    float hoverAnimation_ = 0.0f;  // 0.0 to 1.0
    float dragAnimation_ = 0.0f;   // 0.0 to 1.0

    // Interaction
    juce::Point<int> dragStartPos_;
    float dragStartValue_ = 0.0f;

    // Visual constants
    static constexpr float ROTATION_RANGE = juce::MathConstants<float>::pi * 1.5f;  // 270 degrees
    static constexpr float START_ANGLE = juce::MathConstants<float>::pi * 0.75f;    // Start at 135 degrees

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithKnob)
};

}  // namespace zenith

