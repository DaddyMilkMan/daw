/*
  ==============================================================================

    PluginEditorWindow.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 3: VST3 Plugin Hosting MVP

    Plugin editor window implementation

  ==============================================================================
*/

#include "PluginEditorWindow.h"

namespace zenith {

//==============================================================================
// PluginEditorWindow Implementation
//==============================================================================

PluginEditorWindow::PluginEditorWindow(juce::AudioPluginInstance* pluginInstance, bool useGenericEditor)
    : DocumentWindow(pluginInstance != nullptr ? pluginInstance->getName() : "Plugin",
                     juce::Colours::lightgrey,
                     DocumentWindow::allButtons),
      plugin(pluginInstance)
{
    if (plugin == nullptr)
    {
        DBG("PluginEditorWindow: ERROR - null plugin instance");
        return;
    }

    // Create the plugin editor
    if (useGenericEditor || !plugin->hasEditor())
    {
        // Use generic editor
        editor = new juce::GenericAudioProcessorEditor(*plugin);
    }
    else
    {
        // Use custom plugin editor
        editor = plugin->createEditorIfNeeded();
    }

    if (editor != nullptr)
    {
        // Set the editor as content
        setContentNonOwned(editor.getComponent(), true);

        // Position window in center of screen
        centreWithSize(editor->getWidth(), editor->getHeight());

        // Make visible
        setVisible(true);

        // Bring to front
        toFront(true);

        DBG("PluginEditorWindow: Created for " + plugin->getName());
    }
    else
    {
        DBG("PluginEditorWindow: ERROR - Failed to create editor for " + plugin->getName());
    }
}

PluginEditorWindow::~PluginEditorWindow()
{
    DBG("PluginEditorWindow: Destructor");

    // Clear content first
    clearContentComponent();

    // Delete the editor
    if (editor != nullptr && plugin != nullptr)
    {
        plugin->editorBeingDeleted(editor.getComponent());
        editor.deleteAndZero();
    }
}

void PluginEditorWindow::closeButtonPressed()
{
    // Just delete this window
    // The PluginEditorWindowManager will clean up its reference
    delete this;
}

//==============================================================================
// PluginEditorWindowManager Implementation
//==============================================================================

PluginEditorWindowManager::~PluginEditorWindowManager()
{
    closeAllEditors();
}

PluginEditorWindow* PluginEditorWindowManager::openEditor(juce::AudioPluginInstance* plugin, bool useGenericEditor)
{
    if (plugin == nullptr)
        return nullptr;

    // Check if already open
    auto it = editorWindows.find(plugin);
    if (it != editorWindows.end() && it->second != nullptr)
    {
        // Bring existing window to front
        it->second->toFront(true);
        return it->second.getComponent();
    }

    // Create new window
    auto* window = new PluginEditorWindow(plugin, useGenericEditor);

    // Store in map
    editorWindows[plugin] = window;

    return window;
}

void PluginEditorWindowManager::closeEditor(juce::AudioPluginInstance* plugin)
{
    if (plugin == nullptr)
        return;

    auto it = editorWindows.find(plugin);
    if (it != editorWindows.end())
    {
        // Delete window (will call closeButtonPressed)
        delete it->second.getComponent();

        // Remove from map
        editorWindows.erase(it);
    }
}

void PluginEditorWindowManager::closeAllEditors()
{
    // Copy the map keys to avoid iterator invalidation
    std::vector<juce::AudioPluginInstance*> pluginsToClose;
    pluginsToClose.reserve(editorWindows.size());

    for (const auto& pair : editorWindows)
    {
        pluginsToClose.push_back(pair.first);
    }

    // Close all windows
    for (auto* plugin : pluginsToClose)
    {
        closeEditor(plugin);
    }

    editorWindows.clear();
}

bool PluginEditorWindowManager::hasOpenEditor(juce::AudioPluginInstance* plugin) const
{
    if (plugin == nullptr)
        return false;

    auto it = editorWindows.find(plugin);
    return it != editorWindows.end() && it->second != nullptr;
}

PluginEditorWindow* PluginEditorWindowManager::getEditorWindow(juce::AudioPluginInstance* plugin) const
{
    if (plugin == nullptr)
        return nullptr;

    auto it = editorWindows.find(plugin);
    if (it != editorWindows.end() && it->second != nullptr)
        return it->second.getComponent();

    return nullptr;
}

} // namespace zenith
