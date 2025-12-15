#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkColorSpace.h>
#include <core/SkRefCnt.h>
#include <core/SkSurface.h>
#include <gpu/ganesh/GrBackendSurface.h>
#include <gpu/ganesh/GrDirectContext.h>
#include <gpu/ganesh/SkSurfaceGanesh.h>
#include <gpu/ganesh/gl/GrGLBackendSurface.h>
#include <gpu/ganesh/gl/GrGLDirectContext.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA
/**
 * @brief Reusable Skia OpenGL Renderer that manages OpenGL context and Skia
 * Surface.
 *
 * Inherit from this class to add Skia rendering to any JUCE Component.
 * You must pass the component reference to the constructor.
 *
 * NOTE: Renamed from SkiaRenderer to SkiaOpenGLRenderer to avoid conflict
 * with the standalone SkiaRenderer class in rendering/SkiaRenderer.h
 */
class SkiaOpenGLRenderer : public juce::OpenGLRenderer {
public:
  explicit SkiaOpenGLRenderer(juce::Component *componentToAttach);
  virtual ~SkiaOpenGLRenderer();

  SkiaOpenGLRenderer(const SkiaOpenGLRenderer &) = delete;
  SkiaOpenGLRenderer &operator=(const SkiaOpenGLRenderer &) = delete;

  // OpenGLRenderer overrides
  void newOpenGLContextCreated() override;
  void renderOpenGL() override;
  void openGLContextClosing() override;

protected:
  /**
   * @brief Override this to draw your Skia content
   * @param canvas Skia canvas to draw on
   */
  virtual void drawSkiaContent(SkCanvas *canvas) = 0;

  /**
   * @brief Get the current Skia canvas (valid during renderOpenGL)
   */
  SkCanvas *getSkiaCanvas() { return skiaCanvas_; }

  juce::OpenGLContext openGLContext_;
  sk_sp<GrDirectContext> grContext_;
  sk_sp<SkSurface> surface_;
  SkCanvas *skiaCanvas_ = nullptr;

private:
  juce::Component *targetComponent_ = nullptr;
  bool contextInitialized_ = false;
  int lastWidth_ = 0;
  int lastHeight_ = 0;

  void recreateSurface();
};

/**
 * @brief Base class for main window with Skia rendering
 *
 * Kept for backward compatibility and simple use cases.
 */
class SkiaMainWindowIntegration : public juce::Component,
                                  public SkiaOpenGLRenderer {
public:
  SkiaMainWindowIntegration();
  ~SkiaMainWindowIntegration() override;

  // Component overrides
  void paint(juce::Graphics &g) override;
  void resized() override;

  SkiaMainWindowIntegration(const SkiaMainWindowIntegration &) = delete;
  SkiaMainWindowIntegration &
  operator=(const SkiaMainWindowIntegration &) = delete;
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
