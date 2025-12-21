/*
  ==============================================================================

    SkiaSlider.cpp
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Diego Martinez, Isabella Moretti

    Implementation of SkiaSlider.

  ==============================================================================
*/

#include "SkiaSlider.h"
#include "ZenithAnimation.h"
#include "../ZenithTypography.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkColor.h>
#include <core/SkRect.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <core/SkBlurTypes.h>
#include "RenderTree.h"

namespace zenith {

SkiaSlider::SkiaSlider(const juce::String& name) {
    setSize(40, 150); // Vertical default
    
    // Accessibility
    setDescription(name.isEmpty() ? "Slider" : name);
    setWantsKeyboardFocus(true);
    
    // Initial history
    valueHistory_.push(value_);
}

SkiaSlider::~SkiaSlider() {
}

// ============================================================================
// APPEARANCE
// ============================================================================

void SkiaSlider::setStyle(Style style) {
    if (style_ != style) {
        style_ = style;
        markDirty();
    }
}

void SkiaSlider::setOrientation(Orientation orientation) {
    if (orientation_ != orientation) {
        orientation_ = orientation;
        // Swap dimensions if needed for better default feel
        if (orientation == Orientation::Horizontal && getWidth() < getHeight()) {
            setSize(getHeight(), getWidth());
        } else if (orientation == Orientation::Vertical && getWidth() > getHeight()) {
            setSize(getHeight(), getWidth());
        }
        markDirty();
    }
}

void SkiaSlider::setValueColoring(bool enabled) {
    valueColoring_ = enabled;
    markDirty();
}

// ============================================================================
// VALUE CONTROL
// ============================================================================

void SkiaSlider::setValue(float value) {
    float clampedValue = juce::jlimit(0.0f, 1.0f, value);
    
    if (std::abs(value_ - clampedValue) > 0.0001f) {
        value_ = clampedValue;
        
        if (onValueChange) {
            onValueChange(value_);
        }
        
        markDirty();
    }
}

void SkiaSlider::setDefaultValue(float value) {
    defaultValue_ = juce::jlimit(0.0f, 1.0f, value);
}

void SkiaSlider::setDisplayRange(float min, float max) {
    displayMin_ = min;
    displayMax_ = max;
    markDirty();
}

void SkiaSlider::setSnapToValue(bool enabled, float snapValue, float tolerance) {
    snapEnabled_ = enabled;
    snapValue_ = snapValue;
    snapTolerance_ = tolerance;
}

// ============================================================================
// INTERACTION
// ============================================================================

void SkiaSlider::mouseDown(const juce::MouseEvent& e) {
    // Context Menu
    if (e.mods.isPopupMenu()) {
        showContextMenu();
        return;
    }
    
    // Fine control
    isFineControl_ = e.mods.isShiftDown();
    
    isDragging_ = true;
    valueHistory_.push(value_); // Save for undo
    
    // Jump to value immediately on click (pro behavior)
    setValue(positionToValue(e.getPosition()));
    
    if (onDragStart) {
        onDragStart();
    }
    
    grabKeyboardFocus();
}

void SkiaSlider::mouseDrag(const juce::MouseEvent& e) {
    if (!isDragging_) return;
    
    // Fine control update
    isFineControl_ = e.mods.isShiftDown();
    
    float newValue = positionToValue(e.getPosition());
    
    // Snapping logic
    if (snapEnabled_) {
        if (std::abs(newValue - snapValue_) < snapTolerance_) {
            newValue = snapValue_;
        }
    }
    
    setValue(newValue);
}

void SkiaSlider::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (isDragging_) {
        isDragging_ = false;
        if (onDragEnd) {
            onDragEnd();
        }
    }
}

void SkiaSlider::mouseDoubleClick(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    resetToDefault();
}

void SkiaSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    float delta = (orientation_ == Orientation::Vertical ? wheel.deltaY : wheel.deltaX) * 0.1f;
    if (e.mods.isShiftDown()) delta *= 0.1f;
    
    setValue(juce::jlimit(0.0f, 1.0f, value_ + delta));
}

float SkiaSlider::positionToValue(const juce::Point<int>& pos) const {
    auto bounds = getLocalBounds().toFloat();
    float val = 0.0f;
    
    if (orientation_ == Orientation::Vertical) {
        // Bottom is 0, Top is 1
        val = 1.0f - (pos.y / bounds.getHeight());
    } else {
        // Left is 0, Right is 1
        val = pos.x / bounds.getWidth();
    }
    
    return juce::jlimit(0.0f, 1.0f, val);
}

void SkiaSlider::onHoverEnter() {
    animateWithSpring("glow", 1.0f, 300.0f, 20.0f);
    animateWithSpring("scale", 1.05f, 400.0f, 25.0f);
}

void SkiaSlider::onHoverExit() {
    animateWithSpring("glow", 0.0f, 300.0f, 25.0f);
    animateWithSpring("scale", 1.0f, 300.0f, 25.0f);
}

// ============================================================================
// CONTEXT MENU & UNDO/REDO
// ============================================================================

void SkiaSlider::resetToDefault() {
    valueHistory_.push(value_);
    setValue(defaultValue_);
}

void SkiaSlider::copyValue() {
    juce::SystemClipboard::copyTextToClipboard(juce::String(value_));
}

void SkiaSlider::pasteValue() {
    juce::String text = juce::SystemClipboard::getTextFromClipboard();
    float val = text.getFloatValue();
    if (val >= 0.0f && val <= 1.0f) {
        valueHistory_.push(value_);
        setValue(val);
    }
}

bool SkiaSlider::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    // Undo: Ctrl + Z
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
        if (valueHistory_.canUndo()) {
            setValue(valueHistory_.undo());
            return true;
        }
    }
    
    // Redo: Ctrl + Y or Ctrl + Shift + Z
    if (key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)) {
        if (valueHistory_.canRedo()) {
            setValue(valueHistory_.redo());
            return true;
        }
    }
    
    return SkiaComponent::keyPressed(key, origin);
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
        
        SkRect fillRect = trackRect;
        if (orientation_ == Orientation::Vertical) {
            float h = trackRect.height() * value_;
            fillRect.setXYWH(trackRect.x(), trackRect.bottom() - h, trackRect.width(), h);
        } else {
            fillRect.setXYWH(trackRect.x(), trackRect.y(), trackRect.width() * value_, trackRect.height());
        }
        
        // Glow
        float glowIntensity = getAnimatedValue("glow");
        float globalGlow = design::Settings::getGlowIntensity();
        
        if ((glowIntensity > 0.01f || isHovered()) && globalGlow > 0.01f) {
            SkPaint glowPaint = fillPaint;
            float intensity = std::max(glowIntensity, isHovered() ? 0.5f : 0.0f);
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f * globalGlow));
            glowPaint.setAlpha(static_cast<U8CPU>(150 * intensity));
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
// RENDER STATE CAPTURE
// ============================================================================

render::SliderRenderState SkiaSlider::captureRenderState() const {
    render::SliderRenderState state;
    
    // Bounds
    state.bounds = SkRect::MakeXYWH(
        static_cast<float>(getX()),
        static_cast<float>(getY()),
        static_cast<float>(getWidth()),
        static_cast<float>(getHeight())
    );
    
    // Value
    state.value = value_;
    
    // Interaction state
    state.isHovered = isHovered();
    state.isDragging = isDragging_;
    
    // Color
    state.color = design::colors::CYAN;
    if (valueColoring_) {
        state.color = design::interpolateColor(design::colors::BLUE, design::colors::CYAN, value_);
    }
    
    // Label text (pre-format for thread-safe rendering)
    state.labelText = getDescription();
    
    return state;
}

} // namespace zenith