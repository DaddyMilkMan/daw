/**
 * @file SkiaSliderComponent.cpp
 * @brief Implementation of Skia-rendered slider with spring physics
 */

#include "SkiaSliderComponent.h"

// Skia headers
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkPath.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkBlurTypes.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontTypes.h"
#include "include/effects/SkGradientShader.h"

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

namespace {
    // Spring physics
    constexpr float SPRING_STIFFNESS = 400.0f;
    constexpr float SPRING_DAMPING = 28.0f;
    constexpr float ANIMATION_FPS = 60.0f;
    constexpr float ANIMATION_DT = 1.0f / ANIMATION_FPS;

    // Visual
    constexpr float TRACK_HEIGHT = 6.0f;
    constexpr float THUMB_SIZE = 20.0f;
    constexpr float HOVER_GLOW_ALPHA = 0.4f;
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaSliderComponent::SkiaSliderComponent(Orientation orientation, Style style)
    : orientation_(orientation)
    , style_(style)
{
    // Create Skia renderer
    renderer_ = std::make_unique<SkiaRenderer>(*this);

    // Start animation timer
    startAnimationTimer();

    // Initial thumb position
    thumbProgress_ = (float)value_;
}

SkiaSliderComponent::~SkiaSliderComponent()
{
    stopTimer();
}

//==============================================================================
// Component Interface
//==============================================================================

void SkiaSliderComponent::paint(juce::Graphics& g)
{
    if (!renderer_->isInitialized())
    {
        if (!renderer_->initialize())
        {
            // Fallback rendering
            g.fillAll(juce::Colours::darkgrey);
            g.setColour(juce::Colours::white);
            g.drawText("Skia init failed", getLocalBounds(),
                      juce::Justification::centred);
            return;
        }
    }

    // Render with Skia
    renderer_->render([this](SkCanvas* canvas) {
        drawSlider(canvas);
    });
}

void SkiaSliderComponent::resized()
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

void SkiaSliderComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SkiaSliderComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

void SkiaSliderComponent::mouseDown(const juce::MouseEvent& event)
{
    isDragging_ = true;
    dragStartPos_ = event.getPosition();
    dragStartValue_ = value_;

    // Jump to click position
    double newValue = 0.0;

    if (orientation_ == Orientation::Horizontal)
    {
        float trackWidth = getWidth() - THUMB_SIZE;
        float clickPos = event.x - THUMB_SIZE / 2.0f;
        newValue = juce::jlimit(0.0, 1.0, static_cast<double>(clickPos / trackWidth));
    }
    else
    {
        float trackHeight = getHeight() - THUMB_SIZE;
        float clickPos = trackHeight - (event.y - THUMB_SIZE / 2.0f);
        newValue = juce::jlimit(0.0, 1.0, static_cast<double>(clickPos / trackHeight));
    }

    // Map from 0-1 to actual range
    newValue = minimum_ + newValue * (maximum_ - minimum_);
    setValue(newValue);

    repaint();
}

void SkiaSliderComponent::mouseUp(const juce::MouseEvent&)
{
    isDragging_ = false;
    repaint();
}

void SkiaSliderComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging_)
        return;

    double delta = 0.0;

    if (orientation_ == Orientation::Horizontal)
    {
        float trackWidth = getWidth() - THUMB_SIZE;
        delta = (event.x - dragStartPos_.x) / trackWidth;
    }
    else
    {
        float trackHeight = getHeight() - THUMB_SIZE;
        delta = -(event.y - dragStartPos_.y) / trackHeight;
    }

    // Apply delta to start value
    double newValue = dragStartValue_ + delta * (maximum_ - minimum_);
    setValue(newValue);
}

void SkiaSliderComponent::mouseWheelMove(const juce::MouseEvent&,
                                        const juce::MouseWheelDetails& wheel)
{
    // Scroll to adjust value
    double delta = wheel.deltaY * (maximum_ - minimum_) * 0.05;
    setValue(value_ + delta);
}

//==============================================================================
// Value Control
//==============================================================================

void SkiaSliderComponent::setValue(double value, bool sendNotification)
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

void SkiaSliderComponent::setRange(double minimum, double maximum, double interval)
{
    minimum_ = minimum;
    maximum_ = maximum;
    interval_ = interval;

    // Constrain current value to new range
    setValue(value_, false);
}

void SkiaSliderComponent::setNumSteps(int steps)
{
    numSteps_ = steps;
    if (steps > 0)
    {
        interval_ = (maximum_ - minimum_) / (steps - 1);
        snapToValue_ = true;
    }
}

//==============================================================================
// Animation
//==============================================================================

