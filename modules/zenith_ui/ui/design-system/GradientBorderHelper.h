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

    GradientBorderHelper.h
    Created: 2025-12-26
    Author:  Zenith DAW

    Helper class for drawing gradient borders on UI components.
    Provides consistent gradient border styling across the application.


  ==============================================================================
*/

#pragma once

#include "ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {
namespace design {

/**
 * @brief Gradient direction for border effects
 */
enum class GradientDirection {
  TopToBottom,
  LeftToRight,
  TopLeftToBottomRight,
  Radial
};

/**
 * @brief Helper class for drawing gradient borders
 *
 * Provides utility functions for consistent gradient border styling
 * across all UI components in Zenith DAW.
 */
class GradientBorderHelper {
public:
  /**
   * @brief Draw a gradient border around a rectangle
   *
   * @param canvas      Skia canvas to draw on
   * @param bounds      Rectangle bounds for the border
   * @param radius      Corner radius
   * @param strokeWidth Border stroke width
   * @param startColor  Gradient start color
   * @param endColor    Gradient end color
   * @param direction   Gradient direction
   */
  static void drawGradientBorder(SkCanvas *canvas, const SkRect &bounds,
                                 float radius, float strokeWidth,
                                 SkColor startColor, SkColor endColor,
                                 GradientDirection direction = GradientDirection::TopToBottom) {
    if (!canvas)
      return;

    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(strokeWidth);
    paint.setAntiAlias(true);

    SkPoint points[2];
    switch (direction) {
    case GradientDirection::TopToBottom:
      points[0] = {bounds.centerX(), bounds.fTop};
      points[1] = {bounds.centerX(), bounds.fBottom};
      break;
    case GradientDirection::LeftToRight:
      points[0] = {bounds.fLeft, bounds.centerY()};
      points[1] = {bounds.fRight, bounds.centerY()};
      break;
    case GradientDirection::TopLeftToBottomRight:
      points[0] = {bounds.fLeft, bounds.fTop};
      points[1] = {bounds.fRight, bounds.fBottom};
      break;
    case GradientDirection::Radial:
      // For radial, we'll fall through to a simpler approach
      points[0] = {bounds.centerX(), bounds.centerY()};
      points[1] = {bounds.fRight, bounds.fBottom};
      break;
    }

    SkColor colors[2] = {startColor, endColor};
    sk_sp<SkShader> shader =
        SkGradientShader::MakeLinear(points, colors, nullptr, 2, SkTileMode::kClamp);

    paint.setShader(shader);
    canvas->drawRoundRect(bounds.makeInset(strokeWidth / 2, strokeWidth / 2),
                          radius, radius, paint);
  }

  /**
   * @brief Draw accent gradient border using current theme colors
   *
   * Uses the theme's accent primary color with transparency gradient
   * for a consistent "glow" effect across all UI components.
   *
   * @param canvas      Skia canvas to draw on
   * @param bounds      Rectangle bounds
   * @param radius      Corner radius
   * @param strokeWidth Border width (default 1.5f)
   */
  static void drawAccentGradientBorder(SkCanvas *canvas, const SkRect &bounds,
                                       float radius, float strokeWidth = 1.5f) {
    const auto &palette = ThemeManager::getInstance().getPalette();

    // Create gradient from full accent to semi-transparent accent
    SkColor startColor = palette.accentPrimary;
    SkColor endColor = withAlpha(palette.accentPrimary, 0.3f);

    drawGradientBorder(canvas, bounds, radius, strokeWidth, startColor,
                       endColor, GradientDirection::TopToBottom);
  }

  /**
   * @brief Draw a subtle highlight border (glass effect)
   *
   * @param canvas      Skia canvas to draw on
   * @param bounds      Rectangle bounds
   * @param radius      Corner radius
   */
  static void drawGlassHighlight(SkCanvas *canvas, const SkRect &bounds,
                                 float radius) {
    SkColor topHighlight = 0x20FFFFFF; // 12% white
    SkColor bottomShadow = 0x00FFFFFF; // Transparent

    drawGradientBorder(canvas, bounds, radius, 1.0f, topHighlight, bottomShadow,
                       GradientDirection::TopToBottom);
  }

  /**
   * @brief Draw a focus ring with gradient glow
   *
   * @param canvas      Skia canvas to draw on
   * @param bounds      Rectangle bounds
   * @param radius      Corner radius
   * @param glowRadius  Glow blur radius
   */
  static void drawFocusRing(SkCanvas *canvas, const SkRect &bounds,
                            float radius, float glowRadius = 4.0f) {
    const auto &palette = ThemeManager::getInstance().getPalette();

    // Outer glow
    SkPaint glowPaint;
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(glowRadius * 2);
    glowPaint.setAntiAlias(true);
    glowPaint.setColor(withAlpha(palette.borderFocus, 0.3f));
    glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, glowRadius));

    canvas->drawRoundRect(bounds, radius, radius, glowPaint);

    // Crisp inner border
    drawAccentGradientBorder(canvas, bounds, radius, 1.5f);
  }

  /**
   * @brief Draw dual-tone border (cyberpunk style)
   *
   * Creates a split gradient with two accent colors for
   * a more dynamic visual effect.
   *
   * @param canvas      Skia canvas to draw on
   * @param bounds      Rectangle bounds
   * @param radius      Corner radius
   * @param strokeWidth Border width
   */
  static void drawDualToneBorder(SkCanvas *canvas, const SkRect &bounds,
                                 float radius, float strokeWidth = 1.5f) {
    const auto &palette = ThemeManager::getInstance().getPalette();

    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(strokeWidth);
    paint.setAntiAlias(true);

    // Create a 3-stop gradient: primary -> secondary -> primary
    SkPoint points[2] = {{bounds.fLeft, bounds.fTop},
                         {bounds.fRight, bounds.fBottom}};

    SkColor colors[3] = {palette.accentPrimary, palette.accentSecondary,
                         palette.accentPrimary};
    float positions[3] = {0.0f, 0.5f, 1.0f};

    sk_sp<SkShader> shader =
        SkGradientShader::MakeLinear(points, colors, positions, 3, SkTileMode::kClamp);

    paint.setShader(shader);
    canvas->drawRoundRect(bounds.makeInset(strokeWidth / 2, strokeWidth / 2),
                          radius, radius, paint);
  }
};

} // namespace design
} // namespace zenith
