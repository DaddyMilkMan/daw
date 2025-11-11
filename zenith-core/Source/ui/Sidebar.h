/**
 * @file Sidebar.h
 * @brief Left sidebar component for Zenith DAW
 *
 * Replaces React components:
 * - vexel-daw/src/renderer/components/LeftPanel.tsx (browser panel)
 *
 * Features:
 * - Track list view
 * - File browser
 * - Plugin browser
 * - Favorites
 * - Search functionality
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithLookAndFeel.h"

//==============================================================================
/**
 * @class Sidebar
 * @brief Left sidebar with track list and browser functionality
 *
 * Layout:
 * [Tabs: Tracks | Files | Plugins | Favorites]
 * [Search bar]
 * [Content area - list/tree view]
 */
class Sidebar : public juce::Component
{
public:
    //==========================================================================
    /**
     * @brief Callback when track selected
     */
    std::function<void(int)> onTrackSelected;

    /**
     * @brief Callback when file double-clicked (to load)
     */
    std::function<void(const juce::File&)> onFileSelected;

    //==========================================================================
    Sidebar();
    ~Sidebar() override = default;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Public API
    //==========================================================================

    /**
     * @brief Add a track to the track list
     */
    void addTrack(const juce::String& trackName);

    /**
     * @brief Remove a track from the list
     */
    void removeTrack(int trackIndex);

    /**
     * @brief Clear all tracks
     */
    void clearTracks();

    /**
     * @brief Set the current tab (0=Tracks, 1=Files, 2=Plugins, 3=Favorites)
     */
    void setCurrentTab(int tabIndex);

private:
    //==========================================================================
    // Member variables
    //==========================================================================

    // Tab buttons
    juce::TextButton tracksTab;
    juce::TextButton filesTab;
    juce::TextButton pluginsTab;
    juce::TextButton favoritesTab;

    // Search
    juce::TextEditor searchBox;

    // Content area
    juce::ListBox trackList;
    juce::TreeView fileTree;

    // Track data
    juce::StringArray trackNames;

    int currentTab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sidebar)
};
