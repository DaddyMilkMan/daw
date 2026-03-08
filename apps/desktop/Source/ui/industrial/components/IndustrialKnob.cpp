#include "IndustrialKnob.h"
#include <cmath>
#include <core/SkFont.h>
#include <effects/SkGradientShader.h>

namespace zenith::industrial {

IndustrialKnob::IndustrialKnob(std::string label, std::string unit,
                               float initialValue, IndustrialTheme &theme,
                               Size size, bool expertDecorations)
    : label_(std::move(label)), unit_(std::move(unit)),
      value_(juce::jlimit(0.0f, 1.0f, initialValue)), size_(size),
      expertDecorations_(expertDecorations), theme_(theme) {}

void IndustrialKnob::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  const float sizePx = juce::jmin(bounds_.width(), bounds_.height() - 26.0f);
  const float cx = bounds_.centerX();
  const float cy = bounds_.y() + sizePx * 0.5f + 14.0f;
  const float radius = sizePx * 0.45f;

  SkPaint paint;
  paint.setAntiAlias(true);

  SkColor knobTop = isPressed_ ? 0xFF353535 : IndustrialTheme::STEEL;
  SkColor knobBottom = IndustrialTheme::STEEL_DARK;
  SkPoint points[2] = {{cx, cy - radius}, {cx, cy + radius}};
  SkColor colors[2] = {knobTop, knobBottom};
  paint.setShader(SkGradientShader::MakeLinear(points, colors, nullptr, 2,
                                               SkTileMode::kClamp));
  canvas->drawCircle(cx, cy, radius, paint);

  paint.setShader(nullptr);
  paint.setColor(0x22000000);
  canvas->drawCircle(cx, cy + 1.0f, radius - 3.0f, paint);

  paint.setShader(nullptr);
  paint.setColor(IndustrialTheme::SILVER);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  canvas->drawCircle(cx, cy, radius, paint);

  paint.setColor(0x44707070);
  canvas->drawCircle(cx, cy, radius - 4.0f, paint);

  if (expertDecorations_) {
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(IndustrialTheme::SILVER);
    for (int i = 0; i < 4; ++i) {
      const float angle =
          static_cast<float>(i) * juce::MathConstants<float>::halfPi;
      const float px = cx + std::cos(angle) * (radius + 3.0f);
      const float py = cy + std::sin(angle) * (radius + 3.0f);
      canvas->drawCircle(px, py, 1.0f, paint);
    }

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(0x996A6A6A);
    for (int i = 0; i < 8; ++i) {
      const float angle = juce::MathConstants<float>::pi * 1.25f +
                          static_cast<float>(i) *
                              (juce::MathConstants<float>::pi * 1.5f / 7.0f);
      const float x1 = cx + std::cos(angle) * (radius + 1.5f);
      const float y1 = cy + std::sin(angle) * (radius + 1.5f);
      const float x2 = cx + std::cos(angle) * (radius + 4.0f);
      const float y2 = cy + std::sin(angle) * (radius + 4.0f);
      canvas->drawLine(x1, y1, x2, y2, paint);
    }
  }

  const float startAngle = juce::MathConstants<float>::pi * 1.25f;
  const float sweep = juce::MathConstants<float>::pi * 1.5f * value_;
  const float angle = startAngle + sweep;

  paint.setColor(IndustrialTheme::AMBER);
  paint.setStrokeWidth(isPressed_ ? 3.0f : 2.0f);
  paint.setStyle(SkPaint::kStroke_Style);
  canvas->drawLine(cx, cy, cx + std::cos(angle) * (radius - 4.0f),
                   cy + std::sin(angle) * (radius - 4.0f), paint);

  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(0xFF1A1A1A);
  canvas->drawCircle(cx, cy, radius * 0.2f, paint);

  SkFont labelFont(theme_.getTypeface(IndustrialTheme::FontWeight::Medium),
                   11.0f);
  SkFont valueFont(theme_.getTypeface(IndustrialTheme::FontWeight::Regular),
                   13.0f);
  labelFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
  valueFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);

  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(IndustrialTheme::LIGHT_GRAY);
  const juce::String labelUpper = juce::String(label_).toUpperCase();
  canvas->drawString(labelUpper.toRawUTF8(), bounds_.x() + 2.0f,
                     bounds_.bottom() - 8.0f, labelFont, paint);

  const auto valueText = juce::String(value_ * 100.0f, 1).toStdString();
  SkRect valueBounds;
  valueFont.measureText(valueText.c_str(), valueText.size(),
                        SkTextEncoding::kUTF8, &valueBounds);
  paint.setColor(IndustrialTheme::WHITE);
  const float valueX = bounds_.right() - 8.0f - valueBounds.width();
  canvas->drawString(valueText.c_str(), valueX, bounds_.y() + 12.0f, valueFont,
                     paint);

  if (!unit_.empty()) {
    SkRect unitBounds;
    labelFont.measureText(unit_.c_str(), unit_.size(), SkTextEncoding::kUTF8,
                          &unitBounds);
    paint.setColor(0xFF707070);
    canvas->drawString(unit_.c_str(),
                       bounds_.right() - 6.0f - unitBounds.width(),
                       bounds_.y() + 12.0f, labelFont, paint);
  }

  if (showTooltip_ || isPressed_) {
    const auto tooltip = valueText + (unit_.empty() ? "" : (" " + unit_));
    const SkRect tooltipRect = SkRect::MakeXYWH(
        bounds_.centerX() - 34.0f, bounds_.y() - 17.0f, 68.0f, 14.0f);
    paint.setColor(IndustrialTheme::AMBER);
    canvas->drawRRect(SkRRect::MakeRectXY(tooltipRect, 3.0f, 3.0f), paint);
    paint.setColor(IndustrialTheme::WHITE);
    canvas->drawString(tooltip.c_str(), tooltipRect.x() + 5.0f,
                       tooltipRect.bottom() - 3.0f, labelFont, paint);
  }
}

void IndustrialKnob::handleMouseDown(const MouseEvent &e) {
  if (!hitTest(e.x, e.y)) {
    return;
  }
  isPressed_ = true;
  dragStartValue_ = value_;
  dragStartY_ = e.y;
}

void IndustrialKnob::handleMouseDrag(const MouseEvent &e) {
  if (!isPressed_) {
    return;
  }
  const float delta = (dragStartY_ - e.y) / 200.0f;
  const float scale = e.shiftDown ? 0.1f : 1.0f;
  setValue(dragStartValue_ + delta * scale);
}

void IndustrialKnob::handleMouseUp(const MouseEvent &e) {
  juce::ignoreUnused(e);
  isPressed_ = false;
}

void IndustrialKnob::handleMouseMove(const MouseEvent &e) {
  IndustrialComponent::handleMouseMove(e);
  showTooltip_ = hitTest(e.x, e.y);
}

void IndustrialKnob::setValue(float value) {
  value_ = juce::jlimit(0.0f, 1.0f, value);
  notifyValueChanged(value_);
}

} // namespace zenith::industrial
