/**
 * @file PluginBrowserComponent.h
 * @brief Plugin browser UI for selecting and loading VST3 plugins
 *
 * Displays available plugins from KnownPluginList and allows
 * loading them onto tracks.
 */

#pragma once

#include <JuceHeader.h>

// Forward declarations
class Engine;

namespace zenith {
    class Track;
}

//==============================================================================
/**
 * @class PluginBrowserComponent
 * @brief UI component for browsing and loading plugins
 *
 * Features:
 * - List of available plugins (name, category, manufacturer)
 * - Search/filter box
 * - Load on track button
 * - Double-click to load
 */
class PluginBrowserComponent : public juce::Component,
                                private juce::TableListBoxModel,
                                private juce::TextEditor::Listener
{
public:
    //==========================================================================
    explicit PluginBrowserComponent(Engine& engine);
    ~PluginBrowserComponent() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Plugin browser interface
    //==========================================================================

    /**
     * @brief Set the target track for loading plugins
     * @param track Pointer to track (can be nullptr)
     */
    void setTargetTrack(zenith::Track* track);

    /**
     * @brief Get the currently selected plugin index
     * @return Plugin index or -1 if none selected
     */
    int getSelectedPluginIndex() const;

    /**
     * @brief Load the selected plugin onto the target track
     * @return true if successful
     */
    bool loadSelectedPlugin();

    /**
     * @brief Refresh the plugin list (call after scanning)
     */
    void refresh();

private:
    //==========================================================================
    // TableListBoxModel interface
    //==========================================================================

    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) [[maybe_unused]] override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) [[maybe_unused]] override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;

    //==========================================================================
    // TextEditor::Listener interface
    //==========================================================================

    void textEditorTextChanged(juce::TextEditor& editor) override;

    //==========================================================================
    // Helper methods
    //==========================================================================

    void updateFilteredList();
    void loadPluginAtIndex(int index) [[maybe_unused]];

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine;
    zenith::Track* targetTrack = nullptr;

    // UI Components
    juce::Label titleLabel;
    juce::TextEditor searchBox;
    juce::Label searchLabel;
    juce::TableListBox pluginTable;
    juce::TextButton loadButton;
    juce::Label statusLabel;

    // Track selector (simple combo box for now)
    juce::ComboBox trackSelector;
    juce::Label trackLabel;

    // Filtered list of plugin descriptions
    juce::Array<juce::PluginDescription> filteredPlugins;
    juce::String currentFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserComponent)
};

//==============================================================================
/**
 * @class PluginBrowserWindow
 * @brief Separate window for plugin browser
 */
class PluginBrowserWindow : public juce::DocumentWindow
{
public:
    explicit PluginBrowserWindow(Engine& engine);
    ~PluginBrowserWindow() override;

    void closeButtonPressed() override;

    PluginBrowserComponent* getBrowserComponent() { return browserComponent.get(); }

private:
    std::unique_ptr<PluginBrowserComponent> browserComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserWindow)
};

