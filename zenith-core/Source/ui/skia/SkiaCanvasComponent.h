#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkSurface.h>

#endif

#include "SkiaComponent.h"

namespace zenith {

/**
 * @class SkiaCanvasComponent
 * @brief Base class for components that render using Skia rasterization.
 *
 * This component maintains a juce::Image backing store.
 * On paint(), it wraps the image pixels in a SkSurface and calls paintSkia().
 * Then it draws the resulting image to the JUCE Graphics context.
 *
 * It also implements SkiaComponent to support direct rendering when
 * hosted by a Skia-aware parent (like MainWindow).
 */
class SkiaCanvasComponent : public juce::Component, public SkiaComponent {
public:
  SkiaCanvasComponent();
  ~SkiaCanvasComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // SkiaComponent implementation
  bool supportsSkiaRendering() const override { return true; }
  void paintToSkia(SkCanvas *canvas, SkRect bounds) override;

protected:
  /**
   * @brief Implement this method to draw using Skia.
   * @param canvas The Skia canvas to draw onto.
   * @param bounds The bounds of the component.
   */
#ifdef ZENITH_USE_SKIA
  virtual void paintSkia(SkCanvas &canvas,
                         const juce::Rectangle<int> &bounds) = 0;
#endif

private:
  juce::Image backingImage;

#ifdef ZENITH_USE_SKIA
  sk_sp<SkSurface> skSurface;
#endif

  void recreateSurface();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaCanvasComponent)
};

} // namespace zenith
