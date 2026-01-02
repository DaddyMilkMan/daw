/*
  ==============================================================================

    SkiaMainWindowIntegration.cpp
    Created: 2025-11-28
    Author:  Zenith DAW Team

    Implementation of Skia-rendered main window base class.

  ==============================================================================
*/

#include "SkiaMainWindowIntegration.h"

#ifdef ZENITH_USE_SKIA
#include "../../engine/ZenithLogger.h"
#include "PlatformWindowUtils.h"
#include <cstring>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <juce_opengl/juce_opengl.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

// ============================================================================
// SkiaOpenGLRenderer Implementation
// ============================================================================

SkiaOpenGLRenderer::SkiaOpenGLRenderer(juce::Component *componentToAttach)
    : targetComponent_(componentToAttach) {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: Constructor called");
  
  if (targetComponent_) {
    try {
      // Set up the renderer but DON'T attach yet
      // Attachment will happen when the component gets a peer
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Setting renderer...");
      openGLContext_.setRenderer(this);
      openGLContext_.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
      
      // Following JUCE OpenGLAppComponent pattern: continuous repainting
      openGLContext_.setContinuousRepainting(true);
      
      // Check if component already has a peer (rare but possible)
      if (targetComponent_->isShowing() && targetComponent_->getPeer() != nullptr) {
        ZENITH_LOG_INFO("SkiaOpenGLRenderer: Component already has peer, scheduling deferred attachment...");
        scheduleAttachmentCheck();
      } else {
        ZENITH_LOG_INFO("SkiaOpenGLRenderer: Component has no peer yet, scheduling deferred attachment...");
        // Schedule periodic checks via MessageManager
        // This works because the message loop will process these after initialise() returns
        scheduleAttachmentCheck();
      }
      
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Constructor complete");
    } catch (const std::exception &e) {
      ZENITH_LOG_ERROR(
          std::string("SkiaOpenGLRenderer: Exception in constructor: ") +
          e.what());
    } catch (...) {
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Unknown exception in constructor");
    }
  } else {
    ZENITH_LOG_WARNING(
        "SkiaOpenGLRenderer: WARNING - targetComponent is null!");
  }
}

void SkiaOpenGLRenderer::scheduleAttachmentCheck() {
  // Use a weak reference pattern to avoid accessing destroyed objects
  juce::Component* comp = targetComponent_;
  juce::OpenGLContext* ctx = &openGLContext_;
  
  juce::MessageManager::callAsync([this, comp, ctx]() {
    // Safety check - make sure objects are still valid
    if (!comp || !ctx) return;
    
    std::cerr << "[ASYNC] Checking peer, peer=" 
              << (comp->getPeer() ? "valid" : "null")
              << ", attached=" << (ctx->isAttached() ? "yes" : "no") << std::endl;
    
    if (comp->getPeer() != nullptr && !ctx->isAttached()) {
      std::cerr << "[ASYNC] Peer available! Attaching context now..." << std::endl;
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Async check found peer, attaching context...");
      attachContextNow();
    } else if (!ctx->isAttached()) {
      // No peer yet, schedule another check in 100ms
      juce::Timer::callAfterDelay(100, [this]() {
        scheduleAttachmentCheck();
      });
    }
  });
}

void SkiaOpenGLRenderer::timerCallback() {
  // Fallback timer callback - kept for compatibility but shouldn't be needed
  if (targetComponent_ && targetComponent_->getPeer() != nullptr && !openGLContext_.isAttached()) {
    attachContextNow();
    stopTimer();
  } else if (openGLContext_.isAttached()) {
    stopTimer();
  }
}

void SkiaOpenGLRenderer::attachContextNow() {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: Attaching OpenGL context to component...");
  
  try {
    openGLContext_.attachTo(*targetComponent_);
    
    // Keep component painting enabled - JUCE handles presentation
    // openGLContext_.setComponentPaintingEnabled(false);
    
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: OpenGL context attached!");
  } catch (const std::exception& e) {
    ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Exception during attachTo: " + std::string(e.what()));
  } catch (...) {
    ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Unknown exception during attachTo!");
  }
}


SkiaOpenGLRenderer::~SkiaOpenGLRenderer() {
  surface_.reset();
  grContext_.reset();
  openGLContext_.detach();
}

void SkiaOpenGLRenderer::triggerRepaint() {
  if (openGLContext_.isAttached()) {
    openGLContext_.triggerRepaint();
  } else if (targetComponent_) {
    targetComponent_->repaint();
  }
}

