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

#include "ZenithSlider.h"
#include "../design-system/ColorBridge.h"
#include "ui/design-system/ZenithDesignSystem.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkBlurTypes.h>
#include <core/SkMaskFilter.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithSlider::ZenithSlider() : ZenithControl("") {
  accentColor_ = design::unified::accent_secondary();
}

ZenithSlider::ZenithSlider(const juce::String &name) : ZenithControl(name) {
  accentColor_ = design::unified::accent_secondary();
}

ZenithSlider::ZenithSlider(Orientation orientation)
    : ZenithControl(""), orientation_(orientation) {
  accentColor_ = design::unified::accent_secondary();
}

ZenithSlider::ZenithSlider(const juce::String &name, SkColor color)
    : ZenithControl(name) {
  accentColor_ = color;
}

ZenithSlider::~ZenithSlider() { stopTimer(); }

//==============================================================================
// Value control
//==============================================================================

void ZenithSlider::setValue(float newValue, bool sendNotification) {
  // Clamp value
  if (minValue_ < maxValue_) {
    value_ = juce::jlimit(minValue_, maxValue_, newValue);
  } else {
    value_ = newValue;
  }

  // Update base class too, to keep them in sync if possible,
  // though ZenithSlider seems to manage its own state in this implementation.
  ZenithControl::setValue(value_, sendNotification);

  if (!isTimerRunning()) {
    repaint();
  }

  if (sendNotification && onValueChange) {
    onValueChange(value_);
  }
}

void ZenithSlider::setRange(float min, float max, float defaultValue) {
  minValue_ = min;
  maxValue_ = max;
  defaultValue_ = defaultValue;
  value_ = defaultValue;

  // Sync base class
  ZenithControl::setRange(min, max);
  ZenithControl::setDefaultValue(defaultValue);
  ZenithControl::setValue(value_, false);
}

//==============================================================================
// Skia Rendering
//==============================================================================

void ZenithSlider::drawSkia(SkCanvas *canvas) {
  drawTrack(canvas);
  
  if (showDBScale_) {
    drawDBScale(canvas);
  }
  
  float handlePos = getHandlePosition();
  if (showFillBar_) {
    drawFillBar(canvas, handlePos);
  }
  drawHandle(canvas, handlePos);

  if (isHovered()) {
    drawValueTooltip(canvas, handlePos);
  }
}

float ZenithSlider::getHandlePosition() const {
  if (std::abs(maxValue_ - minValue_) < 0.0001f)
    return 0.0f;
  return (value_ - minValue_) / (maxValue_ - minValue_);
}

void ZenithSlider::mouseDrag(const juce::MouseEvent &e) {
  ZenithControl::mouseDrag(
      e); // Let base handle logic if it has any relevant logic

  // Custom drag logic could go here, but for now we rely on base + setValue
  // If base ZenithControl doesn't update 'value_' member, we might need to.
  // ZenithControl updates its own 'cachedValue_'.
  // We should verify if ZenithControl calls our setValue (it calls virtual
  // setValue? No, it's not virtual). ZenithControl calls 'setValue' in its
  // implementation. Since it's not virtual, it calls ZenithControl::setValue.
  // So 'value_' member of ZenithSlider might NOT be updated by base drag!

  // Fix: sync value from base
  float baseVal = ZenithControl::getValue();
  if (baseVal != value_) {
    value_ = baseVal;
    if (onValueChange)
      onValueChange(value_);
    repaint();
  }
}

void ZenithSlider::mouseEnter(const juce::MouseEvent &e) {
  ZenithControl::mouseEnter(e);
  int duration = design::Settings::isReducedMotionEnabled() ? 0 : design::animation::DURATION_FAST;
  animateTo("hover", 1.0f, duration);
}

void ZenithSlider::mouseExit(const juce::MouseEvent &e) {
  ZenithControl::mouseExit(e);
  int duration = design::Settings::isReducedMotionEnabled() ? 0 : design::animation::DURATION_FAST;
  animateTo("hover", 0.0f, duration);
}

void ZenithSlider::drawFillBar(SkCanvas *canvas, float handlePos) {
  auto bounds = getLocalBounds().toFloat();
  SkPaint paint;
  paint.setColor(accentColor_);

  if (orientation_ == Vertical) {
    float h = bounds.getHeight();
    float y = h * (1.0f - marginEnd_) -
              (h * (1.0f - marginStart_ - marginEnd_) * handlePos);
    float bottom = h * (1.0f - marginEnd_);
    canvas->drawRect(SkRect::MakeLTRB(bounds.getCentreX() - 2, y,
                                      bounds.getCentreX() + 2, bottom),
                     paint);
  } else {
    float w = bounds.getWidth();
    float x =
        w * marginStart_ + (w * (1.0f - marginStart_ - marginEnd_) * handlePos);
    float start = w * marginStart_;
    canvas->drawRect(SkRect::MakeLTRB(start, bounds.getCentreY() - 2, x,
                                      bounds.getCentreY() + 2),
                     paint);
  }
}

