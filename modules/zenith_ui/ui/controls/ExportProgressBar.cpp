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

#include "ExportProgressBar.h"
#include "../design-system/ZenithDesignSystem.h"
#include <cmath>

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkMaskFilter.h"
#include "include/effects/SkGradientShader.h"

namespace zenith {

ExportProgressBar::ExportProgressBar() {
  setSize(400, 50);
}

ExportProgressBar::~ExportProgressBar() {
  stopTimer();
}

void ExportProgressBar::setProgress(float progress) {
  progress_ = juce::jlimit(0.0f, 1.0f, progress);
  
  if (!isTimerRunning()) {
    startAnimation();
  }
  
  repaint();
}

void ExportProgressBar::setStatusMessage(const juce::String& message) {
  statusMessage_ = message;
  repaint();
}

void ExportProgressBar::startAnimation() {
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60); // 60fps animation
}

void ExportProgressBar::stopAnimation() {
  stopTimer();
}

void ExportProgressBar::timerCallback() {
  // Smooth progress animation
  const float smoothingFactor = 0.15f;
  animatedProgress_ += (progress_ - animatedProgress_) * smoothingFactor;
  
  // Animate glow phase
  glowPhase_ += 0.08f;
  if (glowPhase_ > 6.28318f) {
    glowPhase_ -= 6.28318f;
  }
  
  // Stop animation when settled
  if (std::abs(progress_ - animatedProgress_) < 0.001f && progress_ >= 1.0f) {
    animatedProgress_ = progress_;
    stopTimer();
  }
  
  repaint();
}

void ExportProgressBar::drawSkia(SkCanvas* canvas) {
  auto bounds = getLocalBounds().toFloat();
  const float padding = 4.0f;
  const float barHeight = 24.0f;
  const float radius = 12.0f;
  
  // Calculate bar area
  float barY = (bounds.getHeight() - barHeight) / 2.0f;
  if (!statusMessage_.isEmpty()) {
    barY = bounds.getHeight() - barHeight - padding;
  }
  
  SkRect barRect = SkRect::MakeXYWH(padding, barY, bounds.getWidth() - 2 * padding, barHeight);
  SkRRect barRRect = SkRRect::MakeRectXY(barRect, radius, radius);
  
  SkPaint paint;
  paint.setAntiAlias(true);
  
  // Background track
  paint.setColor(SkColorSetARGB(100, 30, 30, 40));
  canvas->drawRRect(barRRect, paint);
  
  // Border
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(60, 255, 255, 255));
  canvas->drawRRect(barRRect, paint);
  
  // Progress fill with gradient
  if (animatedProgress_ > 0.001f) {
    float fillWidth = (barRect.width() - 4.0f) * animatedProgress_;
    SkRect fillRect = SkRect::MakeXYWH(barRect.left() + 2.0f, barRect.top() + 2.0f, 
                                        fillWidth, barRect.height() - 4.0f);
    SkRRect fillRRect = SkRRect::MakeRectXY(fillRect, radius - 2.0f, radius - 2.0f);
    
    // Animated gradient colors
    float glowIntensity = 0.5f + 0.5f * std::sin(glowPhase_);
    
    SkColor colors[] = {
      SkColorSetARGB(255, 0, 200, 255),                     // Cyan
      SkColorSetARGB(255, 128, 0, 255),                     // Purple
      SkColorSetARGB(255, static_cast<uint8_t>(50 + 50 * glowIntensity), 
                         static_cast<uint8_t>(200 + 55 * glowIntensity), 255) // Animated cyan
    };
    SkScalar positions[] = { 0.0f, 0.5f, 1.0f };
    
    SkPoint gradientPoints[] = {
      SkPoint::Make(fillRect.left(), fillRect.centerY()),
      SkPoint::Make(fillRect.right(), fillRect.centerY())
    };
    
    paint.setStyle(SkPaint::kFill_Style);
    paint.setShader(SkGradientShader::MakeLinear(
      gradientPoints, colors, positions, 3, SkTileMode::kClamp));
    canvas->drawRRect(fillRRect, paint);
    paint.setShader(nullptr);
    
    // Glow effect
    float glowAlpha = 80.0f + 40.0f * glowIntensity;
    paint.setColor(SkColorSetARGB(static_cast<uint8_t>(glowAlpha), 0, 200, 255));
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
    canvas->drawRRect(fillRRect, paint);
    paint.setMaskFilter(nullptr);
  }
  
  // Status message
  if (!statusMessage_.isEmpty()) {
    SkFont font = design::getSkFont(12.0f, design::FontWeight::Regular);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetARGB(180, 255, 255, 255));
    
    canvas->drawString(statusMessage_.toRawUTF8(), padding + 2.0f, 16.0f, font, paint);
  }
  
  // Percentage
  if (showPercentage_) {
    int percent = static_cast<int>(animatedProgress_ * 100.0f);
    juce::String percentText = juce::String(percent) + "%";
    
    SkFont font = design::getSkFont(11.0f, design::FontWeight::Bold);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SK_ColorWHITE);
    
    // Center in bar
    SkRect textBounds;
    font.measureText(percentText.toRawUTF8(), percentText.length(), SkTextEncoding::kUTF8, &textBounds);
    float textX = barRect.centerX() - textBounds.width() / 2.0f;
    float textY = barRect.centerY() + textBounds.height() / 2.0f - 2.0f;
    
    canvas->drawString(percentText.toRawUTF8(), textX, textY, font, paint);
  }
}

} // namespace zenith
