#include "IndustrialToggle.h"

namespace zenith::industrial {

IndustrialToggle::IndustrialToggle(IndustrialTheme &theme) : theme_(theme) {}

void IndustrialToggle::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(on_ ? 0xFF303030 : IndustrialTheme::STEEL);
  canvas->drawRRect(SkRRect::MakeRectXY(bounds_, bounds_.height() * 0.5f,
                                        bounds_.height() * 0.5f),
                    paint);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(on_ ? IndustrialTheme::AMBER : IndustrialTheme::ALUMINUM);
  canvas->drawRRect(SkRRect::MakeRectXY(bounds_, bounds_.height() * 0.5f,
                                        bounds_.height() * 0.5f),
                    paint);
  paint.setStyle(SkPaint::kFill_Style);

  const float knobR = bounds_.height() * 0.42f;
  const float cx =
      on_ ? (bounds_.right() - knobR - 1.0f) : (bounds_.x() + knobR + 1.0f);
  paint.setColor(on_ ? IndustrialTheme::AMBER : IndustrialTheme::ALUMINUM);
  canvas->drawCircle(cx, bounds_.centerY(), knobR, paint);

  if (on_) {
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 1.2f));
    paint.setColor(IndustrialTheme::AMBER);
    canvas->drawCircle(cx, bounds_.centerY(), knobR + 1.2f, paint);
    paint.setMaskFilter(nullptr);
  }
}

void IndustrialToggle::handleMouseUp(const MouseEvent &e) {
  if (hitTest(e.x, e.y)) {
    on_ = !on_;
  }
}

} // namespace zenith::industrial
