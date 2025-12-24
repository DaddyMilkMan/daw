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
#include <juce_data_structures/juce_data_structures.h>
#include <juce_opengl/juce_opengl.h> // Critical for openGLContext
#include <memory>
#include <stdexcept>
#include <string>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

// ============================================================================
// SkiaOpenGLRenderer Implementation
// ============================================================================

// Static helper for Safe Mode
static bool checkSafeMode() {
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = "settings";
  options.folderName = "ZenithDAW";
  options.osxLibrarySubFolder = "Application Support";
  options.commonToAllUsers = false;
  options.ignoreCaseOfKeyNames = true;

  juce::PropertiesFile props(options);
  bool safeMode = props.getBoolValue("SafeMode", false);

  if (safeMode) {
    ZENITH_LOG_WARNING("SkiaOpenGLRenderer: SAFE MODE DETECTED. Skipping "
                       "OpenGL initialization.");
  }
  return safeMode;
}

SkiaOpenGLRenderer::SkiaOpenGLRenderer(juce::Component *componentToAttach)
    : targetComponent_(componentToAttach) {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: Constructor called");

  // Robust Initialization Verification [Task 1.2]: Thread Safety
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    // OK
  } else {
    ZENITH_LOG_ERROR("SkiaOpenGLRenderer: FATAL - Constructor called on "
                     "background thread! OpenGL attachment will likely crash.");
    // We can't easily abort here without throwing, but logging is critical.
  }

  // Attach OpenGL context to this component
  if (targetComponent_) {
    try {
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Setting renderer...");
      openGLContext_.setRenderer(this);
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Attaching to component...");
      openGLContext_.attachTo(*targetComponent_); // Critical for GPU rendering

      // DISABLE JUCE component painting - Skia renders via OpenGL
      // Mouse events still work - they're handled by JUCE's event system
      // independently
      openGLContext_.setComponentPaintingEnabled(false);

      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Setting continuous repainting...");
      openGLContext_.setContinuousRepainting(true);
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

SkiaOpenGLRenderer::~SkiaOpenGLRenderer() {
  surface_.reset();
  grContext_.reset();
  openGLContext_.detach();
}

void SkiaOpenGLRenderer::newOpenGLContextCreated() {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: newOpenGLContextCreated called");

  if (checkSafeMode()) {
    ZENITH_LOG_WARNING(
        "SkiaOpenGLRenderer: Safe Mode enabled - preventing GL init.");
    contextInitialized_ = false;
    return;
  }

  try {
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Creating GL interface...");

    // Create platform-specific native interface
    interface_ = PlatformWindowUtils::createNativeGLInterface(openGLContext_);

    if (!interface_) {
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: FAILED to create GL interface! "
                       "Driver may be unsupported.");
      // If we fail here, we should probably set a temporary flag or just fall
      // back for this session
      return;
    }

    ZENITH_LOG_INFO(
        "SkiaOpenGLRenderer: GL interface created, making context...");
    grContext_ = GrDirectContexts::MakeGL(interface_);

    if (!grContext_) {
      ZENITH_LOG_ERROR(
          "SkiaOpenGLRenderer: Failed to create Skia GrDirectContext!");
      // This is a critical failure of Skia-on-GL.
      return;
    }

    ZENITH_LOG_INFO(
        "SkiaOpenGLRenderer: GrDirectContext created successfully!");
    contextInitialized_ = true;

    // Explicitly rebuild surface on new context
    recreateSurface();

  } catch (const std::exception &e) {
    ZENITH_LOG_ERROR(
        std::string(
            "SkiaOpenGLRenderer: Exception in newOpenGLContextCreated: ") +
        e.what());
    // Auto-enable safe mode for next launch if we crash repeatedly?
    // For now just log.
  } catch (...) {
    ZENITH_LOG_ERROR(
        "SkiaOpenGLRenderer: Unknown exception in newOpenGLContextCreated");
  }
}

void SkiaOpenGLRenderer::renderOpenGL() {
  // ZENITH_LOG_TRACE("SkiaOpenGLRenderer::renderOpenGL called"); // Spammy

  if (!contextInitialized_ || !grContext_) {
    static bool loggedContextMissing = false;
    if (!loggedContextMissing) {
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Skipping render - Context not "
                       "initialized or grContext null");
      loggedContextMissing = true;
    }
    return;
  }

  auto width = targetComponent_->getWidth();
  auto height = targetComponent_->getHeight();

  // Only recreate surface if size changed
  if (width != lastWidth_ || height != lastHeight_ || !surface_) {
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Size change or missing surface "
                    "detected. Recreating...");
    recreateSurface();
    lastWidth_ = width;
    lastHeight_ = height;
  }

  if (!surface_) {
    static bool loggedSurfaceMissing = false;
    if (!loggedSurfaceMissing) {
      ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Skipping render - Surface is null "
                       "after recreate attempts");
      loggedSurfaceMissing = true;
    }
    return;
  }

  static int frameCount = 0;
  if (frameCount++ % 120 == 0) {
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Rendering frame " +
                    juce::String(frameCount) + " (Surface valid)");
  }

  // Get canvas and clear
  skiaCanvas_ = surface_->getCanvas();
  if (!skiaCanvas_) {
    ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Canvas is null!");
    return;
  }

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

  // Auto-detect format preferences
  GLint implFormat = 0;
  juce::gl::glGetIntegerv(0x8B9B,
                          &implFormat); // GL_IMPLEMENTATION_COLOR_READ_FORMAT

  SkColorType colorType = kRGBA_8888_SkColorType;
  if (implFormat == 0x80E1) {         // GL_BGRA
    framebufferInfo.fFormat = 0x8058; // Use GL_RGBA8 internals
    colorType = kBGRA_8888_SkColorType;
    ZENITH_LOG_INFO(
        "SkiaOpenGLRenderer: Based on GL_BGRA read format, using BGRA Surface");
  } else {
    framebufferInfo.fFormat = 0x8058;
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Using default RGBA Surface");
  }

  // Create backend render target
  auto backendRT =
      GrBackendRenderTargets::MakeGL(width, height,
                                     samples, // Use actual sample count
                                     8,       // stencil bits
                                     framebufferInfo);

  // Create Skia surface
  surface_ = SkSurfaces::WrapBackendRenderTarget(grContext_.get(), backendRT,
                                                 kBottomLeft_GrSurfaceOrigin,
                                                 colorType, nullptr, nullptr);

  if (!surface_) {
    ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Failed to create Skia surface!");
    // If surface creation fails, we must handle it to avoid black screen.
    // Maybe try to reset context?
  } else {
    ZENITH_LOG_INFO("SkiaOpenGLRenderer: Skia surface created successfully (" +
                    juce::String(width) + "x" + juce::String(height) + ")");

    // Ensure we clear it at least once to avoid garbage
    auto canvas = surface_->getCanvas();
    if (canvas) {
      canvas->clear(SkColorSetARGB(255, 20, 20, 25)); // Safe dark background
      grContext_->flushAndSubmit();
    }
  }
}

