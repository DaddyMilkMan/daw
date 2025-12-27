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
  explicit ZenithSlider(Orientation orientation);
  ZenithSlider(const juce::String &name, SkColor color);
  ~ZenithSlider() override;

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

  // Shadowing base methods to match .cpp implementation
  void setValue(float newValue, bool sendNotification = true);
  void setRange(float min, float max, float defaultValue);

  // Custom callback used in .cpp
  std::function<void(float)> onValueChange;

  // Helper to get display value (assuming just value for now)
  float getDisplayValue() const { return value_; }

  //==========================================================================
  // ZenithControl / SkiaComponent Overrides
  //==========================================================================
  void drawSkia(SkCanvas *canvas) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  // mouseDrag is overridden in .cpp implicitly if base has it, but .cpp
  // implementation doesn't look like it overrides it? .cpp has no mouseDrag
  // implementation shown in view_file. Wait, line 59 in original header
  // declared it. I should keep it if it's there, but .cpp view didn't show it?
  // Let's check .cpp again. step 199.
  // It only shows drawSkia, getHandlePosition, drawFillBar, drawHandle,
  // drawValueTooltip, drawTrack. It does NOT show mouseDrag. But header
  // declared it. If I remove it from header, it uses base. The .cpp DOES NOT
  // implement mouseDrag. So I should REMOVE it from header or standard default?
  // If I remove it, it uses base mouseDrag.
  // I will remove it from header to avoid 'undefined reference' linker error.

private:
  //==========================================================================
  // Internal State
  //==========================================================================
  Orientation orientation_ = Vertical;
  float marginStart_ = 0.1f;
  float marginEnd_ = 0.1f;

  bool showFillBar_ = true;
  bool bipolar_ = false;

  // Members used in .cpp
  float value_ = 0.0f;
  float targetValue_ = 0.0f;
  float minValue_ = 0.0f;
  float maxValue_ = 1.0f;
  float defaultValue_ = 0.0f;

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
