/*
  ==============================================================================

    PluginEditorWindow.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 3: VST3 Plugin Hosting MVP

    Plugin editor window wrapper

    Responsibilities:
    - Display plugin editor in a separate window
    - Handle window lifecycle (open, close, resize)
    - Manage plugin editor lifetime

    Thread Safety:
    - All methods are MESSAGE THREAD ONLY
    - Plugin editors must only be created/destroyed on message thread

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace zenith {

//==============================================================================
/**
    Window that hosts a plugin editor.

    This is a simple DocumentWindow that displays the plugin's editor UI.
    When the window is closed, it deletes itself.
*/
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    //==============================================================================
    /**
     * @brief Create a plugin editor window
     *
     * @param plugin The plugin instance to edit
     * @param useGenericEditor If true, use generic editor even if custom UI available
     */
    PluginEditorWindow(juce::AudioPluginInstance* plugin, bool useGenericEditor = false);

    ~PluginEditorWindow() override;

    //==============================================================================
    /**
     * @brief Get the plugin being edited
     */
    juce::AudioPluginInstance* getPlugin() const { return plugin; }

    //==============================================================================
    // DocumentWindow overrides

    /**
     * @brief Called when user clicks close button
     */
    void closeButtonPressed() override;

private:
    //==============================================================================
    // Member Variables
    //==============================================================================

    // The plugin instance (not owned - must outlive this window)
    juce::AudioPluginInstance* plugin;

    // The plugin editor component (owned by this window's content component)
    juce::Component::SafePointer<juce::AudioProcessorEditor> editor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

//==============================================================================
/**
    Manager for plugin editor windows.

    This class keeps track of open plugin editor windows and ensures
    only one window per plugin instance.
*/
class PluginEditorWindowManager
{
public:
    //==============================================================================
    PluginEditorWindowManager() = default;
    ~PluginEditorWindowManager();

    //==============================================================================
    /**
     * @brief Open or focus a plugin editor window
     *
     * If a window is already open for this plugin, it will be brought to front.
     * Otherwise, a new window will be created.
     *
     * @param plugin The plugin to edit
     * @param useGenericEditor If true, use generic editor
     * @return Pointer to the window (may be newly created or existing)
     */
    PluginEditorWindow* openEditor(juce::AudioPluginInstance* plugin, bool useGenericEditor = false);

    /**
     * @brief Close a plugin editor window
     *
     * @param plugin The plugin whose editor to close
     */
    void closeEditor(juce::AudioPluginInstance* plugin);

    /**
     * @brief Close all plugin editor windows
     */
    void closeAllEditors();

    /**
     * @brief Check if a plugin has an open editor window
     *
     * @param plugin The plugin to check
     * @return true if window is open
     */
    bool hasOpenEditor(juce::AudioPluginInstance* plugin) const;

    /**
     * @brief Get the editor window for a plugin (if open)
     *
     * @param plugin The plugin
     * @return Pointer to window, or nullptr if not open
     */
    PluginEditorWindow* getEditorWindow(juce::AudioPluginInstance* plugin) const;

private:
    //==============================================================================
    // Member Variables
    //==============================================================================

    // Map of plugin instance to editor window (using unique_ptr for safe ownership)
    std::map<juce::AudioPluginInstance*, std::unique_ptr<PluginEditorWindow>> editorWindows;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindowManager)
};

} // namespace zenith
