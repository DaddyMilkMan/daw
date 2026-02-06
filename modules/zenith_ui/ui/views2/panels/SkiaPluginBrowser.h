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


 * @file SkiaPluginBrowser.h
 * @brief Skia-based plugin browser UI for selecting and loading VST3 plugins
 *
 * Displays available plugins from KnownPluginList and allows
 * loading them onto tracks with modern glassmorphic design.


#include "../../framework/SkiaComponent.h"
#include "../../design-system/ZenithTheme.h"
#include "engine/Engine.h"
#include "engine/Track.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <functional>

namespace zenith::ui {

//==============================================================================
/**
 * @class SkiaPluginBrowser
 * @brief Skia-based UI component for browsing and loading plugins
 *
 * Features:
 * - Glassmorphic panel design
 * - List of available plugins (name, category, manufacturer)
 * - Real-time search/filter box
 * - Load on track button
 * - Double-click to load
 * - Category filtering (instruments, effects, utilities)
 */
class SkiaPluginBrowser : public SkiaComponent {
public:
    //==========================================================================
    explicit SkiaPluginBrowser(Engine& engine);
    ~SkiaPluginBrowser() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void drawSkia(SkCanvas* canvas) override;
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

    //==========================================================================
    // Callbacks
    //==========================================================================

    std::function<void()> onRefresh;
    std::function<void()> onClose;
    std::function<void(bool)> onLoadPlugin;

private:
    //==========================================================================
    // Types
    //==========================================================================

    struct PluginItem {
        juce::PluginDescription desc;
        juce::String name;
        juce::String category;
        juce::String manufacturer;
        juce::String description;
        bool isFavorite = false;
    };

    enum class CategoryFilter {
        All,
        Instruments,
        Effects,
        Utilities,
        Synths,
        Drums,
        Mixing,
        Other
    };

    //==========================================================================
    // Layout Constants
    //==========================================================================

    static constexpr float kHeaderHeight = 60.0f;
    static constexpr float kSearchBarHeight = 50.0f;
    static constexpr float kCategoryFilterHeight = 50.0f;
    static constexpr float kFooterHeight = 60.0f;
    static constexpr float kItemHeight = 48.0f;
    static constexpr float kPanelPadding = 20.0f;

    //==========================================================================
    // Drawing Methods
    //==========================================================================

    void drawHeader(SkCanvas* canvas, const SkRect& bounds);
    void drawSearchBar(SkCanvas* canvas, const SkRect& bounds);
    void drawCategoryFilter(SkCanvas* canvas, const SkRect& bounds);
    void drawPluginList(SkCanvas* canvas, const SkRect& bounds);
    void drawFooter(SkCanvas* canvas, const SkRect& bounds);
    void drawPluginItem(SkCanvas* canvas, const SkRect& bounds,
                       const PluginItem& item, int index, bool isSelected, bool isHovered);
    void drawCategoryButton(SkCanvas* canvas, const SkRect& bounds,
                           const juce::String& label, CategoryFilter filter,
                           bool isActive, bool isHovered);

    //==========================================================================
    // Input Handling
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    void updateFilteredList();
    void loadPluginAtIndex(int index);
    void selectCategory(CategoryFilter filter);
    juce::String getPluginDisplayName(const PluginItem& item) const;
    SkColor getCategoryColor(CategoryFilter category) const;
    PluginItem createPluginItem(const juce::PluginDescription& desc) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine;
    zenith::Track* targetTrack = nullptr;

    // UI State
    std::vector<PluginItem> pluginItems;
    std::vector<PluginItem> filteredItems;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    juce::String searchText_;
    CategoryFilter selectedCategory_ = CategoryFilter::All;
    bool isSearching_ = false;

    // Layout
    SkRect headerRect_;
    SkRect searchBarRect_;
    SkRect categoryFilterRect_;
    SkRect listRect_;
    SkRect footerRect_;

    // Track selector state
    juce::StringArray trackNames_;
    int selectedTrack_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPluginBrowser)
};

//==============================================================================
/**
 * @class SkiaPluginBrowserWindow
 * @brief Separate window for Skia plugin browser
 */
class SkiaPluginBrowserWindow : public juce::DocumentWindow {
public:
    explicit SkiaPluginBrowserWindow(Engine& engine);
    ~SkiaPluginBrowserWindow() override;

    void closeButtonPressed() override;

    SkiaPluginBrowser* getBrowserComponent() { return browserComponent.get(); }

private:
    std::unique_ptr<SkiaPluginBrowser> browserComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPluginBrowserWindow)
};

} // namespace zenith::ui