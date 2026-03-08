#include "VisualizerDisplay.h"
#include <cmath>

namespace zenith::industrial {

VisualizerDisplay::VisualizerDisplay(IndustrialTheme &theme) : theme_(theme) {
  samples_.resize(256, 0.0f);
}

void VisualizerDisplay::tick() {
  phase_ += 0.08f;
  for (size_t i = 0; i < samples_.size(); ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(samples_.size());
    samples_[i] =
        0.5f * std::sin(phase_ + t * juce::MathConstants<float>::twoPi * 2.0f) +
        0.2f * std::sin(phase_ * 0.5f +
                        t * juce::MathConstants<float>::twoPi * 7.0f);
  }
}

void VisualizerDisplay::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(IndustrialTheme::CARBON_BLACK);
  canvas->drawRect(bounds_, paint);

  paint.setColor(IndustrialTheme::STEEL_DARK);
  for (int i = 1; i < 8; ++i) {
    const float x =
        bounds_.x() + (bounds_.width() / 8.0f) * static_cast<float>(i);
    canvas->drawLine(x, bounds_.y(), x, bounds_.bottom(), paint);
  }
  for (int i = 1; i < 4; ++i) {
    const float y =
        bounds_.y() + (bounds_.height() / 4.0f) * static_cast<float>(i);
    canvas->drawLine(bounds_.x(), y, bounds_.right(), y, paint);
  }
  paint.setColor(0xFF2E2E2E);
  canvas->drawLine(bounds_.x(), bounds_.centerY(), bounds_.right(),
                   bounds_.centerY(), paint);

  SkPath path;
  const float mid = bounds_.centerY();
  for (size_t i = 0; i < samples_.size(); ++i) {
    const float x = bounds_.x() +
                    bounds_.width() * (static_cast<float>(i) /
                                       static_cast<float>(samples_.size() - 1));
    const float y = mid - samples_[i] * (bounds_.height() * 0.4f);
    if (i == 0) {
      path.moveTo(x, y);
    } else {
      path.lineTo(x, y);
    }
  }

  paint.setColor(IndustrialTheme::CYAN);
  paint.setStrokeWidth(1.5f);
  paint.setStyle(SkPaint::kStroke_Style);
  canvas->drawPath(path, paint);
}

} // namespace zenith::industrial
