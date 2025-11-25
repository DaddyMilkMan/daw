/**
 * @file ZenithKnob.cpp
 * @brief Beautiful custom rotary knob implementation
 * 
 * DESIGN SYSTEM: Updated to use ZenithLookAndFeel design tokens
 */

#include "ZenithKnob.h"
#include "ZenithLookAndFeel.h"  // DESIGN SYSTEM: Include for design tokens

namespace zenith {

//==============================================================================
ZenithKnob::ZenithKnob()
{
    setSize(60, 80);  // Width x Height (knob + label)
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    startTimerHz(60);  // 60 Hz smooth animations
}

ZenithKnob::~ZenithKnob()
{
    stopTimer();
}

//==============================================================================
void ZenithKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Reserve space for label at bottom
    auto labelBounds = bounds.removeFromBottom(20.0f);
    auto knobBounds = bounds.reduced(4.0f);  // Padding for glow

    // Make it square (use minimum dimension)
    float size = juce::jmin(knobBounds.getWidth(), knobBounds.getHeight());
    knobBounds = knobBounds.withSizeKeepingCentre(size, size);

    // Draw knob body
    drawKnobBody(g, knobBounds);

    // Draw value arc
    drawValueArc(g, knobBounds);

    // Draw indicator line
    drawIndicator(g, knobBounds);

    // Draw label
    drawLabel(g, labelBounds);

    // Draw value tooltip when hovering or dragging
    if (isHovered_ || isDragging_) {
        float alpha = juce::jlimit(0.0f, 1.0f, hoverAnimation_ + (isDragging_ ? 0.5f : 0.0f));
        // DESIGN SYSTEM: Tooltip using dp8 elevation
        g.setColour(juce::Colour(ZenithLookAndFeel::Elevation::dp8).withAlpha(0.95f * alpha));

        auto tooltipBounds = knobBounds.withY(knobBounds.getY() - 30.0f).withHeight(24.0f);
        g.fillRoundedRectangle(tooltipBounds, ZenithLookAndFeel::Radius::s);

        // DESIGN SYSTEM: Tooltip text using textPrimary
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary).withAlpha(alpha));
        g.setFont(ZenithLookAndFeel::Typography::getSmallBold());

        juce::String valueText = juce::String(getDisplayValue(), 2) + suffix_;
        g.drawText(valueText, tooltipBounds, juce::Justification::centred, true);
    }
}

void ZenithKnob::drawKnobBody(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    auto center = bounds.getCentre();
    float radius = bounds.getWidth() * 0.5f;

    // Apply hover/drag scaling
    float scale = 1.0f + (hoverAnimation_ * 0.05f) + (dragAnimation_ * 0.03f);
    radius *= scale;

    // DESIGN SYSTEM: Draw outer shadow using dp0
    g.setColour(juce::Colour(ZenithLookAndFeel::Elevation::dp0).withAlpha(0.4f));
    g.fillEllipse(center.x - radius, center.y - radius + 2.0f, radius * 2.0f, radius * 2.0f);

    // DESIGN SYSTEM: Draw knob body with gradient (dp4 to dp1)
    juce::ColourGradient bodyGradient(
        juce::Colour(ZenithLookAndFeel::Elevation::dp4), center.x, center.y - radius,   // Lighter at top
        juce::Colour(ZenithLookAndFeel::Elevation::dp1), center.x, center.y + radius,   // Darker at bottom
        false
    );
    g.setGradientFill(bodyGradient);
    g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

    // DESIGN SYSTEM: Inner highlight using textPrimary with alpha
    juce::Path highlightPath;
    highlightPath.addEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 0.8f);
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary).withAlpha(0.08f));
    g.fillPath(highlightPath);

    // DESIGN SYSTEM: Outer border using borderMedium
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::borderMedium).withAlpha(0.8f));
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // DESIGN SYSTEM: Hover glow using accentPrimary
    if (isHovered_) {
        float glowAlpha = 0.3f * hoverAnimation_;
        g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary).withAlpha(glowAlpha));
        g.drawEllipse(center.x - radius - 2.0f, center.y - radius - 2.0f,
                     (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 2.0f);
    }
}

