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
      
      // CRITICAL: Disable continuous repainting to save battery/CPU.
      // We rely on triggerRepaint() to update the UI when needed.
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
    } 
    // Removed catch(...) to allow critical failures to surface
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
    
    // Debug logging reduced to avoid spam
    // std::cerr << "[ASYNC] Checking peer..." << std::endl;
    
    if (comp->getPeer() != nullptr && !ctx->isAttached()) {
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
    // Zombie callback removed
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
  }
  // Removed catch(...) as unexpected exceptions should crash/dump
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
  }
}

void SkiaOpenGLRenderer::renderOpenGL() {
  // Ensure context is current on this thread
  if (!openGLContext_.makeActive())
    return;

  if (!contextInitialized_)
    return;

  // Use thread-safe dimensions
  int width = safeWidth_.load();
  int height = safeHeight_.load();

  if (width <= 0 || height <= 0)
    return;

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

#if JUCE_WINDOWS
  d3d12Context_ = std::make_unique<SkiaD3D12Context>();
#endif
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
#if JUCE_WINDOWS
  d3d12Context_.reset();
#endif
}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
#if JUCE_WINDOWS
  // 1. Try D3D12 Path
  if (useD3D12_ && d3d12Context_) {
      if (!d3d12Context_->isInitialized()) {
          // Lazy init
          void* hwnd = getPeer() ? getPeer()->getNativeHandle() : nullptr;
          if (hwnd && getWidth() > 0 && getHeight() > 0) {
              if (d3d12Context_->initialize(hwnd, getWidth(), getHeight())) {
                  ZENITH_LOG_INFO("D3D12 Initialized!");
              } else {
                  ZENITH_LOG_ERROR("D3D12 Init Failed - Unknown error");
                  useD3D12_ = false; // Fallback
              }
          }
      }

      if (d3d12Context_->isInitialized()) {
          SkCanvas* canvas = d3d12Context_->beginFrame();
          if (canvas) {
              canvas->clear(SkColorSetARGB(255, 20, 20, 25));
              drawSkiaContent(canvas);
              d3d12Context_->endFrame();
              return; // Done
          }
      }
  }
#endif

  // 2. Try OpenGL Path
  if (openGLContext_.isAttached()) {
    return;
  }

  // 3. Fallback to Software Raster
  static int frameCount = 0;
  frameCount++;
  
  auto bounds = getLocalBounds();
  if (bounds.isEmpty()) return;

  const int width = bounds.getWidth();
  const int height = bounds.getHeight();

  // Create/Recreate Raster Surface if needed
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

  // Draw Content
  SkCanvas* canvas = softwareSurface_->getCanvas();
  // Clear with background color (Dark Slate)
  canvas->clear(SkColorSetARGB(255, 20, 20, 25)); 
  drawSkiaContent(canvas);

  // Blit to JUCE Graphics
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

#if JUCE_WINDOWS
  if (useD3D12_ && d3d12Context_ && d3d12Context_->isInitialized()) {
      d3d12Context_->resize(bounds.getWidth(), bounds.getHeight());
  }
#endif
}

void SkiaMainWindowIntegration::parentHierarchyChanged() {
  juce::String msg = "SkiaMainWindowIntegration: parentHierarchyChanged called, peer=";
  msg += (getPeer() != nullptr ? "valid" : "null");
  msg += ", attached=";
  msg += (openGLContext_.isAttached() ? "yes" : "no");
  ZENITH_LOG_INFO(msg);

  if (isShowing() && getPeer() != nullptr) {
#if JUCE_WINDOWS
      // If we are using D3D12, we trigger a repaint and let paint() handle init
      if (useD3D12_) {
          repaint();
          return;
      }
#endif
      if (!openGLContext_.isAttached()) {
          scheduleAttachmentCheck();
      }
  }
}

void SkiaMainWindowIntegration::visibilityChanged() {
  juce::String msg = "SkiaMainWindowIntegration: visibilityChanged called, visible=";
  msg += (isVisible() ? "yes" : "no");
  msg += ", peer=";
  msg += (getPeer() != nullptr ? "valid" : "null");
  ZENITH_LOG_INFO(msg);

  if (isShowing() && getPeer() != nullptr) {
#if JUCE_WINDOWS
      if (useD3D12_) {
          repaint();
          return;
      }
#endif
      if (!openGLContext_.isAttached()) {
          scheduleAttachmentCheck();
      }
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