void SkiaOpenGLRenderer::newOpenGLContextCreated() {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: newOpenGLContextCreated called");
  
  // LOG OpenGL version info for diagnostics
  const char* glVersion = (const char*)juce::gl::glGetString(juce::gl::GL_VERSION);
  const char* glVendor = (const char*)juce::gl::glGetString(juce::gl::GL_VENDOR);
  const char* glRenderer = (const char*)juce::gl::glGetString(juce::gl::GL_RENDERER);
  ZENITH_LOG_INFO("OpenGL Version: " + juce::String(glVersion ? glVersion : "unknown"));
  ZENITH_LOG_INFO("OpenGL Vendor: " + juce::String(glVendor ? glVendor : "unknown"));
  ZENITH_LOG_INFO("OpenGL Renderer: " + juce::String(glRenderer ? glRenderer : "unknown"));

  try {
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Creating GL interface...");
    // Create platform-specific native interface
    auto glInterface =
        PlatformWindowUtils::createNativeGLInterface(openGLContext_);
    if (!glInterface) {
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: FAILED to create GL interface!");
      return;
    }
    ZENITH_LOG_INFO(
        "SkiaOpenGLRenderer: GL interface created, making context...");
    grContext_ = GrDirectContexts::MakeGL(glInterface);

    if (!grContext_) {
      ZENITH_LOG_ERROR(
          "SkiaOpenGLRenderer: Failed to create Skia GrDirectContext!");
      return;
    }

    ZENITH_LOG_INFO(
        "SkiaOpenGLRenderer: GrDirectContext created successfully!");
    contextInitialized_ = true;
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Initializing surface...");
    recreateSurface();
  } catch (const std::exception &e) {
    ZENITH_LOG_ERROR(
        std::string(
            "SkiaOpenGLRenderer: Exception in newOpenGLContextCreated: ") +
        e.what());
  } catch (...) {
    ZENITH_LOG_ERROR(
        "SkiaOpenGLRenderer: Unknown exception in newOpenGLContextCreated");
  }
}

void SkiaOpenGLRenderer::renderOpenGL() {
  // LOG to confirm rendering is happening
  static int renderCount = 0;
  if (renderCount++ % 300 == 0) { // Every ~5 seconds
    ZENITH_LOG_INFO("renderOpenGL frame " + std::to_string(renderCount) +
      " bounds: " + std::to_string(targetComponent_->getWidth()) + "x" + 
      std::to_string(targetComponent_->getHeight()) + 
      " screen: " + std::to_string(targetComponent_->getScreenX()) + "," + 
      std::to_string(targetComponent_->getScreenY()));
  }

  // Ensure context is current on this thread
  if (!openGLContext_.makeActive()) {
    ZENITH_LOG_ERROR("renderOpenGL: Failed to make context active!");
    return;
  }

  if (!contextInitialized_) {
    static int skipCount = 0;
    if (skipCount++ % 300 == 0)
      ZENITH_LOG_INFO("renderOpenGL: contextInitialized_ is FALSE, skipping. skipCount=" + std::to_string(skipCount));
    return;
  }

  // Use thread-safe dimensions
  int width = safeWidth_.load();
  int height = safeHeight_.load();

  if (width <= 0 || height <= 0) {
    static int dimSkip = 0;
    if (dimSkip++ % 300 == 0)
      ZENITH_LOG_INFO("renderOpenGL: Invalid dimensions " + std::to_string(width) + "x" + std::to_string(height) + ", skipCount=" + std::to_string(dimSkip));
    return;
  }
  // Set viewport
  juce::gl::glViewport(0, 0, width, height);
  
  // Recreate Skia surface if size changed
  if (lastWidth_ != width || lastHeight_ != height) {
    recreateSurface();
    lastWidth_ = width;
    lastHeight_ = height;
  }

  // Render Skia content
  if (surface_) {
    SkCanvas *canvas = surface_->getCanvas();
    if (canvas) {
      // Clear to dark background 
      canvas->clear(SkColorSetARGB(255, 20, 20, 25));
      
      // Draw actual UI content
      drawSkiaContent(canvas);
      
      // Flush Skia commands to OpenGL
      grContext_->flushAndSubmit();
    }
  }
}

void SkiaOpenGLRenderer::openGLContextClosing() {
  if (surface_) {
    surface_.reset();
  }
  if (grContext_) {
    grContext_.reset();
  }
  contextInitialized_ = false;
}

