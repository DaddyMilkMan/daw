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
      
      // Demand-driven rendering: only repaint when triggerRepaint() is called
      // This saves CPU/GPU when nothing is animating
      openGLContext_.setContinuousRepainting(false);
      
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
  // Force Software Fallback by skipping OpenGL attachment
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: attachContextNow() - SKIPPED FOR SOFTWARE FALLBACK");
  return;
  
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: Attaching OpenGL context to component...");
  
  try {
    openGLContext_.attachTo(*targetComponent_);
    
    // Disable JUCE component painting - we handle everything via Skia
    openGLContext_.setComponentPaintingEnabled(false);
    
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
  std::cerr << "[RENDER] renderOpenGL() ENTRY" << std::endl;
  std::cerr.flush();
  if (!contextInitialized_ || !grContext_) {
    static bool loggedOnce = false;
    if (!loggedOnce) {
        ZENITH_LOG_WARNING("SkiaOpenGLRenderer: renderOpenGL called but context not initialized!");
        loggedOnce = true;
    }
    return;
  }

  std::cerr << "[RENDER] renderOpenGL() - context valid" << std::endl;
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: renderOpenGL() START");

  // ROBUSTNESS: Check if we are being destroyed or if peer is gone
  // This prevents accessing invalid window handles during teardown
  if (targetComponent_ == nullptr || targetComponent_->getPeer() == nullptr) {
       return;
  }
  
  if (!targetComponent_->isVisible()) {
      return; 
  }

  // IMPORTANT: Do NOT use MessageManagerLock here!
  // The OpenGL render thread should not block the message thread.
  // This was causing UI events (including close button) to be blocked.

  auto width = safeWidth_.load();
  auto height = safeHeight_.load();
  
  // Fallback to component dimensions if atomic values not yet set
  if (width <= 0 || height <= 0) {
    width = targetComponent_->getWidth();
    height = targetComponent_->getHeight();
  }

  // Only recreate surface if size changed
  if (width != lastWidth_ || height != lastHeight_ || !surface_) {
    lastWidth_ = width;
    lastHeight_ = height;
    recreateSurface();
  }

  if (!surface_) {
    return;
  }

  // Get canvas and clear
  skiaCanvas_ = surface_->getCanvas();
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: Clearing canvas...");
  skiaCanvas_->clear(SkColorSetARGB(255, 10, 10, 15)); // Dark background

  // Let derived class draw
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: calling drawSkiaContent...");
  drawSkiaContent(skiaCanvas_);
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: drawSkiaContent returned.");

  // Flush to GPU
  grContext_->flushAndSubmit();
  skiaCanvas_ = nullptr;
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
    : SkiaOpenGLRenderer(this) {}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
  // Software Rasterization Fallback
  static int frameCount = 0;
  frameCount++;
  if (frameCount == 1 || frameCount % 300 == 0) {
      ZENITH_LOG_INFO("Software Paint Frame " + std::to_string(frameCount) + 
                      ", Size: " + std::to_string(getWidth()) + "x" + std::to_string(getHeight()));
  }

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
  // Surface will be recreated in renderOpenGL if size changed
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
