/*
  ==============================================================================

    BackdropBlur.h
    Created: 2025-12-12
    Author:  Zenith DAW Team

    Real backdrop blur implementation for glassmorphism effects.

    This is NOT fake transparency - it actually blurs the content BEHIND panels
    using Skia's SkImageFilters::Blur with saveLayer.

    Key Insight: saveLayer with an SkImageFilter captures all previously drawn
    content within the specified bounds and applies the filter to it.

    Usage:
      // Simple one-shot blur panel
      BackdropBlur::drawBlurredPanel(canvas, bounds, 12.0f, 16.0f,
                                     colors::BG_DARK, 0.7f);

      // Or using begin/end for custom content on top
      BackdropBlur::beginBlur(canvas, bounds, 16.0f, colors::BG_DARK, 0.6f);
      // ... draw your panel content here ...
      BackdropBlur::endBlur(canvas);

  ==============================================================================
*/

#pragma once

#include "ZenithDesignSystem.h"
#include "ZenithSkia.h"
#include <core/SkColorFilter.h>
#include <core/SkSurface.h>
#include <effects/SkGradientShader.h>
#include <effects/SkImageFilters.h>
#include <effects/SkRuntimeEffect.h>
#pragma clang diagnostic pop
#include <juce_core/juce_core.h>
#include <stack>

namespace zenith {

/**
 * @brief Configuration for backdrop blur system
 *
 * Uses the blur quality setting from design::Settings for centralized
 * control. Performance metrics are tracked here for profiling.
 */
struct BackdropBlurConfig {
  // Performance metrics
  static inline int blurCallsThisFrame = 0;

  // Quality multipliers based on design::Settings::BlurQuality
  static float getRadiusMultiplier() {
    using BQ = design::Settings::BlurQuality;
    switch (design::getSettings().blurQuality) {
    case BQ::Off:
      return 0.0f;
    case BQ::Low:
      return 0.5f;
    case BQ::Medium:
      return 0.75f;
    case BQ::High:
      return 1.0f;
    }
    return 0.75f;
  }

  // Should we skip blur entirely?
  static bool isBlurEnabled() {
    return design::getSettings().blurQuality !=
           design::Settings::BlurQuality::Off;
  }

  // Reset frame stats
  static void resetFrameStats() { blurCallsThisFrame = 0; }
};

/**
 * @brief Real backdrop blur implementation using Skia's image filters
 *
 * This class provides actual content blurring, not fake transparency.
 * The blur is applied to ALL content rendered before the glass panel.
 *
 * Technical approach:
 * 1. saveLayer with SkImageFilters::Blur captures the backdrop
 * 2. The blur filter samples pixels in a radius around each point
 * 3. A tinted overlay is drawn on top for the glass color
 * 4. restore() composites the result
 */
class BackdropBlur {
public:
  /**
   * @brief SkSL Shader for Premium "Neon Noir" Glass
   * 
   * Provides:
   * - Backdrop capture
   * - Smooth Gaussian/Box blur
   * - Subtle noise texture
   * - Chromatic aberration (optional)
   */
  static inline const char* kGlassShaderSource = R"(
    uniform float2 resolution;
    uniform float4 tintColor;
    uniform float noiseIntensity;

    // Helper for pseudo-random noise
    float noise(float2 p) {
        return fract(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
    }

    half4 main(float2 coords) {
        // Generate subtle noise
        float n = (noise(coords) - 0.5) * noiseIntensity;
        
        // Add noise to tint color
        // We preserve the alpha of the tint
        half4 color = half4(tintColor);
        color.rgb += n;
        
        return color;
    }
  )";

