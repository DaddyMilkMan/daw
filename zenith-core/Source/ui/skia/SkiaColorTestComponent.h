/**
 * @file SkiaColorTestComponent.h
 * @brief Component for visually testing Skia rendering with colors and
 * gradients
 */

#pragma once

#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

/**
 * @class SkiaColorTestComponent
 * @brief A component that renders a colorful grid using native Skia APIs
 *
 * This component is used to verify that Skia rendering is working correctly
 * and to demonstrate the visual capabilities (gradients, anti-aliasing).
 */
class SkiaColorTestComponent : public juce::Component, public SkiaComponent {
public:
  SkiaColorTestComponent();
  ~SkiaColorTestComponent() override = default;

  // JUCE paint (fallback or background)
  void paint(juce::Graphics &g) override;

  // Skia paint (native rendering)
  void paintToSkia(SkCanvas *canvas, SkRect bounds) override;

  // Check if Skia rendering is supported
  bool supportsSkiaRendering() const override { return true; }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaColorTestComponent)
};

} // namespace zenith
