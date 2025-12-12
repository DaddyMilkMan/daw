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
#include "GlassmorphicPanel.h"
#include "NeonGlow.h"
#include "ZenithIcons.h"
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

  // 1. Background (Glassmorphic)
  GlassmorphicPanel::draw(canvas, skBounds, GlassmorphicPanel::Style::Elevated);

  // 2. Bottom Border Glow
  NeonGlow::drawGlow(
      canvas, SkRect::MakeXYWH(0, skBounds.height() - 2, skBounds.width(), 2),
      design::colors::CYAN, NeonGlow::Intensity::Subtle);

  // 3. Draw transport buttons using vector icons
  drawTransportButton(canvas, playButtonBounds_, icons::Play(), isPlaying_,
                      design::colors::NEON_GREEN);
  drawTransportButton(canvas, stopButtonBounds_, icons::Stop(), !isPlaying_,
                      design::colors::BLUE);
  drawTransportButton(canvas, recordButtonBounds_, icons::Record(),
                      isRecording_, design::colors::RED);

  // View Toggle - uses ViewToggle icon
  drawTransportButton(canvas, viewToggleButtonBounds_, icons::ViewToggle(),
                      false, design::colors::TEXT_PRIMARY);

  // Settings Button - uses Settings gear icon
  drawTransportButton(canvas, settingsButtonBounds_, icons::Settings(), false,
                      design::colors::TEXT_PRIMARY);

  // 4. Draw Info Text (Tempo & Project)
  SkPaint textPaint; // Stack alloc is cheap
  textPaint.setStyle(SkPaint::kFill_Style);
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  // Tempo with glow
  juce::String tempoStr = juce::String(tempo_, 1) + " BPM";
  NeonGlow::drawTextGlow(canvas, tempoStr.toStdString().c_str(), 260.0f, 38.0f,
                         font_, design::colors::CYAN,
                         NeonGlow::Intensity::Subtle);

  // Project Name (Subtle)
  textPaint.setColor(design::colors::TEXT_SECONDARY);
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
  // Use Mono font for Tempo/BPM display to avoid jitter
  font_ = design::getMonoFont(18.0f, design::FontWeight::Medium);

  // Use UI font for labels
  smallFont_ = design::getSkFont(14.0f, design::FontWeight::Regular);
}

void TransportBar::drawButton(SkCanvas *canvas,
                              const juce::Rectangle<int> &bounds,
                              const char *label, bool isActive,
                              uint32_t color) {
  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());

  if (isActive) {
    // Active State: Glass panel with accent glow
    GlassmorphicPanel::drawWithAccent(canvas, rect, color,
                                      GlassmorphicPanel::Style::ActiveGlow);
  } else {
    // Inactive State: Subtle glass panel
    GlassmorphicPanel::draw(canvas, rect, GlassmorphicPanel::Style::Subtle);
  }

  // Label
  SkFont font = design::getSkFont(22.0f, design::FontWeight::Medium);

  // Center Text logic
  float textWidth =
      font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
  float textX = rect.centerX() - textWidth / 2.0f;
  // Approximation for vertical centering
  float textY = rect.centerY() + 8.0f;

  if (isActive) {
    // Glowing text for active state
    NeonGlow::drawTextGlow(canvas, label, textX, textY, font, SK_ColorWHITE,
                           NeonGlow::Intensity::Strong);
  } else {
    // Normal text for inactive
    SkPaint paint;
    paint.setColor(design::colors::TEXT_SECONDARY);
    paint.setAntiAlias(true);
    canvas->drawString(label, textX, textY, font, paint);
  }
}

void TransportBar::drawTransportButton(SkCanvas *canvas,
                                       const juce::Rectangle<int> &bounds,
                                       const SkPath &iconPath, bool isActive,
                                       uint32_t color) {
  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());

  if (isActive) {
    // Active State: Glass panel with accent glow
    GlassmorphicPanel::drawWithAccent(canvas, rect, color,
                                      GlassmorphicPanel::Style::ActiveGlow);
  } else {
    // Inactive State: Subtle glass panel
    GlassmorphicPanel::draw(canvas, rect, GlassmorphicPanel::Style::Subtle);
  }

  // Calculate icon size (about 60% of button height)
  float iconSize = bounds.getHeight() * 0.6f;

  // Set up icon style
  icons::IconStyle style;
  style.color = isActive ? SK_ColorWHITE : design::colors::TEXT_SECONDARY;
  style.filled = isActive; // Filled when active
  style.strokeWidth = 2.0f;

  if (isActive) {
    style.glowRadius = 6.0f;
    style.glowColor = color;
  }

  // Draw the icon centered in the button
  icons::drawIconCentered(canvas, iconPath, rect, iconSize, style);
}

void TransportBar::drawMeter(SkCanvas *canvas,
                             const juce::Rectangle<int> &bounds, float value,
                             const char *label) {
  SkRect rect =
      SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(),
                       (float)bounds.getWidth(), (float)bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(50, 0, 0, 0));
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(rect, 4.0f, 4.0f, bgPaint);

  // Use NeonGlow helper for the meter bar
  // Note: drawVUMeterGlow takes normalized value
  // We need to draw the filled part ourselves if we want gradient,
  // or we can use the helper if it supports drawing the bar.
  // Checking NeonGlow.h... helper draws "glow at the peak".
  // So we still need to draw the bar itself.
  // Let's implement a consistent bar drawer here or reuse logic.

  // Actually, let's keep it simple and consistent:
  // 1. Draw bar
  float fillWidth = (float)bounds.getWidth() * juce::jlimit(0.0f, 1.0f, value);
  if (fillWidth > 0) {
    SkRect fillRect =
        SkRect::MakeXYWH(rect.left(), rect.top(), fillWidth, rect.height());

    SkPoint pts[2] = {{rect.left(), rect.centerY()},
                      {rect.right(), rect.centerY()}};
    SkColor colors[3] = {design::colors::NEON_GREEN, design::colors::AMBER,
                         design::colors::RED};
    SkScalar pos[3] = {0.0f, 0.6f, 1.0f};

    SkPaint fillPaint;
    fillPaint.setShader(
        SkGradientShader::MakeLinear(pts, colors, pos, 3, SkTileMode::kClamp));
    fillPaint.setAntiAlias(true);
    canvas->drawRoundRect(fillRect, 4.0f, 4.0f, fillPaint);

    // 2. Add Peak Glow
    NeonGlow::drawVUMeterGlow(canvas, rect, value, false); // false = horizontal
  }

  // Label
  SkFont font = design::getSkFont(10.0f, design::FontWeight::Bold);

  SkPaint textPaint;
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString(label, (float)bounds.getX() + 5.0f,
                     (float)bounds.getY() - 5.0f, font, textPaint);
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
