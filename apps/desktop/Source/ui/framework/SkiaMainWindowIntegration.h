#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <atomic>

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
class SkiaOpenGLRenderer : public juce::OpenGLRenderer, 
                           private juce::Timer {
public:
  explicit SkiaOpenGLRenderer(juce::Component *componentToAttach);
  virtual ~SkiaOpenGLRenderer();

  SkiaOpenGLRenderer(const SkiaOpenGLRenderer &) = delete;
  SkiaOpenGLRenderer &operator=(const SkiaOpenGLRenderer &) = delete;

  // OpenGLRenderer overrides
  void newOpenGLContextCreated() override;
  void renderOpenGL() override;
  void openGLContextClosing() override;
  
  // Timer callback for deferred attachment
  void timerCallback() override;
  
  /**
   * @brief Attach the OpenGL context to the target component.
   * Call this when the component has a valid peer (window handle).
   */
  void attachContextNow();
  
  /**
   * @brief Schedule a check for peer availability and attach context when ready.
   */
  /**
   * @brief Schedule a check for peer availability and attach context when ready.
   */
  void scheduleAttachmentCheck();

  /**
   * @brief Check if the Skia context has been successfully initialized.
   */
  bool isContextInitialized() const { return contextInitialized_; }

  /**
   * @brief Request a repaint. Use this instead of relying on continuous repainting.
   * Thread-safe: can be called from any thread.
   */
  void triggerRepaint();

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
  sk_sp<const GrGLInterface> interface_;
  sk_sp<GrDirectContext> grContext_;
  sk_sp<SkSurface> surface_;
  SkCanvas *skiaCanvas_ = nullptr;
  bool contextInitialized_ = false;
  juce::Component *targetComponent_ = nullptr;

private:
  int lastWidth_ = 0;
  int lastHeight_ = 0;
  
  // Thread-safe dimensions (Atomic for lock-free read/write)
  std::atomic<int> safeWidth_{0};
  std::atomic<int> safeHeight_{0};

  void recreateSurface();
public:
  void updateDimensions(int width, int height) {
      safeWidth_.store(width);
      safeHeight_.store(height);
  }
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
  void parentHierarchyChanged() override;
  void visibilityChanged() override;

  SkiaMainWindowIntegration(const SkiaMainWindowIntegration &) = delete;
  SkiaMainWindowIntegration &
  operator=(const SkiaMainWindowIntegration &) = delete;
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