void SkiaSliderComponent::startAnimationTimer()
{
    startTimer((int)(1000.0f / ANIMATION_FPS));
}

void SkiaSliderComponent::timerCallback()
{
    updateAnimations();
}

void SkiaSliderComponent::updateAnimations()
{
    bool needsRepaint = false;

    // Spring physics for thumb position
    float targetThumb = valueToPosition(value_);
    if (std::abs(thumbProgress_ - targetThumb) > 0.001f)
    {
        float force = -SPRING_STIFFNESS * (thumbProgress_ - targetThumb)
                     - SPRING_DAMPING * thumbVelocity_;

        thumbVelocity_ += force * ANIMATION_DT;
        thumbProgress_ += thumbVelocity_ * ANIMATION_DT;

        // Clamp
        thumbProgress_ = std::max(0.0f, std::min(1.0f, thumbProgress_));

        needsRepaint = true;
    }

    // Spring physics for hover glow
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

void SkiaSliderComponent::drawSlider(SkCanvas* canvas)
{
    auto bounds = getLocalBounds();
    float width = (float)bounds.getWidth();
    float height = (float)bounds.getHeight();

    bool isHorizontal = (orientation_ == Orientation::Horizontal);

    // Calculate track bounds
    SkRect trackRect;
    if (isHorizontal)
    {
        float trackY = (height - TRACK_HEIGHT) / 2.0f;
        trackRect = SkRect::MakeXYWH(THUMB_SIZE/2, trackY,
                                     width - THUMB_SIZE, TRACK_HEIGHT);
    }
    else
    {
        float trackX = (width - TRACK_HEIGHT) / 2.0f;
        trackRect = SkRect::MakeXYWH(trackX, THUMB_SIZE/2,
                                     TRACK_HEIGHT, height - THUMB_SIZE);
    }

    //==========================================================================
    // Draw track background
    //==========================================================================

    SkPaint trackBgPaint;
    trackBgPaint.setAntiAlias(true);
    trackBgPaint.setColor(SkColorSetARGB(100, 255, 255, 255));  // 40% white

    SkRRect trackBgRRect = SkRRect::MakeRectXY(trackRect,
                                               TRACK_HEIGHT/2, TRACK_HEIGHT/2);
    canvas->drawRRect(trackBgRRect, trackBgPaint);

    //==========================================================================
    // Draw filled track (value portion)
    //==========================================================================

    SkRect filledRect;
    if (isHorizontal)
    {
        if (style_ == Style::Bipolar)
        {
            // Center-zero style
            float center = trackRect.left() + trackRect.width() / 2.0f;
            float thumbX = trackRect.left() + thumbProgress_ * trackRect.width();

            if (thumbX >= center)
            {
                filledRect = SkRect::MakeLTRB(center, trackRect.top(),
                                             thumbX, trackRect.bottom());
            }
            else
            {
                filledRect = SkRect::MakeLTRB(thumbX, trackRect.top(),
                                             center, trackRect.bottom());
            }
        }
        else
        {
            // Standard left-to-thumb fill
            float thumbX = trackRect.left() + thumbProgress_ * trackRect.width();
            filledRect = SkRect::MakeLTRB(trackRect.left(), trackRect.top(),
                                         thumbX, trackRect.bottom());
        }
    }
    else
    {
        // Vertical: bottom-to-thumb fill
        float thumbY = trackRect.bottom() - thumbProgress_ * trackRect.height();
        filledRect = SkRect::MakeLTRB(trackRect.left(), thumbY,
                                     trackRect.right(), trackRect.bottom());
    }

    SkPaint filledPaint;
    filledPaint.setAntiAlias(true);

    // Gradient from blue to lighter blue
    SkColor colors[] = {
        SkColorSetARGB(255, 10, 132, 255),  // #0A84FF
        SkColorSetARGB(255, 64, 156, 255)   // Lighter
    };

    SkPoint points[] = {
        SkPoint::Make(filledRect.left(), filledRect.centerY()),
        SkPoint::Make(filledRect.right(), filledRect.centerY())
    };

    sk_sp<SkShader> gradient = SkGradientShader::MakeLinear(
        points, colors, nullptr, 2, SkTileMode::kClamp);

    filledPaint.setShader(gradient);

    SkRRect filledRRect = SkRRect::MakeRectXY(filledRect,
                                              TRACK_HEIGHT/2, TRACK_HEIGHT/2);
    canvas->drawRRect(filledRRect, filledPaint);

    //==========================================================================
    // Draw tick marks (for Stepped style)
    //==========================================================================

    if (style_ == Style::Stepped && numSteps_ > 0)
    {
        SkPaint tickPaint;
        tickPaint.setAntiAlias(true);
        tickPaint.setColor(SkColorSetARGB(150, 255, 255, 255));  // 60% white
        tickPaint.setStrokeWidth(1.5f);

        for (int i = 0; i < numSteps_; ++i)
        {
            float t = (float)i / (numSteps_ - 1);

            if (isHorizontal)
            {
                float x = trackRect.left() + t * trackRect.width();
                canvas->drawLine(x, trackRect.top() - 4,
                               x, trackRect.bottom() + 4, tickPaint);
            }
            else
            {
                float y = trackRect.bottom() - t * trackRect.height();
                canvas->drawLine(trackRect.left() - 4, y,
                               trackRect.right() + 4, y, tickPaint);
            }
        }
    }

    //==========================================================================
    // Draw thumb
    //==========================================================================

    auto thumbPos = getThumbPosition();

    // Thumb shadow
    if (!isDragging_)
    {
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, 3.0f));

        canvas->drawCircle(thumbPos.x, thumbPos.y + 2, THUMB_SIZE/2, shadowPaint);
    }

    // Thumb body
    SkPaint thumbPaint;
    thumbPaint.setAntiAlias(true);

    // Gradient (lighter at top)
    SkColor thumbColors[] = {
        SK_ColorWHITE,
        SkColorSetARGB(255, 240, 240, 240)
    };

    SkPoint thumbPoints[] = {
        SkPoint::Make(thumbPos.x, thumbPos.y - THUMB_SIZE/2),
        SkPoint::Make(thumbPos.x, thumbPos.y + THUMB_SIZE/2)
    };

    sk_sp<SkShader> thumbGradient = SkGradientShader::MakeLinear(
        thumbPoints, thumbColors, nullptr, 2, SkTileMode::kClamp);

    thumbPaint.setShader(thumbGradient);

    float thumbRadius = THUMB_SIZE / 2.0f;
    if (isDragging_)
        thumbRadius *= 1.1f;  // Slightly larger when dragging

    canvas->drawCircle(thumbPos.x, thumbPos.y, thumbRadius, thumbPaint);

    // Thumb border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(SkColorSetARGB(50, 0, 0, 0));

    canvas->drawCircle(thumbPos.x, thumbPos.y, thumbRadius, borderPaint);

    // Hover glow
    if (hoverProgress_ > 0.01f)
    {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(3.0f);
        glowPaint.setColor(SkColorSetARGB(
            (int)(HOVER_GLOW_ALPHA * 255 * hoverProgress_),
            10, 132, 255));

        canvas->drawCircle(thumbPos.x, thumbPos.y, thumbRadius + 2, glowPaint);
    }

    //==========================================================================
    // Draw value text
    //==========================================================================

    if (showValue_)
    {
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SK_ColorWHITE);

        SkFont font(nullptr, 11.0f);

        juce::String valueText = getValueText();

        // Position text above/beside slider
        float textX, textY;
        if (isHorizontal)
        {
            textX = thumbPos.x;
            textY = trackRect.top() - 8;
        }
        else
        {
            textX = trackRect.right() + 10;
            textY = thumbPos.y;
        }

        canvas->drawString(valueText.toRawUTF8(), textX, textY, font, textPaint);
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

