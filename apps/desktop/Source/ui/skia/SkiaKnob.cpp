/*
  ==============================================================================
    SkiaKnob.cpp
  ==============================================================================
*/

#include "SkiaKnob.h"
#include "ZenithAnimation.h"
#include "../ZenithTypography.h"
#include <cmath>
#include <core/SkBlurTypes.h> // Explicitly include
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>


// Debug helper
static void logKnob(const juce::String &msg) {
  // DBG("SkiaKnob: " + msg);
}
namespace zenith {

// ============================================================================
// CONSTRUCTION
// ============================================================================

SkiaKnob::SkiaKnob(const juce::String &name) {
  logKnob("Constructor start");
  // Default size
  setSize(60, 80);
  logKnob("setSize done");

  // Accessibility
  setDescription(name.isEmpty() ? "Knob" : name);
  setWantsKeyboardFocus(true);

  // Initial history
  valueHistory_.push(value_);
  logKnob("Constructor done");
}

SkiaKnob::~SkiaKnob() {}

// ============================================================================
// APPEARANCE
// ============================================================================

void SkiaKnob::setStyle(Style style) {
  if (style_ != style) {
    style_ = style;
    markDirty();
  }
}

void SkiaKnob::setRotationRange(float degrees) {
  rotationRange_ = degrees;
  markDirty();
}

void SkiaKnob::setValueColoring(bool enabled) {
  valueColoring_ = enabled;
  markDirty();
}

// ============================================================================
// VALUE CONTROL
// ============================================================================

void SkiaKnob::setValue(float value) {
  float clampedValue = juce::jlimit(0.0f, 1.0f, value);

  if (std::abs(value_ - clampedValue) > 0.0001f) {
    value_ = clampedValue;

    if (onValueChange) {
      onValueChange(value_);
    }

    markDirty();
  }
}

void SkiaKnob::setDefaultValue(float value) {
  defaultValue_ = juce::jlimit(0.0f, 1.0f, value);
}

void SkiaKnob::setDisplayRange(float min, float max) {
  displayMin_ = min;
  displayMax_ = max;
  markDirty(); // For label update
}
void SkiaKnob::setSnapToIncrement(bool snap, float increment) {
  snapEnabled_ = snap;
  snapIncrement_ = increment;
}

void SkiaKnob::setSnapToValue(bool enabled, float snapValue, float tolerance) {
  snapEnabled_ = enabled;
  snapIncrement_ =
      snapValue; // Using snapIncrement_ to store the snap value for simplicity
                 // in this context, though semantics differ slightly
  snapTolerance_ = tolerance;
}

// ============================================================================
// INTERACTION
// ============================================================================

void SkiaKnob::mouseDown(const juce::MouseEvent &e) {
  // Context Menu (Right Click)
  if (e.mods.isPopupMenu()) {
    showContextMenu();
    return;
  }

  // Fine control
  if (e.mods.isShiftDown() || e.mods.isRightButtonDown()) {
    isFineControl_ = true;
  } else {
    isFineControl_ = false;
  }

  isDragging_ = true;
  dragStartValue_ = value_;
  dragStartY_ = e.y;

  // Push current value to history before change
  valueHistory_.push(value_);

  if (onDragStart) {
    onDragStart();
  }

  // Focus for keyboard control
  grabKeyboardFocus();
}

void SkiaKnob::mouseDrag(const juce::MouseEvent &e) {
  if (!isDragging_)
    return;

  float sensitivity = dragSensitivity_ * (isFineControl_ ? 0.1f : 1.0f);
  float delta = (dragStartY_ - e.y) / 200.0f * sensitivity;

  float newValue = juce::jlimit(0.0f, 1.0f, dragStartValue_ + delta);

  if (snapEnabled_) {
    newValue = std::round(newValue / snapIncrement_) * snapIncrement_;
  }

  setValue(newValue);
}

void SkiaKnob::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (isDragging_) {
    isDragging_ = false;
    if (onDragEnd) {
      onDragEnd();
    }
  }
}

void SkiaKnob::mouseDoubleClick(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (doubleClickReset_) {
    resetToDefault();
  }
}

void SkiaKnob::mouseWheelMove(const juce::MouseEvent &e,
                              const juce::MouseWheelDetails &wheel) {
  float delta = wheel.deltaY * 0.1f;
  if (e.mods.isShiftDown())
    delta *= 0.1f;

  setValue(juce::jlimit(0.0f, 1.0f, value_ + delta));
}

void SkiaKnob::onHoverEnter() {
  animateWithSpring("scale", 1.1f, 300.0f, 20.0f); // Bouncy hover
  animateWithSpring("glow", 1.0f, 200.0f, 20.0f);
}

void SkiaKnob::onHoverExit() {
  animateWithSpring("scale", 1.0f, 300.0f, 25.0f);
  animateWithSpring("glow", 0.0f, 300.0f, 25.0f);
}

bool SkiaKnob::keyPressed(const juce::KeyPress &key, juce::Component *origin) {
  // Undo: Ctrl + Z
  if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
    if (valueHistory_.canUndo()) {
      float val = valueHistory_.undo();
      setValue(val);
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
      float val = valueHistory_.redo();
      setValue(val);
      return true;
    }
  }

  // Call base class for context menu shortcut
  return SkiaComponent::keyPressed(key, origin);
}

// ============================================================================
// CONTEXT MENU & UNDO/REDO
// ============================================================================

void SkiaKnob::resetToDefault() {
  valueHistory_.push(value_); // Save before reset
  setValue(defaultValue_);
}

void SkiaKnob::copyValue() {
  juce::SystemClipboard::copyTextToClipboard(juce::String(value_));
}

void SkiaKnob::pasteValue() {
  juce::String text = juce::SystemClipboard::getTextFromClipboard();
  float val = text.getFloatValue();
  if (val >= 0.0f && val <= 1.0f) { // Simple validation
    valueHistory_.push(value_);
    setValue(val);
  }
}

// ============================================================================
// RENDERING
// ============================================================================

std::vector<SkiaComponent::AIElementInfo> SkiaKnob::getInspectableElements() {
  SkiaComponent::AIElementInfo info;

  auto bounds = getLocalBounds().toFloat();
  info.bounds = SkRect::MakeXYWH(bounds.getX(), bounds.getY(),
                                 bounds.getWidth(), bounds.getHeight());

  info.type = "knob";
  info.parameterId = getName();
  info.currentValue = value_;

  return {info};
}

