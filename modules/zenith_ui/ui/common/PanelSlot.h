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
  struct PanelSlot {
    std::unique_ptr<PanelWrapper> wrapper;
    std::unique_ptr<juce::Component> ownedContent;
    float sizeRatio = 1.0f;
  };

  std::vector<PanelSlot> panels_;
  std::vector<std::unique_ptr<PanelDivider>> dividers_;
  std::vector<std::unique_ptr<TabGroup>> tabGroups_;

  SplitDirection splitDirection_ = SplitDirection::Horizontal;
  bool isAnimating_ = false;

  // Layout calculation
  void recalculateLayout();
  void updateDividerPositions();
  void handleDividerDrag(int dividerIndex, float deltaPixels);
  void constrainPanelSizes();

  // Animation
  void animatePanelSizes();

  // Nested layout support
  void buildLayoutFromConfig(const juce::var &layoutNode, int depth);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizablePanelContainer)
};

} // namespace zenith
