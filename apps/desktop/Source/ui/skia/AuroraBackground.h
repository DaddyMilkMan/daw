/*
  ==============================================================================

    AuroraBackground.h
    Created: 2025-12-13
    Author:  Zenith DAW Team

    "Living" Mesh Gradient Background Renderer.
    Replaces the "screensaver from 2005" with a premium shifting fog/aurora.

  ==============================================================================
*/

#pragma once

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkShader.h>
#include <effects/SkGradientShader.h>
#include <effects/SkRuntimeEffect.h>
#endif

namespace zenith {

class AuroraBackground {
public:
  AuroraBackground();
  ~AuroraBackground();

  /**
   * Render the Aurora Mesh Gradient.
   * @param canvas The Skia canvas to draw onto.
   * @param bounds The bounds of the area to fill.
   * @param time   Current animation time in seconds.
   */
#ifdef ZENITH_USE_SKIA
  void draw(SkCanvas *canvas, const SkRect &bounds, float time);
#endif

private:
#ifdef ZENITH_USE_SKIA
  // Runtime Effect for the "Smoke" displacement (if initialized)
  sk_sp<SkRuntimeEffect> noiseEffect_;
  bool hasRuntimeEffect_ = false;

  void initShaders();

  // Helpers for fallback rendering
  void drawMeshGradient(SkCanvas *canvas, const SkRect &bounds, float time);
  void drawVignette(SkCanvas *canvas, const SkRect &bounds);
#endif
};

} // namespace zenith
