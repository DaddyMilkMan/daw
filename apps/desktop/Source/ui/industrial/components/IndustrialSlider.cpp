#include "IndustrialSlider.h"
#include <core/SkFont.h>

namespace zenith::industrial {

IndustrialSlider::IndustrialSlider(std::string label, float initialValue,
                                   IndustrialTheme &theme)
    : label_(std::move(label)), value_(juce::jlimit(0.0f, 1.0f, initialValue)),
      theme_(theme) {}

void IndustrialSlider::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);

  SkRect track = SkRect::MakeXYWH(bounds_.centerX() - 4.0f, bounds_.y() + 16.0f,
                                  8.0f, bounds_.height() - 34.0f);
  paint.setColor(IndustrialTheme::STEEL_DARK);
  canvas->drawRRect(SkRRect::MakeRectXY(track, 4.0f, 4.0f), paint);

  SkRect fill = track;
  fill.fTop = juce::jmap(value_, track.bottom(), track.top());
  paint.setColor(IndustrialTheme::ALUMINUM);
  canvas->drawRect(fill, paint);

  const float capY = juce::jmap(value_, track.bottom(), track.top());
  SkRect cap =
      SkRect::MakeXYWH(bounds_.centerX() - 12.0f, capY - 5.0f, 24.0f, 10.0f);
  paint.setColor(IndustrialTheme::STEEL);
  canvas->drawRRect(SkRRect::MakeRectXY(cap, 3.0f, 3.0f), paint);
  paint.setColor(IndustrialTheme::SILVER);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  canvas->drawRRect(SkRRect::MakeRectXY(cap, 3.0f, 3.0f), paint);
  paint.setStyle(SkPaint::kFill_Style);

  SkFont font(theme_.getTypeface(IndustrialTheme::FontWeight::Medium), 11.0f);
  font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
  for (int i = 0; i <= 4; ++i) {
    const float y =
        juce::jmap(static_cast<float>(i) / 4.0f, track.bottom(), track.top());
    paint.setColor(0xFF5A5A5A);
    canvas->drawLine(track.right() + 3.0f, y, track.right() + 7.0f, y, paint);
  }

  const auto valueText = juce::String(value_ * 100.0f, 0).toStdString();
  paint.setColor(IndustrialTheme::WHITE);
  canvas->drawString(valueText.c_str(), bounds_.x(), bounds_.y() + 10.0f, font,
                     paint);

  paint.setColor(IndustrialTheme::LIGHT_GRAY);
  const juce::String upperLabel = juce::String(label_).toUpperCase();
  canvas->drawString(upperLabel.toRawUTF8(), bounds_.x(),
                     bounds_.bottom() - 2.0f, font, paint);
}

void IndustrialSlider::handleMouseDown(const MouseEvent &e) {
  if (hitTest(e.x, e.y)) {
    dragging_ = true;
    isPressed_ = true;
    dragStartValue_ = value_;
    dragStartY_ = e.y;
  }
}

void IndustrialSlider::handleMouseDrag(const MouseEvent &e) {
  if (!dragging_) {
    return;
  }
  const float delta = (dragStartY_ - e.y) / 180.0f;
  const float scale = e.shiftDown ? 0.1f : 1.0f;
  value_ = juce::jlimit(0.0f, 1.0f, dragStartValue_ + delta * scale);
}

void IndustrialSlider::handleMouseUp(const MouseEvent &e) {
  juce::ignoreUnused(e);
  dragging_ = false;
  isPressed_ = false;
}

} // namespace zenith::industrial
