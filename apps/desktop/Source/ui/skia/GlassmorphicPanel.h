/*
  ==============================================================================

    GlassmorphicPanel.h
    Created: 2025-12-11
    Author:  Zenith DAW Team

    Glassmorphism rendering utilities for Zenith DAW's "Neon Noir" design.
    
    Usage:
      GlassmorphicPanel::draw(canvas, bounds, GlassmorphicPanel::Style::Elevated);
      GlassmorphicPanel::drawWithAccent(canvas, bounds, design::colors::CYAN);

  ==============================================================================
*/

#pragma once

#include "ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkMaskFilter.h>
#include <core/SkBlurTypes.h>
#include <effects/SkGradientShader.h>

namespace zenith {

/**
 * @brief Glassmorphism panel rendering utilities
 * 
 * Provides consistent glass-effect panels throughout the UI with:
 * - Semi-transparent backgrounds with blur
 * - Top edge highlights
 * - Subtle drop shadows
 * - Optional accent color glows
 */
class GlassmorphicPanel {
public:
    enum class Style {
        Flat,       // Minimal: solid dark background
        Subtle,     // Light glass effect (for nested panels)
        Elevated,   // Standard glass with shadow (main panels)
        Floating,   // Strong glass with pronounced shadow (dialogs/popups)
        ActiveGlow  // Glass with neon glow border (focused/active elements)
    };

    struct Options {
        Style style = Style::Elevated;
        float cornerRadius = design::dimensions::RADIUS_LG;
        SkColor accentColor = 0x00000000;  // No accent by default
        float glowIntensity = 1.0f;        // Multiplier for glow effects
        bool drawTopHighlight = true;
        bool drawShadow = true;
    };

    /**
     * @brief Draw a glassmorphic panel
     * @param canvas The Skia canvas
     * @param bounds The panel bounds as SkRect
     * @param style The panel style
     */
    static void draw(SkCanvas* canvas, const SkRect& bounds, Style style = Style::Elevated) {
        Options opts;
        opts.style = style;
        drawWithOptions(canvas, bounds, opts);
    }

    /**
     * @brief Draw a glassmorphic panel with accent color glow
     */
    static void drawWithAccent(SkCanvas* canvas, const SkRect& bounds, 
                               SkColor accentColor, Style style = Style::ActiveGlow) {
        Options opts;
        opts.style = style;
        opts.accentColor = accentColor;
        drawWithOptions(canvas, bounds, opts);
    }

    /**
     * @brief Draw a glassmorphic panel with full options control
     */
    static void drawWithOptions(SkCanvas* canvas, const SkRect& bounds, const Options& opts) {
        using namespace design;
        
        float radius = opts.cornerRadius;
        SkRRect rrect = SkRRect::MakeRectXY(bounds, radius, radius);
        float globalGlow = Settings::getGlowIntensity() * opts.glowIntensity;

        // 1. Drop Shadow (under the panel)
        if (opts.drawShadow && opts.style != Style::Flat) {
            SkPaint shadowPaint;
            shadowPaint.setAntiAlias(true);
            shadowPaint.setColor(colors::GLASS_SHADOW);
            
            float blurAmount = 0.0f;
            float offset = 0.0f;
            
            switch (opts.style) {
                case Style::Subtle:
                    blurAmount = effects::SHADOW_OFFSET_SM;
                    offset = 1.0f;
                    break;
                case Style::Elevated:
                    blurAmount = effects::SHADOW_OFFSET_MD;
                    offset = 2.0f;
                    break;
                case Style::Floating:
                case Style::ActiveGlow:
                    blurAmount = effects::SHADOW_OFFSET_LG;
                    offset = 4.0f;
                    break;
                default:
                    break;
            }
            
            if (blurAmount > 0) {
                shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blurAmount));
                SkRRect shadowRRect = rrect;
                shadowRRect.offset(0, offset);
                canvas->drawRRect(shadowRRect, shadowPaint);
            }
        }

