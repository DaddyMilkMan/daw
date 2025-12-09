/*
  ==============================================================================

    SkiaSlider.cpp
    Created: 2025-11-28
    Author:  Zenith DAW AI Team

    Skia-based UI slider component.

  ==============================================================================
*/

#include "SkiaSlider.h"
#include "ZenithAnimation.h"
#include "../ZenithTypography.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkColor.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <cmath>

namespace zenith {

// ============================================================================
// CONSTRUCTION
// ============================================================================

SkiaSlider::SkiaSlider(juce::String name) : name_(std::move(name)) {
    // Default values
    setRange(0.0, 1.0);
    setValue(0.5);
    setOrientation(Orientation::Vertical);
    setStyle(Style::Fader);
    setValueColoring(true);

    // Make sure to setComponentID for AI interaction
    setComponentID(this->name_);

    // Default animation config for sliders
    animation::Spring::Config sliderSpringConfig = animation::Spring::Config::smooth();
    addAnimationProperty("scale", sliderSpringConfig);
    addAnimationProperty("glow", sliderSpringConfig);
}

SkiaSlider::~SkiaSlider() {
    // Nothing to do
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SkiaSlider::setRange(double min, double max, double interval) {
    range_ = juce::NormalisableRange<double>(min, max, interval);
    // Update current value to fit new range
    setValue(juce::jlimit(range_.start, range_.end, value_));
}

void SkiaSlider::setSkewFactor(double skew) {
    range_.setSkewForCentre(skew);
    // Update current value to fit new range
    setValue(juce::jlimit(range_.start, range_.end, value_));
}

void SkiaSlider::setValue(double newValue, juce::NotificationType notification) {
    if (newValue == value_) return;

    value_ = juce::jlimit(range_.start, range_.end, newValue);

    if (notification != juce::dontSendNotification) {
        // Trigger callback
        if (onValueChange) {
            onValueChange();
        }
    }
    repaint();
}

void SkiaSlider::setDefaultValue(double defValue) {
    defaultValue_ = juce::jlimit(range_.start, range_.end, defValue);
}

// ============================================================================
// MOUSE INTERACTION
// ============================================================================

void SkiaSlider::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) return; // Right click for context menu

    isDragging_ = true;
    dragStartValue_ = value_;
    dragStartMousePosition_ = (orientation_ == Orientation::Vertical) ? e.position.y : e.position.x;

    // Bring to front on click
    toFront(true);
    
    // Quick press-down animation (snappier spring)
    animateWithSpring("scale", 0.98f, 600.0f, 35.0f);

    if (onDragStart) onDragStart();
}

void SkiaSlider::mouseDrag(const juce::MouseEvent& e) {
    if (!isDragging_) return;

    float mousePos = (orientation_ == Orientation::Vertical) ? e.position.y : e.position.x;
    float boundsSize = (orientation_ == Orientation::Vertical) ? (float)getHeight() : (float)getWidth();

    // Invert vertical drag
    float dist = (orientation_ == Orientation::Vertical) ? dragStartMousePosition_ - mousePos : mousePos - dragStartMousePosition_;

    // Sensitivity (can be adjusted)
    float sensitivity = 0.005f;

    // Calculate new normalized value (0.0 - 1.0)
    float delta = dist * sensitivity;
    float newValueNorm = juce::jlimit(0.0f, 1.0f, (float)range_.convertTo0to1(dragStartValue_) + delta);
    
    // Convert back to actual value
    double newValue = range_.convertFrom0to1(newValueNorm);

    // Apply snap to interval if set
    if (range_.interval > 0.0) {
        newValue = range_.snapToLegalValue(newValue);
    }

    setValue(newValue, juce::sendNotification);
}

void SkiaSlider::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    isDragging_ = false;

    // Bouncy release animation
    animateWithSpring("scale", isHovered() ? 1.05f : 1.0f, 300.0f, 15.0f);

    if (onDragEnd) onDragEnd();
}

void SkiaSlider::mouseDoubleClick(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) return; // Right click for context menu
    if (e.originalComponent == this) {
        setValue(defaultValue_, juce::sendNotification);
    }
}

void SkiaSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (isDragging_) return; // Don't interfere with dragging

    float delta = wheel.deltaY * 0.05f; // Adjust sensitivity
    float newValueNorm = juce::jlimit(0.0f, 1.0f, (float)range_.convertTo0to1(value_) + delta);
    double newValue = range_.convertFrom0to1(newValueNorm);

    // Apply snap to interval if set
    if (range_.interval > 0.0) {
        newValue = range_.snapToLegalValue(newValue);
    }
    setValue(newValue, juce::sendNotification);
}

void SkiaSlider::onHoverEnter() {
    animateWithSpring("glow", 1.0f, 300.0f, 20.0f);
    animateWithSpring("scale", 1.05f, 400.0f, 25.0f);
}

void SkiaSlider::onHoverExit() {
    animateWithSpring("glow", 0.0f, 300.0f, 25.0f);
    animateWithSpring("scale", 1.0f, 300.0f, 25.0f);
}

bool SkiaSlider::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    juce::ignoreUnused(origin);
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey) {
        setValue(defaultValue_, juce::sendNotification);
        return true;
    }
    return false;
}

void SkiaSlider::getTextValue(juce::String& text) const {
    text = juce::String(value_, 2); // default 2 decimal places for slider
}

void SkiaSlider::setTextValue(const juce::String& text) {
    double newValue = text.getDoubleValue();
    if (newValue != value_) {
        setValue(newValue, juce::sendNotification);
    }
}

void SkiaSlider::cutValue() {
    juce::SystemClipboard::copyText(juce::String(value_, 2));
    setValue(defaultValue_, juce::sendNotification);
}

void SkiaSlider::copyValue() {
    juce::SystemClipboard::copyText(juce::String(value_, 2));
}

