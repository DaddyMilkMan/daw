/**
 * @file FileMenuComponent.h
 * @brief File menu with New, Open, Save, Save As, Recent files
 *
 * Features:
 * - Modern dropdown menu with smooth animations
 * - New project dialog
 * - Open project file browser
 * - Save/Save As with filename confirmation
 * - Recent projects quick access
 * - Keyboard shortcuts displayed
 * - Apple-inspired design with smooth transitions
 * - Project auto-save notification
 *
 * @phase Phase 0: Foundation
 */

#pragma once

#include <JuceHeader.h>

class ProjectState;
class Engine;

namespace zenith {

/**
 * @class FileMenuComponent
 * @brief Beautiful file operations menu with animations
 */
class FileMenuComponent : public juce::Component,
                         public juce::Button::Listener
{
public:
    //==========================================================================
    FileMenuComponent(ProjectState& state, Engine& eng);
    ~FileMenuComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Button::Listener
    //==========================================================================
    void buttonClicked(juce::Button* button) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Refresh recent files list
     */
    void refreshRecentFiles();

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Show menu with animation
     */
    void showMenu();

    /**
     * @brief Hide menu with animation
     */
    void hideMenu();

    /**
     * @brief Paint menu item
     */
    void paintMenuItem(juce::Graphics& g, const juce::String& label,
                      const juce::String& shortcut, int index, bool isHovered);

    /**
     * @brief Handle New Project
     */
    void handleNewProject();

    /**
     * @brief Handle Open Project
     */
    void handleOpenProject();

    /**
     * @brief Handle Save Project
     */
    void handleSaveProject();

    /**
     * @brief Handle Save As
     */
    void handleSaveAs();

    /**
     * @brief Handle recent file selection
     */
    void handleRecentFile(int index) [[maybe_unused]];

    //==========================================================================
    // Members
    //==========================================================================

    ProjectState& projectState_;
    Engine& engine_;

    // Menu button
    juce::TextButton fileButton;
    bool menuOpen_ = false;
    float menuAlpha_ = 0.0f;  // For animation

    // Menu item bounds
    juce::Rectangle<int> newProjectBounds_;
    juce::Rectangle<int> openProjectBounds_;
    juce::Rectangle<int> saveProjectBounds_;
    juce::Rectangle<int> saveAsBounds_;
    juce::Rectangle<int> recentFilesArea_;

    // Recent files (max 5)
    juce::StringArray recentFiles_;
    juce::Array<juce::Rectangle<int>> recentFileBounds_;

    // Hover state
    int hoveredMenuItemIndex_ = -1;

    // Layout constants
    static constexpr int MENU_WIDTH = 250;
    static constexpr int MENU_ITEM_HEIGHT = 40;
    static constexpr int SEPARATOR_HEIGHT = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileMenuComponent)
};

}  // namespace zenith

