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

#pragma once

#include "../../Settings.h"
#include "BackdropBlur.h"
#include "ZenithDesignSystem.h"
#include <core/SkBitmap.h>
#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <random>

namespace zenith {

/**
 * @brief Glassmorphism panel rendering utilities with REAL backdrop blur
 *
 * Provides consistent glass-effect panels throughout the UI with:
 * - REAL backdrop blur (not fake transparency!)
 * - Top edge highlights
 * - Subtle drop shadows
 * - Optional accent color glows
 *
 * The blur effect captures and blurs whatever was drawn on the canvas
 * BEFORE this panel, creating authentic glassmorphism.
 */
class GlassmorphicPanel {
public:
  enum class Style {
    Flat,     // Minimal: solid dark background (no blur for performance)
    Subtle,   // Light glass effect (8px blur, for nested panels)
    Elevated, // Standard glass with shadow (16px blur, main panels)
    Floating, // Strong glass with pronounced shadow (24px blur, dialogs/popups)
    ActiveGlow // Glass with neon glow border (16px blur, focused/active
               // elements)
  };

  struct Options {
    Style style = Style::Elevated;
    float cornerRadius = design::dimensions::RADIUS_LG;
    SkColor accentColor = 0x00000000; // No accent by default
    float glowIntensity = 1.0f;       // Multiplier for glow effects
    bool drawTopHighlight = true;
    bool drawShadow = true;
    bool useBackdropBlur = true; // NEW: Enable real blur (can disable for perf)
    SkColor customTintColor = 0x00000000; // Optional custom tint
  };

  /**
   * @brief Get blur radius for a given style
   */
  static float getBlurRadiusForStyle(Style style) {
    switch (style) {
    case Style::Flat:
      return 0.0f;
    case Style::Subtle:
      return 8.0f;
    case Style::Elevated:
      return 16.0f;
    case Style::Floating:
      return 24.0f;
    case Style::ActiveGlow:
      return 16.0f;
    }
    return 16.0f;
  }

  /**
   * @brief Get tint color for a given style
   */
  static SkColor getTintColorForStyle(Style style) {
    using namespace design;
    switch (style) {
    case Style::Flat:
      return colors::BG_DARKEST;
    case Style::Subtle:
      return colors::BG_DARK;
    case Style::Elevated:
      return colors::BG_DARK;
    case Style::Floating:
      return colors::BG_MEDIUM;
    case Style::ActiveGlow:
      return colors::BG_DARK;
    }
    return colors::BG_DARK;
  }

  /**
   * @brief Get tint opacity for a given style
   */
  static float getTintOpacityForStyle(Style style) {
    switch (style) {
    case Style::Flat:
      return 1.0f; // Solid
    case Style::Subtle:
      return 0.75f; // More transparent
    case Style::Elevated:
      return 0.80f; // Standard
    case Style::Floating:
      return 0.70f; // More translucent
    case Style::ActiveGlow:
      return 0.75f; // Slightly more visible
    }
    return 0.80f;
  }

  /**
   * @brief Draw a glassmorphic panel
   * @param canvas The Skia canvas
   * @param bounds The panel bounds as SkRect
   * @param style The panel style
   */
  static void draw(SkCanvas *canvas, const SkRect &bounds,
                   Style style = Style::Elevated) {
    Options opts;
    opts.style = style;
    drawWithOptions(canvas, bounds, opts);
  }

  /**
   * @brief Draw a glassmorphic panel with accent color glow
   */
  static void drawWithAccent(SkCanvas *canvas, const SkRect &bounds,
                             SkColor accentColor,
                             Style style = Style::ActiveGlow) {
    Options opts;
    opts.style = style;
    opts.accentColor = accentColor;
    drawWithOptions(canvas, bounds, opts);
  }

