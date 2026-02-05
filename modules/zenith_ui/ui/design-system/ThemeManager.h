/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ThemeManager.h
    Created: 2026-02-02
    Author:  Zenith DAW

    Theme management for the Zenith DAW UI.
    Supports Dark, Darker (OLED), and Light themes with runtime switching.


  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <string>
#include <vector>

namespace zenith {
namespace design {

/**
 * @brief Theme preset enumeration for built-in themes
 */
enum class ThemePreset {
  Dark,   // Neon Noir - vibrant accents on dark backgrounds
  Darker, // OLED Black - pure black backgrounds for power saving
  Light   // Light mode - inverted palette for daylight use
};

/**
 * @brief Listener interface for theme change notifications
 */
class ThemeListener {
public:
  virtual ~ThemeListener() = default;
  virtual void themeChanged(ThemePreset newTheme) = 0;
};

/**
 * @brief Manages UI themes and provides runtime theme switching
 */
class ThemeManager : public juce::ChangeBroadcaster {
public:
  static ThemeManager &getInstance() {
    static ThemeManager instance;
    return instance;
  }

  struct Theme {
    juce::String name;
    std::map<juce::String, uint32_t> colors; // name -> ARGB
  };

  struct ThemePalette {
    // Backgrounds
    uint32_t bgDarkest;
    uint32_t bgDarker;
    uint32_t bgDark;
    uint32_t bgMedium;
    uint32_t bgLight;

    // Accents
    uint32_t accentPrimary;
    uint32_t accentSecondary;

    // Text
    uint32_t textPrimary;
    uint32_t textSecondary;
    uint32_t textTertiary;

    // Borders
    uint32_t borderDefault;
    uint32_t borderSubtle;
    uint32_t borderFocus;

    // Semantic
    uint32_t success;
    uint32_t warning;
    uint32_t error;
  };

  // Built-in preset management
  void setActiveTheme(ThemePreset preset);
  ThemePreset getActiveTheme() const { return activePreset_; }

  // Listener management
  void addListener(ThemeListener *listener);
  void removeListener(ThemeListener *listener);

  // Custom theme management
  void saveTheme(const juce::String &name);
  void loadTheme(const juce::String &name);
  void deleteTheme(const juce::String &name);

  juce::StringArray getAvailableThemes() const;

  // Apply current colors to design system
  void applyTheme(const Theme &theme);

  // Reset state for testing
  void resetToDefault();

  // Palette accessors for current theme
  const ThemePalette &getPalette() const { return currentPalette_; }

private:
  ThemeManager();

  void applyDarkTheme();
  void applyDarkerTheme();
  void applyLightTheme();
  void notifyListeners();

  ThemePreset activePreset_ = ThemePreset::Dark;
  ThemePalette currentPalette_;
  std::vector<ThemeListener *> listeners_;

  juce::File getThemeDir() const {
    auto dir =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW/Themes");
    if (!dir.exists())
      dir.createDirectory();
    return dir;
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeManager)
};

} // namespace design
} // namespace zenith
