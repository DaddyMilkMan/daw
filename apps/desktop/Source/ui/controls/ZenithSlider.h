/*
  ==============================================================================

    ZenithSlider.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Premium slider control using Skia rendering.
    Inherits from ZenithControl for parameter binding and base behavior.

  ==============================================================================
*/

#pragma once

#include "ZenithControl.h"

namespace zenith {

/**
 * @class ZenithSlider
 * @brief Beautiful custom slider component using Skia.
 *
 * Inherits from ZenithControl to provide standard parameter handling.
 * Implements Skia drawing for premium visuals (gradients, shadows, glow).
 */
class ZenithSlider : public ZenithControl {
public:
  enum Orientation { Vertical, Horizontal };

  ZenithSlider();
  ZenithSlider(const juce::String &name, SkColor color);
  ~ZenithSlider() override = default;

  //==========================================================================
  // Configuration
  //==========================================================================
  void setOrientation(Orientation o) {
    orientation_ = o;
    repaint();
  }
  Orientation getOrientation() const { return orientation_; }

  void setBipolar(bool b) {
    bipolar_ = b;
    repaint();
  }
  bool isBipolar() const { return bipolar_; }

  void setShowFillBar(bool show) {
    showFillBar_ = show;
    repaint();
  }

  //==========================================================================
  // ZenithControl / SkiaComponent Overrides
  //==========================================================================
  void drawSkia(SkCanvas *canvas) override;
  void mouseDrag(const juce::MouseEvent &e) override;

private:
  //==========================================================================
  // Internal State
  //==========================================================================
  Orientation orientation_ = Vertical;
  float marginStart_ = 0.1f;
  float marginEnd_ = 0.1f;

  bool showFillBar_ = true;
  bool bipolar_ = false;
  
  // Dimensions
  float trackWidth_ = 4.0f;
  float handleWidth_ = 24.0f;
  float handleHeight_ = 12.0f;

  //==========================================================================
  // Drawing Helpers
  //==========================================================================
  float getHandlePosition() const;
  void drawTrack(SkCanvas *canvas);
  void drawFillBar(SkCanvas *canvas, float handlePos);
  void drawHandle(SkCanvas *canvas, float handlePos);
  void drawValueTooltip(SkCanvas *canvas, float handlePos);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSlider)
};

} // namespace zenith
