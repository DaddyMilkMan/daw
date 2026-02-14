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

#pragma once

#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#endif

namespace zenith {

class ZenithToggle : public SkiaComponent {
public:
  // ----- Toggle Styles -----
  enum class Style {
    Switch,   // iOS-style pill switch
    Checkbox, // Square checkbox with checkmark
    Radio     // Circular radio button
  };

  // ----- Constructors -----
  ZenithToggle();
  explicit ZenithToggle(const juce::String &label);
  ~ZenithToggle() override = default;

  // ----- State -----
  void setToggleState(bool state, bool sendNotification = true);
  bool getToggleState() const { return toggleState_; }

  // ----- Appearance -----
  void setStyle(Style style) {
    style_ = style;
    repaint();
  }
  Style getStyle() const { return style_; }

  void setLabel(const juce::String &label) {
    label_ = label;
    repaint();
  }
  juce::String getLabel() const { return label_; }

  void setLabelOnRight(bool rightSide) {
    labelOnRight_ = rightSide;
    repaint();
  }

  void setActiveColor(SkColor color) {
    activeColor_ = color;
    repaint();
  }
  void setInactiveColor(SkColor color) {
    inactiveColor_ = color;
    repaint();
  }

  // ----- Callbacks -----
  std::function<void(bool)> onToggle;
  std::function<void()> onClick; // Legacy

  // ----- Keyboard -----
  bool keyPressed(const juce::KeyPress &key) override;

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

protected:
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

private:
#ifdef ZENITH_USE_SKIA
  void drawSwitch(SkCanvas *canvas);
  void drawCheckbox(SkCanvas *canvas);
  void drawRadio(SkCanvas *canvas);
  void drawLabel(SkCanvas *canvas);
#endif

  Style style_ = Style::Switch;
  bool toggleState_ = false;
  bool hovered_ = false;
  bool pressed_ = false;

  juce::String label_;
  bool labelOnRight_ = true;

  SkColor activeColor_ = SkColorSetRGB(0, 255, 255);
  SkColor inactiveColor_ = SkColorSetRGB(60, 60, 70);

  // Animation
  float animationProgress_ = 0.0f; // For animated slide

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithToggle)
};

} // namespace zenith
