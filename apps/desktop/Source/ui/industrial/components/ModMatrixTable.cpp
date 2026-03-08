#include "ModMatrixTable.h"
#include <core/SkFont.h>

namespace zenith::industrial {

ModMatrixTable::ModMatrixTable(IndustrialTheme &theme) : theme_(theme) {
  rows_ = {
      {"LFO 1", "CUTOFF", 0.70f},
      {"ENV 2", "PITCH", 0.40f},
      {"VELO", "RES", 0.92f},
  };
}

void ModMatrixTable::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);

  paint.setColor(IndustrialTheme::GRAPHITE);
  canvas->drawRect(bounds_, paint);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(IndustrialTheme::SILVER);
  canvas->drawRect(bounds_, paint);
  paint.setStyle(SkPaint::kFill_Style);

  SkFont font(theme_.getTypeface(IndustrialTheme::FontWeight::Medium), 11.0f);
  font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
  paint.setColor(IndustrialTheme::WHITE);
  canvas->drawString("MODULATION MATRIX", bounds_.x() + 8.0f,
                     bounds_.y() + 14.0f, font, paint);
  paint.setColor(IndustrialTheme::AMBER);
  canvas->drawString("+ ADD SLOT", bounds_.right() - 74.0f, bounds_.y() + 14.0f,
                     font, paint);

  const float top = bounds_.y() + 24.0f;
  const float rowH = 20.0f;
  for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
    const auto &row = rows_[i];
    const float y = top + static_cast<float>(i) * rowH;
    const bool active = i == dragRow_;

    if (active) {
      paint.setColor(0x44FF8800);
      canvas->drawRect(
          SkRect::MakeXYWH(bounds_.x() + 1.0f, y, bounds_.width() - 2.0f, rowH),
          paint);
    }

    paint.setColor(IndustrialTheme::LIGHT_GRAY);
    canvas->drawString("x", bounds_.x() + 6.0f, y + 14.0f, font, paint);
    canvas->drawString(row.source.c_str(), bounds_.x() + 26.0f, y + 14.0f, font,
                       paint);
    canvas->drawString(row.dest.c_str(), bounds_.x() + 112.0f, y + 14.0f, font,
                       paint);

    SkRect meter =
        SkRect::MakeXYWH(bounds_.right() - 100.0f, y + 5.0f, 82.0f, 10.0f);
    paint.setColor(IndustrialTheme::STEEL_DARK);
    canvas->drawRect(meter, paint);
    paint.setColor(IndustrialTheme::ALUMINUM);
    canvas->drawRect(SkRect::MakeXYWH(meter.x(), meter.y(),
                                      meter.width() * row.depth,
                                      meter.height()),
                     paint);
  }
}

void ModMatrixTable::handleMouseDown(const MouseEvent &e) {
  if (!hitTest(e.x, e.y)) {
    return;
  }
  const float top = bounds_.y() + 24.0f;
  const int idx = static_cast<int>((e.y - top) / 20.0f);
  if (idx >= 0 && idx < static_cast<int>(rows_.size())) {
    dragRow_ = idx;
    isPressed_ = true;
  }
}

void ModMatrixTable::handleMouseDrag(const MouseEvent &e) {
  if (!isPressed_ || dragRow_ < 0 ||
      dragRow_ >= static_cast<int>(rows_.size())) {
    return;
  }

  const auto barLeft = bounds_.right() - 100.0f;
  const auto t = juce::jlimit(0.0f, 1.0f, (e.x - barLeft) / 82.0f);
  rows_[dragRow_].depth = t;
}

void ModMatrixTable::handleMouseUp(const MouseEvent &e) {
  juce::ignoreUnused(e);
  isPressed_ = false;
  dragRow_ = -1;
}

} // namespace zenith::industrial
