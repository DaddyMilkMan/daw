#pragma once
#include <juce_core/juce_core.h>
#include "SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#include <include/effects/SkImageFilters.h>

namespace zenith {

class SkiaUtils {
public:
    /**
     * @brief Draws a rounded rectangle with an inner glow/bloom effect
     * Fixes #13 (Bloom/Glow)
     */
    static void drawGlowingRRect(SkCanvas* canvas, const SkRect& rect, float radius,
                                SkColor color, float glowRadius, bool isFilled = true) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(color);

        if (isFilled) {
            paint.setStyle(SkPaint::kFill_Style);
        } else {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(2.0f);
        }

        // Create Bloom effect using Blur Image Filter
        if (glowRadius > 0.0f) {
            paint.setImageFilter(SkImageFilters::Blur(glowRadius, glowRadius, nullptr));
        }

        SkRRect rrect;
        rrect.setRectXY(rect, radius, radius);
        canvas->drawRRect(rrect, paint);

        // Draw core (sharp) shape on top if it was a strong glow
        paint.setImageFilter(nullptr);
        canvas->drawRRect(rrect, paint);
    }

    /**
     * @brief Draws a specialized meter bar (green -> yellow -> red)
     * Fixes #23 (Metering)
     */
    static void drawLevelMeter(SkCanvas* canvas, const SkRect& bounds, float level0to1) {
        SkRect activeRect = bounds;
        activeRect.fTop = bounds.bottom() - (bounds.height() * level0to1);

        // Background rail
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetARGB(30, 50, 50, 50));
        canvas->drawRect(bounds, bgPaint);

        // Gradient Meter
        SkPoint points[2] = { SkPoint::Make(bounds.centerX(), bounds.bottom()),
                              SkPoint::Make(bounds.centerX(), bounds.top()) };

        // Logic Pro / Pro Tools style gradient
        SkColor colors[3] = { SkColorSetRGB(0, 255, 100),   // Green
                              SkColorSetRGB(255, 200, 0),   // Yellow
                              SkColorSetRGB(255, 50, 50) }; // Red
        SkScalar pos[3] = { 0.0f, 0.75f, 1.0f };            // Breakpoints

        SkPaint meterPaint;
        meterPaint.setShader(SkGradientShader::MakeLinear(points, colors, pos, 3, SkTileMode::kClamp));

        canvas->drawRect(activeRect, meterPaint);
    }

    /**
     * @brief Draws a drop shadow for a component
     * Fixes #19 (Drop Shadows)
     */
    static void drawShadow(SkCanvas* canvas, const SkPath& path, float elevation) {
        SkPaint paint;
        paint.setColor(SK_ColorBLACK);
        paint.setAlphaf(0.3f);
        paint.setImageFilter(SkImageFilters::Blur(elevation, elevation, nullptr));

        canvas->save();
        canvas->translate(0, elevation * 0.5f); // Offset downwards
        canvas->drawPath(path, paint);
        canvas->restore();
    }

    /**
     * @brief Draws a rounded rectangle with shadow
     * Fixes #16 (Visual Polish)
     */
    static void drawShadowedRRect(SkCanvas* canvas, const SkRect& rect, float radius,
                                   SkColor fillColor, SkColor shadowColor, float shadowBlur) {
        // Draw shadow first
        SkRect shadowRect = rect.makeOutset(shadowBlur * 0.5f, shadowBlur * 0.5f);
        SkRRect shadowRRect;
        shadowRRect.setRectXY(shadowRect, radius, radius);

        SkPaint shadowPaint;
        shadowPaint.setColor(shadowColor);
        shadowPaint.setAlphaf(0.3f);
        shadowPaint.setImageFilter(SkImageFilters::Blur(shadowBlur, shadowBlur, nullptr));
        canvas->drawRRect(shadowRRect, shadowPaint);

        // Draw main shape
        SkRRect mainRRect;
        mainRRect.setRectXY(rect, radius, radius);

        SkPaint paint;
        paint.setColor(fillColor);
        paint.setAntiAlias(true);
        canvas->drawRRect(mainRRect, paint);
    }

    /**
     * @brief Draws a gradient-filled rounded rectangle
     * Fixes #20 (Gradient fills)
     */
    static void drawGradientRRect(SkCanvas* canvas, const SkRect& rect, float radius,
                                   const SkColor colors[], int colorCount,
                                   SkTileMode tileMode = SkTileMode::kClamp) {
        SkPoint points[2] = {
            SkPoint::Make(rect.left(), rect.top()),
            SkPoint::Make(rect.left(), rect.bottom())
        };

        SkScalar* positions = new SkScalar[colorCount];
        for (int i = 0; i < colorCount; ++i) {
            positions[i] = i / static_cast<float>(colorCount - 1);
        }

        SkRRect rrect;
        rrect.setRectXY(rect, radius, radius);

        SkPaint paint;
        paint.setShader(SkGradientShader::MakeLinear(points, colors, positions, colorCount, tileMode));
        paint.setAntiAlias(true);

        canvas->drawRRect(rrect, paint);

        delete[] positions;
    }

    /**
     * @brief Draws a rounded rectangle with a border/stroke
     * Fixes #15 (Outlined components)
     */
    static void drawOutlinedRRect(SkCanvas* canvas, const SkRect& rect, float radius,
                                   SkColor fillColor, SkColor strokeColor, float strokeWidth) {
        SkRRect rrect;
        rrect.setRectXY(rect, radius, radius);

        // Fill
        SkPaint fillPaint;
        fillPaint.setColor(fillColor);
        fillPaint.setStyle(SkPaint::kFill_Style);
        fillPaint.setAntiAlias(true);
        canvas->drawRRect(rrect, fillPaint);

        // Stroke
        SkPaint strokePaint;
        strokePaint.setColor(strokeColor);
        strokePaint.setStyle(SkPaint::kStroke_Style);
        strokePaint.setStrokeWidth(strokeWidth);
        strokePaint.setAntiAlias(true);
        canvas->drawRRect(rrect, strokePaint);
    }

    /**
     * @brief Draws a circular progress indicator
     * Fixes #37 (Progress indicators)
     */
    static void drawCircularProgress(SkCanvas* canvas, const SkRect& bounds, float progress0to1,
                                      SkColor trackColor, SkColor progressColor, float strokeWidth) {
        // Track (background)
        SkPaint trackPaint;
        trackPaint.setColor(trackColor);
        trackPaint.setStyle(SkPaint::kStroke_Style);
        trackPaint.setStrokeWidth(strokeWidth);
        trackPaint.setAntiAlias(true);

        canvas->drawOval(bounds, trackPaint);

        // Progress arc
        SkPaint progressPaint;
        progressPaint.setColor(progressColor);
        progressPaint.setStyle(SkPaint::kStroke_Style);
        progressPaint.setStrokeWidth(strokeWidth);
        progressPaint.setAntiAlias(true);
        progressPaint.setStrokeCap(SkPaint::kRound_Cap);

        float sweepAngle = 360.0f * progress0to1;
        canvas->drawArc(bounds, -90.0f, sweepAngle, false, progressPaint);
    }

    /**
     * @brief Utility to clamp a value between min and max
     */
    static float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};

}
#endif
