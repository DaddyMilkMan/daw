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
class PanelDivider : public SkiaComponent {
public:
  PanelDivider(bool isHorizontal);
  ~PanelDivider() override = default;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  bool isHorizontal() const { return isHorizontal_; }

  // Position constraints
  void setPositionRatio(float ratio);
  float getPositionRatio() const { return positionRatio_; }
  void setPositionConstraints(float minRatio, float maxRatio);

  // Callbacks
  std::function<void(float deltaPixels)> onDrag;
  std::function<void()> onDragStart;
  std::function<void()> onDragEnd;

  static constexpr int dividerSize = 6;

  // Accessibility
  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

private:
  bool isHorizontal_;
  bool isDragging_ = false;
  bool isHovered_ = false;
  float positionRatio_ = 0.5f;
  float minPositionRatio_ = 0.1f;
  float maxPositionRatio_ = 0.9f;
  int dragStartPos_ = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelDivider)
};

//==============================================================================
// Tab Group
//==============================================================================

/**
 * @brief Tab bar for grouping multiple panels in the same area
 */

} // namespace
