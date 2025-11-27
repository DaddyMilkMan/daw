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
class SkiaColorTestComponent : public SkiaComponent {
public:
  SkiaColorTestComponent();
  ~SkiaColorTestComponent() override = default;

  // JUCE paint (fallback or background)
#ifndef ZENITH_USE_SKIA
  void paint(juce::Graphics &g) override;
#endif

  // Skia paint (native rendering)
#ifdef ZENITH_USE_SKIA
  void drawSkia(SkCanvas *canvas) override;
#endif

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaColorTestComponent)
};

} // namespace zenith
