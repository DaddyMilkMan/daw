/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaComponent.h"
#include "ZenithDesignSystem.h"
#include "LayoutManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace zenith {

// Forward declarations
class PanelHeader : public SkiaComponent {
public:
  PanelHeader(const juce::String &title, bool collapsible = true);
  ~PanelHeader() override = default;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  void setTitle(const juce::String &title);
  void setCollapsed(bool collapsed);
  bool isCollapsed() const { return isCollapsed_; }

  // Callbacks
  std::function<void()> onCollapseClicked;
  std::function<void()> onHeaderDragStart;

  static constexpr int headerHeight = 28;

  // Accessibility
  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

private:
  juce::String title_;
  bool isCollapsible_ = true;
  bool isCollapsed_ = false;
  bool hoverOverCollapse_ = false;

  juce::Rectangle<float> getCollapseButtonBounds() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelHeader)
};

//==============================================================================
// Panel Wrapper
//==============================================================================

/**
 * @brief Wrapper around a content component with header and resize handling
 */

} // namespace
