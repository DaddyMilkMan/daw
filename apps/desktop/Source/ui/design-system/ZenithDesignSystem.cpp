/*
  ==============================================================================

    ZenithDesignSystem.cpp
    Created: 2025-12-01
    Author:  Zenith DAW
    Refactored: 2025-12-27

    Implementation of global design settings.

  ==============================================================================
*/

#include "ZenithDesignSystem.h"

namespace zenith::design {

// ============================================================================
// THEME MANAGER IMPLEMENTATION
// ============================================================================

ThemeManager::ThemeManager() {
  // Initialize with Dark theme
  applyDarkTheme();
}

void ThemeManager::resetToDefault() {
  listeners_.clear();
  setActiveTheme(ThemePreset::Dark);
  applyDarkTheme(); // Force re-apply
  sendChangeMessage();
}

void ThemeManager::setActiveTheme(ThemePreset preset) {
  if (activePreset_ == preset)
    return;

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
  sendChangeMessage(); // JUCE ChangeBroadcaster
}

void ThemeManager::applyTheme(const Theme &theme) {
    // Apply a custom theme map on top of current palette
    // This allows partial overrides
    for (auto const& [key, val] : theme.colors) {
        // This would require a map reflection or manual if/else chain
        // For now, we support the core set as saved in saveTheme
        if (key == "CYAN") currentPalette_.cyan = val;
        else if (key == "MAGENTA") currentPalette_.magenta = val;
        else if (key == "NEON_GREEN") currentPalette_.neonGreen = val;
        else if (key == "BG_DARKEST") currentPalette_.bg00 = val;
        else if (key == "BG_DARKER") currentPalette_.bg01 = val;
        else if (key == "BG_DARK") currentPalette_.bg02 = val;
        // Extend as needed for custom themes
    }
    notifyListeners();
    sendChangeMessage();
}

void ThemeManager::saveTheme(const juce::String &name) {
  auto *obj = new juce::DynamicObject();

  // Save current colors
  // Mapping names to ZenithPalette fields
  obj->setProperty("CYAN", (juce::int64)currentPalette_.cyan);
  obj->setProperty("MAGENTA", (juce::int64)currentPalette_.magenta);
  obj->setProperty("NEON_GREEN", (juce::int64)currentPalette_.neonGreen);
  obj->setProperty("BG_DARKEST", (juce::int64)currentPalette_.bg00);
  obj->setProperty("BG_DARKER", (juce::int64)currentPalette_.bg01);
  obj->setProperty("BG_DARK", (juce::int64)currentPalette_.bg02);

  juce::File file = getThemeDir().getChildFile(name + ".json");
  file.replaceWithText(juce::JSON::toString(juce::var(obj)));
}

void ThemeManager::loadTheme(const juce::String &name) {
  juce::File file = getThemeDir().getChildFile(name + ".json");
  if (!file.existsAsFile())
    return;

  auto json = juce::JSON::parse(file);
  if (auto *obj = json.getDynamicObject()) {
    Theme theme;
    theme.name = name;

    // Load into map
    if (obj->hasProperty("CYAN")) theme.colors["CYAN"] = (uint32_t)(int)obj->getProperty("CYAN");
    if (obj->hasProperty("MAGENTA")) theme.colors["MAGENTA"] = (uint32_t)(int)obj->getProperty("MAGENTA");
    if (obj->hasProperty("NEON_GREEN")) theme.colors["NEON_GREEN"] = (uint32_t)(int)obj->getProperty("NEON_GREEN");
    if (obj->hasProperty("BG_DARKEST")) theme.colors["BG_DARKEST"] = (uint32_t)(int)obj->getProperty("BG_DARKEST");
    if (obj->hasProperty("BG_DARKER")) theme.colors["BG_DARKER"] = (uint32_t)(int)obj->getProperty("BG_DARKER");
    if (obj->hasProperty("BG_DARK")) theme.colors["BG_DARK"] = (uint32_t)(int)obj->getProperty("BG_DARK");
    
    applyTheme(theme);
  }
}

void ThemeManager::deleteTheme(const juce::String &name) {
  juce::File file = getThemeDir().getChildFile(name + ".json");
  file.deleteFile();
}

juce::StringArray ThemeManager::getAvailableThemes() const {
  juce::StringArray themes;
  auto files =
      getThemeDir().findChildFiles(juce::File::findFiles, false, "*.json");
  for (const auto &f : files) {
    themes.add(f.getFileNameWithoutExtension());
  }
  return themes;
}

void ThemeManager::addListener(ThemeListener *listener) {
  if (listener && std::find(listeners_.begin(), listeners_.end(), listener) ==
                      listeners_.end()) {
    listeners_.push_back(listener);
  }
}

void ThemeManager::removeListener(ThemeListener *listener) {
  listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener),
                   listeners_.end());
}

void ThemeManager::notifyListeners() {
  for (auto *listener : listeners_) {
    if (listener)
      listener->themeChanged(activePreset_);
  }
}

// ============================================================================
// THEME PRESETS
// ============================================================================

void ThemeManager::applyDarkTheme() {
  // Reset to default (Neon Noir)
  // ZenithPalette default constructor has Dark Theme values
  currentPalette_ = ZenithPalette();
}

