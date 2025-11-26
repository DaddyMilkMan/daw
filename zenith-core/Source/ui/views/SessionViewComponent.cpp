#include "SessionViewComponent.h"
// Ensure you include the SkiaTheme headers
#include "../skia/SkiaTheme.h"

namespace zenith {

SessionViewComponent::SessionViewComponent() {
  // Make sure we intercept mouse events
  setWantsKeyboardFocus(true);
}

SessionViewComponent::~SessionViewComponent() {}

void SessionViewComponent::mouseDown(const juce::MouseEvent &e) {
  // Placeholder
}

void SessionViewComponent::mouseMove(const juce::MouseEvent &e) {
  // Placeholder
}

// This is called every frame (60fps) by the GPU thread
void SessionViewComponent::paintSkia(SkCanvas &canvas,
                                     const juce::Rectangle<int> &bounds) {
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

  // Draw text (if you have font setup, otherwise stick to shapes first)
  // canvas.drawString("SKIA SESSION VIEW", 10, 20, SkFont(), paint);
}

} // namespace zenith
