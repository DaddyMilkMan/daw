#include "SkiaMainWindowIntegration.h"
#include "../../../src/SimpleLogger.h"
#include "../skia/SkiaComponent.h"
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>


#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaMainWindowIntegration::SkiaMainWindowIntegration() {
  // 1. Setup OpenGL Context
  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(true); // Force 60FPS
  openGLContext.setComponentPaintingEnabled(
      true); // ENABLE JUCE painting for overlays
  openGLContext.setMultisamplingEnabled(true);
  openGLContext.attachTo(*this);
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
  openGLContext.detach();
  renderer_.reset();
}

void SkiaMainWindowIntegration::newOpenGLContextCreated() {
  logToFile("SkiaMainWindowIntegration::newOpenGLContextCreated - Initializing "
            "Skia...");
  renderer_ =
      std::make_unique<SkiaRenderer>(*this, SkiaRenderer::Backend::OpenGL);
  if (!renderer_->initialize()) {
    logToFile("CRITICAL: Skia failed to initialize");
    DBG("CRITICAL: Skia failed to initialize");
  } else {
    logToFile("Skia initialized successfully.");
  }
}

void SkiaMainWindowIntegration::openGLContextClosing() { renderer_.reset(); }

void SkiaMainWindowIntegration::renderOpenGL() {
  static bool logged = false;
  if (!logged) {
    logToFile("SkiaMainWindowIntegration::renderOpenGL - First frame");
    logged = true;
  }

  if (!renderer_)
    return;

  SkSurface *surface = renderer_->getSurface();
  if (!surface)
    return;

  SkCanvas *canvas = surface->getCanvas();
  if (!canvas)
    return;

  canvas->clear(SkColorSetRGB(18, 18, 20)); // Dark DAW background

  // Recursively Render the Component Tree
  // Iterate over children of 'this' (MainComponent)
  for (auto *child : getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  // Flush GPU context
  if (auto *context = renderer_->getGpuContext()) {
    context->flush();
  }
}

void SkiaMainWindowIntegration::renderComponentRecursively(
    juce::Component *comp, SkCanvas *canvas) {
  if (!comp->isVisible())
    return;

  canvas->save();

  auto bounds = comp->getBounds();
  canvas->translate((SkScalar)bounds.getX(), (SkScalar)bounds.getY());

  // Clip to bounds
  canvas->clipRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));

  // Check if this is a Skia-aware component
  if (auto *skiaComp = dynamic_cast<SkiaComponent *>(comp)) {
    skiaComp->paintToSkia(
        canvas, SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));
  }

  // Render Children (Z-Order: Bottom to Top)
  for (auto *child : comp->getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();
}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
  // Fallback: Only used if OpenGL fails
  if (!renderer_) {
    g.fillAll(juce::Colours::red);
    g.drawText("GPU ERROR", getLocalBounds(), juce::Justification::centred,
               true);
  }
}

void SkiaMainWindowIntegration::resized() {
  // MainComponent handles resizing of children
}

} // namespace zenith
#endif
