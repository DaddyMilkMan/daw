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
#include "../design-system/ZenithDesignSystem.h"
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
  // Attach OpenGL context to this component
  if (targetComponent_) {
    openGLContext_.setRenderer(this);
    openGLContext_.attachTo(*targetComponent_);
    openGLContext_.setContinuousRepainting(true);
  }
}

SkiaOpenGLRenderer::~SkiaOpenGLRenderer() {
  surface_.reset();
  grContext_.reset();
  openGLContext_.detach();
}

void SkiaOpenGLRenderer::newOpenGLContextCreated() {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: newOpenGLContextCreated called");
  try {
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Creating GL interface...");
    auto glInterface = GrGLMakeNativeInterface();
    if (!glInterface) {
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: FAILED to create GL interface!");
      return;
    }
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: GL interface created, making context...");
    grContext_ = GrDirectContexts::MakeGL(glInterface);

    if (!grContext_) {
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Failed to create Skia GrDirectContext!");
      return;
    }

    ZENITH_LOG_INFO("SkiaOpenGLRenderer: GrDirectContext created successfully!");
    contextInitialized_ = true;
    
    // Initial surface creation attempt
    const double scale = openGLContext_.getRenderingScale();
    const int w = juce::roundToInt(targetComponent_->getWidth() * scale);
    const int h = juce::roundToInt(targetComponent_->getHeight() * scale);
    recreateSurface(w, h);
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
  if (!contextInitialized_ || !grContext_) {
    return;
  }

// Get scale factor
  const double scale = openGLContext_.getRenderingScale();
  const int physicalWidth = juce::roundToInt(targetComponent_->getWidth() * scale);
  const int physicalHeight = juce::roundToInt(targetComponent_->getHeight() * scale);

  // Check for invalid size
  if (physicalWidth <= 0 || physicalHeight <= 0)
     return;

  // Recreate surface if size changed
  if (physicalWidth != lastWidth_ || physicalHeight != lastHeight_ || !surface_) {
    recreateSurface(physicalWidth, physicalHeight);
    lastWidth_ = physicalWidth;
    lastHeight_ = physicalHeight;
  }

  if (!surface_) {
    return;
  }

  // Get canvas and clear
  skiaCanvas_ = surface_->getCanvas();
  // Clear with a solid color to prevent garbage
  skiaCanvas_->clear(SkColorSetARGB(255, 10, 10, 15)); 

  // Apply DPI scale
  skiaCanvas_->save();
  skiaCanvas_->scale((float)scale, (float)scale);

  // Let derived class draw
  drawSkiaContent(skiaCanvas_);
  
  skiaCanvas_->restore();

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
  juce::ignoreUnused(g);
  // Fallback if OpenGL context is not active or attached
  // This ensures we never see "garbage" or pure black if GL fails
  g.fillAll(juce::Colour(0xff0a0a0f));
  
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.setFont(12.0f);
  g.drawText("Software Renderer (OpenGL Fallback)", getLocalBounds().removeFromBottom(20), juce::Justification::centred, false);
}

void SkiaMainWindowIntegration::resized() {
  // Surface will be recreated in renderOpenGL if size changed
}

void SkiaMainWindowIntegration::mouseMove(const juce::MouseEvent &e) {
  // Update global mouse position for lighting effects
  zenith::design::Settings::mousePosition = {
      (float)e.getPosition().x, (float)e.getPosition().y};
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
