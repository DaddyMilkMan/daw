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

#include "LayoutManager.h"
#include "../common/ResizablePanelContainer.h"
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {
namespace layout {

//==============================================================================
// PanelConfig Serialization
//==============================================================================

juce::var PanelConfig::toVar() const {
  auto *obj = new juce::DynamicObject();
  obj->setProperty("id", id);
  obj->setProperty("name", name);
  obj->setProperty("type", type);
  obj->setProperty("initialSize", initialSize);
  obj->setProperty("minSize", minSize);
  obj->setProperty("maxSize", maxSize);
  obj->setProperty("flex", flex);
  obj->setProperty("isCollapsed", isCollapsed);
  obj->setProperty("isCollapsible", isCollapsible);
  obj->setProperty("isVisible", isVisible);
  obj->setProperty("tabGroupIndex", tabGroupIndex);

  juce::Array<juce::var> tabs;
  for (const auto &tab : tabIds) {
    tabs.add(tab);
  }
  obj->setProperty("tabIds", tabs);
  obj->setProperty("activeTabId", activeTabId);
  obj->setProperty("customData", customData);

  return juce::var(obj);
}

PanelConfig PanelConfig::fromVar(const juce::var &v) {
  PanelConfig config;

  if (auto *obj = v.getDynamicObject()) {
    config.id = obj->getProperty("id").toString();
    config.name = obj->getProperty("name").toString();
    config.type = obj->getProperty("type").toString();
    config.initialSize = (float)obj->getProperty("initialSize");
    config.minSize = (float)obj->getProperty("minSize");
    config.maxSize = (float)obj->getProperty("maxSize");
    config.flex = (float)obj->getProperty("flex");
    config.isCollapsed = obj->getProperty("isCollapsed");
    config.isCollapsible = obj->getProperty("isCollapsible");
    config.isVisible = obj->getProperty("isVisible");
    config.tabGroupIndex = (int)obj->getProperty("tabGroupIndex");

    if (auto *tabs = obj->getProperty("tabIds").getArray()) {
      for (const auto &tab : *tabs) {
        config.tabIds.add(tab.toString());
      }
    }
    config.activeTabId = obj->getProperty("activeTabId").toString();
    config.customData = obj->getProperty("customData");
  }

  return config;
}

//==============================================================================
// DividerConfig Serialization
//==============================================================================

juce::var DividerConfig::toVar() const {
  auto *obj = new juce::DynamicObject();
  obj->setProperty("id", id);
  obj->setProperty("position", position);
  obj->setProperty("isHorizontal", isHorizontal);
  obj->setProperty("minPositionRatio", minPositionRatio);
  obj->setProperty("maxPositionRatio", maxPositionRatio);
  return juce::var(obj);
}

DividerConfig DividerConfig::fromVar(const juce::var &v) {
  DividerConfig config;

  if (auto *obj = v.getDynamicObject()) {
    config.id = obj->getProperty("id").toString();
    config.position = (float)obj->getProperty("position");
    config.isHorizontal = obj->getProperty("isHorizontal");
    config.minPositionRatio = (float)obj->getProperty("minPositionRatio");
    config.maxPositionRatio = (float)obj->getProperty("maxPositionRatio");
  }

  return config;
}

//==============================================================================
// LayoutConfig Serialization
//==============================================================================

juce::var LayoutConfig::toVar() const {
  auto *obj = new juce::DynamicObject();
  obj->setProperty("id", id);
  obj->setProperty("name", name);
  obj->setProperty("description", description);
  obj->setProperty("lastModified", lastModified.toISO8601(true));

  juce::Array<juce::var> panelArray;
  for (const auto &panel : panels) {
    panelArray.add(panel.toVar());
  }
  obj->setProperty("panels", panelArray);

  juce::Array<juce::var> dividerArray;
  for (const auto &divider : dividers) {
    dividerArray.add(divider.toVar());
  }
  obj->setProperty("dividers", dividerArray);

  obj->setProperty("rootLayout", rootLayout);

  return juce::var(obj);
}

LayoutConfig LayoutConfig::fromVar(const juce::var &v) {
  LayoutConfig config;

  if (auto *obj = v.getDynamicObject()) {
    config.id = obj->getProperty("id").toString();
    config.name = obj->getProperty("name").toString();
    config.description = obj->getProperty("description").toString();
    config.lastModified =
        juce::Time::fromISO8601(obj->getProperty("lastModified").toString());

    if (auto *panels = obj->getProperty("panels").getArray()) {
      for (const auto &panel : *panels) {
        config.panels.add(PanelConfig::fromVar(panel));
      }
    }

    if (auto *dividers = obj->getProperty("dividers").getArray()) {
      for (const auto &divider : *dividers) {
        config.dividers.add(DividerConfig::fromVar(divider));
      }
    }

    config.rootLayout = obj->getProperty("rootLayout");
  }

  return config;
}

juce::String LayoutConfig::toJSON() const {
  return juce::JSON::toString(toVar(), true);
}

LayoutConfig LayoutConfig::fromJSON(const juce::String &json) {
  auto parsed = juce::JSON::parse(json);
  return fromVar(parsed);
}

//==============================================================================
// LayoutManager Implementation
//==============================================================================

LayoutManager &LayoutManager::getInstance() {
  static LayoutManager instance;
  return instance;
}

LayoutManager::LayoutManager() { initializeBuiltInPresets(); }

void LayoutManager::registerPanelType(const juce::String &typeId,
                                      const juce::String &displayName,
                                      PanelFactory factory) {
  panelTypes_[typeId.toStdString()] = {displayName, std::move(factory)};
}

std::unique_ptr<juce::Component>
LayoutManager::createPanel(const juce::String &typeId) {
  auto it = panelTypes_.find(typeId.toStdString());
  if (it != panelTypes_.end() && it->second.factory) {
    return it->second.factory();
  }
  return nullptr;
}

juce::StringArray LayoutManager::getRegisteredPanelTypes() const {
  juce::StringArray types;
  for (const auto &pair : panelTypes_) {
    types.add(juce::String(pair.first));
  }
  return types;
}

juce::String
LayoutManager::getPanelDisplayName(const juce::String &typeId) const {
  auto it = panelTypes_.find(typeId.toStdString());
  if (it != panelTypes_.end()) {
    return it->second.displayName;
  }
  return typeId;
}

//==============================================================================
// Built-in Presets
//==============================================================================

void LayoutManager::initializeBuiltInPresets() {
  // Production Layout: Browser + Arranger + Mixer
  {
    LayoutConfig config;
    config.id = "production";
    config.name = "Production";
    config.description =
        "Arranger + Mixer + Browser - Standard production workflow";

    // Root layout: horizontal split [Browser | Main Area]
    auto *rootObj = new juce::DynamicObject();
    rootObj->setProperty("type", "horizontal");
    rootObj->setProperty("id", "root");

    // Browser panel
    PanelConfig browserPanel;
    browserPanel.id = "browser";
    browserPanel.name = "Browser";
    browserPanel.type = "browser";
    browserPanel.minSize = 200.0f;
    browserPanel.initialSize = 300.0f;
    browserPanel.flex = 0.0f;
    browserPanel.isCollapsible = true;
    config.panels.add(browserPanel);

    // Main area: vertical split [Arranger | Mixer]
    PanelConfig arrangerPanel;
    arrangerPanel.id = "arranger";
    arrangerPanel.name = "Arrangement";
    arrangerPanel.type = "arranger";
    arrangerPanel.minSize = 200.0f;
    arrangerPanel.flex = 2.0f;
    config.panels.add(arrangerPanel);

    PanelConfig mixerPanel;
    mixerPanel.id = "mixer";
    mixerPanel.name = "Mixer";
    mixerPanel.type = "mixer";
    mixerPanel.minSize = 150.0f;
    mixerPanel.initialSize = 200.0f;
    mixerPanel.flex = 0.0f;
    mixerPanel.isCollapsible = true;
    config.panels.add(mixerPanel);

    // Layout hierarchy
    juce::Array<juce::var> mainChildren;

    auto *browserNode = new juce::DynamicObject();
    browserNode->setProperty("panelId", "browser");
    mainChildren.add(juce::var(browserNode));

    auto *centerArea = new juce::DynamicObject();
    centerArea->setProperty("type", "vertical");
    centerArea->setProperty("id", "center");

    juce::Array<juce::var> centerChildren;
    auto *arrangerNode = new juce::DynamicObject();
    arrangerNode->setProperty("panelId", "arranger");
    centerChildren.add(juce::var(arrangerNode));

    auto *mixerNode = new juce::DynamicObject();
    mixerNode->setProperty("panelId", "mixer");
    centerChildren.add(juce::var(mixerNode));

    centerArea->setProperty("children", centerChildren);
    mainChildren.add(juce::var(centerArea));

    rootObj->setProperty("children", mainChildren);
    config.rootLayout = juce::var(rootObj);

    // Dividers
    DividerConfig browserDivider;
    browserDivider.id = "browser-center";
    browserDivider.position = 0.2f;
    browserDivider.isHorizontal = false;
    browserDivider.minPositionRatio = 0.1f;
    browserDivider.maxPositionRatio = 0.4f;
    config.dividers.add(browserDivider);

    DividerConfig mixerDivider;
    mixerDivider.id = "arranger-mixer";
    mixerDivider.position = 0.7f;
    mixerDivider.isHorizontal = true;
    mixerDivider.minPositionRatio = 0.3f;
    mixerDivider.maxPositionRatio = 0.9f;
    config.dividers.add(mixerDivider);

    builtInPresets_["production"] = config;
  }

  // Editing Layout: Piano Roll + Arranger
  {
    LayoutConfig config;
    config.id = "editing";
    config.name = "Editing";
    config.description = "Piano Roll + Arranger - MIDI editing workflow";

    PanelConfig arrangerPanel;
    arrangerPanel.id = "arranger";
    arrangerPanel.name = "Arrangement";
    arrangerPanel.type = "arranger";
    arrangerPanel.minSize = 200.0f;
    arrangerPanel.flex = 1.0f;
    config.panels.add(arrangerPanel);

    PanelConfig pianoRollPanel;
    pianoRollPanel.id = "pianoroll";
    pianoRollPanel.name = "Piano Roll";
    pianoRollPanel.type = "pianoroll";
    pianoRollPanel.minSize = 200.0f;
    pianoRollPanel.flex = 1.0f;
    config.panels.add(pianoRollPanel);

    auto *rootObj = new juce::DynamicObject();
    rootObj->setProperty("type", "vertical");
    rootObj->setProperty("id", "root");

    juce::Array<juce::var> children;
    auto *arrangerNode = new juce::DynamicObject();
    arrangerNode->setProperty("panelId", "arranger");
    children.add(juce::var(arrangerNode));

    auto *pianoNode = new juce::DynamicObject();
    pianoNode->setProperty("panelId", "pianoroll");
    children.add(juce::var(pianoNode));

    rootObj->setProperty("children", children);
    config.rootLayout = juce::var(rootObj);

    DividerConfig divider;
    divider.id = "arranger-piano";
    divider.position = 0.5f;
    divider.isHorizontal = true;
    config.dividers.add(divider);

    builtInPresets_["editing"] = config;
  }

  // Mixing Layout: Full-width Mixer + Plugins
  {
    LayoutConfig config;
    config.id = "mixing";
    config.name = "Mixing";
    config.description = "Full Mixer + Plugin slots - Mixing workflow";

    PanelConfig mixerPanel;
    mixerPanel.id = "mixer";
    mixerPanel.name = "Mixer";
    mixerPanel.type = "mixer";
    mixerPanel.minSize = 300.0f;
    mixerPanel.flex = 2.0f;
    config.panels.add(mixerPanel);

    PanelConfig pluginsPanel;
    pluginsPanel.id = "plugins";
    pluginsPanel.name = "Plugin Browser";
    pluginsPanel.type = "plugins";
    pluginsPanel.minSize = 200.0f;
    pluginsPanel.initialSize = 300.0f;
    pluginsPanel.flex = 0.0f;
    pluginsPanel.isCollapsible = true;
    config.panels.add(pluginsPanel);

    auto *rootObj = new juce::DynamicObject();
    rootObj->setProperty("type", "horizontal");
    rootObj->setProperty("id", "root");

    juce::Array<juce::var> children;
    auto *mixerNode = new juce::DynamicObject();
    mixerNode->setProperty("panelId", "mixer");
    children.add(juce::var(mixerNode));

    auto *pluginsNode = new juce::DynamicObject();
    pluginsNode->setProperty("panelId", "plugins");
    children.add(juce::var(pluginsNode));

    rootObj->setProperty("children", children);
    config.rootLayout = juce::var(rootObj);

    DividerConfig divider;
    divider.id = "mixer-plugins";
    divider.position = 0.75f;
    divider.isHorizontal = false;
    divider.minPositionRatio = 0.5f;
    divider.maxPositionRatio = 0.9f;
    config.dividers.add(divider);

    builtInPresets_["mixing"] = config;
  }
}

LayoutConfig LayoutManager::getPreset(const juce::String &presetName) const {
  auto it = builtInPresets_.find(presetName.toStdString());
  if (it != builtInPresets_.end()) {
    return it->second;
  }
  return LayoutConfig();
}

juce::StringArray LayoutManager::getAvailablePresets() const {
  juce::StringArray presets;
  for (const auto &pair : builtInPresets_) {
    presets.add(juce::String(pair.first));
  }
  return presets;
}

void LayoutManager::applyLayout(const LayoutConfig &config,
                                ResizablePanelContainer *container) {
  if (container == nullptr)
    return;

  container->applyLayoutConfig(config);
  sendChangeMessage();
}

void LayoutManager::applyPreset(const juce::String &presetName,
                                ResizablePanelContainer *container) {
  auto preset = getPreset(presetName);
  if (!preset.id.isEmpty()) {
    applyLayout(preset, container);
  }
}

LayoutConfig
LayoutManager::captureCurrentLayout(const ResizablePanelContainer *container,
                                    const juce::String &name) const {
  if (container == nullptr)
    return LayoutConfig();

  return container->captureLayoutConfig(name);
}

//==============================================================================
// File Operations
//==============================================================================

juce::File LayoutManager::getLayoutsDirectory() const {
  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
  return appDataDir.getChildFile("Zenith DAW").getChildFile("Layouts");
}

bool LayoutManager::saveLayoutToFile(const LayoutConfig &config,
                                     const juce::File &file) {
  auto json = config.toJSON();

  auto result = file.create();
  if (result.failed())
    return false;

  return file.replaceWithText(json);
}

LayoutConfig LayoutManager::loadLayoutFromFile(const juce::File &file) const {
  if (!file.existsAsFile())
    return LayoutConfig();

  auto json = file.loadFileAsString();
  return LayoutConfig::fromJSON(json);
}

juce::Array<juce::File> LayoutManager::getSavedLayouts() const {
  auto dir = getLayoutsDirectory();
  if (!dir.isDirectory())
    return {};

  return dir.findChildFiles(juce::File::findFiles, false, "*.json");
}

void LayoutManager::saveLastLayout(const ResizablePanelContainer *container) {
  auto config = captureCurrentLayout(container, "Last Session");
  config.id = "last_session";
  config.lastModified = juce::Time::getCurrentTime();

  auto layoutsDir = getLayoutsDirectory();
  layoutsDir.createDirectory();

  auto lastLayoutFile = layoutsDir.getChildFile("_last_session.json");
  saveLayoutToFile(config, lastLayoutFile);
}

LayoutConfig LayoutManager::loadLastLayout() const {
  auto lastLayoutFile =
      getLayoutsDirectory().getChildFile("_last_session.json");
  return loadLayoutFromFile(lastLayoutFile);
}

bool LayoutManager::hasLastLayout() const {
  auto lastLayoutFile =
      getLayoutsDirectory().getChildFile("_last_session.json");
  return lastLayoutFile.existsAsFile();
}

//==============================================================================
// Settings
//==============================================================================

void LayoutManager::setAnimationSettings(const AnimationSettings &settings) {
  animationSettings_ = settings;
  sendChangeMessage();
}

void LayoutManager::setEditModeEnabled(bool enabled) {
  editModeEnabled_ = enabled;
  sendChangeMessage();
}

} // namespace layout
} // namespace zenith
