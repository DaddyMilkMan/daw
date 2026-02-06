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

#include "ThemeManager.h"
#include "ZenithDesignSystem.h"
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {
namespace design {

ThemeManager::ThemeManager() {
  // Initialize with default dark theme
  applyDarkTheme();
}

void ThemeManager::setActiveTheme(ThemePreset preset) {
  activePreset_ = preset;

  switch (preset) {
    case ThemePreset::Dark:
      applyDarkTheme();
      break;
    case ThemePreset::Darker:
      applyDarkerTheme();
      break;
    case ThemePreset::Light:
      applyLightTheme();
      break;
  }

  notifyListeners();
}

void ThemeManager::addListener(ThemeListener *listener) {
  listeners_.push_back(listener);
}

void ThemeManager::removeListener(ThemeListener *listener) {
  listeners_.erase(
      std::remove(listeners_.begin(), listeners_.end(), listener),
      listeners_.end());
}

void ThemeManager::saveTheme(const juce::String &name) {
  // Create theme from current colors
  Theme theme;
  theme.name = name;

  // Export all current color values
  theme.colors["CYAN"] = colors::CYAN;
  theme.colors["MAGENTA"] = colors::MAGENTA;
  theme.colors["VIOLET"] = colors::VIOLET;
  theme.colors["ORANGE"] = colors::ORANGE;
  theme.colors["NEON_GREEN"] = colors::NEON_GREEN;
  theme.colors["NEON_PINK"] = colors::NEON_PINK;
  theme.colors["NEON_RED"] = colors::NEON_RED;
  theme.colors["NEON_YELLOW"] = colors::NEON_YELLOW;
  theme.colors["NEON_PURPLE"] = colors::NEON_PURPLE;
  theme.colors["CYAN_DARK"] = colors::CYAN_DARK;

  theme.colors["BLUE"] = colors::BLUE;
  theme.colors["GREEN"] = colors::GREEN;
  theme.colors["YELLOW"] = colors::YELLOW;
  theme.colors["RED"] = colors::RED;
  theme.colors["PINK"] = colors::PINK;
  theme.colors["AMBER"] = colors::AMBER;

  theme.colors["BG_00"] = colors::BG_00;
  theme.colors["BG_01"] = colors::BG_01;
  theme.colors["BG_02"] = colors::BG_02;
  theme.colors["BG_03"] = colors::BG_03;
  theme.colors["BG_04"] = colors::BG_04;

  theme.colors["TEXT_PRIMARY"] = colors::TEXT_PRIMARY;
  theme.colors["TEXT_SECONDARY"] = colors::TEXT_SECONDARY;
  theme.colors["TEXT_TERTIARY"] = colors::TEXT_TERTIARY;
  theme.colors["TEXT_DISABLED"] = colors::TEXT_DISABLED;
  theme.colors["TEXT_INVERSE"] = colors::TEXT_INVERSE;

  theme.colors["BORDER_SUBTLE"] = colors::BORDER_SUBTLE;
  theme.colors["BORDER_DEFAULT"] = colors::BORDER_DEFAULT;
  theme.colors["BORDER_STRONG"] = colors::BORDER_STRONG;

  // Create JSON object
  juce::DynamicObject::Ptr json = new juce::DynamicObject();
  json->setProperty("name", name);

  juce::DynamicObject::Ptr colorsObj = new juce::DynamicObject();
  for (const auto& [colorName, colorValue] : theme.colors) {
    colorsObj->setProperty(colorName, static_cast<int>(colorValue));
  }
  json->setProperty("colors", juce::var(colorsObj.get()));

  // Write to file
  auto themeFile = getThemeDir().getChildFile(name + ".json");
  juce::FileOutputStream output(themeFile);
  output.truncate();

  if (output.openedOk()) {
    juce::var jsonVar(json.get());
    juce::JSON::writeToStream(output, jsonVar, juce::JSON::FormatOptions());
  }
}

void ThemeManager::loadTheme(const juce::String &name) {
  auto themeFile = getThemeDir().getChildFile(name + ".json");

  if (!themeFile.existsAsFile()) {
    return;
  }

  // Parse JSON
  juce::var json = juce::JSON::parse(themeFile);

  if (auto* jsonObj = json.getDynamicObject()) {
    Theme theme;
    auto nameVar = jsonObj->getProperty("name");
    theme.name = nameVar.isVoid() ? "Unknown" : nameVar.toString();

    if (auto* colorsObj = jsonObj->getProperty("colors").getDynamicObject()) {
      for (const auto& colorName : colorsObj->getProperties()) {
        uint32_t colorValue = static_cast<uint32_t>((int)colorName.value);
        theme.colors[colorName.name.toString()] = colorValue;
      }
    }

    applyTheme(theme);
  }
}

void ThemeManager::deleteTheme(const juce::String &name) {
  auto themeFile = getThemeDir().getChildFile(name + ".json");
  themeFile.deleteFile();
}

juce::StringArray ThemeManager::getAvailableThemes() const {
  juce::StringArray themes;

  for (auto& entry : getThemeDir().findChildFiles(
      juce::File::findFiles, false, "*.json")) {
    themes.add(entry.getFileNameWithoutExtension());
  }

  return themes;
}

void ThemeManager::applyTheme(const Theme &theme) {
  // Apply theme colors to design system using a comprehensive map
  for (const auto& [name, color] : theme.colors) {
    if (name == "CYAN") colors::CYAN = color;
    else if (name == "MAGENTA") colors::MAGENTA = color;
    else if (name == "VIOLET") colors::VIOLET = color;
    else if (name == "ORANGE") colors::ORANGE = color;
    else if (name == "NEON_GREEN") colors::NEON_GREEN = color;
    else if (name == "NEON_PINK") colors::NEON_PINK = color;
    else if (name == "NEON_RED") colors::NEON_RED = color;
    else if (name == "NEON_YELLOW") colors::NEON_YELLOW = color;
    else if (name == "NEON_PURPLE") colors::NEON_PURPLE = color;
    else if (name == "CYAN_DARK") colors::CYAN_DARK = color;

    else if (name == "BLUE") colors::BLUE = color;
    else if (name == "GREEN") colors::GREEN = color;
    else if (name == "YELLOW") colors::YELLOW = color;
    else if (name == "RED") colors::RED = color;
    else if (name == "PINK") colors::PINK = color;
    else if (name == "AMBER") colors::AMBER = color;

    else if (name == "BG_00") colors::BG_00 = color;
    else if (name == "BG_01") colors::BG_01 = color;
    else if (name == "BG_02") colors::BG_02 = color;
    else if (name == "BG_03") colors::BG_03 = color;
    else if (name == "BG_04") colors::BG_04 = color;

    else if (name == "TEXT_PRIMARY") colors::TEXT_PRIMARY = color;
    else if (name == "TEXT_SECONDARY") colors::TEXT_SECONDARY = color;
    else if (name == "TEXT_TERTIARY") colors::TEXT_TERTIARY = color;
    else if (name == "TEXT_DISABLED") colors::TEXT_DISABLED = color;
    else if (name == "TEXT_INVERSE") colors::TEXT_INVERSE = color;

    else if (name == "BORDER_SUBTLE") colors::BORDER_SUBTLE = color;
    else if (name == "BORDER_DEFAULT") colors::BORDER_DEFAULT = color;
    else if (name == "BORDER_STRONG") colors::BORDER_STRONG = color;
  }

  // Update current palette
  currentPalette_.bgDarkest = colors::BG_00;
  currentPalette_.bgDarker = colors::BG_01;
  currentPalette_.bgDark = colors::BG_02;
  currentPalette_.bgMedium = colors::BG_03;
  currentPalette_.bgLight = colors::BG_04;

  currentPalette_.accentPrimary = colors::CYAN;
  currentPalette_.accentSecondary = colors::MAGENTA;

  currentPalette_.textPrimary = colors::TEXT_PRIMARY;
  currentPalette_.textSecondary = colors::TEXT_SECONDARY;
  currentPalette_.textTertiary = colors::TEXT_TERTIARY;

  currentPalette_.borderDefault = colors::BORDER_DEFAULT;
  currentPalette_.borderSubtle = colors::BORDER_SUBTLE;
  currentPalette_.borderFocus = colors::BORDER_FOCUS;

  currentPalette_.success = colors::GREEN;
  currentPalette_.warning = colors::AMBER;
  currentPalette_.error = colors::RED;

  // Notify listeners of the theme change
  notifyListeners();
}

void ThemeManager::resetToDefault() {
  colors::resetToDefault();
  applyDarkTheme();
}

void ThemeManager::applyDarkTheme() {
  // Neon Noir theme colors
  colors::CYAN = 0xFF00F0FF;
  colors::MAGENTA = 0xFFFF00D4;
  colors::VIOLET = 0xFF7000FF;
  colors::ORANGE = 0xFFFF8800;
  colors::NEON_GREEN = 0xFF00FF9D;
  colors::NEON_PINK = 0xFFFF1493;
  colors::NEON_RED = 0xFFFF073A;
  colors::NEON_YELLOW = 0xFFFFFF00;
  colors::NEON_PURPLE = 0xFFAA00FF;
  colors::CYAN_DARK = 0xFF008888;

  colors::BLUE = 0xFF3B82F6;
  colors::GREEN = 0xFF10B981;
  colors::YELLOW = 0xFFF59E0B;
  colors::RED = 0xFFEF4444;
  colors::PINK = 0xFFEC4899;
  colors::AMBER = 0xFFFFAB00;

  colors::BG_00 = 0xFF0A0A0A;
  colors::BG_01 = 0xFF121212;
  colors::BG_02 = 0xFF1A1A1A;
  colors::BG_03 = 0xFF242424;
  colors::BG_04 = 0xFF2E2E2E;

  colors::TEXT_PRIMARY = 0xFFF2F2F7;
  colors::TEXT_SECONDARY = 0xFFA1A1AA;
  colors::TEXT_TERTIARY = 0xFF71717A;
  colors::TEXT_DISABLED = 0xFF52525B;
  colors::TEXT_INVERSE = 0xFF111111;

  colors::BORDER_SUBTLE = 0x0FFFFFFF;
  colors::BORDER_DEFAULT = 0x1FFFFFFF;
  colors::BORDER_STRONG = 0x33FFFFFF;
  colors::BORDER_FOCUS = colors::CYAN;

  // Update palette
  currentPalette_.bgDarkest = colors::BG_00;
  currentPalette_.bgDarker = colors::BG_01;
  currentPalette_.bgDark = colors::BG_02;
  currentPalette_.bgMedium = colors::BG_03;
  currentPalette_.bgLight = colors::BG_04;

  currentPalette_.accentPrimary = colors::CYAN;
  currentPalette_.accentSecondary = colors::MAGENTA;

  currentPalette_.textPrimary = colors::TEXT_PRIMARY;
  currentPalette_.textSecondary = colors::TEXT_SECONDARY;
  currentPalette_.textTertiary = colors::TEXT_TERTIARY;

  currentPalette_.borderDefault = colors::BORDER_DEFAULT;
  currentPalette_.borderSubtle = colors::BORDER_SUBTLE;
  currentPalette_.borderFocus = colors::BORDER_FOCUS;

  currentPalette_.success = colors::GREEN;
  currentPalette_.warning = colors::AMBER;
  currentPalette_.error = colors::RED;
}

void ThemeManager::applyDarkerTheme() {
  // OLED Black theme - deeper blacks for power saving
  colors::BG_00 = 0xFF000000;
  colors::BG_01 = 0xFF050505;
  colors::BG_02 = 0xFF0A0A0A;
  colors::BG_03 = 0xFF0F0F0F;
  colors::BG_04 = 0xFF141414;

  // Keep accent colors the same
  colors::CYAN = 0xFF00F0FF;
  colors::MAGENTA = 0xFFFF00D4;

  // Update palette
  currentPalette_.bgDarkest = colors::BG_00;
  currentPalette_.bgDarker = colors::BG_01;
  currentPalette_.bgDark = colors::BG_02;
  currentPalette_.bgMedium = colors::BG_03;
  currentPalette_.bgLight = colors::BG_04;

  currentPalette_.accentPrimary = colors::CYAN;
  currentPalette_.accentSecondary = colors::MAGENTA;
}

void ThemeManager::applyLightTheme() {
  // Light mode - inverted palette
  colors::BG_00 = 0xFFFFFFFF;
  colors::BG_01 = 0xFFF8F8F8;
  colors::BG_02 = 0xFFF0F0F0;
  colors::BG_03 = 0xFFE8E8E8;
  colors::BG_04 = 0xFFE0E0E0;

  // Darker accents for light mode
  colors::CYAN = 0xFF0088AA;
  colors::MAGENTA = 0xFFAA0088;

  colors::TEXT_PRIMARY = 0xFF111111;
  colors::TEXT_SECONDARY = 0xFF666666;
  colors::TEXT_TERTIARY = 0xFF999999;

  colors::BORDER_SUBTLE = 0x0D000000;
  colors::BORDER_DEFAULT = 0x1A000000;
  colors::BORDER_STRONG = 0x33000000;

  // Update palette
  currentPalette_.bgDarkest = colors::BG_00;
  currentPalette_.bgDarker = colors::BG_01;
  currentPalette_.bgDark = colors::BG_02;
  currentPalette_.bgMedium = colors::BG_03;
  currentPalette_.bgLight = colors::BG_04;

  currentPalette_.accentPrimary = colors::CYAN;
  currentPalette_.accentSecondary = colors::MAGENTA;

  currentPalette_.textPrimary = colors::TEXT_PRIMARY;
  currentPalette_.textSecondary = colors::TEXT_SECONDARY;
  currentPalette_.textTertiary = colors::TEXT_TERTIARY;
}

void ThemeManager::notifyListeners() {
  for (auto* listener : listeners_) {
    listener->themeChanged(activePreset_);
  }
  sendChangeMessage();
}

} // namespace design
} // namespace zenith
