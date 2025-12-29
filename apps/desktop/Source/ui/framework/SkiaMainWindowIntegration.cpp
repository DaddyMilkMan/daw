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
    updateDimensions(targetComponent_->getWidth(), targetComponent_->getHeight());
    try {
      // Set up the renderer but DON'T attach yet
      // Attachment will happen when the component gets a peer
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Setting renderer...");
      openGLContext_.setRenderer(this);
      
      // Demand-driven rendering: only repaint when triggerRepaint() is called
      // This saves CPU/GPU when nothing is animating
      openGLContext_.setContinuousRepainting(false);
      
      // Check if component already has a peer (rare but possible)
      if (targetComponent_->getPeer() != nullptr) {
        ZENITH_LOG_INFO("SkiaOpenGLRenderer: Component already has peer, attaching now...");
        attachContextNow();
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
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Critical unknown exception in constructor - Renderer may be unstable");
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
  if (!targetComponent_ || openGLContext_.isAttached()) {
    return;
  }
  
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: Attaching OpenGL context to component...");
  openGLContext_.attachTo(*targetComponent_);
  
  // Disable JUCE component painting - we handle everything via Skia
  openGLContext_.setComponentPaintingEnabled(false);
  
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: OpenGL context attached!");
}

SkiaOpenGLRenderer::~SkiaOpenGLRenderer() {
  surface_.reset();
  grContext_.reset();
  openGLContext_.detach();
}

void SkiaOpenGLRenderer::triggerRepaint() {
  if (openGLContext_.isAttached()) {
    openGLContext_.triggerRepaint();
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
    
    // Use current safe dimensions or component dimensions
    recreateSurface(safeWidth_.get(), safeHeight_.get());
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
  if (!isContextInitialized() || !grContext_) {
    static bool loggedOnce = false;
    if (!loggedOnce) {
        ZENITH_LOG_WARNING("SkiaOpenGLRenderer: renderOpenGL called but context not initialized!");
        loggedOnce = true;
    }
    return;
  }

  // ROBUSTNESS: Check if we are being destroyed or if peer is gone
  // This prevents accessing invalid window handles during teardown
  if (targetComponent_ == nullptr || targetComponent_->getPeer() == nullptr) {
       return;
  }
  
  if (!targetComponent_->isVisible()) {
      return; 
  }

  // NOTE: We removed the MessageManagerLock here to prevent DEADLOCKS.
  // Instead, we use std::atomic metrics updated from the main thread (in resized())
  // to avoid data races while keeping the render loop lock-free.

  int width = safeWidth_.get();
  int height = safeHeight_.get();

  // Only recreate surface if size changed
  if (width != lastWidth_ || height != lastHeight_ || !surface_) {
    recreateSurface(width, height);
    lastWidth_ = width;
    lastHeight_ = height;
  }

  if (!surface_) {
    return;
  }

  // CRITICAL: Update OpenGL viewport to match physical pixels
  juce::gl::glViewport(0, 0, width, height);

  // Get canvas and clear
  skiaCanvas_ = surface_->getCanvas();
  skiaCanvas_->clear(SkColorSetARGB(255, 10, 10, 15)); // Dark background

  // CRITICAL: Scale canvas to handle HiDPI (logical to physical mapping)
  if (targetComponent_) {
    float logicalW = (float)targetComponent_->getWidth();
    if (logicalW > 0) {
      float scale = (float)width / logicalW;
      skiaCanvas_->scale(scale, scale);
    }
  }

  // Let derived class draw
  drawSkiaContent(skiaCanvas_);

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

void SkiaOpenGLRenderer::recreateSurface(int width, int height) {
  if (!grContext_) {
    return;
  }
  
  // Use passed dimensions


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
  if (!isContextInitialized()) {
    g.fillAll(juce::Colours::darkred);
    g.setColour(juce::Colours::white);
    g.drawText("Waiting for Skia/OpenGL Initialization...", getLocalBounds(), juce::Justification::centred);
  }
}

void SkiaMainWindowIntegration::resized() {
  // Update thread-safe dimensions for the render thread (using physical pixels for HiDPI)
  float scale = (float)juce::Desktop::getInstance().getDisplays()
                 .findDisplayForPoint(getScreenBounds().getCentre()).scale;
  
  if (scale <= 0.1f) scale = 1.0f; // Sanity check

  updateDimensions((int)std::round((float)getWidth() * scale), 
                   (int)std::round((float)getHeight() * scale));
  
  // Surface will be recreated in renderOpenGL if size changed
}

void SkiaMainWindowIntegration::parentHierarchyChanged() {
  ZENITH_LOG_INFO("SkiaMainWindowIntegration: parentHierarchyChanged called, peer=" + 
                  std::string(getPeer() != nullptr ? "valid" : "null") +
                  ", attached=" + std::string(openGLContext_.isAttached() ? "yes" : "no"));
  if (getPeer() != nullptr && !openGLContext_.isAttached()) {
    attachContextNow();
  }
}

void SkiaMainWindowIntegration::visibilityChanged() {
  ZENITH_LOG_INFO("SkiaMainWindowIntegration: visibilityChanged called, visible=" + 
                  std::string(isVisible() ? "yes" : "no") +
                  ", peer=" + std::string(getPeer() != nullptr ? "valid" : "null"));
  if (isVisible() && getPeer() != nullptr && !openGLContext_.isAttached()) {
    attachContextNow();
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