  /**
   * @brief Draw a glassmorphic panel with full options control
   *
   * This is the main rendering function. It:
   * 1. Draws a drop shadow (optional)
   * 2. Applies REAL backdrop blur to content behind (or solid fallback)
   * 3. Draws top edge highlights
   * 4. Draws border
   * 5. Draws accent glow (optional)
   */
  static void drawWithOptions(SkCanvas *canvas, const SkRect &bounds,
                              const Options &opts) {
    using namespace design;

    float radius = opts.cornerRadius;
    SkRRect rrect = SkRRect::MakeRectXY(bounds, radius, radius);
    float globalGlow = ::zenith::Settings::getInstance().getGlowIntensity() *
                       opts.glowIntensity;

    // 1. Drop Shadow (under the panel)
    if (opts.drawShadow && opts.style != Style::Flat) {
      drawDropShadow(canvas, rrect, opts.style);
    }

    // 2. Background: REAL backdrop blur OR solid fallback
    float blurRadius = getBlurRadiusForStyle(opts.style);
    SkColor tintColor = opts.customTintColor != 0 ? opts.customTintColor : getTintColorForStyle(opts.style);
    float tintOpacity = getTintOpacityForStyle(opts.style);

    bool isBlurEnabled = opts.useBackdropBlur && blurRadius > 0.0f &&
                         BackdropBlurConfig::isBlurEnabled();

    if (isBlurEnabled) {
      // REAL GLASSMORPHISM - blur the content behind!
      BackdropBlur::drawBlurredPanel(
          canvas, bounds, radius, blurRadius, tintColor, tintOpacity,
          false // We draw our own highlight below for more control
      );
    } else {
      // Fallback to solid gradient (flat mode or performance reasons)
      drawSolidBackground(canvas, rrect, opts.style, opts.customTintColor);
    }

    // NEW: Noise Texture (Subtle tactility)
    // Only draw noise if looking for premium feel (not Flat)
    if (opts.style != Style::Flat) {
      drawNoiseTexture(canvas, rrect, 0.03f); // 3% opacity
    }

    // 3. Top Edge Highlight (glass effect) -> Refactored to Rim Light
    // We keep the old highlight for backward compatibility or layer it
    if (opts.drawTopHighlight && opts.style != Style::Flat) {
      // drawTopHighlight(canvas, rrect, bounds); // Replaced/augmented by Rim
      // Light
      drawRimLight(canvas, rrect, bounds);
    }

    // 4. Border
    drawBorder(canvas, rrect, opts);

    // 5. Accent Glow (for ActiveGlow style or explicit accent)
    if (opts.accentColor != 0x00000000 && globalGlow > 0.01f) {
      drawAccentGlow(canvas, rrect, opts.accentColor, globalGlow);
    }
  }

  /**
   * @brief Draw a horizontal divider line with subtle glow
   */
  static void drawDivider(SkCanvas *canvas, float x1, float y, float x2) {
    using namespace design;

    SkPaint dividerPaint;
    dividerPaint.setAntiAlias(true);
    dividerPaint.setStrokeWidth(0.6f);
    dividerPaint.setColor(colors::BORDER_SUBTLE);
    canvas->drawLine(x1, y, x2, y, dividerPaint);

    // Subtle highlight below
    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setStrokeWidth(1.0f);
    highlightPaint.setColor(SkColorSetARGB(10, 255, 255, 255));
    canvas->drawLine(x1, y + 1.0f, x2, y + 1.0f, highlightPaint);
  }

  /**
   * @brief Fill entire canvas with the darkest background gradient
   *
   * This is the base layer that glass panels blur.
   */
  static void fillBackground(SkCanvas *canvas, const SkRect &bounds) {
    using namespace design;

    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    // Radial vignette-style gradient: slightly lighter in center
    SkPoint gradPoints[2] = {
        {bounds.centerX(), bounds.centerY() * 0.4f}, // Near top-center
        {bounds.centerX(), bounds.bottom()}};
    SkColor gradColors[3] = {
        colors::BG_DARKER,  // Slightly lighter at top
        colors::BG_DARKEST, // Dark in middle
        0xFF08080C          // Even darker at bottom (vignette)
    };
    SkScalar positions[3] = {0.0f, 0.5f, 1.0f};

    bgPaint.setShader(SkGradientShader::MakeLinear(
        gradPoints, gradColors, positions, 3, SkTileMode::kClamp));

    canvas->drawRect(bounds, bgPaint);
  }

private:
  GlassmorphicPanel() = delete; // Static-only class

