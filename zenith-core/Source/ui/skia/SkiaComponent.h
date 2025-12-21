/*
  ==============================================================================
    SkiaComponent.h
    Inherit from this instead of juce::Component for your custom controls.

    DIRECT RENDERING MODE:
    - Components are rendered directly to OpenGL framebuffer
    - SkiaMainWindowIntegration calls drawSkia() with the main canvas
    - No intermediate surfaces or blitting required
  ==============================================================================
*/
#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#else
class SkCanvas;
#endif

namespace zenith {

/**
 * @class SkiaComponent
 * @brief Base class for Skia-rendered components
 *
 * DIRECT RENDERING ARCHITECTURE:
 * - Inherits from juce::Component for hierarchy and event handling
 * - drawSkia() is called by SkiaMainWindowIntegration during renderOpenGL()
 * - Canvas is the main framebuffer canvas (no intermediate surfaces)
 * - paint() is NOT called (OpenGL bypasses JUCE rendering)
 */
class SkiaComponent : public juce::Component {
public:
  SkiaComponent() {
    setOpaque(false);  // Don't let JUCE draw background
  }

  virtual ~SkiaComponent() = default;

  /**
   * @brief Draw this component with Skia
   * @param canvas The Skia canvas (pre-transformed to component's local coordinates)
   *
   * LIFECYCLE AND THREADING:
   * - Called from the OpenGL rendering thread at ~60 FPS (continuous repainting)
   * - Called by SkiaMainWindowIntegration::renderOpenGL() during component tree traversal
   * - Canvas is already translated to this component's local coordinates (0,0 is top-left)
   * - Canvas is clipped to this component's bounds automatically
   *
   * CRITICAL SAFETY RULES:
   * - The SkCanvas* pointer is ONLY valid during this call - DO NOT store it
   * - DO NOT call repaint(), resized(), or any JUCE GUI methods from here
   * - DO NOT access mutable state without synchronization
   * - Use only const member variables or thread-safe reads
   *
   * DRAWING COORDINATES:
   * - (0, 0) is the top-left corner of THIS component
   * - getWidth() and getHeight() give you the component's size
   * - Children are rendered automatically after this returns
   *
   * EXAMPLE:
   * @code
   * void MyButton::drawSkia(SkCanvas* canvas) {
   *     SkPaint paint;
   *     paint.setColor(SK_ColorBLUE);
   *     paint.setAntiAlias(true);
   *
   *     // Draw button background (0,0 is already at our top-left)
   *     SkRect rect = SkRect::MakeWH(getWidth(), getHeight());
   *     canvas->drawRoundRect(rect, 4.0f, 4.0f, paint);
   * }
   * @endcode
   */
  virtual void drawSkia(SkCanvas * const canvas) = 0;

  /**
   * @brief JUCE paint override - should never be called
   *
   * When OpenGL rendering is active, JUCE's paint system is disabled.
   * If this is called, it means OpenGL failed to initialize.
   */
  void paint(juce::Graphics &g) override {
    // Fallback for when OpenGL rendering is not active
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::red);
    g.drawText("OpenGL rendering not active!", getLocalBounds(),
               juce::Justification::centred);
  }

  /**
   * @brief Optional override point for resize logic
   */
  void resized() override {
    onResized();
  }

  /**
   * @brief Override this if you need custom resize handling
   */
  virtual void onResized() {}
};

} // namespace zenith
