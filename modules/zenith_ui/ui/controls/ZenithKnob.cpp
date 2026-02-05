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

    ZenithKnob.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the premium Zenith rotary knob.

  ==============================================================================

*/

#include "ZenithKnob.h"

#include "ZenithKnob.h"
#include <algorithm>
#include <cmath>
#include <string>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <utils/SkTextUtils.h>
#endif

namespace zenith {

ZenithKnob::ZenithKnob() : ZenithControl("") {
  accentColor_ = design::colors::CYAN;
}

ZenithKnob::ZenithKnob(const juce::String &name, SkColor color)
    : ZenithControl(name) {
  accentColor_ = color;
}

ZenithKnob::ZenithKnob(juce::Value valueToControl)
    : ZenithControl(""), value(valueToControl) {
  value.addListener(this);
  // Initial sync
  setValue(value.getValue(), false);
}

ZenithKnob::~ZenithKnob() { value.removeListener(this); }

void ZenithKnob::mouseDrag(const juce::MouseEvent &e) {
  if (!isDragging_ || !isEnabled())
    return;

  // Calculate drag distance (vertical for intuitive control)
  float dragDistance = dragStartPos_.y - e.position.y;

  // Apply sensitivity (pixels per full range)
  float sensitivity = 200.0f;
  if (isFineMode_) {
    sensitivity *= 1.0f / fineControlMultiplier_;
  }

  float rangeDelta = dragDistance / sensitivity;
  float newNormValue = juce::jlimit(0.0f, 1.0f,
                                    (dragStartValue_ - range_.start) /
                                            (range_.end - range_.start) +
                                        rangeDelta);

  // Snap to ticks if enabled
  if (snapToTicks_ && tickCount_ > 1) {
    float step = 1.0f / (tickCount_ - 1);
    newNormValue = std::round(newNormValue / step) * step;
  }

  float newValue = range_.start + newNormValue * (range_.end - range_.start);

  if (!value.getValue().isVoid()) {
    value.setValue(newValue);
  } else {
    setValue(newValue, true);
  }

  // Trigger animation
  lastChangeTime_ = juce::Time::currentTimeMillis();
  animatedGlow_ = 1.0f;

  if (onValueChange) {
    onValueChange();
  }
}

void ZenithKnob::mouseEnter(const juce::MouseEvent &e) {
  ZenithControl::mouseEnter(e);
  int duration = design::Settings::isReducedMotionEnabled() ? 0 : design::animation::DURATION_FAST;
  animateTo("hover", 1.0f, duration);
}

void ZenithKnob::mouseExit(const juce::MouseEvent &e) {
  ZenithControl::mouseExit(e);
  int duration = design::Settings::isReducedMotionEnabled() ? 0 : design::animation::DURATION_FAST;
  animateTo("hover", 0.0f, duration);
}

void ZenithKnob::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  auto bounds = getLocalBounds().toFloat();
  float cx = bounds.getCentreX();
  float cy = bounds.getCentreY();

  // Account for label space
  float availableHeight = showLabel_ ? bounds.getHeight() - labelYOffset_ - 5.0f
                                     : bounds.getHeight();
  float radius = std::min(bounds.getWidth(), availableHeight) * 0.4f;

  // Decay animation
  juce::int64 now = juce::Time::currentTimeMillis();
  float timeSinceChange = static_cast<float>(now - lastChangeTime_) / 1000.0f;
  animatedGlow_ = std::max(0.0f, 1.0f - timeSinceChange * 3.0f);

  // 1. Background Track (Deep Groove)
  drawTrack(canvas, cx, cy, radius);

  // 2. Tick Marks (optional)
  if (showTicks_) {
    drawTickMarks(canvas, cx, cy, radius);
  }

  // 3. Value Arc with Gradient
  drawValueArc(canvas, cx, cy, radius);

  // 3.5 Modulation Ring
  if (std::abs(modulationAmount_) > 0.001f) {
    drawModulationRing(canvas, cx, cy, radius);
  }

  // 4. Center Cap (Metallic)
  drawCenterCap(canvas, cx, cy, radius);

  // 5. Indicator Line
  drawIndicator(canvas, cx, cy, radius);

  // 6. Value Display (on hover/drag)
  if ((isHovered_ && showValueOnHover_) ||
      (isDragging_ && showValueWhileDragging_)) {
    drawValueTooltip(canvas, cx, cy, radius);
  }

  // 7. Label
  if (showLabel_) {
    drawLabel(canvas, cx, cy, radius);
  }
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

void ZenithKnob::drawTrack(SkCanvas *canvas, float cx, float cy, float radius) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(trackWidth_);
  paint.setStrokeCap(SkPaint::kRound_Cap);

