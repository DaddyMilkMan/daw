/**
 * @file ZenithButton.cpp
 * @brief Implementation of beautiful custom button component
 * 
 * DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens
 */

#include "ui/ZenithButton.h"
#include "AudioFeedback.h"
#include "ZenithLookAndFeel.h"  // DESIGN SYSTEM: Include for design tokens

namespace zenith {

ZenithButton::ZenithButton(const juce::String& buttonText)
    : buttonText_(buttonText)
{
    // Start animation timer at 60 Hz
    startTimerHz(60);
}

ZenithButton::~ZenithButton()
{
    stopTimer();
}

//==============================================================================
// Button configuration
//==============================================================================

void ZenithButton::setToggleState(bool shouldBeToggled, bool sendNotification)
{
    if (isToggled_ == shouldBeToggled) return;

    isToggled_ = shouldBeToggled;

    if (sendNotification && onClick)
    {
        onClick();
    }

    repaint();
}

void ZenithButton::setEnabled(bool shouldBeEnabled)
{
    Component::setEnabled(shouldBeEnabled);
    repaint();
}

//==============================================================================
// Component interface
//==============================================================================

void ZenithButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawButton(g, bounds);
}

void ZenithButton::resized()
{
    // No special resizing needed
}

void ZenithButton::timerCallback()
{
    const float animationSpeed = 0.2f;

    // Hover animation
    float targetHover = isHovered_ ? 1.0f : 0.0f;
    hoverAnimation_ += (targetHover - hoverAnimation_) * animationSpeed;

    // Press animation (spring effect)
    float targetPress = isPressed_ ? 1.0f : 0.0f;
    pressAnimation_ += (targetPress - pressAnimation_) * animationSpeed * 1.5f;

    // Toggle animation
    float targetToggle = isToggled_ ? 1.0f : 0.0f;
    toggleAnimation_ += (targetToggle - toggleAnimation_) * animationSpeed;

    // Repaint if any animation is active
    if (std::abs(hoverAnimation_ - targetHover) > 0.01f ||
        std::abs(pressAnimation_ - targetPress) > 0.01f ||
        std::abs(toggleAnimation_ - targetToggle) > 0.01f)
    {
        repaint();
    }
}

//==============================================================================
// Mouse interaction
//==============================================================================

void ZenithButton::mouseDown(const juce::MouseEvent& event)
{
    if (!isEnabled()) return;

    isPressed_ = true;
}

void ZenithButton::mouseUp(const juce::MouseEvent& event)
{
    if (!isEnabled()) return;

    isPressed_ = false;

    // Only trigger click if mouse is still over button
    if (event.mouseWasClicked() && getLocalBounds().contains(event.getPosition()))
    {
        if (isToggleable_)
        {
            // Play toggle sound (two-tone)
            AudioFeedback::getInstance().playSound(AudioFeedback::Toggle, 0.25f);
            setToggleState(!isToggled_, true);
        }
        else
        {
            // Play subtle click sound for normal buttons
            AudioFeedback::getInstance().playSound(AudioFeedback::Click, 0.25f);

            if (onClick)
            {
                onClick();
            }
        }
    }
}

void ZenithButton::mouseEnter(const juce::MouseEvent& event)
{
    if (!isEnabled()) return;

    isHovered_ = true;
}

void ZenithButton::mouseExit(const juce::MouseEvent& event)
{
    isHovered_ = false;
    isPressed_ = false;
}

//==============================================================================
// Drawing methods
//==============================================================================

