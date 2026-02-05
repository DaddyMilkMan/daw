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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    NeonGlow.h
    Created: 2025-12-11
    Author:  Zenith DAW Team

    Neon glow effect utilities for Zenith DAW's "Neon Noir" design.

    Usage:

      NeonGlow::drawGlow(canvas, bounds, design::colors::CYAN);
      NeonGlow::drawTextGlow(canvas, "Title", x, y, font,
  design::colors::MAGENTA); NeonGlow::drawActiveRing(canvas, center, radius,
  design::colors::CYAN);

  ==============================================================================
*/

#pragma once

#include "../../Settings.h"
#include "../../Settings.h"
#include "ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
// #include "../design-system/ZenithTheme.h" // Deprecated
#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>

namespace zenith {

/**
 * @brief Neon glow effect rendering utilities
 *
 * Provides consistent neon glow effects throughout the UI:
 * - Outer glows for buttons, panels, controls
 * - Text glows for headers and important labels
 * - Active element indicators
 * - Animated pulse effects
 */
class NeonGlow {
public:
  enum class Intensity {
    Subtle, // Hover states
    Medium, // Active states
    Strong, // Focus/selected states
    Intense // Warnings/errors
  };

  /**
   * @brief Draw a glow effect around a rectangular area
   * @param canvas The Skia canvas
   * @param bounds The area to glow around
   * @param color The glow color
   * @param intensity The glow intensity
   * @param cornerRadius Corner radius for rounded rectangles
   */
  static void drawGlow(SkCanvas *canvas, const SkRect &bounds, SkColor color,
                       Intensity intensity = Intensity::Medium,
                       float cornerRadius = 0.0f) {
    using namespace design;

    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity();
    if (globalGlow < 0.01f)
      return;

    float blurRadius = getBlurRadius(intensity) * globalGlow;
    float alpha = getAlpha(intensity) * globalGlow;

    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kFill_Style);
    glowPaint.setColor(withAlpha(color, alpha));
    glowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blurRadius));

    if (cornerRadius > 0.0f) {
      SkRRect rrect = SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius);
      canvas->drawRRect(rrect, glowPaint);
    } else {
      canvas->drawRect(bounds, glowPaint);
    }
  }

  /**
   * @brief Draw a glow outline (stroke-based glow)
   */
  static void drawGlowOutline(SkCanvas *canvas, const SkRect &bounds,
                              SkColor color,
                              Intensity intensity = Intensity::Medium,
                              float cornerRadius = 0.0f,
                              float strokeWidth = 2.0f) {
    using namespace design;

    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity();
    if (globalGlow < 0.01f)
      return;

    float blurRadius = getBlurRadius(intensity) * globalGlow;
    float alpha = getAlpha(intensity) * globalGlow;

    // Outer glow layer
    SkPaint outerPaint;
    outerPaint.setAntiAlias(true);
    outerPaint.setStyle(SkPaint::kStroke_Style);
    outerPaint.setStrokeWidth(strokeWidth + 4.0f);
    outerPaint.setColor(withAlpha(color, alpha * 0.3f));
    outerPaint.setMaskFilter(SkMaskFilter::MakeBlur(
        SkBlurStyle::kNormal_SkBlurStyle, blurRadius * 1.5f));

    // Inner glow layer
    SkPaint innerPaint;
    innerPaint.setAntiAlias(true);
    innerPaint.setStyle(SkPaint::kStroke_Style);
    innerPaint.setStrokeWidth(strokeWidth);
    innerPaint.setColor(withAlpha(color, alpha));
    innerPaint.setMaskFilter(SkMaskFilter::MakeBlur(
        SkBlurStyle::kNormal_SkBlurStyle, blurRadius * 0.5f));

    // Core (solid) layer
    SkPaint corePaint;
    corePaint.setAntiAlias(true);
    corePaint.setStyle(SkPaint::kStroke_Style);
    corePaint.setStrokeWidth(strokeWidth);
    corePaint.setColor(withAlpha(color, std::min(1.0f, alpha * 1.5f)));

    if (cornerRadius > 0.0f) {
      SkRRect rrect = SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius);
      canvas->drawRRect(rrect, outerPaint);
      canvas->drawRRect(rrect, innerPaint);
      canvas->drawRRect(rrect, corePaint);
    } else {
      canvas->drawRect(bounds, outerPaint);
      canvas->drawRect(bounds, innerPaint);
      canvas->drawRect(bounds, corePaint);
    }
  }

  /**
   * @brief Draw glowing text
   */
  static void drawTextGlow(SkCanvas *canvas, const char *text, float x, float y,
                           const SkFont &font, SkColor color,
                           Intensity intensity = Intensity::Medium) {
    using namespace design;

    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity();
    if (globalGlow < 0.01f) {
      // No glow, just draw text
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(color);
      canvas->drawString(text, x, y, font, textPaint);
      return;
    }

    float blurRadius = getBlurRadius(intensity) * globalGlow * 0.5f;
    float alpha = getAlpha(intensity) * globalGlow;

    // Glow layer
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setColor(withAlpha(color, alpha * 0.6f));
    glowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blurRadius));
    canvas->drawString(text, x, y, font, glowPaint);

    // Main text
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(color);
    canvas->drawString(text, x, y, font, textPaint);
  }

  /**
   * @brief Draw an active/focus ring around a circular element
   */
  static void drawActiveRing(SkCanvas *canvas, SkPoint center, float radius,
                             SkColor color,
                             Intensity intensity = Intensity::Strong) {
    using namespace design;

    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity();
    if (globalGlow < 0.01f)
      return;

    float blurRadius = getBlurRadius(intensity) * globalGlow;
    float alpha = getAlpha(intensity) * globalGlow;

    // Outer glow
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(3.0f);
    glowPaint.setColor(withAlpha(color, alpha * 0.5f));
    glowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blurRadius));
    canvas->drawCircle(center.x(), center.y(), radius, glowPaint);

    // Inner ring
    SkPaint ringPaint;
    ringPaint.setAntiAlias(true);
    ringPaint.setStyle(SkPaint::kStroke_Style);
    ringPaint.setStrokeWidth(2.0f);
    ringPaint.setColor(withAlpha(color, alpha));
    canvas->drawCircle(center.x(), center.y(), radius, ringPaint);
  }

  /**
   * @brief Draw a pulsing glow animation
   * @param animProgress Animation progress from 0.0 to 1.0 (use sin wave for
   * pulse)
   */
  static void drawPulsingGlow(SkCanvas *canvas, const SkRect &bounds,
                              SkColor color, float animProgress,
                              float cornerRadius = 0.0f) {
    using namespace design;

    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity();
    if (globalGlow < 0.01f)
      return;

    // Pulse between Subtle and Strong
    float pulseIntensity = 0.3f + 0.7f * animProgress;
    float blurRadius =
        design::effects::GLOW_MEDIUM * pulseIntensity * globalGlow;
    float alpha = 0.2f + 0.4f * pulseIntensity * globalGlow;

    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(2.0f + pulseIntensity * 2.0f);
    glowPaint.setColor(withAlpha(color, alpha));
    glowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blurRadius));

    if (cornerRadius > 0.0f) {
      SkRRect rrect = SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius);
      canvas->drawRRect(rrect, glowPaint);
    } else {
      canvas->drawRect(bounds, glowPaint);
    }
  }

  /**
   * @brief Draw a gradient VU meter with neon glow at the peak
   * @param bounds Meter bounds
   * @param value Normalized value 0.0-1.0
   * @param vertical If true, meter fills from bottom to top
   */
  static void drawVUMeterGlow(SkCanvas *canvas, const SkRect &bounds,
                              float value, bool vertical = true) {
    using namespace design;

    value = juce::jlimit(0.0f, 1.0f, value);
    if (value < 0.001f)
      return;

    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity();

    // Determine peak color based on level
    // Determine peak color based on level
    SkColor peakColor;
    
    if (value > 0.95f) {
      peakColor = design::colors::DANGER; // Clipping
    } else if (value > 0.8f) {
      peakColor = design::colors::WARNING; // Hot
    } else if (value > 0.5f) {
      peakColor = design::colors::SUCCESS; // Good
    } else {
      peakColor = design::colors::ACCENT_PRIMARY; // Low
    }

    // Draw glow at peak position
    if (globalGlow > 0.01f) {
      SkRect peakRect;
      if (vertical) {
        float peakY = bounds.bottom() - (bounds.height() * value);
        peakRect =
            SkRect::MakeXYWH(bounds.left(), peakY - 4.0f, bounds.width(), 8.0f);
      } else {
        float peakX = bounds.left() + (bounds.width() * value);
        peakRect =
            SkRect::MakeXYWH(peakX - 4.0f, bounds.top(), 8.0f, bounds.height());
      }

      float blurRadius = design::effects::GLOW_MEDIUM * globalGlow;
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setColor(withAlpha(peakColor, 0.6f * globalGlow));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blurRadius));
      canvas->drawRect(peakRect, glowPaint);
    }
  }

  /**
   * @brief Draw a playhead line with glow
   */
  static void drawPlayheadGlow(SkCanvas *canvas, float x, float top,
                               float bottom, SkColor color = 0xFFFFFFFF) {
    using namespace design;

    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity();

    // Glow layer
    if (globalGlow > 0.01f) {
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setStrokeWidth(4.0f);
      glowPaint.setColor(withAlpha(color, 0.3f * globalGlow));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle,
                                 design::effects::GLOW_MEDIUM * globalGlow));
      canvas->drawLine(x, top, x, bottom, glowPaint);
    }

    // Core line
    SkPaint linePaint;
    linePaint.setAntiAlias(true);
    linePaint.setStrokeWidth(2.0f);
    linePaint.setColor(color);
    canvas->drawLine(x, top, x, bottom, linePaint);

    // Bright center
    SkPaint brightPaint;
    brightPaint.setAntiAlias(true);
    brightPaint.setStrokeWidth(1.0f);
    brightPaint.setColor(SK_ColorWHITE);
    canvas->drawLine(x, top, x, bottom, brightPaint);
  }

private:
  static float getBlurRadius(Intensity intensity) {
    using namespace design;
    switch (intensity) {
    case Intensity::Subtle:
      return design::effects::GLOW_SUBTLE;
    case Intensity::Medium:
      return design::effects::GLOW_MEDIUM;
    case Intensity::Strong:
      return design::effects::GLOW_STRONG;
    case Intensity::Intense:
      return design::effects::GLOW_INTENSE;
    }
    return design::effects::GLOW_MEDIUM;
  }

  static float getAlpha(Intensity intensity) {
    using namespace design;
    switch (intensity) {
    case Intensity::Subtle:
      return design::effects::OPACITY_SUBTLE;
    case Intensity::Medium:
      return design::effects::OPACITY_MEDIUM;
    case Intensity::Strong:
      return design::effects::OPACITY_STRONG;
    case Intensity::Intense:
      return design::effects::OPACITY_INTENSE;
    }
    return design::effects::OPACITY_MEDIUM;
  }

  NeonGlow() = delete; // Static-only class
};

} // namespace zenith