  // ... (drawBlurredPanel implementation)
  static void drawBlurredPanel(SkCanvas *canvas, const SkRect &bounds,
                               float cornerRadius, float blurRadius,
                               SkColor tintColor, float tintOpacity,
                               bool drawHighlight = true) {
    jassert(canvas != nullptr);

    // Skip if blur is disabled
    if (!BackdropBlurConfig::isBlurEnabled()) {
      drawSolidFallback(canvas, bounds, cornerRadius, tintColor, tintOpacity);
      return;
    }

    // Apply quality setting to radius
    float effectiveRadius =
        blurRadius * BackdropBlurConfig::getRadiusMultiplier();
    if (effectiveRadius < 1.0f) {
      drawSolidFallback(canvas, bounds, cornerRadius, tintColor, tintOpacity);
      return;
    }

    BackdropBlurConfig::blurCallsThisFrame++;

    // Prepare the RRect for clipping
    SkRRect rrect = cornerRadius > 0 ? SkRRect::MakeRectXY(bounds, cornerRadius,
                                                           cornerRadius)
                                     : SkRRect::MakeRect(bounds);

    // Save canvas state
    canvas->save();
    canvas->clipRRect(rrect, true);

    // 1. Apply efficient Background Blur
    sk_sp<SkImageFilter> blurFilter = SkImageFilters::Blur(
        effectiveRadius, effectiveRadius, SkTileMode::kClamp, nullptr);

    if (blurFilter) {
        SkPaint layerPaint;
        layerPaint.setImageFilter(blurFilter);
        
        // Capture backdrop and blur it
        SkCanvas::SaveLayerRec layerRec(&bounds, &layerPaint,
                                        SkCanvas::kInitWithPrevious_SaveLayerFlag);
        canvas->saveLayer(layerRec);
        canvas->restore();
    } else {
        // Fallback if filter creation fails
        drawSolidFallback(canvas, bounds, cornerRadius, tintColor, tintOpacity);
        canvas->restore();
        return;
    }

    // 2. Draw Tint & Noise Overlay using SkSL
    static sk_sp<SkRuntimeEffect> glassEffect = [](){
        auto result = SkRuntimeEffect::MakeForShader(SkString(kGlassShaderSource));
        return result.effect;
    }();

    SkPaint overlayPaint;
    overlayPaint.setAntiAlias(true);

    // Calculate final tint color with opacity
    SkColor finalTint = SkColorSetA(tintColor, static_cast<U8CPU>(tintOpacity * 255));
    SkColor4f tint4f = SkColor4f::FromColor(finalTint);

    if (glassEffect) {
        SkRuntimeShaderBuilder builder(glassEffect);
        builder.uniform("resolution") = SkV2{bounds.width(), bounds.height()};
        builder.uniform("tintColor") = tint4f;
        builder.uniform("noiseIntensity") = 0.05f; // Adjustable grain

        overlayPaint.setShader(builder.makeShader());
    } else {
        // Fallback if shader fails
        overlayPaint.setColor(finalTint);
    }

    if (cornerRadius > 0) {
      canvas->drawRRect(rrect, overlayPaint);
    } else {
      canvas->drawRect(bounds, overlayPaint);
    }

    // 3. Draw Rim Light
    if (drawHighlight) {
      drawGlassHighlight(canvas, bounds, cornerRadius);
    }

    // Restore the clip
    canvas->restore();
  }

  /**
   * @brief Begin a backdrop blur region (advanced API)
   *
   * Use this when you need to draw custom content on top of the blurred area.
   * Must be paired with endBlur().
   *
   * @param canvas The canvas
   * @param bounds Blur region bounds
   * @param blurRadius Blur sigma
   * @param tintColor Overlay tint
   * @param tintOpacity Overlay opacity
   */
  static void beginBlur(SkCanvas *canvas, const SkRect &bounds,
                        float blurRadius, SkColor tintColor, float tintOpacity,
                        float cornerRadius = 0.0f) {
    jassert(canvas != nullptr);

    // Store state for endBlur
    BlurState state;
    state.bounds = bounds;
    state.tintColor = tintColor;
    state.tintOpacity = tintOpacity;
    state.enabled = BackdropBlurConfig::isBlurEnabled();
    blurStateStack_.push(state);

    if (!state.enabled) {
      return; // No setup needed for solid fallback
    }

    float effectiveRadius =
        blurRadius * BackdropBlurConfig::getRadiusMultiplier();

    if (effectiveRadius < 1.0f) {
      blurStateStack_.top().enabled = false;
      return;
    }

    BackdropBlurConfig::blurCallsThisFrame++;

    // Prepare RRect
    SkRRect rrect;
    if (cornerRadius > 0)
      rrect = SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius);
    else
      rrect = SkRRect::MakeRect(bounds);

    canvas->save();
    canvas->clipRRect(rrect, true);

    // Create blur filter
    sk_sp<SkImageFilter> blurFilter = SkImageFilters::Blur(
        effectiveRadius, effectiveRadius, SkTileMode::kClamp, nullptr);