void SkiaKnob::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();

  // Calculate radius (leave room for label)
  float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.35f;

  // Apply hover scale - use spring physics value
  float scale = getAnimatedValue("scale");
  // If scale is 0 (uninitialized), default to 1
  if (scale < 0.01f) scale = 1.0f;
  
  if (std::abs(scale - 1.0f) > 0.001f) {
    canvas->translate(cx, cy);
    canvas->scale(scale, scale);
    canvas->translate(-cx, -cy);
  }

  // Draw Arc
  float startAngle = -rotationRange_ / 2.0f - 90.0f;
  
  SkRect arcRect =
      SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

  // 1. Background track (Darker, more subtle)
  SkPaint trackPaint;
  trackPaint.setStyle(SkPaint::kStroke_Style);
  trackPaint.setStrokeWidth(3.0f); 
  trackPaint.setColor(SkColorSetARGB(40, 255, 255, 255)); // 15% white
  trackPaint.setAntiAlias(true);
  trackPaint.setStrokeCap(SkPaint::kRound_Cap);
  
  canvas->drawArc(arcRect, startAngle, rotationRange_, false, trackPaint);
  
  // 2. Value arc
  SkPaint valuePaint;
  valuePaint.setStyle(SkPaint::kStroke_Style);
  valuePaint.setStrokeWidth(3.0f);
  valuePaint.setAntiAlias(true);
  valuePaint.setStrokeCap(SkPaint::kRound_Cap);

  // Color gradient
  SkColor color = design::colors::CYAN;
  if (valueColoring_) {
    color = design::interpolateColor(design::colors::BLUE, design::colors::NEON_GREEN, value_);
  }
  valuePaint.setColor(color);

  // Draw active arc
  if (value_ > 0.001f) {
      canvas->drawArc(arcRect, startAngle, value_ * rotationRange_, false, valuePaint);
  }

  // 3. Glow effect (Dynamic based on interaction)
  float glowIntensity = getAnimatedValue("glow");
  float globalGlow = design::Settings::getGlowIntensity();
  
  if (glowIntensity > 0.01f && globalGlow > 0.01f) {
      SkPaint glowPaint = valuePaint;
      glowPaint.setStrokeWidth(3.0f);
      // More intense glow when active
      glowPaint.setColor(design::withAlpha(color, 0.6f * glowIntensity * globalGlow));
      glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f * glowIntensity));
      
      if (value_ > 0.001f) {
        canvas->drawArc(arcRect, startAngle, value_ * rotationRange_, false, glowPaint);
      }
  }

  // 4. Dot indicator / Handle
  if (style_ == Style::Dot || style_ == Style::ArcAndDot) {
    float endAngleRad = (startAngle + value_ * rotationRange_) * (3.14159f / 180.0f);
    float dotRadius = 3.5f;
    
    // Position on the ring
    float dotX = cx + std::cos(endAngleRad) * radius;
    float dotY = cy + std::sin(endAngleRad) * radius;

    // Dot Glow
    if (glowIntensity > 0.01f) {
        SkPaint dotGlowPaint;
        dotGlowPaint.setColor(design::withAlpha(SK_ColorWHITE, 0.5f * glowIntensity));
        dotGlowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        canvas->drawCircle(dotX, dotY, dotRadius + 2.0f, dotGlowPaint);
    }

    SkPaint dotPaint;
    dotPaint.setColor(SK_ColorWHITE);
    dotPaint.setAntiAlias(true);
    canvas->drawCircle(dotX, dotY, dotRadius, dotPaint);
  }

  // 5. Label
  if (labelPosition_ != LabelPosition::None) {
    SkFont font = ZenithTypography::valueFont(); // Use mono font for values
    
    juce::String labelText =
        juce::String(displayMin_ + value_ * (displayMax_ - displayMin_), 1);

    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);

    float textY = cy;
    if (labelPosition_ == LabelPosition::Below)
      textY += radius + 15.0f;
    if (labelPosition_ == LabelPosition::Above)
      textY -= radius + 15.0f;

    // Center text
    float width = font.measureText(labelText.toRawUTF8(), labelText.length(), SkTextEncoding::kUTF8);
    canvas->drawString(labelText.toRawUTF8(), cx - width / 2.0f, textY + 4.0f, font, textPaint);
  }
}

// ============================================================================
// RENDER STATE CAPTURE
// ============================================================================

render::KnobRenderState SkiaKnob::captureRenderState() const {
  render::KnobRenderState state;

  state.bounds = SkRect::MakeXYWH(
      static_cast<float>(getX()), static_cast<float>(getY()),
      static_cast<float>(getWidth()), static_cast<float>(getHeight()));

  state.value = value_;
  state.defaultValue = defaultValue_;
  state.displayMin = displayMin_;
  state.displayMax = displayMax_;
  state.labelText = juce::String(displayMin_ + value_ * (displayMax_ - displayMin_), 1);
  state.isHovered = isHovered();
  state.isDragging = isDragging_;
  
  state.baseColor = design::colors::CYAN;
  if (valueColoring_) {
    state.baseColor = design::interpolateColor(design::colors::BLUE, design::colors::NEON_GREEN, value_);
  }
  state.glowColor = state.baseColor;
  state.glowIntensity = getAnimatedValue("glow");
  state.scale = getAnimatedValue("scale");
  state.cachedGlowAlpha = static_cast<uint8_t>(state.glowIntensity * 102); 

  return state;
}

} // namespace zenith