double SkiaSliderComponent::constrainValue(double value) const
{
    value = juce::jlimit(minimum_, maximum_, value);

    if (snapToValue_ && interval_ > 0.0)
    {
        value = minimum_ + std::round((value - minimum_) / interval_) * interval_;
    }

    return value;
}

double SkiaSliderComponent::valueToPosition(double value) const
{
    return (value - minimum_) / (maximum_ - minimum_);
}

double SkiaSliderComponent::positionToValue(double position) const
{
    return minimum_ + position * (maximum_ - minimum_);
}

juce::Point<float> SkiaSliderComponent::getThumbPosition() const
{
    auto bounds = getLocalBounds();

    if (orientation_ == Orientation::Horizontal)
    {
        float trackWidth = bounds.getWidth() - THUMB_SIZE;
        float thumbX = THUMB_SIZE/2 + thumbProgress_ * trackWidth;
        float thumbY = bounds.getHeight() / 2.0f;
        return {thumbX, thumbY};
    }
    else
    {
        float trackHeight = bounds.getHeight() - THUMB_SIZE;
        float thumbX = bounds.getWidth() / 2.0f;
        float thumbY = bounds.getHeight() - THUMB_SIZE/2 - thumbProgress_ * trackHeight;
        return {thumbX, thumbY};
    }
}

juce::String SkiaSliderComponent::getValueText() const
{
    return juce::String(value_, textPrecision_) + textSuffix_;
}

} // namespace zenith