void ZenithSlider::drawHandle(SkCanvas *canvas, float handlePos) {
  auto bounds = getLocalBounds().toFloat();
  SkPaint paint;
  paint.setColor(SK_ColorWHITE);
  paint.setAntiAlias(true);

  float cx, cy;
  if (orientation_ == Vertical) {
    float h = bounds.getHeight();
    cy = h * (1.0f - marginEnd_) -
         (h * (1.0f - marginStart_ - marginEnd_) * handlePos);
    cx = bounds.getCentreX();
  } else {
    float w = bounds.getWidth();
    cx =
        w * marginStart_ + (w * (1.0f - marginStart_ - marginEnd_) * handlePos);
    cy = bounds.getCentreY();
  }

  // Handle glow on hover
  float hoverAnim = getAnimatedValue("hover");
  if (hoverAnim > 0.01f) {
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setColor(SkColorSetA(accentColor_, (uint8_t)(100 * hoverAnim)));
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f * hoverAnim));
    canvas->drawCircle(cx, cy, 6.0f, glowPaint);
  }

  canvas->drawCircle(cx, cy, 6.0f, paint);
}

void ZenithSlider::drawValueTooltip(SkCanvas *canvas, float handlePos) {
  // Optional tooltip implementation
}

void ZenithSlider::drawTrack(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(design::unified::bg_01());

  SkRect trackRect;
  float cornerRadius = 2.0f;

  if (orientation_ == Orientation::Vertical) {
    float cx = w / 2.0f;
    float trackStart = h * marginStart_;
    float trackEnd = h * (1.0f - marginEnd_);
    trackRect = SkRect::MakeXYWH(cx - trackWidth_ / 2, trackStart, trackWidth_,
                                 trackEnd - trackStart);
  } else {
    float cy = h / 2.0f;
    float trackStart = w * marginStart_;
    float trackEnd = w * (1.0f - marginEnd_);
    trackRect = SkRect::MakeXYWH(trackStart, cy - trackWidth_ / 2,
                                 trackEnd - trackStart, trackWidth_);
  }

  SkRRect trackRRect =
      SkRRect::MakeRectXY(trackRect, cornerRadius, cornerRadius);
  canvas->drawRRect(trackRRect, paint);

  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(design::unified::border_default());
  canvas->drawRRect(trackRRect, paint);
}

void ZenithSlider::drawDBScale(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();
  
  // Standard positions for typical DAW fader (+6dB max, 0dB @ ~0.75)
  struct Tick { float normPos; const char* label; bool major; };
  // Visual tweaks to match typical log taper where 0dB is comfortably high
  const Tick ticks[] = {
      { 1.0f,  "+6", true },
      { 0.75f, "0",  true },
      { 0.6f,  "-6", false },
      { 0.45f, "-12", false },
      { 0.3f,  "-24", false },
      { 0.15f, "-48", false },
      { 0.0f,  "-inf", true }
  };

  SkPaint tickPaint;
  tickPaint.setAntiAlias(true);
  tickPaint.setColor(design::unified::text_tertiary());
  tickPaint.setStrokeWidth(1.0f);
  
  SkFont font;
  font.setSize(9.0f); // Small font for db
  
  for (const auto& tick : ticks) {
      if (orientation_ == Vertical) {
          float y = h * (1.0f - marginEnd_) - (h * (1.0f - marginStart_ - marginEnd_) * tick.normPos);
          float cx = w / 2.0f;
          float rightEdge = cx + trackWidth_ / 2.0f + 6.0f;
          float tickLen = tick.major ? 6.0f : 4.0f;
          
          canvas->drawLine(rightEdge, y, rightEdge + tickLen, y, tickPaint);
          
          if (tick.major) {
             SkPaint textPaint;
             textPaint.setColor(design::unified::text_secondary());
             textPaint.setAntiAlias(true);
             canvas->drawString(tick.label, rightEdge + tickLen + 3.0f, y + 3.0f, font, textPaint);
          }
      } 
      // Horizontal implementation omitted for brevity as faders are usually vertical, 
      // but could be added if needed.
  }

  // Unity Snap Marker
  if (unitySnap_) {
      SkPaint snapPaint;
      snapPaint.setColor(design::unified::accent_secondary());
      snapPaint.setStrokeWidth(2.0f); // Prominent
      
      float ySnap = h * (1.0f - marginEnd_) - (h * (1.0f - marginStart_ - marginEnd_) * unityValue_);
      float cx = w / 2.0f;
      float halfWidth = trackWidth_/2.0f + 4.0f;
      canvas->drawLine(cx - halfWidth, ySnap, cx + halfWidth, ySnap, snapPaint);
  }
}

} // namespace zenith