void ThemeManager::applyDarkerTheme() {
  // Reset first
  currentPalette_ = ZenithPalette();
    
  // OLED Black - pure black for power saving, subtle accents
  currentPalette_.bg00 = 0xFF000000;
  currentPalette_.bg01 = 0xFF080808;
  currentPalette_.bg02 = 0xFF101010;
  currentPalette_.bg03 = 0xFF181818;
  currentPalette_.bg04 = 0xFF202020;

  currentPalette_.cyan = 0xFF00D4FF;   // Softer Cyan
  currentPalette_.magenta = 0xFFE000C0; // Softer Magenta
  
  // Propagate to accents
  currentPalette_.accentPrimary = currentPalette_.cyan;
  currentPalette_.accentSecondary = currentPalette_.magenta;

  currentPalette_.textPrimary = 0xFFE8E8E8;
  currentPalette_.textSecondary = 0xFF888888;
  currentPalette_.textTertiary = 0xFF555555;

  currentPalette_.borderDefault = 0x18FFFFFF;
  currentPalette_.borderSubtle = 0x0AFFFFFF;
  currentPalette_.borderFocus = currentPalette_.cyan;

  currentPalette_.success = 0xFF00C853;
  currentPalette_.warning = 0xFFFF9800;
  currentPalette_.danger = 0xFFFF3D00;
}

void ThemeManager::applyLightTheme() {
  // Reset first
  currentPalette_ = ZenithPalette();
    
  // Light mode - inverted palette for daylight use
  currentPalette_.bg00 = 0xFFFFFFFF;
  currentPalette_.bg01 = 0xFFF8F8FA;
  currentPalette_.bg02 = 0xFFF0F0F4;
  currentPalette_.bg03 = 0xFFE8E8EC;
  currentPalette_.bg04 = 0xFFDCDCE0;

  currentPalette_.cyan = 0xFF0066CC;   // Deep Blue
  currentPalette_.magenta = 0xFF7C3AED; // Purple
  
  // Propagate to accents
  currentPalette_.accentPrimary = currentPalette_.cyan;
  currentPalette_.accentSecondary = currentPalette_.magenta;

  currentPalette_.textPrimary = 0xFF1A1A1A;
  currentPalette_.textSecondary = 0xFF666666;
  currentPalette_.textTertiary = 0xFF999999;
  currentPalette_.textDisabled = 0xFFCCCCCC; 
  currentPalette_.textInverse = 0xFFFFFFFF;

  currentPalette_.borderDefault = 0x20000000;
  currentPalette_.borderSubtle = 0x10000000;
  currentPalette_.borderFocus = currentPalette_.cyan;
  currentPalette_.borderGreeting = 0x20000000; // Darker border for light mode

  currentPalette_.success = 0xFF059669;
  currentPalette_.warning = 0xFFD97706;
  currentPalette_.danger = 0xFFDC2626;
  currentPalette_.info = 0xFF0066CC; // Blue
  
  // Update Viz colors which depend on functional colors
  currentPalette_.waveformAudio = currentPalette_.info;
  currentPalette_.waveformMidi = currentPalette_.magenta;
  
  // Update Glass
  currentPalette_.glassHighlight = 0x20FFFFFF; // Keep white highlight? Or dark? 
  // Usually glass on light mode needs dark shadow, white highlight still works if bg is off-white.
  currentPalette_.glassShadow = 0x20000000;
  currentPalette_.glassHover = 0x10000000; // Darker hover
}


// ============================================================================
// LAYOUT MANAGER IMPLEMENTATION
// ============================================================================

void LayoutManager::setPanelState(const juce::String &panelId,
                                  const PanelState &state) {
  panels_[panelId] = state;
}

LayoutManager::PanelState
LayoutManager::getPanelState(const juce::String &panelId) const {
  auto it = panels_.find(panelId);
  if (it != panels_.end())
    return it->second;

  // Default state if not found
  return PanelState{panelId, juce::Rectangle<float>(0, 0, 1, 1), true, 0};
}

void LayoutManager::saveLayout(const juce::String &name) {
  auto *root = new juce::DynamicObject();
  juce::Array<juce::var> panelArray;

  for (const auto &[id, state] : panels_) {
    auto *p = new juce::DynamicObject();
    p->setProperty("id", id);
    p->setProperty("x", state.relativeBounds.getX());
    p->setProperty("y", state.relativeBounds.getY());
    p->setProperty("w", state.relativeBounds.getWidth());
    p->setProperty("h", state.relativeBounds.getHeight());
    p->setProperty("visible", state.isVisible);
    panelArray.add(juce::var(p));
  }

  root->setProperty("panels", panelArray);

  juce::File file = getLayoutDir().getChildFile(name + ".layout");
  file.replaceWithText(juce::JSON::toString(juce::var(root)));
}

void LayoutManager::loadLayout(const juce::String &name) {
  juce::File file = getLayoutDir().getChildFile(name + ".layout");
  if (!file.existsAsFile())
    return;

  auto json = juce::JSON::parse(file);
  if (auto *root = json.getDynamicObject()) {
    auto panelArray = root->getProperty("panels").getArray();
    if (panelArray) {
      for (auto &v : *panelArray) {
        if (auto *p = v.getDynamicObject()) {
          juce::String id = p->getProperty("id").toString();
          PanelState state;
          state.id = id;
          state.relativeBounds = juce::Rectangle<float>(
              (float)p->getProperty("x"), (float)p->getProperty("y"),
              (float)p->getProperty("w"), (float)p->getProperty("h"));
          state.isVisible = (bool)p->getProperty("visible");
          panels_[id] = state;
        }
      }
    }
  }
}

} // namespace zenith::design
