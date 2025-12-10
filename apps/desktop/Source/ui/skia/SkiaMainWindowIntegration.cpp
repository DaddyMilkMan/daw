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
#include "../../SimpleLogger.h"
#include <core/SkSurface.h>
#include <gpu/ganesh/gl/GrGLInterface.h>
#include <juce_opengl/juce_opengl.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

// ============================================================================
// SkiaOpenGLRenderer Implementation
// ============================================================================

SkiaOpenGLRenderer::SkiaOpenGLRenderer(juce::Component *componentToAttach)
    : targetComponent_(componentToAttach) {
  logToFile("SkiaOpenGLRenderer: Constructor called");
  // Attach OpenGL context to this component
  if (targetComponent_) {
    try {
      logToFile("SkiaOpenGLRenderer: Setting renderer...");
      openGLContext_.setRenderer(this);
      logToFile("SkiaOpenGLRenderer: Attaching to component...");
      openGLContext_.attachTo(*targetComponent_);
      
      // DISABLE JUCE COMPONENT PAINTING - Pure Skia Mode
      openGLContext_.setComponentPaintingEnabled(false);
      
      logToFile("SkiaOpenGLRenderer: Setting continuous repainting...");
      openGLContext_.setContinuousRepainting(true);
      logToFile("SkiaOpenGLRenderer: Constructor complete");
    } catch (const std::exception &e) {
      logToFile(std::string("SkiaOpenGLRenderer: Exception in constructor: ") +
                e.what());
    } catch (...) {
      logToFile("SkiaOpenGLRenderer: Unknown exception in constructor");
    }
  } else {
    logToFile("SkiaOpenGLRenderer: WARNING - targetComponent is null!");
  }
}

SkiaOpenGLRenderer::~SkiaOpenGLRenderer() {
  surface_.reset();
  grContext_.reset();
  openGLContext_.detach();
}

void SkiaOpenGLRenderer::newOpenGLContextCreated() {
  logToFile("SkiaOpenGLRenderer: newOpenGLContextCreated called");
  try {
    logToFile("SkiaOpenGLRenderer: Creating GL interface...");
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface) {
      logToFile("SkiaOpenGLRenderer: FAILED to create GL interface!");
      return;
    }
    logToFile("SkiaOpenGLRenderer: GL interface created, making context...");
    grContext_ = GrDirectContexts::MakeGL(glInterface);

    if (!grContext_) {
      logToFile("SkiaOpenGLRenderer: Failed to create Skia GrDirectContext!");
      return;
    }

    logToFile("SkiaOpenGLRenderer: GrDirectContext created successfully!");
    contextInitialized_ = true;
    recreateSurface();
  } catch (const std::exception &e) {
    logToFile(
        std::string(
            "SkiaOpenGLRenderer: Exception in newOpenGLContextCreated: ") +
        e.what());
  } catch (...) {
    logToFile(
        "SkiaOpenGLRenderer: Unknown exception in newOpenGLContextCreated");
  }
}

void SkiaOpenGLRenderer::renderOpenGL() {
  if (!contextInitialized_ || !grContext_) {
    return;
  }

  auto width = targetComponent_->getWidth();
  auto height = targetComponent_->getHeight();

  // Only recreate surface if size changed
  if (width != lastWidth_ || height != lastHeight_ || !surface_) {
    recreateSurface();
    lastWidth_ = width;
    lastHeight_ = height;
  }

  if (!surface_) {
    return;
  }

  // Get canvas and clear
  skiaCanvas_ = surface_->getCanvas();
  skiaCanvas_->clear(SkColorSetARGB(255, 10, 10, 15)); // Dark background

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
    logToFile("SkiaOpenGLRenderer: Failed to create Skia surface!");
  } else {
    logToFile("SkiaOpenGLRenderer: Skia surface created successfully (" +
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
  juce::ignoreUnused(g);
  // OpenGL rendering handles everything
  // This is just a fallback
  // g.fillAll(juce::Colour(0xff0a0a0f));
}

void SkiaMainWindowIntegration::resized() {
  // Surface will be recreated in renderOpenGL if size changed
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
