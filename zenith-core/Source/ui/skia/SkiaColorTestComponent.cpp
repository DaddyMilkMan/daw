/**
 * @file SkiaColorTestComponent.cpp
 * @brief Implementation of SkiaColorTestComponent
 */

#include "SkiaColorTestComponent.h"

#ifdef ZENITH_USE_SKIA
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkTypeface.h"
#include "include/effects/SkGradientShader.h"

#endif

namespace zenith {

SkiaColorTestComponent::SkiaColorTestComponent() {
  // Set a default size
  setSize(400, 300);
}

void SkiaColorTestComponent::paint(juce::Graphics &g) {
  // Leave empty to allow Skia rendering (from parent) to show through.
  // We can draw a subtle border to show the component exists if needed,
  // but for the final look, we want it clean.
}

void SkiaColorTestComponent::paintToSkia(SkCanvas *canvas, SkRect bounds) {
#ifdef ZENITH_USE_SKIA
  // Draw a semi-transparent background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(220, 30, 30, 30));
  canvas->drawRect(bounds, bgPaint);

  // Draw a grid of colorful gradients
  int rows = 5;
  int cols = 5;
  float padding = 10.0f;
  float cellW = (bounds.width() - padding * (cols + 1)) / cols;
  float cellH = (bounds.height() - padding * (rows + 1)) / rows;

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float x = bounds.left() + padding + c * (cellW + padding);
      float y = bounds.top() + padding + r * (cellH + padding);
      SkRect cell = SkRect::MakeXYWH(x, y, cellW, cellH);

      // Create a unique gradient for each cell based on position
      SkColor colors[2];
      float hue1 = (r * cols + c) * (360.0f / (rows * cols));
      float hue2 = hue1 + 45.0f;
      if (hue2 > 360.0f)
        hue2 -= 360.0f;

      SkScalar hsv1[3] = {hue1, 0.8f, 0.9f};
      colors[0] = SkHSVToColor(hsv1);
      SkScalar hsv2[3] = {hue2, 1.0f, 0.6f};
      colors[1] = SkHSVToColor(hsv2);

      SkPoint pts[2] = {{cell.left(), cell.top()},
                        {cell.right(), cell.bottom()}};

      SkPaint cellPaint;
      cellPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                       SkTileMode::kClamp));
      cellPaint.setAntiAlias(true);

      // Draw rounded rect
      canvas->drawRoundRect(cell, 8.0f, 8.0f, cellPaint);

      // Draw a white border
      SkPaint borderPaint;
      borderPaint.setColor(SK_ColorWHITE);
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(2.0f);
      borderPaint.setAntiAlias(true);
      canvas->drawRoundRect(cell, 8.0f, 8.0f, borderPaint);
    }
  }

  // Draw a title
  SkFont font;
  font.setSize(24.0f);
  font.setEdging(SkFont::Edging::kAntiAlias);

  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE);
  textPaint.setAntiAlias(true);

  // Draw a shadow for text
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(128, 0, 0, 0));
  shadowPaint.setAntiAlias(true);

  const char *title = "Skia Gradient Test";
  canvas->drawString(title, bounds.centerX() - 90, bounds.centerY(), font,
                     shadowPaint);
  canvas->drawString(title, bounds.centerX() - 92, bounds.centerY() - 2, font,
                     textPaint);

#endif
}

} // namespace zenith
