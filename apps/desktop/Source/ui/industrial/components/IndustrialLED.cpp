#include "IndustrialLED.h"

namespace zenith::industrial {

IndustrialLED::IndustrialLED(IndustrialTheme& theme) : theme_(theme) {}

void IndustrialLED::render(SkCanvas* canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(active_ ? IndustrialTheme::AMBER : IndustrialTheme::STEEL);

  const SkRect rect = SkRect::MakeXYWH(bounds_.x(), bounds_.y(), 3.0f, 3.0f);
  canvas->drawRect(rect, paint);

  if (active_) {
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 1.0f));
    paint.setColor(IndustrialTheme::AMBER);
    canvas->drawRect(SkRect::MakeXYWH(bounds_.x() - 0.5f, bounds_.y() - 0.5f, 4.0f, 4.0f), paint);
    paint.setMaskFilter(nullptr);
  }
}

} // namespace zenith::industrial
