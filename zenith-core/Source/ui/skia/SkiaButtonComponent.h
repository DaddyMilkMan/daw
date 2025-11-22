/**
 * @file SkiaButtonComponent.h
 * @brief First proof-of-concept: Button rendered with Skia instead of JUCE
 *
 * This demonstrates the JUCE + Skia architecture:
 * - JUCE Component handles windowing, events, layout
 * - Skia handles all rendering (GPU-accelerated)
 * - Spring physics for smooth animations
 */

#pragma once

#include <JuceHeader.h>
#include "../../rendering/SkiaRenderer.h"
#include <functional>

namespace zenith {

/**
 * @class SkiaButtonComponent
 * @brief Custom button rendered entirely with Skia
 *
 * Features:
 * - GPU-accelerated rendering with blur/shadows
 * - Spring physics animations (not just easing)
 * - High refresh rate support (120Hz+)
 * - Modern Apple-inspired design
 *
 * This is the first step toward replacing all JUCE UI with Skia.
 */
class SkiaButtonComponent : public juce::Component,
                            private juce::Timer
{
public:
    /**
     * Button visual style
     */
    enum class Style
    {
        Primary,    ///< Blue accent (0xff0A84FF)
        Secondary,  ///< Gray (neutral)
        Success,    ///< Green (0xff34C759)
        Danger,     ///< Red (0xffFF3B30)
        Warning     ///< Orange (0xffFF9500)
    };

    /**
     * @brief Construct button with text
     * @param buttonText Text to display
     * @param style Visual style
     */
    explicit SkiaButtonComponent(const juce::String& buttonText = {},
                                Style style = Style::Primary);

    /// Destructor
    ~SkiaButtonComponent() override;

    //==========================================================================
    // Component Interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Mouse Events
    //==========================================================================

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set button text
     */
    void setButtonText(const juce::String& text);

    /**
     * @brief Get button text
     */
    juce::String getButtonText() const { return buttonText_; }

    /**
     * @brief Set visual style
     */
    void setStyle(Style style) [[maybe_unused]];

    /**
     * @brief Get current style
     */
    Style getStyle() const { return style_; }

    /**
     * @brief Set click callback
     */
    std::function<void()> onClick;

    /**
     * @brief Enable/disable blur effect
     * @param enable true to enable frosted glass blur
     */
    void setBlurEnabled(bool enable) [[maybe_unused]];

    /**
     * @brief Set corner radius
     * @param radius Corner radius in pixels (default: 8.0)
     */
    void setCornerRadius(float radius) [[maybe_unused]];

private:
    // Skia renderer
    std::unique_ptr<SkiaRenderer> renderer_;

    // State
    juce::String buttonText_;
    Style style_;
    bool isHovered_ = false;
    bool isPressed_ = false;

    // Visual properties
    float cornerRadius_ = 8.0f;
    bool blurEnabled_ = true;

    // Animation state (spring physics)
    float hoverProgress_ = 0.0f;      ///< 0.0 to 1.0
    float pressProgress_ = 0.0f;      ///< 0.0 to 1.0
    float hoverVelocity_ = 0.0f;
    float pressVelocity_ = 0.0f;

    // Methods
    void timerCallback() override;
    void startAnimationTimer();
    void updateAnimations();
    void drawButton(class SkCanvas* canvas);
    juce::Colour getStyleColour() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaButtonComponent)
};

} // namespace zenith

