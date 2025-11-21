/**
 * @file ZenithButton.h
 * @brief Beautiful custom button with gradients and animations
 *
 * Modern button design with:
 * - Multiple styles (primary, secondary, danger, success)
 * - Rounded corners with gradient background
 * - Hover effects with glow and scaling
 * - Press animation with spring effect
 * - Toggle state support
 * - Icon and text label support
 * - Smooth 60 Hz animations
 * - NO JUCE default Button - completely custom drawn
 */

#pragma once

#include <JuceHeader.h>

namespace zenith {

/**
 * @class ZenithButton
 * @brief Beautiful custom button component
 *
 * Features:
 * - Gradient background (lighter at top, darker at bottom)
 * - Hover glow effect with scaling
 * - Press animation with spring/bounce
 * - Toggle state (can be used as toggle button)
 * - Multiple color styles
 * - Icon support (optional)
 * - Text label
 * - Disabled state
 * - 60 Hz smooth animations
 */
class ZenithButton : public juce::Component,
                     public juce::Timer
{
public:
    enum ButtonStyle
    {
        Primary,    // Apple blue
        Secondary,  // Dark gray
        Success,    // Apple green
        Danger,     // Apple red
        Warning     // Apple orange
    };

    ZenithButton(const juce::String& buttonText = juce::String());
    ~ZenithButton() override;

    //==========================================================================
    // Button configuration
    //==========================================================================

    /**
     * @brief Set the button text label
     */
    void setButtonText(const juce::String& text) { buttonText_ = text; repaint(); }

    /**
     * @brief Get the button text
     */
    juce::String getButtonText() const { return buttonText_; }

    /**
     * @brief Set the button style (Primary, Secondary, Success, Danger, Warning)
     */
    void setButtonStyle(ButtonStyle style) [[maybe_unused]] { style_ = style; repaint(); }

    /**
     * @brief Set whether this button can be toggled
     */
    void setToggleable(bool shouldBeToggleable) [[maybe_unused]] { isToggleable_ = shouldBeToggleable; }

    /**
     * @brief Get the toggle state
     */
    bool getToggleState() const { return isToggled_; }

    /**
     * @brief Set the toggle state
     * @param shouldBeToggled New toggle state
     * @param sendNotification Whether to call onClick callback
     */
    void setToggleState(bool shouldBeToggled, bool sendNotification = true);

    /**
     * @brief Enable or disable the button
     */
    void setEnabled(bool shouldBeEnabled) [[maybe_unused]] override;

    /**
     * @brief Callback when button is clicked
     */
    std::function<void()> onClick;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // Mouse interaction
    //==========================================================================

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    // Drawing methods
    //==========================================================================

    void drawButton(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    juce::Colour getBaseColour() const;
    juce::Colour getHoverColour() const;

    //==========================================================================
    // Member variables
    //==========================================================================

    juce::String buttonText_;
    ButtonStyle style_ = Primary;

    // State
    bool isToggleable_ = false;
    bool isToggled_ = false;
    bool isHovered_ = false;
    bool isPressed_ = false;

    // Animation state
    float hoverAnimation_ = 0.0f;    // 0.0 to 1.0 for smooth hover animation
    float pressAnimation_ = 0.0f;    // 0.0 to 1.0 for press animation
    float toggleAnimation_ = 0.0f;   // 0.0 to 1.0 for toggle state animation

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithButton)
};

}  // namespace zenith

