#include "IndustrialButton.h"
#include <core/SkFont.h>

namespace zenith::industrial {

IndustrialButton::IndustrialButton(std::string label, IndustrialTheme &theme,
                                   Shape shape, bool toggle)
    : label_(std::move(label)), theme_(theme), shape_(shape), toggle_(toggle) {}

void IndustrialButton::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(isOn_ ? IndustrialTheme::ALUMINUM : IndustrialTheme::STEEL);
  if (isPressed_) {
    paint.setColor(0xFF242424);
  } else if (isHovered_ && !isOn_) {
    paint.setColor(0xFF303030);
  }

  const float radius = shape_ == Shape::Pill ? bounds_.height() * 0.5f : 4.0f;
  if (shape_ == Shape::Circle) {
    canvas->drawCircle(bounds_.centerX(), bounds_.centerY(),
                       juce::jmin(bounds_.width(), bounds_.height()) * 0.5f,
                       paint);
  } else {
    canvas->drawRRect(SkRRect::MakeRectXY(bounds_, radius, radius), paint);
  }

  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(isOn_ ? IndustrialTheme::AMBER
                       : (isHovered_ ? IndustrialTheme::SILVER
                                     : IndustrialTheme::ALUMINUM));

  if (shape_ == Shape::Circle) {
    canvas->drawCircle(
        bounds_.centerX(), bounds_.centerY(),
        juce::jmin(bounds_.width(), bounds_.height()) * 0.5f - 0.5f, paint);
  } else {
    canvas->drawRRect(SkRRect::MakeRectXY(bounds_, radius, radius), paint);
  }

  SkFont font(theme_.getTypeface(IndustrialTheme::FontWeight::Medium), 11.0f);
  font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(isOn_ ? IndustrialTheme::WHITE : IndustrialTheme::LIGHT_GRAY);

  SkRect textBounds;
  font.measureText(label_.c_str(), label_.size(), SkTextEncoding::kUTF8,
                   &textBounds);
  const float textX = bounds_.centerX() - textBounds.width() * 0.5f;
  const float textY = bounds_.centerY() + textBounds.height() * 0.5f - 1.0f;
  canvas->drawString(label_.c_str(), textX, textY, font, paint);
}

void IndustrialButton::handleMouseDown(const MouseEvent &e) {
  if (hitTest(e.x, e.y)) {
    isPressed_ = true;
  }
}

void IndustrialButton::handleMouseUp(const MouseEvent &e) {
  if (isPressed_ && hitTest(e.x, e.y)) {
    if (toggle_) {
      isOn_ = !isOn_;
    }
    if (onToggle) {
      onToggle(isOn_);
    }
  }
  isPressed_ = false;
}

} // namespace zenith::industrial
