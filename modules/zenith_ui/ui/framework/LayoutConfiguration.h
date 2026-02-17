/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaComponent.h"
#include "SkiaLayout.h"
#include "ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

// Forward declarations for UI components
namespace zenith {
namespace layout {
class LayoutConfiguration {
public:
  static LayoutConfiguration &getInstance();

  // Panel visibility
  void setBrowserVisible(bool visible);
  bool isBrowserVisible() const;

  void setRightPanelVisible(bool visible);
  bool isRightPanelVisible() const;

  void setBottomPanelVisible(bool visible);
  bool isBottomPanelVisible() const;

  void setMixerVisible(bool visible);
  bool isMixerVisible() const;

  // Panel sizes
  void setBrowserWidth(int width);
  int getBrowserWidth() const;

  void setRightPanelWidth(int width);
  int getRightPanelWidth() const;

  void setBottomPanelHeight(int height);
  int getBottomPanelHeight() const;

  void setMixerHeight(int height);
  int getMixerHeight() const;

  // Layout presets
  juce::StringArray getLayoutPresets() const;
  void saveLayoutPreset(const juce::String &name);
  void loadLayoutPreset(const juce::String &name);
  void deleteLayoutPreset(const juce::String &name);

  // Window state
  void setWindowBounds(const juce::Rectangle<int> &bounds);
  juce::Rectangle<int> getWindowBounds() const;

  void setWindowMaximized(bool maximized);
  bool isWindowMaximized() const;

private:
  LayoutConfiguration() = default;
  ~LayoutConfiguration() = default;

  juce::CriticalSection lock_;
};

// ============================================================================
// User Preferences
// ============================================================================

} // namespace
