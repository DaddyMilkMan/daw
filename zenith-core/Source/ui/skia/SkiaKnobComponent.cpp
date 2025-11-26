/**
 * @file SkiaKnobComponent.cpp
 * @brief Professional rotary knob with spring physics
 */

#include "SkiaKnobComponent.h"
#include "SkiaTheme.h"

#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPath.h>
#include <include/effects/SkGradientShader.h>

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace zenith {

//==============================================================================
// Construction
//==============================================================================

SkiaKnobComponent::SkiaKnobComponent(Style style)
    : style_(style), isDragging_(false), dragStartValue_(0.0)
{
    setSize(100, 130);
    setRepaintsOnMouseActivity(true);
    startAnimationTimer();
}

SkiaKnobComponent::~SkiaKnobComponent() = default;

//==============================================================================
// Component Interface
//==============================================================================

void SkiaKnobComponent::paint(juce::Graphics& g)
{
    // Fallback: Should be rendered through Skia
    g.fillAll(juce::Colours::darkgrey);
}

void SkiaKnobComponent::resized()
{
    // Layout handled automatically
}

//==============================================================================
// Mouse Events
//==============================================================================

void SkiaKnobComponent::mouseEnter(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovered_ = true;
    startAnimationTimer();
}

void SkiaKnobComponent::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovered_ = false;
}

void SkiaKnobComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown()) {
        isDragging_ = true;
        dragStartPos_ = event.getPosition();
        dragStartValue_ = value_;
    }

    if (event.getNumberOfClicks() >= 2) {
        setValue(defaultValue_, true);
    }
}

void SkiaKnobComponent::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isDragging_ = false;
}

void SkiaKnobComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging_) return;

    auto delta = event.getPosition() - dragStartPos_;
    float sensitivity = event.mods.isShiftDown() ? 0.002f : 0.005f;
    float valueDelta = -delta.getY() * sensitivity;

    setValue(dragStartValue_ + valueDelta, true);
}

void SkiaKnobComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    setValue(defaultValue_, true);
}

void SkiaKnobComponent::mouseWheelMove(const juce::MouseEvent& event,
                                       const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(event);
    float delta = wheel.deltaY * 0.01f;
    setValue(value_ + delta, true);
}

//==============================================================================
// Value Control
//==============================================================================

void SkiaKnobComponent::setValue(double value, bool sendNotification)
{
    double clamped = constrainValue(value);

    if (std::abs(value_ - clamped) > 0.0001) {
        value_ = clamped;

        if (sendNotification && onValueChange) {
            onValueChange(value_);
        }

        repaint();
    }
}

void SkiaKnobComponent::setRange(double minimum, double maximum)
{
    minimum_ = minimum;
    maximum_ = maximum;
    repaint();
}

void SkiaKnobComponent::setNumSteps(int steps)
{
    numSteps_ = steps;
    repaint();
}

//==============================================================================
// Helper Methods
//==============================================================================

double SkiaKnobComponent::constrainValue(double value) const
{
    return juce::jlimit(0.0, 1.0, value);
}

float SkiaKnobComponent::valueToAngle(double value) const
{
    const float minAngle = -135.0f * (M_PI / 180.0f);
    const float maxAngle = 135.0f * (M_PI / 180.0f);
    return minAngle + static_cast<float>(value) * (maxAngle - minAngle);
}

double SkiaKnobComponent::angleToValue(float angle) const
{
    const float minAngle = -135.0f * (M_PI / 180.0f);
    const float maxAngle = 135.0f * (M_PI / 180.0f);
    return static_cast<double>((angle - minAngle) / (maxAngle - minAngle));
}

juce::String SkiaKnobComponent::getValueText() const
{
    return juce::String(value_, textPrecision_) + textSuffix_;
}

//==============================================================================
// Animation
//==============================================================================

void SkiaKnobComponent::startAnimationTimer()
{
    startTimerHz(60);
}

void SkiaKnobComponent::timerCallback()
{
    updateAnimations();
}

void SkiaKnobComponent::updateAnimations()
{
    // Spring physics for rotation
    const float targetAngle = valueToAngle(value_);
    const float angleDelta = targetAngle - rotationAngle_;

    rotationVelocity_ += angleDelta * 0.2f;  // Spring stiffness
    rotationVelocity_ *= 0.92f;              // Damping
    rotationAngle_ += rotationVelocity_;

    // Hover animation
    const float targetHover = isHovered_ ? 1.0f : 0.0f;
    hoverVelocity_ += (targetHover - hoverProgress_) * 0.2f;
    hoverVelocity_ *= 0.92f;
    hoverProgress_ += hoverVelocity_;

    repaint();
}

//==============================================================================
// Skia Rendering
//==============================================================================

void SkiaKnobComponent::drawKnob(SkCanvas* canvas)
{
    // This would be called from the Skia rendering pipeline
    // For now, just a placeholder
    juce::ignoreUnused(canvas);
}

} // namespace zenith
