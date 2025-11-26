#include "SessionViewComponent.h"
#include "../../../src/SimpleLogger.h"
#include "../skia/SkiaTheme.h"

namespace zenith {

SessionViewComponent::SessionViewComponent() { setWantsKeyboardFocus(true); }

SessionViewComponent::~SessionViewComponent() {}

void SessionViewComponent::mouseDown(const juce::MouseEvent &e) {
  // Placeholder
}

void SessionViewComponent::mouseMove(const juce::MouseEvent &e) {
  // Placeholder
}

void SessionViewComponent::paintSkia(SkCanvas &canvas,
                                     const juce::Rectangle<int> &bounds) {
  static int frameCount = 0;
  if (frameCount++ % 60 == 0) {
    logToFile("SessionViewComponent::paintSkia - Frame " +
              std::to_string(frameCount) +
              " Bounds: " + std::to_string(bounds.getWidth()) + "x" +
              std::to_string(bounds.getHeight()));
  }

  // 1. Background
  SkPaint paint;
  paint.setColor(SkColorSetRGB(30, 30, 35)); // Dark grey panel
  canvas.drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), paint);

  // 2. Draw Grid
  paint.setColor(SkColorSetRGB(0, 160, 255)); // Cyan
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(2.0f);

  // Draw a test circle to prove Skia is working
  canvas.drawCircle(bounds.getWidth() / 2, bounds.getHeight() / 2, 50, paint);
}

} // namespace zenith
