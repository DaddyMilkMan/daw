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

    TitleBarComponent.h
    Created: 2025-12-30
    Author:  Zenith DAW

  ==============================================================================
*/


#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../framework/SkiaComponent.h"
#include "../ZenithSkia.h"
#include "../framework/GlassmorphicPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "MenuBar.h"

namespace zenith {

class TitleBarComponent : public SkiaComponent {
public:
  TitleBarComponent();
  ~TitleBarComponent() override;

  void resized() override;
  
  // Custom Skia drawing
  void drawSkia(SkCanvas* canvas) override;

  // Interaction
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseDoubleClick(const juce::MouseEvent& e) override;
  void mouseMove(const juce::MouseEvent& e) override;
  void mouseExit(const juce::MouseEvent& e) override;

  void setTransparentBackground(bool shouldBeTransparent) { transparentBackground_ = shouldBeTransparent; }
  void setShowTitle(bool shouldShow) { showTitle_ = shouldShow; repaint(); }

  std::function<void()> onClose;
  std::function<void()> onMinimize;
  std::function<void()> onMaximize;

  ZenithMenuBar& getMenuBar() { return *menuBar_; }

private:
  SkFont titleFont_;
  SkPaint textPaint_;
  
  std::unique_ptr<ZenithMenuBar> menuBar_;
  SkRect logoBounds_;
  
  // Window control buttons (top right)
  juce::Rectangle<int> closeButtonBounds_;
  juce::Rectangle<int> minimizeButtonBounds_;
  juce::Rectangle<int> maximizeButtonBounds_;
  bool closeHovered_ = false;
  bool minimizeHovered_ = false;
  bool maximizeHovered_ = false;
  
  // Dragger
  juce::ComponentDragger dragger_;

  bool transparentBackground_ = false;
  bool showTitle_ = true;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TitleBarComponent)
};

} // namespace zenith
