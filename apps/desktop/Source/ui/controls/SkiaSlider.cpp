/*
  ==============================================================================

    SkiaSlider.cpp
    Created: 2025-11-30
    Authors: Kenji Nakamura (lead), Leo Rossi, Diego Martinez, Isabella Moretti

    Implementation of SkiaSlider.

  ==============================================================================
*/

#include "SkiaSlider.h"
#include "RenderTree.h"
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>

namespace zenith {

SkiaSlider::SkiaSlider(const juce::String &name) {
  setSize(40, 150); // Vertical default

  // Accessibility
  setDescription(name.isEmpty() ? "Slider" : name);
  setWantsKeyboardFocus(true);

  // Initial history
  valueHistory_.push(value_);

  // Smooth value animation via VBlank
  vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(this, [this] {
    float diff = value_ - displayValue_;
    if (std::abs(diff) > 0.0001f) {
      displayValue_ += diff * 0.3f; // Smooth interpolation
      markDirty();
    }
  });
}

SkiaSlider::~SkiaSlider() {}

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
    } else if (orientation == Orientation::Vertical &&
               getWidth() > getHeight()) {
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

void SkiaSlider::setSnapToValue(bool enabled, float snapValue,
                                float tolerance) {
  snapEnabled_ = enabled;
  snapValue_ = snapValue;
  snapTolerance_ = tolerance;
}

// ============================================================================
// INTERACTION
// ============================================================================

void SkiaSlider::mouseDown(const juce::MouseEvent &e) {
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

void SkiaSlider::mouseDrag(const juce::MouseEvent &e) {
  if (!isDragging_)
    return;

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

void SkiaSlider::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (isDragging_) {
    isDragging_ = false;
    if (onDragEnd) {
      onDragEnd();
    }
  }
}

void SkiaSlider::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  resetToDefault();
}

void SkiaSlider::mouseWheelMove(const juce::MouseEvent &e,
                                const juce::MouseWheelDetails &wheel) {
  float delta =
      (orientation_ == Orientation::Vertical ? wheel.deltaY : wheel.deltaX) *
      0.1f;
  if (e.mods.isShiftDown())
    delta *= 0.1f;

  setValue(juce::jlimit(0.0f, 1.0f, value_ + delta));
}

float SkiaSlider::positionToValue(const juce::Point<int> &pos) const {
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
  animateTo("glow", 1.0f, design::animation::DURATION_FAST);
}

void SkiaSlider::onHoverExit() {
  animateTo("glow", 0.0f, design::animation::DURATION_FAST);
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

bool SkiaSlider::keyPressed(const juce::KeyPress &key,
                            juce::Component *origin) {
  // Undo: Ctrl + Z
  if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
    if (valueHistory_.canUndo()) {
      setValue(valueHistory_.undo());
      return true;
    }
  }

  // Redo: Ctrl + Y or Ctrl + Shift + Z
  if (key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0) ||
      key == juce::KeyPress('z',
                            juce::ModifierKeys::commandModifier |
                                juce::ModifierKeys::shiftModifier,
                            0)) {
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

void SkiaSlider::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Track
  SkPaint trackPaint;
  trackPaint.setColor(design::withAlpha(design::colors::BG_LIGHT, 0.3f));
  trackPaint.setAntiAlias(true);

  SkRect trackRect;
  if (orientation_ == Orientation::Vertical) {
    float w = (style_ == Style::Line) ? 4.0f : bounds.getWidth() * 0.3f;
    trackRect = SkRect::MakeXYWH(bounds.getCentreX() - w / 2.0f, 0.0f, w,
                                 bounds.getHeight());
  } else {
    float h = (style_ == Style::Line) ? 4.0f : bounds.getHeight() * 0.3f;
    trackRect = SkRect::MakeXYWH(0.0f, bounds.getCentreY() - h / 2.0f,
                                 bounds.getWidth(), h);
  }

  canvas->drawRoundRect(trackRect, 4.0f, 4.0f, trackPaint);

  // Fill (for Bar style)
  if (style_ == Style::Bar) {
    SkPaint fillPaint;
    SkColor color = valueColoring_
                        ? design::interpolateColor(design::colors::BLUE,
                                                   design::colors::CYAN, value_)
                        : design::colors::CYAN;

    fillPaint.setColor(color);
    fillPaint.setAntiAlias(true);

    SkRect fillRect = trackRect;
    if (orientation_ == Orientation::Vertical) {
      float h = trackRect.height() * displayValue_;
      fillRect.setXYWH(trackRect.x(), trackRect.bottom() - h, trackRect.width(),
                       h);
    } else {
      fillRect.setXYWH(trackRect.x(), trackRect.y(),
                       trackRect.width() * displayValue_, trackRect.height());
    }

    // Glow
    if (isGlowEnabled() || isHovered()) {
      SkPaint glowPaint = fillPaint;
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kNormal, 8.0f));
      glowPaint.setAlpha(100);
      canvas->drawRoundRect(fillRect, 4.0f, 4.0f, glowPaint);
    }

    canvas->drawRoundRect(fillRect, 4.0f, 4.0f, fillPaint);
  }

  // Handle (for Line and Fader styles)
  if (style_ != Style::Bar) {
    SkRect handleRect;
    float handleSize =
        (style_ == Style::Fader) ? 20.0f : 12.0f; // Reduced from 30.0f
    float handleThickness =
        (style_ == Style::Fader) ? 10.0f : 12.0f; // Reduced from 15.0f

    if (orientation_ == Orientation::Vertical) {
      float y = bounds.getHeight() * (1.0f - displayValue_);
      handleRect = SkRect::MakeXYWH(bounds.getCentreX() - handleSize / 2.0f,
                                    y - handleThickness / 2.0f, handleSize,
                                    handleThickness);
    } else {
      float x = bounds.getWidth() * displayValue_;
      handleRect = SkRect::MakeXYWH(x - handleThickness / 2.0f,
                                    bounds.getCentreY() - handleSize / 2.0f,
                                    handleThickness, handleSize);
    }

    SkPaint handlePaint;
    handlePaint.setColor(design::colors::TEXT_PRIMARY);
    handlePaint.setAntiAlias(true);

    // Fader cap detail
    if (style_ == Style::Fader) {
      handlePaint.setColor(design::colors::BG_LIGHT);
      canvas->drawRoundRect(handleRect, 2.0f, 2.0f, handlePaint);

      // Center line
      SkPaint linePaint;
      linePaint.setColor(design::colors::CYAN);
      linePaint.setStrokeWidth(2.0f);
      if (orientation_ == Orientation::Vertical) {
        canvas->drawLine(handleRect.left(), handleRect.centerY(),
                         handleRect.right(), handleRect.centerY(), linePaint);
      } else {
        canvas->drawLine(handleRect.centerX(), handleRect.top(),
                         handleRect.centerX(), handleRect.bottom(), linePaint);
      }
    } else {
      // Simple dot/circle
      canvas->drawCircle(handleRect.centerX(), handleRect.centerY(),
                         handleSize / 2.0f, handlePaint);
    }

    // Handle Glow
    if (isHovered()) {
      SkPaint glowPaint;
      glowPaint.setColor(design::colors::CYAN);
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kNormal, 10.0f));
      glowPaint.setAlpha(128);
      canvas->drawRect(handleRect, glowPaint);
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
      static_cast<float>(getX()), static_cast<float>(getY()),
      static_cast<float>(getWidth()), static_cast<float>(getHeight()));

  // Value
  state.value = value_;

  // Interaction state
  state.isHovered = isHovered();
  state.isDragging = isDragging_;

  // Color
  state.color = design::colors::CYAN;
  if (valueColoring_) {
    state.color = design::interpolateColor(design::colors::BLUE,
                                           design::colors::CYAN, value_);
  }

  // Label text (pre-format for thread-safe rendering)
  state.labelText = getDescription();

  return state;
}

} // namespace zenith