void ZenithKnob::drawValueArc(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    auto center = bounds.getCentre();
    float radius = bounds.getWidth() * 0.42f;

    // Calculate angles
    float angle = START_ANGLE + (value_ * ROTATION_RANGE);

    // DESIGN SYSTEM: Draw background arc using dp4
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(center.x, center.y, radius, radius,
                               0.0f, START_ANGLE, START_ANGLE + ROTATION_RANGE, true);

    g.setColour(juce::Colour(ZenithLookAndFeel::Elevation::dp4));
    g.strokePath(backgroundArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw value arc with gradient
    juce::Path valueArc;
    valueArc.addCentredArc(center.x, center.y, radius, radius,
                          0.0f, START_ANGLE, angle, true);

    // DESIGN SYSTEM: Gradient from accentPrimary to success based on value
    juce::Colour arcColor = juce::Colour(ZenithLookAndFeel::Colors::accentPrimary);  // Cyan for low values
    if (value_ > 0.5f) {
        arcColor = juce::Colour(ZenithLookAndFeel::Colors::success);  // Green for high values
    }

    g.setColour(arcColor.withAlpha(0.9f));
    g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw glow on arc when dragging
    if (isDragging_) {
        g.setColour(arcColor.withAlpha(0.3f));
        g.strokePath(valueArc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void ZenithKnob::drawIndicator(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    auto center = bounds.getCentre();
    float radius = bounds.getWidth() * 0.5f * 0.8f;  // 80% of knob radius

    // Calculate angle
    float angle = START_ANGLE + (value_ * ROTATION_RANGE);

    // Calculate indicator line end point
    float indicatorLength = radius * 0.6f;
    float endX = center.x + std::cos(angle - juce::MathConstants<float>::halfPi) * indicatorLength;
    float endY = center.y + std::sin(angle - juce::MathConstants<float>::halfPi) * indicatorLength;

    // DESIGN SYSTEM: Draw indicator line using textPrimary
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary).withAlpha(0.9f));
    g.drawLine(center.x, center.y, endX, endY, 2.5f);

    // DESIGN SYSTEM: Draw indicator tip using accentPrimary
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
    g.fillEllipse(endX - 3.0f, endY - 3.0f, 6.0f, 6.0f);
}

void ZenithKnob::drawLabel(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    if (label_.isEmpty()) return;

    // DESIGN SYSTEM: Label using textSecondary
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    g.setFont(ZenithLookAndFeel::Typography::getTiny());
    g.drawText(label_, bounds.toNearestInt(), juce::Justification::centred, true);
}

//==============================================================================
void ZenithKnob::resized()
{
    // Nothing to do - all drawing is dynamic
}

void ZenithKnob::timerCallback()
{
    const float animationSpeed = 0.15f;

    // Smooth value animation
    if (std::abs(value_ - targetValue_) > 0.001f) {
        value_ += (targetValue_ - value_) * animationSpeed;
        repaint();
    }

    // Hover animation
    float targetHover = isHovered_ ? 1.0f : 0.0f;
    if (std::abs(hoverAnimation_ - targetHover) > 0.01f) {
        hoverAnimation_ += (targetHover - hoverAnimation_) * animationSpeed;
        repaint();
    }

    // Drag animation
    float targetDrag = isDragging_ ? 1.0f : 0.0f;
    if (std::abs(dragAnimation_ - targetDrag) > 0.01f) {
        dragAnimation_ += (targetDrag - dragAnimation_) * animationSpeed;
        repaint();
    }
}

//==============================================================================
void ZenithKnob::mouseDown(const juce::MouseEvent& event)
{
    isDragging_ = true;
    dragStartPos_ = event.getPosition();
    dragStartValue_ = value_;
    repaint();
}

void ZenithKnob::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging_) return;

    // Calculate drag delta (vertical drag)
    int deltaY = dragStartPos_.y - event.getPosition().y;

    // Sensitivity: 100 pixels = full range
    float sensitivity = 0.01f;
    float newValue = juce::jlimit(0.0f, 1.0f, dragStartValue_ + (deltaY * sensitivity));

    setValue(newValue, true);
}

void ZenithKnob::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isDragging_ = false;
    repaint();
}

void ZenithKnob::mouseEnter(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovered_ = true;
    repaint();
}

void ZenithKnob::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovered_ = false;
    repaint();
}

void ZenithKnob::mouseDoubleClick(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    setValue(defaultValue_, true);
}

//==============================================================================
void ZenithKnob::setValue(float newValue, bool sendNotification)
{
    newValue = juce::jlimit(0.0f, 1.0f, newValue);

    if (std::abs(newValue - targetValue_) < 0.001f) return;

    targetValue_ = newValue;

    if (sendNotification && onValueChange) {
        onValueChange(getDisplayValue());
    }

    repaint();
}

void ZenithKnob::setRange(float min, float max, float defaultValue)
{
    minValue_ = min;
    maxValue_ = max;
    defaultValue_ = juce::jlimit(0.0f, 1.0f, (defaultValue - min) / (max - min));
}

float ZenithKnob::getDisplayValue() const
{
    return minValue_ + (value_ * (maxValue_ - minValue_));
}

}  // namespace zenith
