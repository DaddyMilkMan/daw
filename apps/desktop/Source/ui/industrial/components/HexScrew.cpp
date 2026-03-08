#include "HexScrew.h"
#include <cmath>

namespace zenith::industrial {

void HexScrew::render(SkCanvas* canvas) {
  if (!isVisible()) {
    return;
  }

  const float r = juce::jmin(bounds_.width(), bounds_.height()) * 0.5f;
  theme_.drawHexScrew(canvas, bounds_.centerX(), bounds_.centerY(), r);
}

} // namespace zenith::industrial