  // Deep dark track
  paint.setColor(design::colors::BG_DARKEST);

  SkRect arcRect =
      SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);
  canvas->drawArc(arcRect, startAngle_, sweepRange_, false, paint);

  // Inner shadow simulation
  paint.setStrokeWidth(1.0f);
  paint.setColor(design::withAlpha(SK_ColorBLACK, 0.3f));
  canvas->drawArc(arcRect, startAngle_, sweepRange_, false, paint);
}

void ZenithKnob::drawTickMarks(SkCanvas *canvas, float cx, float cy,
                               float radius) {
  if (tickCount_ < 2)
    return;

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.5f);
  paint.setStrokeCap(SkPaint::kRound_Cap);

  float tickRadius = radius + trackWidth_ / 2 + 4.0f;
  float tickLength = 4.0f;

  for (int i = 0; i < tickCount_; ++i) {
    float normalizedPos = static_cast<float>(i) / (tickCount_ - 1);
    float angle = startAngle_ + normalizedPos * sweepRange_;
    float radians = angle * 3.14159265f / 180.0f;

    float x1 = cx + std::cos(radians) * tickRadius;
    float y1 = cy + std::sin(radians) * tickRadius;
    float x2 = cx + std::cos(radians) * (tickRadius + tickLength);
    float y2 = cy + std::sin(radians) * (tickRadius + tickLength);

    // Highlight tick at current value position
    float normValue = getNormalizedValue();
    float tickTolerance = 0.5f / (tickCount_ - 1);

    if (std::abs(normalizedPos - normValue) < tickTolerance) {
      paint.setColor(accentColor_);
    } else {
      paint.setColor(design::colors::TEXT_TERTIARY);
    }

    canvas->drawLine(x1, y1, x2, y2, paint);
  }
}

void ZenithKnob::drawValueArc(SkCanvas *canvas, float cx, float cy,
                              float radius) {
  float normValue = getNormalizedValue();
  if (normValue < 0.001f && mode_ == Mode::Unipolar)
    return;

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(trackWidth_);
  paint.setStrokeCap(SkPaint::kRound_Cap);

  SkRect arcRect =
      SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);

  float arcStartAngle = startAngle_;
  float arcSweep = normValue * sweepRange_;

  // Bipolar mode: draw from center
  if (mode_ == Mode::Bipolar) {
    float centerAngle = startAngle_ + sweepRange_ * 0.5f;
    if (normValue < 0.5f) {
      arcStartAngle = startAngle_ + normValue * sweepRange_;
      arcSweep = (0.5f - normValue) * sweepRange_;
    } else {
      arcStartAngle = centerAngle;
      arcSweep = (normValue - 0.5f) * sweepRange_;
    }
  }

  // Create gradient shader
  paint.setShader(createArcGradient(cx, cy, radius));

  // Glow effect
  float hoverAnim = getAnimatedValue("hover");
  float glowAmount =
      (glowIntensity_ + animatedGlow_ * 0.5f) *
      design::interpolate(design::effects::GLOW_SUBTLE, design::effects::GLOW_STRONG, hoverAnim);
  if (glowAmount > 0.0f) {
    paint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, glowAmount));
    canvas->drawArc(arcRect, arcStartAngle, arcSweep, false, paint);
    paint.setMaskFilter(nullptr);
  }

  // Main arc (sharp)
  canvas->drawArc(arcRect, arcStartAngle, arcSweep, false, paint);
  paint.setShader(nullptr);
}

void ZenithKnob::drawModulationRing(SkCanvas *canvas, float cx, float cy, float radius) {
  float normValue = getNormalizedValue();
  float modTarget = juce::jlimit(0.0f, 1.0f, normValue + modulationAmount_);
  
  if (std::abs(modTarget - normValue) < 0.001f) return;

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(trackWidth_ * 0.8f); // Slightly thinner than main arc
  paint.setStrokeCap(SkPaint::kRound_Cap);
  paint.setColor(modulationColor_);

  SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);

  // Calculate angles
  float startDeg = startAngle_ + normValue * sweepRange_;
  float endDeg = startAngle_ + modTarget * sweepRange_;
  float sweepDeg = endDeg - startDeg;

  // Draw glow for modulation
  paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
  paint.setStrokeWidth(trackWidth_ + 2.0f);
  paint.setColor(SkColorSetA(modulationColor_, 100)); // Transparent glow
  canvas->drawArc(arcRect, startDeg, sweepDeg, false, paint);

  // Draw main modulation arc
  paint.setMaskFilter(nullptr);
  paint.setStrokeWidth(trackWidth_ * 0.8f);
  paint.setColor(modulationColor_);
  canvas->drawArc(arcRect, startDeg, sweepDeg, false, paint);
}

