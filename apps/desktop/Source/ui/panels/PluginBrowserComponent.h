/**
 * @file PluginBrowserComponent.h
 * @brief Plugin browser UI for selecting and loading VST3 plugins
 *
 * Displays available plugins from KnownPluginList and allows
 * loading them onto tracks.
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../controls/SkiaButton.h"
#include "../controls/SkiaComboBox.h"
#include "../controls/SkiaLabel.h"
#include "../controls/SkiaListBox.h"
#include "../controls/SkiaTextInput.h"

namespace zenith {

class Engine;
class Track;

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
                               private SkiaListBox::Model
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
    // SkiaListBox::Model interface
    //==========================================================================

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, SkCanvas& canvas, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int rowNumber, const juce::MouseEvent& e) override;

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
    SkiaTextInput searchBox;
    SkiaListBox pluginList;
    SkiaButton loadButton;
    SkiaLabel statusLabel;
    SkiaComboBox trackSelector;

    // Filtered list of plugin descriptions
    juce::Array<juce::PluginDescription> filteredPlugins;
    juce::String currentFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserComponent)
};

} // namespace zenith
