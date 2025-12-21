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
#include <core/SkCanvas.h>
#include <core/SkColorFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkSurface.h>
#include <effects/SkGradientShader.h>
#include <effects/SkImageFilters.h>
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
    switch (design::Settings::getBlurQuality()) {
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
    return design::Settings::getBlurQuality() !=
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
   * @brief Draw a complete blurred glass panel (recommended API)
   *
   * This is the simplest way to add a glassmorphic panel. It:
   * 1. Blurs the content behind the panel bounds
   * 2. Draws a tinted semi-transparent overlay
   * 3. Optionally adds a highlight edge at the top
   *
   * @param canvas The Skia canvas (must have content already drawn behind)
   * @param bounds Rectangle defining the panel area
   * @param cornerRadius Rounded corner radius (0 for sharp)
   * @param blurRadius Blur sigma (8-24 recommended)
   * @param tintColor Color to tint the blurred area
   * @param tintOpacity Opacity of the tint (0.5-0.8 recommended)
   * @param drawHighlight Whether to add a top edge highlight
   */
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

    // Clip to panel bounds (important for blur containment)
    canvas->clipRRect(rrect, true);

    // =========================================================================
    // CORRECT BACKDROP BLUR TECHNIQUE
    // =========================================================================
    // We use saveLayer with kInitWithPrevious_SaveLayerFlag to capture the
    // existing canvas content (the "backdrop") into the layer. Then we apply
    // a blur filter when restoring, which blurs the captured backdrop.
    //
    // The key insight: saveLayer normally starts with a transparent layer.
    // With kInitWithPrevious, it copies what's already drawn into the layer.
    // When we restore with a blur filter, the copied content gets blurred.
    //
    // After that, we draw the tint overlay on top of the blurred area.
    // =========================================================================

    // Create the blur image filter
    sk_sp<SkImageFilter> blurFilter = SkImageFilters::Blur(
        effectiveRadius, effectiveRadius, SkTileMode::kClamp, nullptr);

    if (!blurFilter) {
      canvas->restore();
      drawSolidFallback(canvas, bounds, cornerRadius, tintColor, tintOpacity);
      return;
    }

    // Method: Use saveLayerAlphaf with blur filter applied during layer
    // compositing The saveLayer captures the current state of the canvas within
    // bounds
    SkPaint layerPaint;
    layerPaint.setImageFilter(blurFilter);

    // Use SaveLayerRec with F32 flag for backdrop operations
    // This creates a layer with the blur filter that will be applied on restore
    SkCanvas::SaveLayerRec layerRec(&bounds, &layerPaint,
                                    SkCanvas::kInitWithPrevious_SaveLayerFlag);
    canvas->saveLayer(layerRec);

    // The layer now contains a blurred copy of what was behind it.
    // We just restore to composite it back.
    canvas->restore();

    // Now draw the tinted overlay on top of the blurred area
    SkPaint overlayPaint;
    overlayPaint.setAntiAlias(true);
    overlayPaint.setColor(
        SkColorSetA(tintColor, static_cast<U8CPU>(tintOpacity * 255)));

    if (cornerRadius > 0) {
      canvas->drawRRect(rrect, overlayPaint);
    } else {
      canvas->drawRect(bounds, overlayPaint);
    }

    // Draw top highlight for that extra glass effect
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
