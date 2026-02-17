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
class PanelWrapper : public SkiaComponent {
public:
  PanelWrapper(const juce::String &panelId, juce::Component *content,
               const layout::PanelConfig &config);
  ~PanelWrapper() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // Panel management
  const juce::String &getPanelId() const { return panelId_; }
  juce::Component *getContent() const { return content_; }
  const layout::PanelConfig &getConfig() const { return config_; }

  // Size management
  float getMinSize() const { return config_.minSize; }
  float getMaxSize() const {
    return config_.maxSize > 0 ? config_.maxSize : 10000.0f;
  }
  float getFlex() const { return config_.flex; }
  float getCurrentSize() const { return currentSize_; }
  void setCurrentSize(float size);

  // Collapse support
  bool isCollapsible() const { return config_.isCollapsible; }
  bool isCollapsed() const { return isCollapsed_; }
  void setCollapsed(bool collapsed, bool animate = true);
  void toggleCollapse(bool animate = true);

  // Animation
  void timerCallback() override;

  // Callbacks
  std::function<void(PanelWrapper *)> onCollapseStateChanged;
  std::function<void(PanelWrapper *)> onSizeChanged;

private:
  juce::String panelId_;
  juce::Component *content_ = nullptr;
  layout::PanelConfig config_;

  std::unique_ptr<PanelHeader> header_;
  bool isCollapsed_ = false;
  float currentSize_ = 0.0f;
  float targetSize_ = 0.0f;
  float preCollapseSize_ = 0.0f;
  float animationProgress_ = 1.0f;
  juce::uint32 animationStartTime_ = 0; // For time-based animation

  static constexpr int collapsedHeight = PanelHeader::headerHeight;
  static constexpr float animationDurationMs =
      300.0f; // Animation duration in ms

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelWrapper)
};

//==============================================================================
// Panel Divider
//==============================================================================

/**
 * @brief Draggable divider between panels
 */

} // namespace
