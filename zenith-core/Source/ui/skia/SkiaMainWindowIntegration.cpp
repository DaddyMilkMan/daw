#include "SkiaMainWindowIntegration.h"
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
  // Setup OpenGL Context
  openGLContext.setRenderer(this);
  openGLContext.setContinuousRepainting(true); // 60 FPS continuous rendering
  openGLContext.setComponentPaintingEnabled(
      false); // DISABLE JUCE painting - using pure OpenGL/Skia
  openGLContext.setMultisamplingEnabled(true);
  openGLContext.attachTo(*this);
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
  openGLContext.detach();
  // Clean up Skia resources
  cachedSurface_.reset();
  grContext_.reset();
}

void SkiaMainWindowIntegration::newOpenGLContextCreated() {
  DBG("SkiaMainWindowIntegration: OpenGL context created, will initialize Skia on first render");
  // Defer actual initialization to first renderOpenGL() call
  // when we're guaranteed to be in the rendering thread
  rendererInitialized_ = false;

  // Reset cached surface on context recreation
  cachedSurface_.reset();
  cachedWidth_ = 0;
  cachedHeight_ = 0;
}

void SkiaMainWindowIntegration::openGLContextClosing() {
  DBG("SkiaMainWindowIntegration: OpenGL context closing, releasing Skia resources");

  // Invalidate surface first (thread-safe signal)
  surfaceValid_.store(false, std::memory_order_release);

  // Release cached surface
  cachedSurface_.reset();

  // Flush and submit pending GPU work before releasing context
  if (grContext_) {
    grContext_->flushAndSubmit(GrSyncCpu::kYes);
    grContext_.reset();
  }

  rendererInitialized_ = false;
}

void SkiaMainWindowIntegration::renderOpenGL() {
  // Lazy initialization on first render when OpenGL context is guaranteed to be active
  if (!rendererInitialized_) {
    DBG("SkiaMainWindowIntegration: Initializing Skia renderer for direct framebuffer rendering");

    // Create GrDirectContext
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface) {
      DBG("ERROR: Failed to create GL interface");
      return;
    }

    grContext_ = GrDirectContexts::MakeGL(glInterface);
    if (!grContext_) {
      DBG("ERROR: Failed to create GrDirectContext");
      return;
    }

    DBG("Successfully created Skia GrDirectContext for framebuffer rendering");
    rendererInitialized_ = true;
  }

  if (!grContext_) {
    DBG("ERROR: No GrDirectContext available");
    return;
  }

  // Get framebuffer dimensions from component bounds
  int fbWidth = getWidth();
  int fbHeight = getHeight();

  if (fbWidth <= 0 || fbHeight <= 0) {
    return; // Component not ready yet
  }

  // ===========================================================================
  // SURFACE CACHING - Only recreate when size changes (fixes memory leak bug)
  // Thread-safe check using atomic flag to prevent resize race condition
  // ===========================================================================
  if (!surfaceValid_.load(std::memory_order_acquire) || !cachedSurface_ ||
      cachedWidth_ != fbWidth || cachedHeight_ != fbHeight) {
    DBG("SkiaMainWindowIntegration: Creating new surface (" << fbWidth << "x" << fbHeight << ")");

    // Create backend render target info for the default framebuffer (FBO 0)
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = 0;       // Default framebuffer
    fbInfo.fFormat = 0x8058; // GL_RGBA8

    // FIXED: Correct MSAA and stencil bits for default framebuffer (0, 0 not 1, 8)
    // Default FBO has no MSAA samples and no stencil buffer
    GrBackendRenderTarget backendRT =
        GrBackendRenderTargets::MakeGL(fbWidth, fbHeight, 0, 0, fbInfo);

    SkSurfaceProps props(0, kRGB_H_SkPixelGeometry);
    cachedSurface_ = SkSurfaces::WrapBackendRenderTarget(
        grContext_.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType, nullptr, &props);

    if (!cachedSurface_) {
      if (!loggedSurfaceError_) {
        DBG("ERROR: Failed to create Skia surface from framebuffer");
        loggedSurfaceError_ = true;
      }
      surfaceValid_.store(false, std::memory_order_release);
      return;
    }

    // Update cached dimensions and mark surface valid
    cachedWidth_ = fbWidth;
    cachedHeight_ = fbHeight;
    surfaceValid_.store(true, std::memory_order_release);
    loggedSurfaceError_ = false; // Reset error flag on successful creation
  }

  // Get canvas from cached surface
  SkCanvas *canvas = cachedSurface_->getCanvas();
  if (!canvas) {
    DBG("ERROR: No canvas from surface");
    return;
  }

  // Clear with dark background
  canvas->clear(SkColorSetRGB(40, 40, 45));

  // Recursively render the component tree
  int childCount = 0;
  for (auto *child : getChildren()) {
    childCount++;
    renderComponentRecursively(child, canvas);
  }

  // Log component tree on first render (member variable instead of static bool)
  if (!loggedComponentTree_) {
    DBG("SkiaMainWindowIntegration: Rendering " << childCount << " child components");
    loggedComponentTree_ = true;
  }

  // Flush all Skia GPU commands to the framebuffer
  grContext_->flush();

  // Log success on first render (member variable instead of static bool)
  if (!loggedSuccess_) {
    DBG("Skia framebuffer rendering initialized successfully!");
    loggedSuccess_ = true;
  }
}

void SkiaMainWindowIntegration::renderComponentRecursively(
    juce::Component *comp, SkCanvas *canvas) {
  if (!comp || !comp->isVisible())
    return;

  // Validate canvas pointer
  if (!canvas) {
    DBG("ERROR: Null canvas passed to renderComponentRecursively");
    return;
  }

  canvas->save();

  auto bounds = comp->getBounds();

  // Validate bounds (prevent negative width/height)
  jassert(bounds.getWidth() >= 0 && bounds.getHeight() >= 0);
  if (bounds.getWidth() < 0 || bounds.getHeight() < 0) {
    DBG("WARNING: Component has negative bounds: " << bounds.toString());
    canvas->restore();
    return;
  }

  canvas->translate(static_cast<SkScalar>(bounds.getX()),
                    static_cast<SkScalar>(bounds.getY()));

  // Clip to bounds
  canvas->clipRect(SkRect::MakeWH(static_cast<SkScalar>(bounds.getWidth()),
                                   static_cast<SkScalar>(bounds.getHeight())));

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
  juce::ignoreUnused(g);
}

void SkiaMainWindowIntegration::resized() {
  // Invalidate cached surface on resize - it will be recreated in renderOpenGL()
  // Signal to OpenGL thread that surface needs recreation (thread-safe)
  surfaceValid_.store(false, std::memory_order_release);
  cachedSurface_.reset();
  cachedWidth_ = 0;
  cachedHeight_ = 0;
}

} // namespace zenith
#endif