void ZenithKnob::drawCenterCap(SkCanvas *canvas, float cx, float cy,
                               float radius) {
  float capRadius = radius * 0.7f;

  SkPaint paint;
  paint.setAntiAlias(true);

  // Drop shadow
  SkPaint shadowPaint;
  shadowPaint.setAntiAlias(true);
  shadowPaint.setColor(design::colors::GLASS_SHADOW);
  shadowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, design::effects::GLOW_MEDIUM));
  canvas->drawCircle(cx, cy + 2.0f, capRadius, shadowPaint);

  // Cap gradient (subtle convex look)
  SkColor capColors[2] = {design::colors::BG_MEDIUM, design::colors::BG_DARKER};
  SkPoint capPts[2] = {{cx, cy - capRadius}, {cx, cy + capRadius}};
  paint.setShader(SkGradientShader::MakeLinear(capPts, capColors, nullptr, 2,
                                               SkTileMode::kClamp));
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawCircle(cx, cy, capRadius, paint);
  paint.setShader(nullptr);

  // Cap rim highlight
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(design::colors::GLASS_HIGHLIGHT);
  canvas->drawCircle(cx, cy, capRadius, paint);
}

void ZenithKnob::drawIndicator(SkCanvas *canvas, float cx, float cy,
                               float radius) {
  float capRadius = radius * 0.7f;
  float normValue = getNormalizedValue();
  float angle = startAngle_ + normValue * sweepRange_;

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(design::colors::TEXT_PRIMARY);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(2.0f);
  paint.setStrokeCap(SkPaint::kRound_Cap);

  canvas->save();
  canvas->rotate(angle, cx, cy);
  canvas->drawLine(cx, cy - capRadius * 0.5f, cx, cy - capRadius + 4.0f, paint);
  canvas->restore();
}

void ZenithKnob::drawValueTooltip(SkCanvas *canvas, float cx, float cy,
                                  float radius) {
  juce::String valueText = getValueAsText();
  std::string str = valueText.toStdString();

  SkFont font = design::getMonoFont(design::typography::FONT_XS);
  font.setSubpixel(true);

  float textWidth =
      font.measureText(str.c_str(), str.length(), SkTextEncoding::kUTF8);

  // Position tooltip above the knob, clamped to top
  float tooltipY = std::max(2.0f, cy - radius - 15.0f);
  float tooltipX = cx - textWidth / 2.0f;

  // Background pill
  float padding = 6.0f;
  SkRect bgRect = SkRect::MakeXYWH(tooltipX - padding, tooltipY - 12.0f,
                                   textWidth + padding * 2, 16.0f);
  SkRRect bgRRect = SkRRect::MakeRectXY(bgRect, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM);

  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::withAlpha(design::colors::BG_DARKEST, 0.8f));
  canvas->drawRRect(bgRRect, bgPaint);

  // Border
  bgPaint.setStyle(SkPaint::kStroke_Style);
  bgPaint.setStrokeWidth(1.0f);
  bgPaint.setColor(design::colors::BORDER_DEFAULT);
  canvas->drawRRect(bgRRect, bgPaint);

  // Text
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                         tooltipX, tooltipY, font, textPaint);
}

void ZenithKnob::drawLabel(SkCanvas *canvas, float cx, float cy, float radius) {
  if (name_.isEmpty())
    return;

  SkFont font = design::getSkFont(design::typography::FONT_XS);
  font.setSubpixel(true);

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(design::colors::TEXT_SECONDARY);

  std::string labelStr = name_.toStdString();
  float textWidth = font.measureText(labelStr.c_str(), labelStr.length(),
                                     SkTextEncoding::kUTF8);

  canvas->drawSimpleText(labelStr.c_str(), labelStr.length(),
                         SkTextEncoding::kUTF8, cx - textWidth / 2,
                         cy + radius + labelYOffset_, font, paint);
}

float ZenithKnob::getAngleForValue(float normalizedValue) const {
  return startAngle_ + normalizedValue * sweepRange_;
}

sk_sp<SkShader> ZenithKnob::createArcGradient(float cx, float cy,
                                              float radius) const {
  // Sweep gradient following the arc for that "Neon Glow" look
  SkColor colors[4] = {accentColor_, design::colors::MAGENTA,
                       design::colors::CYAN, accentColor_};

  // Conical/Sweep gradient setup
  // Note: Skia angles are in degrees, 0 is at 3 o'clock.
  // We need to rotate the sweep to match our startAngle_
  SkMatrix matrix;
  matrix.setRotate(startAngle_, cx, cy);

  return SkGradientShader::MakeSweep(cx, cy, colors, nullptr, 4,
                                     SkTileMode::kClamp, 0, sweepRange_, 0,
                                     &matrix);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
