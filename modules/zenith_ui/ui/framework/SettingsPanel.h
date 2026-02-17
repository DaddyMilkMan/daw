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
class SettingsPanel : public SkiaComponent {
public:
  SettingsPanel();
  ~SettingsPanel();

  // Category management
  void addCategory(const juce::String &name, const juce::String &icon);
  void setCurrentCategory(const juce::String &name);
  juce::String getCurrentCategory() const;

  // Settings management
  void addSetting(const juce::String &category, const juce::String &key,
                  const juce::String &label, const ConfigValue &defaultValue);

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

private:

} // namespace