// ============================================================================
// SkiaMainWindowIntegration Implementation
// ============================================================================

SkiaMainWindowIntegration::SkiaMainWindowIntegration()
    : SkiaOpenGLRenderer(this) {}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
  // Check if OpenGL is trying to run
  if (contextInitialized_ && grContext_) {
    // If we are initialized but this is called, it means JUCE is doing a paint.
    // E.g. a resized() might trigger a paint before the GL thread catches up,
    // or we are intentionally mixing. Check if checks are passing.

    // If we have a valid context, we generally want to avoid Raster painting
    // as it causes flicker or black screens if they fight.
    // However, if the user forced Safe Mode, contextInitialized_ would be
    // false.
  }

  // If Safe Mode (not initialized), this is where we render!
  if (!contextInitialized_) {
    ZENITH_LOG_TRACE("SkiaMainWindowIntegration: Render fallback active "
                     "(SafeMode or GL Init Failed)");
  } else {
    // If we ARE initialized, we should probably output nothing and let GL
    // handle it, UNLESS we want to specifically support hybrid rendering
    // (unlikely for main window root). But clearing to black here is risky if
    // GL is transparent. Let's just log and proceed to raster fallback as a
    // safety net if GL isn't swapping buffers yet. actually, if
    // contextInitialized_ is true, we should probably NOT clear screen to
    // red/black here.
  }

  ZENITH_LOG_TRACE("SkiaMainWindowIntegration::paint called (Raster Fallback)");

  const int width = getWidth();
  const int height = getHeight();

  if (width <= 0 || height <= 0)
    return;

  // Create Raster Surface
  SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
  auto rasterSurface = SkSurfaces::Raster(info);

  if (!rasterSurface) {
    g.fillAll(juce::Colours::red);
    return;
  }

  SkCanvas *canvas = rasterSurface->getCanvas();
  canvas->clear(SkColorSetARGB(255, 10, 10, 15));
  drawSkiaContent(canvas);

  // Convert to JUCE Image
  sk_sp<SkImage> img(rasterSurface->makeImageSnapshot());
  if (img) {
    SkPixmap pixmap;
    if (img->peekPixels(&pixmap)) {
      juce::Image juceImage(juce::Image::ARGB, width, height, true);
      juce::Image::BitmapData bd(juceImage, juce::Image::BitmapData::writeOnly);

      if (pixmap.readPixels(SkImageInfo::Make(width, height,
                                              kBGRA_8888_SkColorType,
                                              kPremul_SkAlphaType),
                            bd.data, bd.lineStride)) {
        g.drawImageAt(juceImage, 0, 0);
        return;
      }
    }
  }

  // Fallback if skia fails
  g.fillAll(juce::Colour(0xff202020));
  g.setColour(juce::Colours::red);
  g.drawText("Skia Rasterization Failed", getLocalBounds(),
             juce::Justification::centred, true);
}

void SkiaMainWindowIntegration::resized() {
  // Surface will be recreated in renderOpenGL if size changed
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
