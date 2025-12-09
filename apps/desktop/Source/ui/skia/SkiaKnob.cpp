#include "SkiaKnob.h"
#include "ZenithAnimation.h"
#include "../ZenithTypography.h"
#include <cmath>
#include <core/SkBlurTypes.h> // Explicitly include
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkColor.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>

// Debug helper
static void logKnob(const juce::String& msg) {
#ifdef JUCE_DEBUG
  DBG(msg);
#endif
}

// ============================================================================
// CONSTRUCTION
// ============================================================================

namespace zenith {

SkiaKnob::SkiaKnob(juce::String name) : name_(std::move(name)) {
    // Default values
    setRange(0.0, 1.0);
    setValue(0.5);
    setLabelPosition(LabelPosition::Below);
    setRotationRange(300.0f); // Default to 300 degrees
    setValueColoring(false);
    setStyle(Style::ArcAndDot);

    // Make sure to setComponentID for AI interaction
    setComponentID(this->name_);

    // Default animation config for knobs
    animation::Spring::Config knobSpringConfig = animation::Spring::Config::smooth();
    addAnimationProperty("scale", knobSpringConfig);
    addAnimationProperty("glow", knobSpringConfig);
}

SkiaKnob::~SkiaKnob() {
    // Nothing to do
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SkiaKnob::setRange(double min, double max, double interval) {
    range_ = juce::NormalisableRange<double>(min, max, interval);
    displayMin_ = min;
    displayMax_ = max;
    if (interval == 0.0) {
        // Continuous range, use 3 decimal places
        decimalPlaces_ = 3;
    } else {
        // Discrete range, infer decimal places
        juce::String intervalStr = juce::String(interval);
        if (intervalStr.containsChar('.')) {
            decimalPlaces_ = intervalStr.substring(intervalStr.indexOfChar('.') + 1).length();
        } else {
            decimalPlaces_ = 0;
        }
    }
    // Update current value to fit new range
    setValue(juce::jlimit(range_.start, range_.end, value_));
}

void SkiaKnob::setSkewFactor(double skew) {
    range_.setSkewForCentre(skew);
    // Update current value to fit new range
    setValue(juce::jlimit(range_.start, range_.end, value_));
}

void SkiaKnob::setValue(double newValue, juce::NotificationType notification) {
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

void SkiaKnob::setDefaultValue(double defValue) {
    defaultValue_ = juce::jlimit(range_.start, range_.end, defValue);
}

// ============================================================================
// MOUSE INTERACTION
// ============================================================================

void SkiaKnob::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) return; // Right click for context menu

    isDragging_ = true;
    dragStartAngle_ = valueToAngle(value_);
    dragStartValue_ = value_;
    dragStartMouseY_ = e.position.y;

    // Bring to front on click
    toFront(true);
    
    // Quick press-down animation (snappier spring)
    animateWithSpring("scale", 0.95f, 600.0f, 35.0f);

    if (onDragStart) onDragStart();
}

void SkiaKnob::mouseDrag(const juce::MouseEvent& e) {
    if (!isDragging_) return;

    // Calculate vertical distance dragged
    float dist = dragStartMouseY_ - e.position.y;

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

void SkiaKnob::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    isDragging_ = false;
    
    // Bouncy release animation
    animateWithSpring("scale", isHovered() ? 1.04f : 1.0f, 300.0f, 15.0f);

    if (onDragEnd) onDragEnd();
}

void SkiaKnob::mouseDoubleClick(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) return; // Right click for context menu
    if (e.originalComponent == this) {
        setValue(defaultValue_, juce::sendNotification);
    }
}

void SkiaKnob::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (isDragging_) return; // Don't interfere with dragging

    float delta = wheel.deltaY * 0.1f; // Adjust sensitivity
    float newValueNorm = juce::jlimit(0.0f, 1.0f, (float)range_.convertTo0to1(value_) + delta);
    double newValue = range_.convertFrom0to1(newValueNorm);

    // Apply snap to interval if set
    if (range_.interval > 0.0) {
        newValue = range_.snapToLegalValue(newValue);
    }
    setValue(newValue, juce::sendNotification);
}

void SkiaKnob::onHoverEnter() {
  animateWithSpring("scale", 1.1f, 300.0f, 20.0f); // Bouncy hover
  animateWithSpring("glow", 1.0f, 200.0f, 20.0f);
}

void SkiaKnob::onHoverExit() {
  animateWithSpring("scale", 1.0f, 300.0f, 25.0f);
  animateWithSpring("glow", 0.0f, 300.0f, 25.0f);
}

bool SkiaKnob::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    juce::ignoreUnused(origin);
    if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey) {
        setValue(defaultValue_, juce::sendNotification);
        return true;
    }
    return false;
}

void SkiaKnob::getTextValue(juce::String& text) const {
    text = juce::String(value_, decimalPlaces_);
}

void SkiaKnob::setTextValue(const juce::String& text) {
    double newValue = text.getDoubleValue();
    if (newValue != value_) {
        setValue(newValue, juce::sendNotification);
    }
}

void SkiaKnob::cutValue() {
    juce::SystemClipboard::copyText(juce::String(value_, decimalPlaces_));
    setValue(defaultValue_, juce::sendNotification);
}

void SkiaKnob::copyValue() {
    juce::SystemClipboard::copyText(juce::String(value_, decimalPlaces_));
}

void SkiaKnob::pasteValue() {
    juce::String clipboardText = juce::SystemClipboard::getTextFromClipboard();
    if (clipboardText.isNotEmpty()) {
        setTextValue(clipboardText);
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
// DEBUGGING / AI
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