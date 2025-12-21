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
Settings::BlurQuality Settings::blurQuality = Settings::BlurQuality::Medium;

// ============================================================================
// THEME MANAGER IMPLEMENTATION
// ============================================================================

void ThemeManager::saveTheme(const juce::String &name) {
  auto *obj = new juce::DynamicObject();

  // Save current colors
  obj->setProperty("CYAN", (int64_t)colors::CYAN);
  obj->setProperty("MAGENTA", (int64_t)colors::MAGENTA);
  obj->setProperty("NEON_GREEN", (int64_t)colors::NEON_GREEN);
  obj->setProperty("BG_DARKEST", (int64_t)colors::BG_DARKEST);
  obj->setProperty("BG_DARKER", (int64_t)colors::BG_DARKER);
  obj->setProperty("BG_DARK", (int64_t)colors::BG_DARK);

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

    if (obj->hasProperty("CYAN"))
      colors::CYAN = (uint32_t)static_cast<int64_t>(obj->getProperty("CYAN"));
    if (obj->hasProperty("MAGENTA"))
      colors::MAGENTA =
          (uint32_t)static_cast<int64_t>(obj->getProperty("MAGENTA"));
    if (obj->hasProperty("NEON_GREEN"))
      colors::NEON_GREEN =
          (uint32_t)static_cast<int64_t>(obj->getProperty("NEON_GREEN"));
    if (obj->hasProperty("BG_DARKEST"))
      colors::BG_DARKEST =
          (uint32_t)static_cast<int64_t>(obj->getProperty("BG_DARKEST"));
    if (obj->hasProperty("BG_DARKER"))
      colors::BG_DARKER =
          (uint32_t)static_cast<int64_t>(obj->getProperty("BG_DARKER"));
    if (obj->hasProperty("BG_DARK"))
      colors::BG_DARK =
          (uint32_t)static_cast<int64_t>(obj->getProperty("BG_DARK"));

    // Trigger repaint globally (would need a listener, but for now relies on
    // repaint calls)
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