        // 2. Background Fill (gradient from dark to darker)
        {
            SkPaint bgPaint;
            bgPaint.setAntiAlias(true);
            
            SkColor bgTop, bgBottom;
            
            switch (opts.style) {
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
            }
            
            SkPoint gradPoints[2] = {
                {bounds.centerX(), bounds.top()},
                {bounds.centerX(), bounds.bottom()}
            };
            SkColor gradColors[2] = {bgTop, bgBottom};
            
            bgPaint.setShader(SkGradientShader::MakeLinear(
                gradPoints, gradColors, nullptr, 2, SkTileMode::kClamp));
            canvas->drawRRect(rrect, bgPaint);
        }

        // 3. Top Edge Highlight (glass effect)
        if (opts.drawTopHighlight && opts.style != Style::Flat) {
            SkPaint highlightPaint;
            highlightPaint.setAntiAlias(true);
            highlightPaint.setStyle(SkPaint::kStroke_Style);
            highlightPaint.setStrokeWidth(1.0f);
            
            // Gradient from visible white at top to transparent
            SkPoint hlPoints[2] = {
                {bounds.left(), bounds.top()},
                {bounds.left(), bounds.top() + bounds.height() * 0.3f}
            };
            SkColor hlColors[2] = {
                colors::GLASS_HIGHLIGHT,  // ~10% white
                0x00FFFFFF                // Transparent
            };
            highlightPaint.setShader(SkGradientShader::MakeLinear(
                hlPoints, hlColors, nullptr, 2, SkTileMode::kClamp));
            
            SkRRect hlRRect = rrect;
            hlRRect.inset(0.5f, 0.5f);
            canvas->drawRRect(hlRRect, highlightPaint);
        }

        // 4. Border
        {
            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(1.0f);
            
            if (opts.style == Style::ActiveGlow && opts.accentColor != 0x00000000) {
                borderPaint.setColor(withAlpha(opts.accentColor, 0.6f));
            } else {
                borderPaint.setColor(colors::BORDER_DEFAULT);
            }
            
            SkRRect borderRRect = rrect;
            borderRRect.inset(0.5f, 0.5f);
            canvas->drawRRect(borderRRect, borderPaint);
        }

        // 5. Accent Glow (for ActiveGlow style or explicit accent)
        if (opts.accentColor != 0x00000000 && globalGlow > 0.01f) {
            SkPaint glowPaint;
            glowPaint.setAntiAlias(true);
            glowPaint.setStyle(SkPaint::kStroke_Style);
            glowPaint.setStrokeWidth(2.0f);
            glowPaint.setColor(withAlpha(opts.accentColor, 0.4f * globalGlow));
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(
                kNormal_SkBlurStyle, effects::GLOW_MEDIUM * globalGlow));
            
            canvas->drawRRect(rrect, glowPaint);
        }
    }

    /**
     * @brief Draw a horizontal divider line with subtle glow
     */
    static void drawDivider(SkCanvas* canvas, float x1, float y, float x2) {
        using namespace design;
        
        SkPaint dividerPaint;
        dividerPaint.setAntiAlias(true);
        dividerPaint.setStrokeWidth(1.0f);
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
     */
    static void fillBackground(SkCanvas* canvas, const SkRect& bounds) {
        using namespace design;
        
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        
        // Radial vignette-style gradient: slightly lighter in center
        SkPoint gradPoints[2] = {
            {bounds.centerX(), bounds.centerY() * 0.4f},  // Near top-center
            {bounds.centerX(), bounds.bottom()}
        };
        SkColor gradColors[3] = {
            colors::BG_DARKER,   // Slightly lighter at top
            colors::BG_DARKEST,  // Dark in middle
            0xFF08080C           // Even darker at bottom (vignette)
        };
        SkScalar positions[3] = {0.0f, 0.5f, 1.0f};
        
        bgPaint.setShader(SkGradientShader::MakeLinear(
            gradPoints, gradColors, positions, 3, SkTileMode::kClamp));
        
        canvas->drawRect(bounds, bgPaint);
    }

private:
    GlassmorphicPanel() = delete;  // Static-only class
};

} // namespace zenith
