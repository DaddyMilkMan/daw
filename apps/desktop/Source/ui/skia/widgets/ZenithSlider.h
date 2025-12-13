/*
  ==============================================================================

    ZenithSlider.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Premium vertical/horizontal fader with:
    - Value display on hover/drag
    - Bipolar mode
    - Double-click reset
    - Shift+drag fine control
    - Mouse wheel support
    - Metallic handle with glow

  ==============================================================================
*/

#pragma once

#include "ZenithControl.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

class ZenithSlider : public ZenithControl {
public:
  // ----- Slider Orientation -----
  enum class Orientation { Vertical, Horizontal };

  // ----- Slider Styles -----
  enum class Style {
    Standard, // Default track and handle
    Minimal,  // Thin track
    Fader,    // DAW-style fader
    Level     // VU meter style
  };

  // ----- Constructors -----
  ZenithSlider();
  explicit ZenithSlider(const juce::String &name,
                        SkColor color = SkColorSetRGB(255, 0, 255));
  ~ZenithSlider() override = default;

  // ----- Orientation & Style -----
  void setOrientation(Orientation orient) {
    orientation_ = orient;
    repaint();
  }
  Orientation getOrientation() const { return orientation_; }

  void setStyle(Style style) {
    style_ = style;
    repaint();
  }
  Style getStyle() const { return style_; }

  // ----- Bipolar Mode -----
  void setBipolar(bool bipolar) {
    bipolar_ = bipolar;
    repaint();
  }
  bool isBipolar() const { return bipolar_; }

  // ----- Visual -----
  void setTrackWidth(float width) {
    trackWidth_ = width;
    repaint();
  }
  void setHandleWidth(float width) {
    handleWidth_ = width;
    repaint();
  }
  void setHandleHeight(float height) {
    handleHeight_ = height;
    repaint();
  }
  void setShowFillBar(bool show) {
    showFillBar_ = show;
    repaint();
  }

  // ----- Callbacks -----

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

protected:
  void mouseDrag(const juce::MouseEvent &e) override;

private:
#ifdef ZENITH_USE_SKIA
  void drawTrack(SkCanvas *canvas);
  void drawFillBar(SkCanvas *canvas, float handlePos);
  void drawHandle(SkCanvas *canvas, float handlePos);
  void drawValueTooltip(SkCanvas *canvas, float handlePos);

  float getHandlePosition() const;

#endif

  // Settings
  Orientation orientation_ = Orientation::Vertical;
  Style style_ = Style::Standard;
  bool bipolar_ = false;

  // Geometry
  float trackWidth_ = 4.0f;
  float handleWidth_ = 24.0f;
  float handleHeight_ = 12.0f;
  bool showFillBar_ = true;

  // Layout margins
  float marginStart_ = 0.1f; // 10% from top/left
  float marginEnd_ = 0.1f;   // 10% from bottom/right

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSlider)
};

} // namespace zenith
