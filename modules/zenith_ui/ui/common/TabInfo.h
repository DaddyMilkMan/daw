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
  struct TabInfo {
    juce::String id;
    juce::String title;
    juce::Component *content = nullptr;
    juce::Rectangle<float> bounds;
    bool isHovered = false;
    bool isBeingDragged = false;
  };

  std::vector<TabInfo> tabs_;
  int activeTabIndex_ = -1;
  bool dragReorderEnabled_ = true;
  juce::String
      accessibilityTitle_; // Unique title for screen reader identification
  int draggedTabIndex_ = -1;
  juce::Point<int> dragStartPos_;

  void updateTabBounds();
  int getTabIndexAtPosition(const juce::Point<int> &pos) const;
  void animateTabSwitch();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabGroup)
};

//==============================================================================
// Resizable Panel Container
//==============================================================================

/**
 * @brief Main container that manages a hierarchy of resizable panels
 *
 * Supports:
 * - Horizontal/Vertical splits
 * - Nested layouts
 * - Drag-to-resize
 * - Collapsible panels
 * - Tab groups
 * - Layout persistence
 */

} // namespace
