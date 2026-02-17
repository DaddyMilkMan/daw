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
class TabGroup : public SkiaComponent {
public:
  /**
   * @brief Constructor with optional accessibility title
   * @param accessibilityTitle Title for screen readers to distinguish multiple
   * TabGroups
   */
  explicit TabGroup(const juce::String &accessibilityTitle = "Tab List");
  ~TabGroup() override;

  /** @brief Set the accessibility title for this TabGroup */
  void setAccessibilityTitle(const juce::String &title);

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

  // Tab management
  void addTab(const juce::String &tabId, const juce::String &title,
              juce::Component *content);
  void removeTab(const juce::String &tabId);
  void setActiveTab(const juce::String &tabId, bool animate = true);
  juce::String getActiveTabId() const;
  int getTabCount() const;

  // Drag reordering
  void setDragReorderEnabled(bool enabled) { dragReorderEnabled_ = enabled; }

  // Callbacks
  std::function<void(const juce::String &tabId)> onTabChanged;
  std::function<void(const juce::String &tabId, int newIndex)> onTabReordered;
  std::function<void(const juce::String &tabId)> onTabClosed;
  std::function<void(const juce::String &tabId, const juce::Point<int> &pos)>
      onTabDraggedOut;

  static constexpr int tabBarHeight = 32;

  // Accessibility
  std::unique_ptr<juce::AccessibilityHandler>
  createAccessibilityHandler() override;

private:

} // namespace