  /**
   * @brief Draw drop shadow under the panel
   */
  static void drawDropShadow(SkCanvas *canvas, const SkRRect &rrect,
                             Style style) {
    using namespace design;

    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(colors::GLASS_SHADOW);

    float blurAmount = 0.0f;
    float offset = 0.0f;

    switch (style) {
    case Style::Subtle:
      blurAmount = design::effects::SHADOW_OFFSET_SM;
      offset = 1.0f;
      break;
    case Style::Elevated:
      blurAmount = design::effects::SHADOW_OFFSET_MD;
      offset = 2.0f;
      break;
    case Style::Floating:
    case Style::ActiveGlow:
      blurAmount = design::effects::SHADOW_OFFSET_LG;
      offset = 4.0f;
      break;
    default:
      break;
    }

    if (blurAmount > 0) {
      shadowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blurAmount));
      SkRRect shadowRRect = rrect;
      shadowRRect.offset(0, offset);
      canvas->drawRRect(shadowRRect, shadowPaint);
    }
  }

  /**
   * @brief Draw solid gradient background (fallback when blur is disabled)
   */
  static void drawSolidBackground(SkCanvas *canvas, const SkRRect &rrect,
                                  Style style, SkColor customTint = 0) {
    using namespace design;

    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    if (customTint != 0) {
        // Use custom tint flat color
        bgPaint.setColor(customTint);
        canvas->drawRRect(rrect, bgPaint);
        return;
    }

    SkColor bgTop, bgBottom;


    switch (style) {
    case Style::Flat:
      bgTop = colors::BG_DARKEST;
      bgBottom = colors::BG_DARKEST;
      break;
    case Style::Subtle:
      bgTop = withAlpha(colors::BG_DARK, 0.8f);
      bgBottom = withAlpha(colors::BG_DARKER, 0.8f);
      break;
    case Style::Elevated:
      bgTop = colors::BG_DARK;
      bgBottom = colors::BG_DARKER;
      break;
    case Style::Floating:
      bgTop = colors::BG_MEDIUM;
      bgBottom = colors::BG_DARK;
      break;
    case Style::ActiveGlow:
      bgTop = colors::BG_DARK;
      bgBottom = colors::BG_DARKEST;
      break;
      bgTop = colors::BG_DARK;
      bgBottom = colors::BG_DARKEST;
      break;
    }


    SkRect bounds = rrect.getBounds();
    SkPoint gradPoints[2] = {{bounds.centerX(), bounds.top()},
                             {bounds.centerX(), bounds.bottom()}};
    SkColor gradColors[2] = {bgTop, bgBottom};

    bgPaint.setShader(SkGradientShader::MakeLinear(
        gradPoints, gradColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRRect(rrect, bgPaint);
  }

  /**
   * @brief Draw subtle noise texture for tactility
   */
  static void drawNoiseTexture(SkCanvas *canvas, const SkRRect &rrect,
                               float opacity) {
    // Generate static noise texture (once)
    static sk_sp<SkShader> noiseShader = []() {
      const int w = 128;
      const int h = 128; // Power of 2
      SkBitmap bitmap;
      bitmap.allocN32Pixels(w, h); // Allocate pixel memory

      // Use modern random generator
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> distrib(0, 255);

      // Fill with random noise
      for (int y = 0; y < h; ++y) {
        // Get row pointer for speed
        uint32_t *row = bitmap.getAddr32(0, y);
        for (int x = 0; x < w; ++x) {
          uint8_t val = (uint8_t)distrib(gen);
          // Pack into ARGB (native format), make it fully opaque initially
          row[x] = SkColorSetARGB(255, val, val, val);
        }
      }
      bitmap.setImmutable();

      // Create shader with Repeat mode (updated API)
      SkSamplingOptions sampling(SkFilterMode::kNearest);
      SkMatrix localMatrix = SkMatrix::I();
      return bitmap.makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat,
                               sampling, localMatrix);
    }();

    SkPaint noisePaint;
    noisePaint.setAntiAlias(true);
    noisePaint.setBlendMode(SkBlendMode::kOverlay);
    noisePaint.setAlphaf(opacity);

    if (noiseShader) {
      noisePaint.setShader(noiseShader);
      canvas->drawRRect(rrect, noisePaint);
    }
  }

  /**
   * @brief Draw Rim Light effect (premium bevel)
   * Replacing the simple top highlight with a directional top-left gradient
   * stroke
   */
  static void drawRimLight(SkCanvas *canvas, const SkRRect &rrect,
                           const SkRect &bounds) {
    using namespace design;

    SkPaint rimPaint;
    rimPaint.setAntiAlias(true);
    rimPaint.setStyle(SkPaint::kStroke_Style);
    rimPaint.setStrokeWidth(0.8f); // 0.8px stroke

    // Gradient from Top-Left (White) to Bottom-Right (Transparent)
    // This simulates light catching the top-left edge
    SkPoint pts[2] = {
        {bounds.left(), bounds.top()},
        {bounds.right() * 0.5f, bounds.bottom() * 0.5f} // Fade out halfway
    };

    SkColor colors[2] = {
        SkColorSetA(SK_ColorWHITE, 60), // Reduced from 180 (Too bright/pill-like)
        SkColorSetA(SK_ColorWHITE, 0)    // Transparent
    };

    rimPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                    SkTileMode::kClamp));

    // Inset slightly to sit ON the border area
    SkRRect rimRRect = rrect;
    rimRRect.inset(0.5f, 0.5f);

    canvas->drawRRect(rimRRect, rimPaint);

    // Optional: Add a subtle secondary reflection at bottom-right for realism?
    // For now, prompt asked for "generated 1px white gradient stroke on the
    // top-left edges"
  }

  /**
   * @brief Draw top edge glass highlight (Legacy/Supplemental)
   */
  static void drawTopHighlight(SkCanvas *canvas, const SkRRect &rrect,
                               const SkRect &bounds) {
    using namespace design;

    // Kept for code structure but effectively replaced by RimLight logic in
    // standard path or can be used for extra shine.
    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setStyle(SkPaint::kStroke_Style);
    highlightPaint.setStrokeWidth(0.8f);

    // Gradient from visible white at top to transparent
    SkPoint hlPoints[2] = {
        {bounds.left(), bounds.top()},
        {bounds.left(), bounds.top() + bounds.height() * 0.3f}};
    SkColor hlColors[2] = {
        colors::GLASS_HIGHLIGHT, // ~10% white
        0x00FFFFFF               // Transparent
    };
    highlightPaint.setShader(SkGradientShader::MakeLinear(
        hlPoints, hlColors, nullptr, 2, SkTileMode::kClamp));

    SkRRect hlRRect = rrect;
    hlRRect.inset(0.5f, 0.5f);
    canvas->drawRRect(hlRRect, highlightPaint);
  }

  /**
   * @brief Draw panel border
   */
  static void drawBorder(SkCanvas *canvas, const SkRRect &rrect,
                         const Options &opts) {
    using namespace design;

    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(0.8f);

    SkRect bounds = rrect.getBounds();
    SkRRect borderRRect = rrect;
    borderRRect.inset(0.5f, 0.5f);

    if (opts.style == Style::ActiveGlow && opts.accentColor != 0x00000000) {
      borderPaint.setColor(withAlpha(opts.accentColor, 0.6f));
    } else {
      // PREMIUM: Linear gradient border (Top-Left Highlight to Bottom-Right
      // Subtle)
      SkPoint pts[2] = {{bounds.left(), bounds.top()},
                        {bounds.right(), bounds.bottom()}};
      SkColor colors[2] = {
          SkColorSetA(SK_ColorWHITE, 30), // Was 60
          SkColorSetA(SK_ColorWHITE, 10)  // Was 20
      };
      borderPaint.setShader(SkGradientShader::MakeLinear(
          pts, colors, nullptr, 2, SkTileMode::kClamp));
    }

    canvas->drawRRect(borderRRect, borderPaint);
  }

  /**
   * @brief Draw accent glow effect
   */
  static void drawAccentGlow(SkCanvas *canvas, const SkRRect &rrect,
                             SkColor accentColor, float globalGlow) {
    using namespace design;

    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setStyle(SkPaint::kStroke_Style);
    glowPaint.setStrokeWidth(2.0f);
    glowPaint.setColor(withAlpha(accentColor, 0.4f * globalGlow));
    glowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle,
                               design::effects::GLOW_MEDIUM * globalGlow));

    canvas->drawRRect(rrect, glowPaint);
  }
};

} // namespace zenith
