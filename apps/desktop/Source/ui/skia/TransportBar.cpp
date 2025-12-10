/*
  ==============================================================================

    TransportBar.cpp
    Created: 2025-11-28
    Author:  Leo "Lil Bit" Rossi

    Implementation of transport controls with Neon Noir styling.

  ==============================================================================
*/

#include "TransportBar.h"

#include <core/SkBlurTypes.h> // Explicitly include
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

#ifdef ZENITH_USE_SKIA
#include <effects/SkGradientShader.h>

namespace zenith {

TransportBar::TransportBar() { setSize(800, 60); }

void TransportBar::resized() {
  auto area = getLocalBounds();
  int buttonWidth = 50;
  int spacing = 10;

  auto leftSection = area.removeFromLeft(250);
  playButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(spacing);
  leftSection.removeFromLeft(spacing);
  stopButtonBounds_ = leftSection.removeFromLeft(buttonWidth).reduced(spacing);
  leftSection.removeFromLeft(spacing);
  recordButtonBounds_ =
      leftSection.removeFromLeft(buttonWidth).reduced(spacing);

  // View Toggle Button (Right side)
  auto rightSection = area.removeFromRight(120); // Increased width
  settingsButtonBounds_ = rightSection.removeFromRight(60).reduced(10);
  viewToggleButtonBounds_ = rightSection.removeFromRight(60).reduced(10);

  // Update cached resources on Message Thread (Safe)
  SkRect skBounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());
  updateCachedPaints(skBounds);
  cachedBounds_ = skBounds;
}

void TransportBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // 1. Background Gradient (Zero allocation)
  canvas->drawRect(skBounds, bgPaint_);

  // 2. Bottom Border Glow
  canvas->drawLine(0.0f, skBounds.height(), skBounds.width(), skBounds.height(),
                   borderPaint_);

  // 3. Draw buttons (Delegated helper - still allocates, needs future fix but
  // acceptable for now)
  drawButton(canvas, playButtonBounds_, "▶", isPlaying_, 0xFF00FF64);  // Green
  drawButton(canvas, stopButtonBounds_, "■", !isPlaying_, 0xFF6464FF); // Blue
  drawButton(canvas, recordButtonBounds_, "●", isRecording_, 0xFFFF3232); // Red

  // View Toggle
  drawButton(canvas, viewToggleButtonBounds_, "↹", false, 0xFFFFFFFF);

  // Settings Button
  drawButton(canvas, settingsButtonBounds_, "⚙", false, 0xFFFFFFFF);

  // 4. Draw Info Text (Tempo & Project)
  SkPaint textPaint; // Stack alloc is cheap
  textPaint.setStyle(SkPaint::kFill_Style);
  textPaint.setColor(SK_ColorWHITE);
  textPaint.setAntiAlias(true);

  // Tempo
  juce::String tempoStr = juce::String(tempo_, 1) + " BPM";
  canvas->drawString(tempoStr.toStdString().c_str(), 260.0f, 38.0f, font_,
                     textPaint);

  // Project Name (Subtle)
  textPaint.setColor(SkColorSetARGB(150, 255, 255, 255));
  canvas->drawString(projectName_.toStdString().c_str(), 380.0f, 37.0f,
                     smallFont_, textPaint);

  // 5. Draw CPU meter
  juce::Rectangle<int> cpuBounds((int)bounds.getWidth() - 250, 20, 100, 20);
  drawMeter(canvas, cpuBounds, cpuUsage_ / 100.0f, "CPU");
}

void TransportBar::updateCachedPaints(const SkRect &bounds) {
  // 1. Background Paint
  bgPaint_.setAntiAlias(true);
  SkPoint pts[2] = {{0, 0}, {0, bounds.height()}};
  SkColor colors[2] = {SkColorSetARGB(240, 20, 20, 25),
                       SkColorSetARGB(240, 10, 10, 15)};
  bgPaint_.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                  SkTileMode::kClamp));
  bgPaint_.setStyle(SkPaint::kFill_Style);

  // 2. Border Paint
  borderPaint_.setAntiAlias(true);
  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setStrokeWidth(1.0f);
  borderPaint_.setColor(SkColorSetARGB(50, 0, 255, 255)); // Cyan glow

  // 3. Fonts
  font_.setSize(18.0f);
  font_.setSubpixel(true);

  smallFont_.setSize(14.0f);
  smallFont_.setSubpixel(true);
}