void ZenithButton::drawButton(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    auto center = bounds.getCentre();

    // Apply press scaling (spring effect)
    float scale = 1.0f - (pressAnimation_ * 0.08f);
    scale += (hoverAnimation_ * 0.02f);  // Slight expand on hover
    auto scaledBounds = bounds.withSizeKeepingCentre(
        bounds.getWidth() * scale, bounds.getHeight() * scale);

    // DESIGN SYSTEM: Use Radius::l for corner radius
    float cornerRadius = ZenithLookAndFeel::Radius::l;

    // DESIGN SYSTEM: Shadow using dp0 (base background)
    if (pressAnimation_ < 0.5f && isEnabled())
    {
        float shadowAlpha = 0.4f * (1.0f - pressAnimation_);
        g.setColour(juce::Colour(ZenithLookAndFeel::Elevation::dp0).withAlpha(shadowAlpha));
        g.fillRoundedRectangle(scaledBounds.translated(0.0f, 2.0f), cornerRadius);
    }

    // Get colors based on style and state
    juce::Colour baseColor = getBaseColour();

    // DESIGN SYSTEM: Disabled state using dp4
    if (!isEnabled())
    {
        baseColor = juce::Colour(ZenithLookAndFeel::Elevation::dp4);
    }
    // Toggle state (brighter)
    else if (isToggled_)
    {
        baseColor = baseColor.brighter(0.3f);
    }
    // Hover state (slightly brighter)
    else if (isHovered_)
    {
        baseColor = baseColor.brighter(0.1f * hoverAnimation_);
    }

    // Button body with Ableton-style gradient
    juce::ColourGradient buttonGradient(
        baseColor.brighter(0.2f), center.x, scaledBounds.getY(),
        baseColor.darker(0.3f), center.x, scaledBounds.getBottom(),
        false);
    g.setGradientFill(buttonGradient);
    g.fillRoundedRectangle(scaledBounds, cornerRadius);

    // DESIGN SYSTEM: Inner highlight using textPrimary with alpha
    if (isEnabled())
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary).withAlpha(0.12f));
        auto highlightBounds = scaledBounds.withHeight(scaledBounds.getHeight() * 0.3f);
        g.fillRoundedRectangle(highlightBounds, cornerRadius);
    }

    // Hover glow
    if (isHovered_ && isEnabled() && hoverAnimation_ > 0.01f)
    {
        float glowAlpha = 0.4f * hoverAnimation_;
        g.setColour(getHoverColour().withAlpha(glowAlpha));
        g.drawRoundedRectangle(scaledBounds.expanded(2.0f), cornerRadius, 2.5f);
    }

    // Toggle glow (stronger than hover)
    if (isToggled_ && isEnabled() && toggleAnimation_ > 0.01f)
    {
        float toggleGlowAlpha = 0.5f * toggleAnimation_;
        g.setColour(baseColor.brighter(0.5f).withAlpha(toggleGlowAlpha));
        g.drawRoundedRectangle(scaledBounds.expanded(3.0f), cornerRadius, 3.0f);
    }

    // DESIGN SYSTEM: Button text using textPrimary/textDisabled
    if (buttonText_.isNotEmpty())
    {
        juce::Colour textColor = isEnabled()
            ? juce::Colour(ZenithLookAndFeel::Colors::textPrimary).withAlpha(0.95f)
            : juce::Colour(ZenithLookAndFeel::Colors::textDisabled);

        g.setColour(textColor);
        g.setFont(ZenithLookAndFeel::Typography::getBody());
        g.drawText(buttonText_, scaledBounds, juce::Justification::centred);
    }

    // DESIGN SYSTEM: Pressed overlay using dp0
    if (isPressed_ && pressAnimation_ > 0.01f)
    {
        g.setColour(juce::Colour(ZenithLookAndFeel::Elevation::dp0).withAlpha(0.2f * pressAnimation_));
        g.fillRoundedRectangle(scaledBounds, cornerRadius);
    }
}

juce::Colour ZenithButton::getBaseColour() const
{
    // DESIGN SYSTEM: Map button styles to design tokens
    switch (style_)
    {
        case Primary:
            return juce::Colour(ZenithLookAndFeel::Colors::accentPrimary);  // Cyan
        case Secondary:
            return juce::Colour(ZenithLookAndFeel::Elevation::dp4);  // Elevated surface
        case Success:
            return juce::Colour(ZenithLookAndFeel::Colors::success);  // Green
        case Danger:
            return juce::Colour(ZenithLookAndFeel::Colors::danger);  // Red
        case Warning:
            return juce::Colour(ZenithLookAndFeel::Colors::warning);  // Amber
        default:
            return juce::Colour(ZenithLookAndFeel::Colors::accentPrimary);
    }
}

juce::Colour ZenithButton::getHoverColour() const
{
    // Hover glow is always slightly brighter than base
    return getBaseColour().brighter(0.3f);
}

}  // namespace zenith
