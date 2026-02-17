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

#include "PluginEditorWindow.h"

namespace zenith {

//==============================================================================
// PluginEditorWindow - Single plugin editor window
//==============================================================================

class PluginEditorWindow : public juce::DocumentWindow {
public:
    PluginEditorWindow(juce::AudioProcessorEditor* editor, 
                       const juce::String& title,
                       std::function<void()> onClose)
        : juce::DocumentWindow(title,
                               juce::Colours::darkgrey,
                               juce::DocumentWindow::allButtons),
          onCloseCallback_(std::move(onClose)) {
        setUsingNativeTitleBar(true);
        setContentOwned(editor, true);
        setResizable(true, false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }
    
    void closeButtonPressed() override {
        if (onCloseCallback_) {
            onCloseCallback_();
        }
    }
    
private:
    std::function<void()> onCloseCallback_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

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
    
    // Check if already open
    if (openWindows_.find(key.toStdString()) != openWindows_.end()) {
        // Bring to front
        if (auto* window = openWindows_[key.toStdString()].get()) {
            window->toFront(true);
        }
        return;
    }
    
    // Create editor
    auto* editor = processor->createEditor();
    if (!editor) {
        DBG("PluginEditorWindowManager: Failed to create editor");
        return;
    }
    
    // Create window
    juce::String title = processor->getName() + " - " + trackName;
    auto window = std::make_unique<PluginEditorWindow>(
        editor, 
        title,
        [this, key]() {
            // Close callback - remove from map
            juce::MessageManager::callAsync([this, key]() {
                openWindows_.erase(key.toStdString());
            });
        }
    );
    
    openWindows_[key.toStdString()] = std::move(window);
    DBG("PluginEditorWindowManager: Opened editor for " + title);
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
