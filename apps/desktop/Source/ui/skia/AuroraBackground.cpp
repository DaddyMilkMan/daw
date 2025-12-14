/*
  ==============================================================================

    AuroraBackground.cpp
    Created: 2025-12-13
    Author:  Zenith DAW Team

  ==============================================================================
*/

#include "AuroraBackground.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithDesignSystem.h"
#include <cmath>
#include <core/SkPicture.h>
#include <core/SkPictureRecorder.h>
#include <core/SkString.h>
#include <core/SkSurface.h>
#endif

namespace zenith {

AuroraBackground::AuroraBackground() {
#ifdef ZENITH_USE_SKIA
  initShaders();
#endif
}

AuroraBackground::~AuroraBackground() {}

#ifdef ZENITH_USE_SKIA
void AuroraBackground::initShaders() {
  // NOTE: Advanced SKSL noise distortion was planned but not implemented.
  // SkPerlinNoiseShader is not available in this Skia build configuration.
  // The current implementation uses layered radial gradients with vignette,
  // which provides a beautiful "aurora" effect without runtime shaders.
  //
  // If you want to enable SKSL distortion in the future:
  // 1. Ensure SkRuntimeEffect is available in your Skia build
  // 2. Create a noise shader (e.g., via SkShaders::Fractal or
  // SkPerlinNoiseShader)
  // 3. Compose the noise with the mesh gradient using SkShaders::Blend
  //
  // For now, hasRuntimeEffect_ stays false and we use the direct mesh approach.
  hasRuntimeEffect_ = false;
}

void AuroraBackground::draw(SkCanvas *canvas, const SkRect &bounds,
                            float time) {
  // 1. Prepare Base Mesh Gradient (The "Content")
  // We record the "blobs" moving into a shader.
  SkPictureRecorder recorder;
  SkCanvas *recCanvas = recorder.beginRecording(bounds);

  // Clear with base dark color
  recCanvas->clear(design::colors::BG_DARKEST);

  // Draw Moving Blobs (The "Mesh")
  const int numBlobs = 5;
  struct Blob {
    float xBase, yBase;
    float xAmp, yAmp;
    float freqX, freqY;
    float size;
    SkColor color;
  };

  // Use available colors from ZenithDesignSystem
  Blob blobs[numBlobs] = {
      {0.2f, 0.3f, 0.2f, 0.1f, 0.3f, 0.4f, 600.0f, design::colors::VIOLET},
      {0.8f, 0.2f, 0.15f, 0.2f, 0.2f, 0.3f, 700.0f, design::colors::CYAN},
      {0.5f, 0.8f, 0.2f, 0.1f, 0.4f, 0.2f, 650.0f, design::colors::BLUE},
      {0.1f, 0.8f, 0.1f, 0.15f, 0.3f, 0.3f, 500.0f, design::colors::MAGENTA},
      {0.9f, 0.6f, 0.15f, 0.1f, 0.25f, 0.35f, 550.0f,
       design::colors::NEON_PURPLE}};

  SkPaint blobPaint;
  blobPaint.setAntiAlias(true);
  blobPaint.setBlendMode(SkBlendMode::kScreen); // Blend blobs additively-ish

  float w = bounds.width();
  float h = bounds.height();

  for (int i = 0; i < numBlobs; ++i) {
    float x =
        (blobs[i].xBase + blobs[i].xAmp * std::sin(time * blobs[i].freqX + i)) *
        w;
    float y =
        (blobs[i].yBase + blobs[i].yAmp * std::cos(time * blobs[i].freqY + i)) *
        h;

    // Use a radial gradient for soft edges
    SkColor colors[] = {blobs[i].color, SkColorSetA(blobs[i].color, 0)};
    SkPoint center = {x, y};
    float radius = blobs[i].size;

    sk_sp<SkShader> radialShader = SkGradientShader::MakeRadial(
        center, radius, colors, nullptr, 2, SkTileMode::kClamp);

    SkPaint p;
    p.setShader(radialShader);
    p.setBlendMode(SkBlendMode::kScreen);
    recCanvas->drawRect(bounds, p);
  }

  sk_sp<SkPicture> pic = recorder.finishRecordingAsPicture();
  sk_sp<SkShader> contentShader =
      pic->makeShader(SkTileMode::kClamp, SkTileMode::kClamp,
                      SkFilterMode::kLinear, nullptr, nullptr);

  // 2. Draw the blobs directly (SkPerlinNoiseShader not available in this Skia
  // build) Just draw the picture without noise distortion - still looks great
  canvas->drawPicture(pic, nullptr, nullptr);

  // 4. Overlay Vignette (Darken corners)
  drawVignette(canvas, bounds);
}

void AuroraBackground::drawMeshGradient(SkCanvas *canvas, const SkRect &bounds,
                                        float time) {
  // Implemented in draw() directly
  (void)canvas;
  (void)bounds;
  (void)time;
}

void AuroraBackground::drawVignette(SkCanvas *canvas, const SkRect &bounds) {
  SkPoint center = {bounds.centerX(), bounds.centerY()};
  float radius = std::max(bounds.width(), bounds.height()) * 0.8f;

  // Create radial gradient: transparent center -> dark edges
  SkColor colors[] = {SkColorSetARGB(0, 0, 0, 0), design::colors::BG_DARKEST};
  SkScalar pos[] = {0.4f, 1.0f};

  sk_sp<SkShader> vignette = SkGradientShader::MakeRadial(
      center, radius, colors, pos, 2, SkTileMode::kClamp);

  SkPaint p;
  p.setShader(vignette);
  p.setBlendMode(SkBlendMode::kSrcOver);

  canvas->drawRect(bounds, p);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
