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
class ThemeConfiguration {
public:
  static ThemeConfiguration &getInstance();

  // Theme management
  void setTheme(const juce::String &themeName);
  juce::String getCurrentTheme() const;

  juce::StringArray getAvailableThemes() const;
  void registerTheme(const juce::String &name,
                     const juce::DynamicObject::Ptr &themeData);

  // Color management
  void setAccentColor(SkColor color);
  SkColor getAccentColor() const;

  void setBackgroundColor(SkColor color);
  SkColor getBackgroundColor() const;

  // OLED mode
  void setOledMode(bool enabled);
  bool isOledMode() const;

  // Glow effects
  void setGlowIntensity(float intensity);
  float getGlowIntensity() const;

  // UI scaling
  void setUiScale(float scale);
  float getUiScale() const;

  // Apply theme to design system
  void applyCurrentTheme();

private:
  ThemeConfiguration() = default;
  ~ThemeConfiguration() = default;

  juce::String currentTheme_ = "NeonNoir";
  juce::HashMap<juce::String, juce::DynamicObject::Ptr> themes_;
  juce::CriticalSection lock_;
};

// ============================================================================
// Layout Configuration
// ============================================================================

} // namespace