void SkiaOpenGLRenderer::recreateSurface() {
  if (!grContext_) {
    return;
  }

  auto width = targetComponent_->getWidth();
  auto height = targetComponent_->getHeight();

  if (width <= 0 || height <= 0) {
    return;
  }

  // Get framebuffer info
  GLint currentFBO = 0;
  GLint samples = 0;
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif

  juce::gl::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
  juce::gl::glGetIntegerv(GL_SAMPLES, &samples);

  // Clamp samples to valid range (0 or 1 means no MSAA to Skia)
  if (samples < 0)
    samples = 0;

  GrGLFramebufferInfo framebufferInfo;
  framebufferInfo.fFBOID = (GrGLuint)currentFBO;
  framebufferInfo.fFormat = 0x8058; // GL_RGBA8

  // Create backend render target
  auto backendRT =
      GrBackendRenderTargets::MakeGL(width, height,
                                     samples, // Use actual sample count
                                     8,       // stencil bits
                                     framebufferInfo);

  // Create Skia surface
  surface_ = SkSurfaces::WrapBackendRenderTarget(
      grContext_.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, nullptr, nullptr);

  if (!surface_) {
    ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Failed to create Skia surface!");
  } else {
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Skia surface created successfully (" +
                    std::to_string(width) + "x" + std::to_string(height) + ")");
  }
}

// ============================================================================
// SkiaMainWindowIntegration Implementation
// ============================================================================

SkiaMainWindowIntegration::SkiaMainWindowIntegration()
    : SkiaOpenGLRenderer(this) {
  // CRITICAL: Following JUCE OpenGLAppComponent pattern
  // setOpaque(true) is required for OpenGL rendering on Linux!
  setOpaque(true);
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
  // If OpenGL is attached, it handles rendering - skip software fallback
  if (openGLContext_.isAttached()) {
    return;
  }

  // Software fallback rendering (only when OpenGL fails)
  static int frameCount = 0;
  frameCount++;
  
  auto bounds = getLocalBounds();
  if (bounds.isEmpty()) return;

  const int width = bounds.getWidth();
  const int height = bounds.getHeight();

  // 1. Create/Recreate Raster Surface if needed
  if (!softwareSurface_ || softwareSurface_->width() != width || softwareSurface_->height() != height) {
      SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
      softwareSurface_ = SkSurfaces::Raster(info);
      softwareImage_ = juce::Image(juce::Image::ARGB, width, height, true);
      ZENITH_LOG_INFO("Recreated Software Surface for UI: " + std::to_string(width) + "x" + std::to_string(height));
  }
  
  if (!softwareSurface_) {
      g.fillAll(juce::Colours::darkred);
      g.drawText("Failed to create Skia Raster Surface", bounds, juce::Justification::centred);
      return;
  }

  // 2. Draw Content
  SkCanvas* canvas = softwareSurface_->getCanvas();
  // Clear with background color (Dark Slate)
  canvas->clear(SkColorSetARGB(255, 20, 20, 25)); 
  drawSkiaContent(canvas);

  // 3. Blit to JUCE Graphics
  SkPixmap pixmap;
  if (softwareSurface_->peekPixels(&pixmap)) {
      juce::Image::BitmapData bd(softwareImage_, juce::Image::BitmapData::writeOnly);
      const size_t size = (size_t)width * height * 4;
      std::memcpy(bd.data, pixmap.addr(), size);
      
      g.drawImageAt(softwareImage_, 0, 0);
  }
}

void SkiaMainWindowIntegration::resized() {
  // Update dimensions for OpenGL rendering
  auto bounds = getLocalBounds();
  updateDimensions(bounds.getWidth(), bounds.getHeight());
}

void SkiaMainWindowIntegration::parentHierarchyChanged() {
  juce::String msg = "SkiaMainWindowIntegration: parentHierarchyChanged called, peer=";
  msg += (getPeer() != nullptr ? "valid" : "null");
  msg += ", attached=";
  msg += (openGLContext_.isAttached() ? "yes" : "no");
  ZENITH_LOG_INFO(msg);

  if (isShowing() && getPeer() != nullptr && !openGLContext_.isAttached()) {
    scheduleAttachmentCheck();
  }
}

void SkiaMainWindowIntegration::visibilityChanged() {
  juce::String msg = "SkiaMainWindowIntegration: visibilityChanged called, visible=";
  msg += (isVisible() ? "yes" : "no");
  msg += ", peer=";
  msg += (getPeer() != nullptr ? "valid" : "null");
  ZENITH_LOG_INFO(msg);

  if (isShowing() && getPeer() != nullptr && !openGLContext_.isAttached()) {
    scheduleAttachmentCheck();
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
