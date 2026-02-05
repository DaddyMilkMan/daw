/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ZenithKnob.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Premium neon-glow rotary knob with:
    - Value display on hover/drag
    - Tick marks for snap values

    - Bipolar mode (center-out for pan/detune)
    - Double-click reset to default
    - Shift+drag for fine control
    - Mouse wheel support
    - Animated glow on value change

  ==============================================================================
*/

#pragma once

#include "ZenithControl.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

class ZenithKnob : public ZenithControl,
                   public juce::Value::Listener {
public:
  // ----- Knob Modes -----
  enum class Mode {
    Unipolar, // 0 to max (default)
    Bipolar,  // Center-out (e.g., pan: -100 to +100)
    Symmetric // Always from center, but 0-based display
  };

  // ----- Knob Styles -----
  enum class Style {
    Standard, // Default arc style
    Minimal,  // Thin arc, no cap gradient
    Vintage,  // Skeuomorphic with notches
    LED       // Digital LED-style segments
  };

  // ----- Constructors -----
  ZenithKnob();
  explicit ZenithKnob(const juce::String &name,
                      SkColor color = SkColorSetRGB(0, 255, 255));
  explicit ZenithKnob(juce::Value valueToControl);
  ~ZenithKnob() override;

  // ----- Mode & Style -----
  void setMode(Mode mode) {
    mode_ = mode;
    repaint();
  }
  Mode getMode() const { return mode_; }

  void setStyle(Style style) {
    style_ = style;
    repaint();
  }
  Style getStyle() const { return style_; }

  // ----- Tick Marks -----
  void setShowTicks(bool show) {
    showTicks_ = show;
    repaint();
  }
  void setTickCount(int count) {
    tickCount_ = count;
    repaint();
  }
  void setSnapToTicks(bool snap) { snapToTicks_ = snap; }

  // ----- Arc Geometry -----
  void setStartAngle(float degrees) {
    startAngle_ = degrees;
    repaint();
  }
  void setSweepAngle(float degrees) {
    sweepRange_ = degrees;
    repaint();
  }
  void setTrackWidth(float width) {
    trackWidth_ = width;
    repaint();
  }

  // ----- Visual -----
  void setShowLabel(bool show) {
    showLabel_ = show;
    repaint();
  }
  void setLabelPosition(float yOffset) {
    labelYOffset_ = yOffset;
    repaint();
  }
  void setGlowIntensity(float intensity) {
    glowIntensity_ = juce::jlimit(0.0f, 1.0f, intensity);
  }

  // ----- Modulation -----
  void setModulationAmount(float amount) {
    modulationAmount_ = juce::jlimit(-1.0f, 1.0f, amount);
    repaint();
  }
  float getModulationAmount() const { return modulationAmount_; }

  void setModulationColor(SkColor color) {
    modulationColor_ = color;
    repaint();
  }

  // ----- Callbacks -----
  std::function<void()> onValueChange; // Legacy compatibility

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

  // juce::Value::Listener
  void valueChanged(juce::Value& v) override {
      if (v.refersToSameSourceAs(value)) {
          setValue(v.getValue(), false);
          markDirty();
      }
  }

protected:
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

private:
#ifdef ZENITH_USE_SKIA
  void drawTrack(SkCanvas *canvas, float cx, float cy, float radius);
  void drawTickMarks(SkCanvas *canvas, float cx, float cy, float radius);
  void drawValueArc(SkCanvas *canvas, float cx, float cy, float radius);
  void drawModulationRing(SkCanvas *canvas, float cx, float cy, float radius);
  void drawCenterCap(SkCanvas *canvas, float cx, float cy, float radius);
  void drawIndicator(SkCanvas *canvas, float cx, float cy, float radius);
  void drawValueTooltip(SkCanvas *canvas, float cx, float cy, float radius);
  void drawLabel(SkCanvas *canvas, float cx, float cy, float radius);

  float getAngleForValue(float normalizedValue) const;
  sk_sp<SkShader> createArcGradient(float cx, float cy, float radius) const;
#endif

  // Settings
  Mode mode_ = Mode::Unipolar;
  Style style_ = Style::Standard;

  // Tick marks
  bool showTicks_ = false;
  int tickCount_ = 11;
  bool snapToTicks_ = false;

  // Geometry
  float startAngle_ = 135.0f;
  float sweepRange_ = 270.0f;
  float trackWidth_ = 6.0f;

  // Visual
  bool showLabel_ = true;
  float labelYOffset_ = 15.0f;
  float glowIntensity_ = 0.5f;

  // Modulation
  float modulationAmount_ = 0.0f;
  SkColor modulationColor_ = SkColorSetRGB(255, 100, 0); // Orange default

  // Animation
  float animatedGlow_ = 0.0f;
  juce::int64 lastChangeTime_ = 0;

  juce::Value value;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithKnob)
};

// Legacy alias for backward compatibility
using SkiaKnob = ZenithKnob;

} // namespace zenith