void SkiaSlider::pasteValue() {
    juce::String clipboardText = juce::SystemClipboard::getTextFromClipboard();
    if (clipboardText.isNotEmpty()) {
        setTextValue(clipboardText);
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void SkiaSlider::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    float scale = getAnimatedValue("scale");
    if (scale < 0.01f) scale = 1.0f;
    
    // Apply slight scale on hover
    if (std::abs(scale - 1.0f) > 0.001f) {
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        canvas->translate(cx, cy);
        canvas->scale(scale, scale);
        canvas->translate(-cx, -cy);
    }

    // Track
    SkPaint trackPaint;
    trackPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
    trackPaint.setAntiAlias(true);

    SkRect trackRect;
    if (orientation_ == Orientation::Vertical) {
        float w = (style_ == Style::Line) ? 2.0f : 6.0f; // Thinner track
        trackRect = SkRect::MakeXYWH(bounds.getCentreX() - w/2.0f, 0.0f, w, bounds.getHeight());
    } else {
        float h = (style_ == Style::Line) ? 2.0f : 6.0f;
        trackRect = SkRect::MakeXYWH(0.0f, bounds.getCentreY() - h/2.0f, bounds.getWidth(), h);
    }

    canvas->drawRoundRect(trackRect, 2.0f, 2.0f, trackPaint);

    // Fill (for Bar style)
    if (style_ == Style::Bar) {
        SkPaint fillPaint;
        SkColor color = valueColoring_ ?
            design::interpolateColor(design::colors::BLUE, design::colors::NEON_GREEN, value_) : 
            design::colors::CYAN;

        fillPaint.setColor(color);
        fillPaint.setAntiAlias(true);

        SkRect fillRect;
        if (orientation_ == Orientation::Vertical) {
            fillRect = SkRect::MakeXYWH(trackRect.x(), trackRect.bottom() - (trackRect.height() * value_), trackRect.width(), trackRect.height() * value_);
        } else {
            fillRect = SkRect::MakeXYWH(trackRect.x(), trackRect.y(), trackRect.width() * value_, trackRect.height());
        }

        // Glow
        float glowIntensity = getAnimatedValue("glow");
        float globalGlow = design::Settings::getGlowIntensity();
        
        if ((glowIntensity > 0.01f || isHovered()) && globalGlow > 0.01f) {
            SkPaint glowPaint = fillPaint;
            float intensity = std::max(glowIntensity, isHovered() ? 0.5f : 0.0f);
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
            glowPaint.setAlpha(static_cast<U8CPU>(100 * intensity));
            canvas->drawRoundRect(fillRect, 2.0f, 2.0f, glowPaint);
        }

        canvas->drawRoundRect(fillRect, 2.0f, 2.0f, fillPaint);
    }

    // Handle (for Line and Fader styles)
    if (style_ != Style::Bar) {
        SkRect handleRect;
        float handleSize = (style_ == Style::Fader) ? 24.0f : 14.0f;
        float handleThickness = (style_ == Style::Fader) ? 12.0f : 14.0f;

        if (orientation_ == Orientation::Vertical) {
            float y = bounds.getHeight() * (1.0f - value_);
            // Clamp handle within bounds
            y = juce::jlimit(handleThickness/2.0f, bounds.getHeight() - handleThickness/2.0f, y);
            handleRect = SkRect::MakeXYWH(bounds.getCentreX() - handleSize/2.0f, y - handleThickness/2.0f, handleSize, handleThickness);
        } else {
            float x = bounds.getWidth() * value_;
            x = juce::jlimit(handleThickness/2.0f, bounds.getWidth() - handleThickness/2.0f, x);
            handleRect = SkRect::MakeXYWH(x - handleThickness/2.0f, bounds.getCentreY() - handleSize/2.0f, handleThickness, handleSize);
        }

        SkPaint handlePaint;
        handlePaint.setAntiAlias(true);

        // Fader cap detail
        if (style_ == Style::Fader) {
            // Modern Fader Cap
            handlePaint.setColor(design::colors::BG_LIGHT); // Dark body
            canvas->drawRoundRect(handleRect, 2.0f, 2.0f, handlePaint);

            // Border
            SkPaint borderPaint;
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(1.0f);
            borderPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
            borderPaint.setAntiAlias(true);
            canvas->drawRoundRect(handleRect, 2.0f, 2.0f, borderPaint);

            // Center Indicator Line
            SkPaint linePaint;
            linePaint.setColor(design::colors::CYAN);
            linePaint.setStrokeWidth(2.0f);
            linePaint.setAntiAlias(true);

            // Glow on indicator
            float glow = getAnimatedValue("glow");
            if (glow > 0.01f) {
                linePaint.setMaskFilter(SkMaskFilter::MakeBlur(kSolid_SkBlurStyle, 3.0f * glow));
            }

            if (orientation_ == Orientation::Vertical) {
                canvas->drawLine(handleRect.left() + 2, handleRect.centerY(), handleRect.right() - 2, handleRect.centerY(), linePaint);
            } else {
                canvas->drawLine(handleRect.centerX(), handleRect.top() + 2, handleRect.centerX(), handleRect.bottom() - 2, linePaint);
            }
        } else {
            // Simple dot/circle for Line style
            handlePaint.setColor(SK_ColorWHITE);
            canvas->drawCircle(handleRect.centerX(), handleRect.centerY(), handleSize/2.0f, handlePaint);
        }

        // Handle Hover Glow
        float glow = getAnimatedValue("glow");
        if (glow > 0.01f) {
            SkPaint glowPaint;
            glowPaint.setColor(design::colors::CYAN);
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
            glowPaint.setAlpha(static_cast<U8CPU>(100 * glow));
            canvas->drawRoundRect(handleRect, 2.0f, 2.0f, glowPaint);
        }
    }
}

// ============================================================================
// DEBUGGING / AI
// ============================================================================

std::vector<SkiaComponent::AIElementInfo> SkiaSlider::getInspectableElements() {
  SkiaComponent::AIElementInfo info;

  auto bounds = getLocalBounds().toFloat();
  info.bounds = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                 bounds.getWidth(), bounds.getHeight());

  info.type = "slider";
  info.parameterId = getName();
  info.currentValue = value_;

  return {info};
}

} // namespace zenith
