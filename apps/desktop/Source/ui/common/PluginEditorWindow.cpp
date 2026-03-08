/*
  ==============================================================================
    PluginEditorWindow.cpp
    Plugin editor window management
    
    Manages floating windows for VST3/AU plugin editors.
  ==============================================================================
*/

#include "PluginEditorWindow.h"
#include "../controls/SkiaAlertWindow.h"

namespace zenith {

//==============================================================================
// PluginEditorWindowManager Implementation
//==============================================================================

PluginEditorWindowManager::PluginEditorWindowManager() {
    DBG("PluginEditorWindowManager created");
}

PluginEditorWindowManager::~PluginEditorWindowManager() {
    closeAllWindows();
}

void PluginEditorWindowManager::openEditor(juce::AudioProcessor* processor, 
                                            const juce::String& trackName,
                                            int slotIndex) {
    if (!processor || !processor->hasEditor()) {
        DBG("PluginEditorWindowManager: Processor has no editor");
        return;
    }
    
    // Create unique key for this plugin slot
    juce::String key = trackName + "_" + juce::String(slotIndex);
    
    const std::string keyStd = key.toStdString();

    // Check if already open
    if (openWindows_.find(keyStd) != openWindows_.end() && openWindows_[keyStd]) {
        return;
    }

    openWindows_[keyStd] = true;

    juce::String title = "Plugin Editor Unavailable";
    juce::String pluginName = processor->getName();
    juce::String message =
        "Skia-only mode is enabled.\n\n"
        "Native JUCE plugin editor windows are disabled for:\n" + pluginName +
        "\n\nUse mixer controls/parameters in the Skia UI.";

    SkiaAlertWindow::showMessageBoxAsync(
        SkiaAlertWindow::IconType::InfoIcon,
        title,
        message,
        "OK");

    openWindows_[keyStd] = false;
    DBG("PluginEditorWindowManager: Skia-only mode fallback shown for " + pluginName);
}

void PluginEditorWindowManager::closeEditor(const juce::String& trackName, int slotIndex) {
    juce::String key = trackName + "_" + juce::String(slotIndex);
    auto it = openWindows_.find(key.toStdString());
    if (it != openWindows_.end()) {
        openWindows_.erase(it);
        DBG("PluginEditorWindowManager: Closed editor for " + key);
    }
}

void PluginEditorWindowManager::closeAllWindows() {
    openWindows_.clear();
    DBG("PluginEditorWindowManager: Closed all editor windows");
}

bool PluginEditorWindowManager::isEditorOpen(const juce::String& trackName, int slotIndex) const {
    juce::String key = trackName + "_" + juce::String(slotIndex);
    return openWindows_.find(key.toStdString()) != openWindows_.end();
}

} // namespace zenith
