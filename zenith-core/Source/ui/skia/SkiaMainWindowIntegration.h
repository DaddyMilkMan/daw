#pragma once
#include "../../rendering/SkiaRenderer.h"
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>


#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkTypeface.h>

#endif

namespace zenith {

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
  std::unique_ptr<SkiaRenderer> renderer_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaMainWindowIntegration)
};
} // namespace zenith