void TransportBar::drawButton(SkCanvas *canvas,
                              const juce::Rectangle<int> &bounds,
                              const char *label, bool isActive,
                              uint32_t color) {
  SkPaint paint;
  paint.setAntiAlias(true);

  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());
  SkRRect rrect = SkRRect::MakeRectXY(rect, 6.0f, 6.0f);

  // Button Background (Gradient)
  SkPoint pts[2] = {{rect.left(), rect.top()}, {rect.left(), rect.bottom()}};
  SkColor bgColors[2];

  if (isActive) {
    // Active: Glowy Gradient
    bgColors[0] = SkColorSetA(color, 100);
    bgColors[1] = SkColorSetA(color, 50);
  } else {
    // Inactive: Dark Glass
    bgColors[0] = SkColorSetARGB(50, 255, 255, 255);
    bgColors[1] = SkColorSetARGB(20, 255, 255, 255);
  }

  paint.setShader(SkGradientShader::MakeLinear(pts, bgColors, nullptr, 2,
                                               SkTileMode::kClamp));
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRRect(rrect, paint);
  paint.setShader(nullptr);

  // Active Glow (Outer)
  float globalGlow = design::Settings::getGlowIntensity();
  if (isActive && globalGlow > 0.01f) {
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(2.0f);
    glowPaint.setColor(SkColorSetA(color, 150));
    glowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur((SkBlurStyle)0, 8.0f * globalGlow));
    canvas->drawRRect(rrect, glowPaint);
  }

  // Border (Rim Light)
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  SkColor borderColors[2] = {SkColorSetARGB(100, 255, 255, 255),
                             SkColorSetARGB(50, 0, 0, 0)};
  paint.setShader(SkGradientShader::MakeLinear(pts, borderColors, nullptr, 2,
                                               SkTileMode::kClamp));
  canvas->drawRRect(rrect, paint);
  paint.setShader(nullptr);

  // Label
  SkFont font;
  font.setSize(22.0f); // Larger icons
  font.setSubpixel(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(isActive ? SK_ColorWHITE : SkColorSetARGB(200, 255, 255, 255));

  // Text Shadow
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur((SkBlurStyle)0, 2.0f));

  float textWidth =
      font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
  float textX = (float)bounds.getCentreX() - textWidth / 2.0f;
  float textY = (float)bounds.getCentreY() + 8.0f;

  canvas->drawSimpleText(label, strlen(label), SkTextEncoding::kUTF8, textX,
                         textY + 1.0f, font, shadowPaint);
  canvas->drawSimpleText(label, strlen(label), SkTextEncoding::kUTF8, textX,
                         textY, font, paint);
}

void TransportBar::drawMeter(SkCanvas *canvas,
                             const juce::Rectangle<int> &bounds, float value,
                             const char *label) {
  SkPaint paint;
  paint.setAntiAlias(true);

  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());
  SkRRect rrect = SkRRect::MakeRectXY(rect, 4.0f, 4.0f);

  // Background Track
  paint.setColor(SkColorSetARGB(50, 0, 0, 0));
  canvas->drawRRect(rrect, paint);

  // Fill Gradient
  float fillWidth = (float)bounds.getWidth() * juce::jlimit(0.0f, 1.0f, value);
  if (fillWidth > 0) {
    SkRect fillRect =
        SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(), fillWidth,
                         (float)bounds.getHeight());
    SkRRect fillRRect = SkRRect::MakeRectXY(fillRect, 4.0f, 4.0f);

    SkPoint pts[2] = {{rect.left(), rect.centerY()},
                      {rect.right(), rect.centerY()}};
    SkColor colors[3] = {0xFF00FF64, 0xFFFFC800,
                         0xFFFF3232}; // Green -> Amber -> Red
    SkScalar pos[3] = {0.0f, 0.6f, 1.0f};

    paint.setShader(
        SkGradientShader::MakeLinear(pts, colors, pos, 3, SkTileMode::kClamp));
    canvas->drawRRect(fillRRect, paint);
    paint.setShader(nullptr);
  }

  // Label
  SkFont font;
  font.setSize(12.0f);
  font.setSubpixel(true);
  paint.setColor(SK_ColorWHITE);
  canvas->drawString(label, (float)bounds.getX() + 5.0f,
                     (float)bounds.getY() - 5.0f, font, paint);
}

void TransportBar::mouseDown(const juce::MouseEvent &e) {
  if (playButtonBounds_.contains(e.getPosition())) {
    if (onPlayClicked)
      onPlayClicked();
  } else if (stopButtonBounds_.contains(e.getPosition())) {
    if (onStopClicked)
      onStopClicked();
  } else if (recordButtonBounds_.contains(e.getPosition())) {
    if (onRecordClicked)
      onRecordClicked();
  } else if (viewToggleButtonBounds_.contains(e.getPosition())) {
    if (onViewToggleClicked)
      onViewToggleClicked();
  } else if (settingsButtonBounds_.contains(e.getPosition())) {
    if (onSettingsClicked)
      onSettingsClicked();
  }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
