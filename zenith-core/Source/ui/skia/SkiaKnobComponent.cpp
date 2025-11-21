/**
 * @file SkiaKnobComponent.cpp
 * @brief Implementation of Skia-rendered knob with spring physics
 */

#include "SkiaKnobComponent.h"

// Skia headers
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkColor.h"
#include "include/effects/SkGradientShader.h"

#include <cmath>

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

namespace {
    // Spring physics
    constexpr float SPRING_STIFFNESS = 300.0f;
    constexpr float SPRING_DAMPING = 22.0f;
    constexpr float ANIMATION_FPS = 60.0f;
    constexpr float ANIMATION_DT = 1.0f / ANIMATION_DT;

    // Visual
    constexpr float MIN_ANGLE = -150.0f;  // Degrees
    constexpr float MAX_ANGLE = 150.0f;
    constexpr float ARC_WIDTH = 4.0f;
    constexpr float INDICATOR_LENGTH = 0.4f;  // As fraction of radius
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaKnobComponent::SkiaKnobComponent(Style style)
    : style_(style)
{
    // Create Skia renderer
    renderer_ = std::make_unique<SkiaRenderer>(*this);

    // Start animation timer
    startAnimationTimer();

    // Initial rotation
    rotationAngle_ = valueToAngle(value_);
}

SkiaKnobComponent::~SkiaKnobComponent()
{
    animationTimer_.stopTimer();
}

//==============================================================================
// Component Interface
//==============================================================================

void SkiaKnobComponent::paint(juce::Graphics& g)
{
    if (!renderer_->isInitialized())
    {
        if (!renderer_->initialize())
        {
            // Fallback
            g.fillAll(juce::Colours::darkgrey);
            g.setColour(juce::Colours::white);
            g.drawText("Skia init failed", getLocalBounds(),
                      juce::Justification::centred);
            return;
        }
    }

    renderer_->render([this](SkCanvas* canvas) {
        drawKnob(canvas);
    });
}

void SkiaKnobComponent::resized()
{
    if (renderer_ && renderer_->isInitialized())
    {
        auto bounds = getLocalBounds();
        renderer_->resize(bounds.getWidth(), bounds.getHeight());
    }
}

//==============================================================================
// Mouse Events
//==============================================================================

void SkiaKnobComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SkiaKnobComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

void SkiaKnobComponent::mouseDown(const juce::MouseEvent& event)
{
    isDragging_ = true;
    dragStartPos_ = event.getPosition();
    dragStartValue_ = value_;
    repaint();
}

void SkiaKnobComponent::mouseUp(const juce::MouseEvent&)
{
    isDragging_ = false;
    repaint();
}

void SkiaKnobComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging_)
        return;

    // Calculate drag delta (vertical movement)
    int deltaY = dragStartPos_.y - event.y;

    // Apply sensitivity (with shift for fine control)
    float effectiveSensitivity = sensitivity_;
    if (event.mods.isShiftDown())
        effectiveSensitivity *= 4.0f;  // 4x more pixels for fine control

    // Convert pixels to value change
    double valueDelta = (double)deltaY / effectiveSensitivity;

    setValue(dragStartValue_ + valueDelta);
}

void SkiaKnobComponent::mouseDoubleClick(const juce::MouseEvent&)
{
    // Reset to default value
    setValue(defaultValue_);
}

void SkiaKnobComponent::mouseWheelMove(const juce::MouseEvent&,
                                      const juce::MouseWheelDetails& wheel)
{
    // Scroll to adjust value
    double delta = wheel.deltaY * (maximum_ - minimum_) * 0.05;
    setValue(value_ + delta);
}

//==============================================================================
// Value Control
//==============================================================================

void SkiaKnobComponent::setValue(double value, bool sendNotification)
{
    value = constrainValue(value);

    if (value_ != value)
    {
        value_ = value;

        if (sendNotification && onValueChange)
        {
            onValueChange(value_);
        }

        repaint();
    }
}

void SkiaKnobComponent::setRange(double minimum, double maximum)
{
    minimum_ = minimum;
    maximum_ = maximum;
    setValue(value_, false);
}

void SkiaKnobComponent::setNumSteps(int steps)
{
    numSteps_ = steps;
}

//==============================================================================
// Animation
//==============================================================================

void SkiaKnobComponent::startAnimationTimer()
{
    animationTimer_.setCallback([this]() {
        updateAnimations();
    });

    animationTimer_.startTimer((int)(1000.0f / ANIMATION_FPS));
}

