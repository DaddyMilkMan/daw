#include "SkiaMainWindowIntegration.h"
#include "../../../src/SimpleLogger.h"
#include "../skia/SkiaComponent.h"
#include <gl/GL.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkSurface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <juce_opengl/juce_opengl.h>

#ifdef ZENITH_USE_SKIA

namespace zenith {

SkiaMainWindowIntegration::SkiaMainWindowIntegration() {
  // 1. Setup OpenGL Context
  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(true); // Force 60FPS
  openGLContext.setComponentPaintingEnabled(
      false); // DISABLE JUCE painting - we're using pure OpenGL/Skia
  openGLContext.setMultisamplingEnabled(true);
  openGLContext.attachTo(*this);
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
  openGLContext.detach();
  renderer_.reset();
}

void SkiaMainWindowIntegration::newOpenGLContextCreated() {
  logToFile(
      "SkiaMainWindowIntegration::newOpenGLContextCreated - OpenGL context "
      "created, will initialize Skia on first render");
  // Defer actual initialization to first renderOpenGL() call
  // when we're guaranteed to be in the rendering thread
  rendererInitialized_ = false;
}

void SkiaMainWindowIntegration::openGLContextClosing() {
  grContext_.reset();
  renderer_.reset();
}

void SkiaMainWindowIntegration::renderOpenGL() {
  // Lazy initialization on first render when OpenGL context is guaranteed to be
  // active
  if (!rendererInitialized_) {
    logToFile("SkiaMainWindowIntegration::renderOpenGL - Initializing Skia "
              "renderer for direct framebuffer rendering");

    // Create GrDirectContext
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface) {
      logToFile("ERROR: Failed to create GL interface");
      return;
    }

    grContext_ = GrDirectContexts::MakeGL(glInterface);
    if (!grContext_) {
      logToFile("ERROR: Failed to create GrDirectContext");
      return;
    }

    logToFile(
        "Successfully created Skia GrDirectContext for framebuffer rendering");
    rendererInitialized_ = true;
  }

  if (!grContext_) {
    logToFile("ERROR: No GrDirectContext available");
    return;
  }

  // Get framebuffer dimensions from component bounds
  int fbWidth = getWidth();
  int fbHeight = getHeight();

  if (fbWidth <= 0 || fbHeight <= 0) {
    return; // Component not ready yet
  }

  // Create backend render target info for the default framebuffer (FBO 0)
  GrGLFramebufferInfo fbInfo;
  fbInfo.fFBOID = 0;       // Default framebuffer
  fbInfo.fFormat = 0x8058; // GL_RGBA8

  // Create backend render target wrapping the default framebuffer using factory
  // method
  GrBackendRenderTarget backendRT =
      GrBackendRenderTargets::MakeGL(fbWidth, fbHeight, 1, 8, fbInfo);

  SkSurfaceProps props(0, kRGB_H_SkPixelGeometry);
  sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(
      grContext_.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, nullptr, &props);

  if (!surface) {
    static bool loggedError = false;
    if (!loggedError) {
      logToFile("ERROR: Failed to create Skia surface from framebuffer");
      loggedError = true;
    }
    return;
  }

  // Get canvas and start drawing
  SkCanvas *canvas = surface->getCanvas();
  if (!canvas) {
    logToFile("ERROR: No canvas from surface");
    return;
  }

  // Clear with dark background
  canvas->clear(SkColorSetRGB(40, 40, 45));

  // Recursively render the component tree
  // Iterate over all children of the MainComponent
  int childCount = 0;
  for (auto *child : getChildren()) {
    childCount++;
    // Render ALL visible children, not just SkiaComponents
    renderComponentRecursively(child, canvas);
  }

  static bool loggedComponentTree = false;
  if (!loggedComponentTree) {
    logToFile("SkiaMainWindowIntegration: Rendering " +
              std::to_string(childCount) + " child components");
    loggedComponentTree = true;
  }

  // Flush all Skia GPU commands to the framebuffer
  grContext_->flush();

  static bool loggedSuccess = false;
  if (!loggedSuccess) {
    logToFile("Skia framebuffer rendering initialized successfully!");
    loggedSuccess = true;
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
    skiaComp->drawSkia(canvas);
  }

  // Render Children (Z-Order: Bottom to Top)
  for (auto *child : comp->getChildren()) {
    renderComponentRecursively(child, canvas);
  }

  canvas->restore();
}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
  // OpenGL rendering is active - this paint() method should not be called
  // when setComponentPaintingEnabled(false) is set.
  // If you see this, OpenGL context failed to attach.
  // Leave empty - OpenGL handles all rendering
}

void SkiaMainWindowIntegration::resized() {
  // MainComponent handles resizing of children
}

} // namespace zenith
#endif