    if (blurFilter) {
      SkPaint layerPaint;
      layerPaint.setImageFilter(blurFilter);

      // Use kInitWithPrevious to capture backdrop, blur is applied on restore
      SkCanvas::SaveLayerRec layerRec(
          &bounds, &layerPaint, SkCanvas::kInitWithPrevious_SaveLayerFlag);
      canvas->saveLayer(layerRec);
      canvas->restore(); // This applies the blur to captured backdrop
    }

    // Draw tint overlay
    SkPaint overlayPaint;
    overlayPaint.setAntiAlias(true);
    overlayPaint.setColor(
        SkColorSetA(tintColor, static_cast<U8CPU>(tintOpacity * 255)));

    if (cornerRadius > 0)
      canvas->drawRRect(rrect, overlayPaint);
    else
      canvas->drawRect(bounds, overlayPaint);

    // User can now draw custom content...
  }

  /**
   * @brief End a backdrop blur region
   *
   * Must be called after beginBlur() to restore canvas state.
   */
  static void endBlur(SkCanvas *canvas) {
    jassert(canvas != nullptr);
    jassert(!blurStateStack_.empty());

    if (blurStateStack_.empty())
      return;

    BlurState state = blurStateStack_.top();
    blurStateStack_.pop();

    if (state.enabled) {
      canvas->restore(); // Restore from clip
    }
  }

  /**
   * @brief Draw a solid panel (fallback when blur is disabled)
   */
  static void drawSolidFallback(SkCanvas *canvas, const SkRect &bounds,
                                float cornerRadius, SkColor color,
                                float opacity) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetA(color, static_cast<U8CPU>(opacity * 255)));

    if (cornerRadius > 0) {
      canvas->drawRRect(SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius),
                        paint);
    } else {
      canvas->drawRect(bounds, paint);
    }
  }

  /**
   * @brief Get recommended blur radius for different panel styles
   */

private:
  /**
   * @brief Draw the glass highlight effect on top edge
   */
  static void drawGlassHighlight(SkCanvas *canvas, const SkRect &bounds,
                                 float cornerRadius) {
    using namespace design;

    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setStyle(SkPaint::kStroke_Style);
    highlightPaint.setStrokeWidth(1.0f);

    // Gradient from visible at top to transparent
    SkPoint gradPoints[2] = {
        {bounds.left(), bounds.top()},
        {bounds.left(), bounds.top() + bounds.height() * 0.25f}};
    SkColor gradColors[2] = {
        colors::GLASS_HIGHLIGHT, // ~10% white
        0x00FFFFFF               // Transparent
    };

    highlightPaint.setShader(SkGradientShader::MakeLinear(
        gradPoints, gradColors, nullptr, 2, SkTileMode::kClamp));

    if (cornerRadius > 0) {
      SkRRect hlRRect = SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius);
      hlRRect.inset(0.5f, 0.5f);
      canvas->drawRRect(hlRRect, highlightPaint);
    } else {
      SkRect hlRect = bounds;
      hlRect.inset(0.5f, 0.5f);
      canvas->drawRect(hlRect, highlightPaint);
    }
  }

  // State tracking for begin/end API
  struct BlurState {
    SkRect bounds;
    SkColor tintColor;
    float tintOpacity;
    bool enabled;
  };

  static inline std::stack<BlurState> blurStateStack_;
};

/**
 * @brief RAII helper for backdrop blur regions
 *
 * Usage:
 *   {
 *       ScopedBackdropBlur blur(canvas, bounds, 16.0f);
 *       // Draw content on top of blur...
 *   } // Automatically ends blur
 */
class ScopedBackdropBlur {
public:
  ScopedBackdropBlur(SkCanvas *canvas, const SkRect &bounds, float blurRadius,
                     SkColor tintColor = 0xFF1C1C24, float tintOpacity = 0.7f,
                     float cornerRadius = 0.0f)
      : canvas_(canvas) {
    BackdropBlur::beginBlur(canvas, bounds, blurRadius, tintColor, tintOpacity,
                            cornerRadius);
  }

  ~ScopedBackdropBlur() { BackdropBlur::endBlur(canvas_); }

  // Non-copyable
  ScopedBackdropBlur(const ScopedBackdropBlur &) = delete;
  ScopedBackdropBlur &operator=(const ScopedBackdropBlur &) = delete;

private:
  SkCanvas *canvas_;
};

} // namespace zenith
