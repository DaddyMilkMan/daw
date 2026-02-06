/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "FreezeProgressOverlay.h"
#include "../design-system/ZenithTypography.h"

// Skia Includes
#ifdef ZENITH_USE_SKIA
#include "ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <effects/SkImageFilters.h>

#endif

namespace zenith {

// Constants for layout and magic numbers
namespace layout {
constexpr float kHalfButtonWidth = 50.0f;
constexpr float kButtonHeight = 30.0f;
constexpr float kButtonYOffset = 60.0f;
constexpr float kTextYOffset = -15.0f;
constexpr float kStatusYOffset = 20.0f;
constexpr float kProgressRadius = 80.0f;
constexpr float kStrokeWidth = 12.0f;
} // namespace layout

FreezeProgressOverlay::FreezeProgressOverlay() {
  setAlwaysOnTop(true);
  setInterceptsMouseClicks(true, true); // Block clicks to underlying components
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60);                     // Animation loop
}

FreezeProgressOverlay::~FreezeProgressOverlay() {}

void FreezeProgressOverlay::setProgress(float progress) {
  progress_ = juce::jlimit(0.0f, 1.0f, progress);
  repaint();
}

void FreezeProgressOverlay::setStatus(const juce::String &text) {
  statusText_ = text;
  repaint();
}

void FreezeProgressOverlay::resized() {
  // Calculate cancel button area
  auto center = getLocalBounds().getCentre().toFloat();
  cancelButtonRect_ = juce::Rectangle<float>(
      center.x - layout::kHalfButtonWidth, center.y + layout::kButtonYOffset,
      layout::kHalfButtonWidth * 2.0f, layout::kButtonHeight);
}

void FreezeProgressOverlay::timerCallback() {
  if (isVisible()) {
    pulsePhase_ += 0.05f;
    if (pulsePhase_ > juce::MathConstants<float>::twoPi)
      pulsePhase_ -= juce::MathConstants<float>::twoPi;
    repaint();
  }
}

void FreezeProgressOverlay::mouseDown(const juce::MouseEvent &e) {
  if (cancelButtonRect_.contains(e.position)) {
    if (onCancel)
      onCancel();
  }
}

void FreezeProgressOverlay::paint(juce::Graphics &g) {
  // Skia rendering used - see drawSkia()
  juce::ignoreUnused(g);
}

void FreezeProgressOverlay::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  using namespace zenith::design;

  auto bounds = getLocalBounds();
  auto center = bounds.getCentre();
  float cx = (float)center.getX();
  float cy = (float)center.getY();

  // 1. Dim Background (Backdrop blur logic implies semi-transparent overlay)
  SkPaint dimPaint;
  dimPaint.setColor(SkColorSetA(SK_ColorBLACK, 150));
  canvas->drawRect(
      SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()),
      dimPaint);

  // 2. Circular Progress
  float radius = layout::kProgressRadius;
  float strokeWidth = layout::kStrokeWidth;
  SkRect circleRect =
      SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);

  // Track
  SkPaint trackPaint;
  trackPaint.setStyle(SkPaint::kStroke_Style);
  trackPaint.setStrokeWidth(strokeWidth);
  trackPaint.setColor(SkColorSetA(colors::BG_DARKER, 50));
  trackPaint.setStrokeCap(SkPaint::kRound_Cap);
  canvas->drawArc(circleRect, 0, 360, false, trackPaint);

  // Active Arc
  SkPaint progressPaint;
  progressPaint.setStyle(SkPaint::kStroke_Style);
  progressPaint.setStrokeWidth(strokeWidth);
  progressPaint.setColor(colors::NEON_CYAN); // Default Brand Color
  progressPaint.setStrokeCap(SkPaint::kRound_Cap);

  // Add glow
  progressPaint.setImageFilter(SkImageFilters::Blur(2.0f, 2.0f, nullptr));

  // Swing animation or simple progress
  float sweepAngle = progress_ * 360.0f;

  // Rotary animation offset if indeterminate (optional, here we assume
  // determined)
  float startAngle = -90.0f; // Limit to top

  canvas->drawArc(circleRect, startAngle, sweepAngle, false, progressPaint);
#endif
}

} // namespace zenith
