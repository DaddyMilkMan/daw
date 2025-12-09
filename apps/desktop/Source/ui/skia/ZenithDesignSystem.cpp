/*
  ==============================================================================

    ZenithDesignSystem.cpp
    Created: 2025-12-01
    Author:  Zenith DAW

    Implementation of global design settings.

  ==============================================================================
*/

#include "ZenithDesignSystem.h"

namespace zenith::design {

// ============================================================================
// STATIC MEMBER DEFINITIONS (Settings struct)
// ============================================================================

float Settings::glowIntensity = 1.0f;
float Settings::uiScale = 1.0f;
Settings::Theme Settings::currentTheme = Settings::Theme::NeonNoir;

// ============================================================================
// THEME MANAGER IMPLEMENTATION
// ============================================================================

void ThemeManager::saveTheme(const juce::String &name) {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    
    // Save all colors from current theme
    auto colorMap = currentTheme_.toMap();
    for (const auto& [key, value] : colorMap) {
        obj->setProperty(key, (int64_t)value);
    }
    
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
        
        // Load all colors
        std::map<juce::String, uint32_t> colorMap;
        for (const auto& prop : obj->getProperties()) {
            if (prop.name != "name") {
                colorMap[prop.name.toString()] = (uint32_t)static_cast<int64_t>(prop.value);
            }
        }
        theme.fromMap(colorMap);
        
        // Apply the loaded theme
        applyTheme(theme);
    }
}

void ThemeManager::applyTheme(const Theme& theme) {
    currentTheme_ = theme;
    
    // Update global colors namespace
    colors::CYAN = theme.primary;
    colors::MAGENTA = theme.secondary;
    colors::NEON_GREEN = theme.accent;
    colors::AMBER = theme.warning;
    colors::RED = theme.danger;
    colors::BLUE = theme.info;
    colors::BG_DARKEST = theme.bgDarkest;
    colors::BG_DARKER = theme.bgDarker;
    colors::BG_DARK = theme.bgDark;
    colors::BG_MEDIUM = theme.bgMedium;
    colors::BG_LIGHT = theme.bgLight;
    colors::TEXT_PRIMARY = theme.textPrimary;
    colors::TEXT_SECONDARY = theme.textSecondary;
    colors::TEXT_DISABLED = theme.textDisabled;
    
    // Notify listeners of theme change
    sendChangeMessage();
}

void ThemeManager::setColor(const juce::String& colorName, SkColor color) {
    // Update the specific color in current theme
    if (colorName == "primary") currentTheme_.primary = color;
    else if (colorName == "secondary") currentTheme_.secondary = color;
    else if (colorName == "accent") currentTheme_.accent = color;
    else if (colorName == "warning") currentTheme_.warning = color;
    else if (colorName == "danger") currentTheme_.danger = color;
    else if (colorName == "info") currentTheme_.info = color;
    else if (colorName == "bgDarkest") currentTheme_.bgDarkest = color;
    else if (colorName == "bgDarker") currentTheme_.bgDarker = color;
    else if (colorName == "bgDark") currentTheme_.bgDark = color;
    else if (colorName == "bgMedium") currentTheme_.bgMedium = color;
    else if (colorName == "bgLight") currentTheme_.bgLight = color;
    else if (colorName == "textPrimary") currentTheme_.textPrimary = color;
    else if (colorName == "textSecondary") currentTheme_.textSecondary = color;
    else if (colorName == "textDisabled") currentTheme_.textDisabled = color;
    
    // Re-apply theme to update global colors
    applyTheme(currentTheme_);
}

SkColor ThemeManager::getColor(const juce::String& colorName) const {
    auto colorMap = currentTheme_.toMap();
    auto it = colorMap.find(colorName);
    return it != colorMap.end() ? it->second : 0xFF000000;
}

void ThemeManager::resetToDefault() {
    Theme defaultTheme;
    applyTheme(defaultTheme);
}

void ThemeManager::deleteTheme(const juce::String &name) {
    juce::File file = getThemeDir().getChildFile(name + ".json");
    file.deleteFile();
}

juce::StringArray ThemeManager::getAvailableThemes() const {
    juce::StringArray themes;
    themes.add("Default (Neon Noir)");  // Built-in theme
    auto files = getThemeDir().findChildFiles(juce::File::findFiles, false, "*.json");
    for (const auto &f : files) {
        themes.add(f.getFileNameWithoutExtension());
    }
    return themes;
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