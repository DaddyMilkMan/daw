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
  juce::Component* comp = targetComponent_;
  juce::OpenGLContext* ctx = &openGLContext_;
  
  ZENITH_LOG_INFO("SkiaOpenGLRenderer::scheduleAttachmentCheck called");
  
  juce::MessageManager::callAsync([this, comp, ctx]() {
    if (!comp || !ctx) {
      ZENITH_LOG_ERROR("scheduleAttachmentCheck: Component or context is null!");
      return;
    }
    
    // Log current state
    bool hasPeer = comp->getPeer() != nullptr;
    bool hasValidSize = comp->getWidth() >= 400 && comp->getHeight() >= 300;
    bool isAttached = ctx->isAttached();
    
    ZENITH_LOG_INFO(juce::String::formatted(
      "scheduleAttachmentCheck: hasPeer=%s, size=%dx%d, validSize=%s, isAttached=%s",
      hasPeer ? "YES" : "NO",
      comp->getWidth(), comp->getHeight(),
      hasValidSize ? "YES" : "NO",
      isAttached ? "YES" : "NO"
    ));
    
    // CRITICAL: Only attach when component has valid dimensions (prevents tiny window)
    if (hasPeer && hasValidSize && !isAttached) {
      ZENITH_LOG_INFO("Conditions met - attempting to attach context");
      attachContextNow();
    } else if (!isAttached) {
      // Reschedule - either no peer yet or dimensions too small
      ZENITH_LOG_INFO("Conditions not met - rescheduling attachment check in 100ms");
      juce::Timer::callAfterDelay(100, [this]() {
        scheduleAttachmentCheck();
      });
    } else {
      ZENITH_LOG_INFO("Context already attached - no action needed");
    }
  });
}

void SkiaOpenGLRenderer::timerCallback() {
  if (targetComponent_ && targetComponent_->getPeer() != nullptr && !openGLContext_.isAttached()) {
    attachContextNow();
    stopTimer();
  } else if (openGLContext_.isAttached()) {
    stopTimer();
  }
}

void SkiaOpenGLRenderer::attachContextNow() {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer::attachContextNow called");
  
  if (!targetComponent_) {
    ZENITH_LOG_ERROR("attachContextNow: targetComponent is null!");
    return;
  }
  
  if (openGLContext_.isAttached()) {
    ZENITH_LOG_INFO("attachContextNow: Context already attached, skipping");
    return;
  }
  
  ZENITH_LOG_INFO(juce::String::formatted(
    "attachContextNow: Component size=%dx%d, hasPeer=%s",
    targetComponent_->getWidth(), targetComponent_->getHeight(),
    targetComponent_->getPeer() != nullptr ? "YES" : "NO"
  ));
  
  try {
    ZENITH_LOG_INFO("Calling openGLContext_.attachTo()...");
    openGLContext_.attachTo(*targetComponent_);
    ZENITH_LOG_INFO("OpenGL context attached successfully!");
  } catch (const std::exception& e) {
    ZENITH_LOG_ERROR("Exception during attachTo: " + std::string(e.what()));
  } catch (...) {
    ZENITH_LOG_ERROR("Unknown exception during attachTo!");
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
  ZENITH_LOG_INFO("========================================");
  ZENITH_LOG_INFO("SkiaOpenGLRenderer::newOpenGLContextCreated called");
  ZENITH_LOG_INFO("========================================");
  
  // Log OpenGL version
  const char* glVersion = (const char*)juce::gl::glGetString(juce::gl::GL_VERSION);
  ZENITH_LOG_INFO("OpenGL Version: " + juce::String(glVersion ? glVersion : "unknown"));

  try {
    ZENITH_LOG_INFO("Creating Skia GL interface...");
    auto glInterface = PlatformWindowUtils::createNativeGLInterface(openGLContext_);
    if (!glInterface) {
      ZENITH_LOG_ERROR("Failed to create GL interface!");
      return;
    }
    ZENITH_LOG_INFO("GL interface created successfully");
    
    ZENITH_LOG_INFO("Creating Skia GrDirectContext...");
    grContext_ = GrDirectContexts::MakeGL(glInterface);
    if (!grContext_) {
      ZENITH_LOG_ERROR("Failed to create GrDirectContext!");
      return;
    }
    ZENITH_LOG_INFO("GrDirectContext created successfully");

    contextInitialized_ = true;
    ZENITH_LOG_INFO("Skia context initialized successfully");
    
    ZENITH_LOG_INFO("Creating Skia surface...");
    recreateSurface();
    
    if (surface_) {
      ZENITH_LOG_INFO("Skia surface created successfully");
    } else {
      ZENITH_LOG_ERROR("Failed to create Skia surface!");
    }
    
  } catch (const std::exception& e) {
    ZENITH_LOG_ERROR("Exception in newOpenGLContextCreated: " + std::string(e.what()));
  } catch (...) {
    ZENITH_LOG_ERROR("Unknown exception in newOpenGLContextCreated");
  }
  
  ZENITH_LOG_INFO("========================================");
}

void SkiaOpenGLRenderer::renderOpenGL() {
  if (!openGLContext_.makeActive()) return;
  if (!contextInitialized_) return;

  if (grContext_ && grContext_->abandoned()) {
    surface_.reset();
    grContext_.reset();
    contextInitialized_ = false;
    newOpenGLContextCreated();
    if (!contextInitialized_) return;
  }

  const double scale = openGLContext_.getRenderingScale();
  int logicalWidth = safeWidth_.load();
  int logicalHeight = safeHeight_.load();

  if (logicalWidth <= 0 || logicalHeight <= 0) return;

  const int physicalWidth = juce::roundToInt(logicalWidth * scale);
  const int physicalHeight = juce::roundToInt(logicalHeight * scale);

  juce::gl::glViewport(0, 0, physicalWidth, physicalHeight);
  
  if (lastWidth_ != physicalWidth || lastHeight_ != physicalHeight) {
    recreateSurfaceWithSize(physicalWidth, physicalHeight);
    lastWidth_ = physicalWidth;
    lastHeight_ = physicalHeight;
  }

  if (surface_) {
    SkCanvas *canvas = surface_->getCanvas();
    if (canvas) {
      canvas->save();
      canvas->scale(static_cast<float>(scale), static_cast<float>(scale));
      canvas->clear(SkColorSetARGB(255, 20, 20, 25));
      drawSkiaContent(canvas);
      canvas->restore();
      grContext_->flushAndSubmit();
    }
  } else {
      juce::gl::glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
      juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);
  }
}