void SkiaKnobComponent::updateAnimations()
{
    bool needsRepaint = false;

    // Spring physics for rotation
    float targetAngle = valueToAngle(value_);
    if (std::abs(rotationAngle_ - targetAngle) > 0.1f)
    {
        float force = -SPRING_STIFFNESS * (rotationAngle_ - targetAngle)
                     - SPRING_DAMPING * rotationVelocity_;

        rotationVelocity_ += force * ANIMATION_DT;
        rotationAngle_ += rotationVelocity_ * ANIMATION_DT;

        needsRepaint = true;
    }

    // Spring physics for hover
    float hoverTarget = (isHovered_ || isDragging_) ? 1.0f : 0.0f;
    if (std::abs(hoverProgress_ - hoverTarget) > 0.001f)
    {
        float force = -SPRING_STIFFNESS * (hoverProgress_ - hoverTarget)
                     - SPRING_DAMPING * hoverVelocity_;

        hoverVelocity_ += force * ANIMATION_DT;
        hoverProgress_ += hoverVelocity_ * ANIMATION_DT;

        hoverProgress_ = std::max(0.0f, std::min(1.0f, hoverProgress_));

        needsRepaint = true;
    }

    if (needsRepaint)
    {
        repaint();
    }
}

//==============================================================================
// Skia Drawing
//==============================================================================

