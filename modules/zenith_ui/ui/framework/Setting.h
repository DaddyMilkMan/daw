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
  struct Setting {
    juce::String key;
    juce::String label;
    ConfigValue defaultValue;
    std::unique_ptr<SkiaComponent> editor;
  };

  juce::HashMap<juce::String, Category> categories_;
  juce::HashMap<juce::String, Setting> settings_;
  juce::String currentCategory_;

  // UI Components
  std::unique_ptr<layout::SkiaListBox> categoryList_;
  std::unique_ptr<layout::SkiaVerticalLayout> settingsLayout_;

  void createUI();
  void refreshSettingsView();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};

} // namespace config
} // namespace zenith