void SkiaOpenGLRenderer::openGLContextClosing() {
  if (surface_) surface_.reset();
  if (grContext_) grContext_.reset();
  contextInitialized_ = false;
}

void SkiaOpenGLRenderer::recreateSurface() {
  if (!targetComponent_) return;
  const double scale = openGLContext_.getRenderingScale();
  int physicalWidth = juce::roundToInt(targetComponent_->getWidth() * scale);
  int physicalHeight = juce::roundToInt(targetComponent_->getHeight() * scale);
  recreateSurfaceWithSize(physicalWidth, physicalHeight);
}

void SkiaOpenGLRenderer::recreateSurfaceWithSize(int width, int height) {
  if (!grContext_ || width <= 0 || height <= 0) return;

  GLint currentFBO = 0;
  GLint samples = 0;
  GLint stencilBits = 0;
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif
#ifndef GL_STENCIL_BITS
#define GL_STENCIL_BITS 0x0D57
#endif

  juce::gl::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
  juce::gl::glGetIntegerv(GL_SAMPLES, &samples);
  juce::gl::glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

  if (samples < 0) samples = 0;
  if (stencilBits < 0) stencilBits = 0;

  GrGLFramebufferInfo framebufferInfo;
  framebufferInfo.fFBOID = (GrGLuint)currentFBO;
  framebufferInfo.fFormat = 0x8058; // GL_RGBA8

  auto backendRT = GrBackendRenderTargets::MakeGL(width, height, samples, stencilBits, framebufferInfo);

  surface_ = SkSurfaces::WrapBackendRenderTarget(
      grContext_.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
}

// ============================================================================
// SkiaMainWindowIntegration Implementation
// ============================================================================

SkiaMainWindowIntegration::SkiaMainWindowIntegration()
    : SkiaOpenGLRenderer(this) {
  setOpaque(true);
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
  // If OpenGL is attached, it handles rendering
  if (openGLContext_.isAttached()) return;

  // Software fallback rendering
  auto bounds = getLocalBounds();
  if (bounds.isEmpty()) return;

  const int width = bounds.getWidth();
  const int height = bounds.getHeight();

  // Create/Recreate Raster Surface
  if (!softwareSurface_ || softwareSurface_->width() != width || softwareSurface_->height() != height) {
      SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
      softwareSurface_ = SkSurfaces::Raster(info);
      softwareImage_ = juce::Image(juce::Image::ARGB, width, height, true);
  }
  
  if (!softwareSurface_) {
      g.fillAll(juce::Colours::darkred);
      return;
  }

  SkCanvas* canvas = softwareSurface_->getCanvas();
  canvas->clear(SkColorSetARGB(255, 20, 20, 25)); 
  drawSkiaContent(canvas);

  SkPixmap pixmap;
  if (softwareSurface_->peekPixels(&pixmap)) {
      juce::Image::BitmapData bd(softwareImage_, juce::Image::BitmapData::writeOnly);
      const size_t size = (size_t)width * height * 4;
      std::memcpy(bd.data, pixmap.addr(), size);
      
      g.drawImageAt(softwareImage_, 0, 0);
  }
}

void SkiaMainWindowIntegration::resized() {
  auto bounds = getLocalBounds();
  updateDimensions(bounds.getWidth(), bounds.getHeight());
}

void SkiaMainWindowIntegration::parentHierarchyChanged() {
  if (isShowing() && getPeer() != nullptr && !openGLContext_.isAttached()) {
    scheduleAttachmentCheck();
  }
}

void SkiaMainWindowIntegration::visibilityChanged() {
  if (isShowing() && getPeer() != nullptr && !openGLContext_.isAttached()) {
    scheduleAttachmentCheck();
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith