#pragma once
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <atomic>


#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkSurface.h>
#include <include/core/SkTypeface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>

#endif

namespace zenith {

/**
 * @class SkiaMainWindowIntegration
 * @brief Direct OpenGL framebuffer rendering with Skia
 *
 * Architecture:
 * - Creates single GrDirectContext on OpenGL context creation
 * - Caches SkSurface wrapping default framebuffer (FBO 0)
 * - Recreates surface only on resize
 * - Renders at 60 FPS via continuous repainting
 * - Recursively traverses JUCE component tree and calls drawSkia()
 */
class SkiaMainWindowIntegration : public juce::Component,
                                  public juce::OpenGLRenderer {
public:
  SkiaMainWindowIntegration();
  ~SkiaMainWindowIntegration() override;

  void newOpenGLContextCreated() override;
  void renderOpenGL() override;
  void openGLContextClosing() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  // Recursive helper to traverse the JUCE tree and find Skia components
  void renderComponentRecursively(juce::Component *comp, SkCanvas *canvas);

  juce::OpenGLContext openGLContext;
  bool rendererInitialized_ = false;

#ifdef ZENITH_USE_SKIA
  // GPU context (shared across all components)
  sk_sp<GrDirectContext> grContext_;

  // Cached surface for default framebuffer (recreated on resize)
  sk_sp<SkSurface> cachedSurface_;
  int cachedWidth_ = 0;
  int cachedHeight_ = 0;

  // Thread-safe surface validity flag (fixes resize race condition)
  std::atomic<bool> surfaceValid_{false};

  // One-time logging flags (member variables to avoid static bool anti-pattern)
  bool loggedSurfaceError_ = false;
  bool loggedComponentTree_ = false;
  bool loggedSuccess_ = false;
#endif

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMainWindowIntegration)
};
} // namespace zenith