void SkiaKnobComponent::drawKnob(SkCanvas* canvas)
{
    auto bounds = getLocalBounds();
    float width = (float)bounds.getWidth();
    float height = (float)bounds.getHeight();

    // Calculate knob dimensions
    float knobSize = std::min(width, height) * 0.7f;
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;
    float radius = knobSize / 2.0f;

    //==========================================================================
    // Draw outer glow (when hovered)
    //==========================================================================

    if (hoverProgress_ > 0.01f)
    {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(3.0f);
        glowPaint.setColor(SkColorSetARGB(
            (int)(100 * hoverProgress_),
            10, 132, 255));

        canvas->drawCircle(centerX, centerY, radius + 4, glowPaint);
    }

    //==========================================================================
    // Draw knob shadow
    //==========================================================================

    if (!isDragging_)
    {
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 5.0f));

        canvas->drawCircle(centerX, centerY + 3, radius, shadowPaint);
    }

    //==========================================================================
    // Draw knob body (3D gradient)
    //==========================================================================

    SkPaint knobPaint;
    knobPaint.setAntiAlias(true);

    // Radial gradient for 3D effect
    SkColor knobColors[] = {
        SkColorSetARGB(255, 80, 80, 85),   // Center (darker)
        SkColorSetARGB(255, 50, 50, 55)    // Edge (darkest)
    };

    sk_sp<SkShader> knobGradient = SkGradientShader::MakeRadial(
        SkPoint::Make(centerX, centerY - radius * 0.2f),  // Light from top
        radius,
        knobColors, nullptr, 2,
        SkTileMode::kClamp);

    knobPaint.setShader(knobGradient);

    canvas->drawCircle(centerX, centerY, radius, knobPaint);

    //==========================================================================
    // Draw value arc
    //==========================================================================

    SkPaint arcPaint;
    arcPaint.setAntiAlias(true);
    arcPaint.setStyle(SkPaint::kStroke_Style);
    arcPaint.setStrokeWidth(ARC_WIDTH);
    arcPaint.setStrokeCap(SkPaint::kRound_Cap);

    // Arc path
    SkPath arcPath;
    float arcRadius = radius + 8.0f;

    SkRect arcBounds = SkRect::MakeXYWH(
        centerX - arcRadius,
        centerY - arcRadius,
        arcRadius * 2,
        arcRadius * 2
    );

    // Background arc (full range)
    SkPaint bgArcPaint = arcPaint;
    bgArcPaint.setColor(SkColorSetARGB(60, 255, 255, 255));

    arcPath.addArc(arcBounds, MIN_ANGLE + 90, MAX_ANGLE - MIN_ANGLE);
    canvas->drawPath(arcPath, bgArcPaint);

    // Value arc
    arcPath.reset();

    SkPaint valueArcPaint = arcPaint;

    // Gradient for value arc
    SkColor arcColors[] = {
        SkColorSetARGB(255, 10, 132, 255),
        SkColorSetARGB(255, 64, 200, 255)
    };

    float sweepAngle = rotationAngle_ - MIN_ANGLE;

    arcPath.addArc(arcBounds, MIN_ANGLE + 90, sweepAngle);

    sk_sp<SkShader> arcGradient = SkGradientShader::MakeSweep(
        centerX, centerY,
        arcColors, nullptr, 2);

    valueArcPaint.setShader(arcGradient);

    canvas->drawPath(arcPath, valueArcPaint);

    //==========================================================================
    // Draw indicator line
    //==========================================================================

    SkPaint indicatorPaint;
    indicatorPaint.setAntiAlias(true);
    indicatorPaint.setColor(SK_ColorWHITE);
    indicatorPaint.setStrokeWidth(2.5f);
    indicatorPaint.setStrokeCap(SkPaint::kRound_Cap);

    // Calculate indicator position
    float angleRad = (rotationAngle_ + 90) * M_PI / 180.0f;
    float indicatorStartRadius = radius * (1.0f - INDICATOR_LENGTH);
    float indicatorEndRadius = radius * 0.85f;

    float startX = centerX + std::cos(angleRad) * indicatorStartRadius;
    float startY = centerY + std::sin(angleRad) * indicatorStartRadius;
    float endX = centerX + std::cos(angleRad) * indicatorEndRadius;
    float endY = centerY + std::sin(angleRad) * indicatorEndRadius;

    canvas->drawLine(startX, startY, endX, endY, indicatorPaint);

    //==========================================================================
    // Draw center dot (3D highlight)
    //==========================================================================

    SkPaint centerDotPaint;
    centerDotPaint.setAntiAlias(true);

    // Small gradient dot
    SkColor dotColors[] = {
        SkColorSetARGB(150, 255, 255, 255),
        SkColorSetARGB(0, 255, 255, 255)
    };

    sk_sp<SkShader> dotGradient = SkGradientShader::MakeRadial(
        SkPoint::Make(centerX, centerY - 1),
        radius * 0.15f,
        dotColors, nullptr, 2,
        SkTileMode::kClamp);

    centerDotPaint.setShader(dotGradient);

    canvas->drawCircle(centerX, centerY, radius * 0.15f, centerDotPaint);

    //==========================================================================
    // Draw value text (in center)
    //==========================================================================

    if (showValue_)
    {
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SK_ColorWHITE);

        SkFont font(nullptr, 12.0f);

        juce::String valueText = getValueText();

        // Measure text
        SkRect textBounds;
        font.measureText(valueText.toRawUTF8(), valueText.length(),
                        SkTextEncoding::kUTF8, &textBounds);

        // Draw centered
        float textX = centerX - textBounds.width() / 2;
        float textY = centerY + radius + 16;

        canvas->drawString(valueText.toRawUTF8(), textX, textY, font, textPaint);
    }

    //==========================================================================
    // Draw label (below value)
    //==========================================================================

    if (label_.isNotEmpty())
    {
        SkPaint labelPaint;
        labelPaint.setAntiAlias(true);
        labelPaint.setColor(SkColorSetARGB(200, 255, 255, 255));

        SkFont labelFont(nullptr, 10.0f);

        SkRect labelBounds;
        labelFont.measureText(label_.toRawUTF8(), label_.length(),
                             SkTextEncoding::kUTF8, &labelBounds);

        float labelX = centerX - labelBounds.width() / 2;
        float labelY = centerY + radius + 30;

        canvas->drawString(label_.toRawUTF8(), labelX, labelY,
                          labelFont, labelPaint);
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

double SkiaKnobComponent::constrainValue(double value) const
{
    value = juce::jlimit(minimum_, maximum_, value);

    if (numSteps_ > 0)
    {
        double interval = (maximum_ - minimum_) / (numSteps_ - 1);
        value = minimum_ + std::round((value - minimum_) / interval) * interval;
    }

    return value;
}

float SkiaKnobComponent::valueToAngle(double value) const
{
    double normalized = (value - minimum_) / (maximum_ - minimum_);
    return MIN_ANGLE + normalized * (MAX_ANGLE - MIN_ANGLE);
}

double SkiaKnobComponent::angleToValue(float angle) const
{
    double normalized = (angle - MIN_ANGLE) / (MAX_ANGLE - MIN_ANGLE);
    return minimum_ + normalized * (maximum_ - minimum_);
}

juce::String SkiaKnobComponent::getValueText() const
{
    return juce::String(value_, textPrecision_) + textSuffix_;
}

} // namespace zenith

