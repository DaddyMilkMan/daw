/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    SkiaKnob.cpp
  ==============================================================================
*/


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

  updateCachedPaints();

  // Smooth value animation via VBlank
  vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(this, [this] {
    // Lerp displayValue toward actual value
    float diff = value_ - displayValue_;
    if (std::abs(diff) > 0.0001f) {
      displayValue_ += diff * 0.25f; // Smooth interpolation
      markDirty();
    }
  });
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

void SkiaKnob::setLabelPosition(LabelPosition pos) {
  if (labelPosition_ != pos) {
    labelPosition_ = pos;
    markDirty();
  }
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
  animateTo("scale", 1.05f, design::animation::DURATION_FAST);
  animateTo("glow", 1.0f, design::animation::DURATION_FAST);
}

void SkiaKnob::onHoverExit() {
  animateTo("scale", 1.0f, design::animation::DURATION_FAST);
  animateTo("glow", 0.0f, design::animation::DURATION_FAST);
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

void SkiaKnob::resized() { updateCachedPaints(); }

void SkiaKnob::updateCachedPaints() {
  // Background track
  trackPaint_.setStyle(SkPaint::kStroke_Style);
  trackPaint_.setStrokeWidth(2.5f); // Thinner for pro look (was 4.0f)
  trackPaint_.setColor(design::withAlpha(design::colors::BG_LIGHT, 0.3f));
  trackPaint_.setAntiAlias(true);
  trackPaint_.setStrokeCap(SkPaint::kRound_Cap);

  // Value Paint
  valuePaint_.setStyle(SkPaint::kStroke_Style);
  valuePaint_.setStrokeWidth(2.5f); // Match track width
  valuePaint_.setAntiAlias(true);
  valuePaint_.setStrokeCap(SkPaint::kRound_Cap);
  valuePaint_.setColor(design::colors::CYAN); // Default

  // Text Paint
  textPaint_.setColor(design::colors::TEXT_SECONDARY);

  // Dot Paint
  dotPaint_.setColor(SK_ColorWHITE);
  dotPaint_.setAntiAlias(true);

  // Font
  font_.setSize(12.0f);
}

std::vector<SkiaComponent::AIElementInfo> SkiaKnob::getInspectableElements() {
  SkiaComponent::AIElementInfo info;

  auto bounds = getLocalBounds().toFloat();
  // Convert juce::Rectangle to SkRect
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

  // Apply hover scale
  float scale = getAnimatedValue("scale");
  if (scale > 0.0f) {
    canvas->translate(cx, cy);
    canvas->scale(scale, scale);
    canvas->translate(-cx, -cy);
  }

  // Draw Arc
  float startAngle = -rotationRange_ / 2.0f - 90.0f;
  float endAngle = startAngle + (displayValue_ * rotationRange_);
  juce::ignoreUnused(endAngle); // Used for dot calculation

  SkRect arcRect =
      SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
  canvas->drawArc(arcRect, startAngle, rotationRange_, false, trackPaint_);

  // Color
  SkColor color = design::colors::CYAN;
  if (valueColoring_) {
    // Gradient from Blue to Cyan
    // Simple interpolation for now
    color = design::interpolateColor(design::colors::BLUE, design::colors::CYAN,
                                     value_);
  }
  valuePaint_.setColor(color);

  canvas->drawArc(arcRect, startAngle, displayValue_ * rotationRange_, false,
                  valuePaint_);

  // Glow
  float globalGlow = design::Settings::getGlowIntensity();
  if ((isGlowEnabled() || isHovered()) && globalGlow > 0.01f) {
    SkPaint glowPaint = valuePaint_;
    glowPaint.setStrokeWidth(5.0f); // Reduced from 8.0f
    glowPaint.setColor(
        design::withAlpha(color, 0.4f * getAnimatedValue("glow") * globalGlow));

    if (globalGlow > 0.5f) {
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f * globalGlow));
    }

    canvas->drawArc(arcRect, startAngle, displayValue_ * rotationRange_, false,
                    glowPaint);
  }

  // Dot indicator
  if (style_ == Style::Dot || style_ == Style::ArcAndDot) {
    float angleRad = (endAngle) * (3.14159f / 180.0f);
    float dotX = cx + std::cos(angleRad) * radius;
    float dotY = cy + std::sin(angleRad) * radius;

    canvas->drawCircle(dotX, dotY, 3.0f, dotPaint_);
  }

  // Label
  if (labelPosition_ != LabelPosition::None) {
    juce::String labelText = juce::String(
        displayMin_ + displayValue_ * (displayMax_ - displayMin_), 1);

    float textY = cy;
    if (labelPosition_ == LabelPosition::Below)
      textY += radius + 15.0f;
    if (labelPosition_ == LabelPosition::Above)
      textY -= radius + 15.0f;

    // Simple center text (Skia text centering is manual)
    float width = font_.measureText(labelText.toRawUTF8(), labelText.length(),
                                    SkTextEncoding::kUTF8);
    canvas->drawString(labelText.toRawUTF8(), cx - width / 2.0f, textY, font_,
                       textPaint_);
  }
}

// ============================================================================
// RENDER STATE CAPTURE
// ============================================================================

render::KnobRenderState SkiaKnob::captureRenderState() const {
  render::KnobRenderState state;

  // Bounds and geometry
  state.bounds = SkRect::MakeXYWH(
      static_cast<float>(getX()), static_cast<float>(getY()),
      static_cast<float>(getWidth()), static_cast<float>(getHeight()));

  // Value and display
  state.value = value_;
  state.defaultValue = defaultValue_;
  state.displayMin = displayMin_;
  state.displayMax = displayMax_;

  // Pre-format label text
  state.labelText =
      juce::String(displayMin_ + value_ * (displayMax_ - displayMin_), 1);

  // Interaction state
  state.isHovered = isHovered();
  state.isDragging = isDragging_;

  // Colors
  state.baseColor = design::colors::CYAN;
  if (valueColoring_) {
    state.baseColor = design::interpolateColor(design::colors::BLUE,
                                               design::colors::CYAN, value_);
  }
  state.glowColor = state.baseColor;

  // Animation state
  state.glowIntensity = getAnimatedValue("glow");
  state.scale = getAnimatedValue("scale");

  // Cache glow layer for performance optimization
  // The glow intensity is pre-computed so rendering can skip blur calculations
  // when glow is minimal (< 0.1) - enables GPU shader reuse
  state.cachedGlowAlpha =
      static_cast<uint8_t>(state.glowIntensity * 102); // 40% max alpha

  return state;
}

} // namespace zenith
