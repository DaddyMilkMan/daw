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

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithTheme.h"
#include "zenith_core/instruments/ZenithPresetManager.h"
#include "zenith_core/instruments/Instrument.h"
#include "zenith_core/engine/Engine.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
#include <optional>

namespace zenith {

//==============================================================================
// Types
//==============================================================================

/**
 * @brief Display mode for the preset browser
 */
enum class PresetBrowserMode {
    Browser,    ///< Browse and load presets
    Saver,      ///< Save current state as preset
    Manager     ///< Manage presets (delete, organize)
};

/**
 * @brief Sort criteria for preset list
 */
enum class PresetSortCriteria {
    Name,
    Category,
    Author,
    DateModified,
    Rating,
    Popularity
};

//==============================================================================
// Main Component
//==============================================================================

/**
 * @class PresetBrowserComponent
 * @brief Full-featured preset browser with Skia rendering
 * 
 * Production-ready implementation featuring:
 * - Integration with ZenithPresetManager
 * - Real-time fuzzy search
 * - Category/tag filtering
 * - Preset preview audio
 * - Favorites and ratings
 * - Smooth animations
 */
class PresetBrowserComponent : public SkiaComponent {
public:
    //==========================================================================
    // Construction
    //==========================================================================
    
    PresetBrowserComponent();
    ~PresetBrowserComponent() override;
    
    //==========================================================================
    // Configuration
    //==========================================================================
    
    /**
     * @brief Set the engine for preset operations
     */
    void setEngine(Engine* engine);
    
    /**
     * @brief Set target instrument for loading presets
     */
    void setInstrumentId(const juce::String& instrumentId);
    
    /**
     * @brief Get current instrument ID
     */
    juce::String getInstrumentId() const { return currentInstrumentId_; }
    
    /**
     * @brief Set callbacks for preset operations
     */
    using LoadPresetCallback = std::function<void(const Preset&)>;
    using CaptureStateCallback = std::function<Preset()>;
    
    void setLoadPresetCallback(LoadPresetCallback callback);
    void setCaptureStateCallback(CaptureStateCallback callback);
    
    //==========================================================================
    // Mode Control
    //==========================================================================
    
    /**
     * @brief Set the browser mode
     */
    void setMode(PresetBrowserMode mode);
    PresetBrowserMode getMode() const { return currentMode_; }
    
    //==========================================================================
    // Preset Operations
    //==========================================================================
    
    /**
     * @brief Refresh the preset list from disk
     */
    void refreshPresetList();
    
    /**
     * @brief Load the selected preset
     * @return true if successful
     */
    bool loadSelectedPreset();
    
    /**
     * @brief Save current state as a new preset
     */
    void saveCurrentAsPreset(const juce::String& name, const juce::String& category);
    
    /**
     * @brief Delete the selected preset
     */
    bool deleteSelectedPreset();
    
    /**
     * @brief Toggle favorite status of selected preset
     */
    void toggleFavoriteSelected();
    
    /**
     * @brief Set rating for selected preset (0-5)
     */
    void setRatingForSelected(int rating);
    
    //==========================================================================
    // Search and Filter
    //==========================================================================
    
    /**
     * @brief Set search text
     */
    void setSearchText(const juce::String& text);
    
    /**
     * @brief Filter by category (empty = all)
     */
    void setCategoryFilter(const juce::String& category);
    
    /**
     * @brief Filter by tag (empty = all)
     */
    void setTagFilter(const juce::String& tag);
    
    /**
     * @brief Set sort criteria
     */
    void setSortCriteria(PresetSortCriteria criteria);
    
    /**
     * @brief Show only favorites
     */
    void setShowOnlyFavorites(bool showOnly);
    
    /**
     * @brief Show only user presets (vs factory)
     */
    void setShowOnlyUserPresets(bool showOnly);
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
    void onAnimationTick(float deltaMs) override;
    
    //==========================================================================
    // Callbacks
    //==========================================================================
    
    std::function<void(const Preset&)> onPresetLoaded;
    std::function<void(const Preset&)> onPresetSaved;
    std::function<void(const juce::String&)> onError;
    std::function<void()> onClose;
    
private:
    //==========================================================================
    // Layout Constants
    //==========================================================================
    
    static constexpr float kHeaderHeight = 64.0f;
    static constexpr float kSearchBarHeight = 44.0f;
    static constexpr float kFilterBarHeight = 40.0f;
    static constexpr float kPresetItemHeight = 72.0f;
    static constexpr float kPresetItemPadding = 12.0f;
    static constexpr float kSidebarWidth = 200.0f;
    static constexpr float kPreviewPanelHeight = 180.0f;
    static constexpr float kFooterHeight = 56.0f;
    static constexpr float kPanelPadding = 16.0f;
    static constexpr float kCornerRadius = 12.0f;
    static constexpr float kScrollBarWidth = 8.0f;
    
    //==========================================================================
    // Internal Types
    //==========================================================================
    
    struct DisplayPreset {
        PresetMetadata metadata;
        bool isFavorite = false;
        bool isUserPreset = false;
        float hoverProgress = 0.0f;
        float selectionProgress = 0.0f;
    };
    
    struct CategoryInfo {
        juce::String name;
        int count = 0;
        SkColor color;
    };
    
    //==========================================================================
    // Drawing Methods
    //==========================================================================
    
    void drawBackground(SkCanvas* canvas);
    void drawHeader(SkCanvas* canvas, const SkRect& bounds);
    void drawSearchBar(SkCanvas* canvas, const SkRect& bounds);
    void drawFilterBar(SkCanvas* canvas, const SkRect& bounds);
    void drawSidebar(SkCanvas* canvas, const SkRect& bounds);
    void drawPresetList(SkCanvas* canvas, const SkRect& bounds);
    void drawPresetItem(SkCanvas* canvas, const SkRect& bounds, 
                       const DisplayPreset& preset, int index);
    void drawMiniWaveform(SkCanvas* canvas, const SkRect& bounds, 
                         const std::vector<float>& samples);
    void drawPreviewPanel(SkCanvas* canvas, const SkRect& bounds);
    void drawFooter(SkCanvas* canvas, const SkRect& bounds);
    void drawScrollBar(SkCanvas* canvas);
    void drawCategoryButton(SkCanvas* canvas, const SkRect& bounds,
                           const CategoryInfo& category, bool isSelected);
    void drawTagChip(SkCanvas* canvas, const SkRect& bounds,
                    const juce::String& tag, bool isSelected);
    void drawRatingStars(SkCanvas* canvas, const SkRect& bounds, 
                        int rating, int hoverRating, bool interactive);
    void drawEmptyState(SkCanvas* canvas, const SkRect& bounds);
    
    //==========================================================================
    // Hit Testing
    //==========================================================================
    
    int hitTestPreset(float x, float y) const;
    int hitTestCategory(float x, float y) const;
    int hitTestTag(float x, float y) const;
    bool hitTestSearchBar(float x, float y) const;
    bool hitTestScrollBar(float x, float y) const;
    int hitTestRating(float x, float y) const;
    int hitTestFooterButton(float x, float y) const;
    
    //==========================================================================
    // Layout Calculation
    //==========================================================================
    
    SkRect getHeaderRect() const;
    SkRect getSearchBarRect() const;
    SkRect getFilterBarRect() const;
    SkRect getSidebarRect() const;
    SkRect getPresetListRect() const;
    SkRect getPreviewPanelRect() const;
    SkRect getFooterRect() const;
    float getContentWidth() const;
    float getContentHeight() const;
    
    //==========================================================================
    // Data Management
    //==========================================================================
    
    void loadPresetsFromManager();
    void applyFilters();
    void applySorting();
    void extractCategories();
    void extractTags();
    void updateScrollLimits();
    
    bool matchesSearch(const PresetMetadata& preset, const juce::String& search) const;
    bool matchesCategory(const PresetMetadata& preset, const juce::String& category) const;
    bool matchesTag(const PresetMetadata& preset, const juce::String& tag) const;
    
    //==========================================================================
    // Animation
    //==========================================================================
    
    void startHoverAnimation(int index, bool hovering);
    void startSelectionAnimation(int index);
    float getSmoothValue(float current, float target, float speed);
    
    //==========================================================================
    // Persistence
    //==========================================================================
    
    void loadFavorites();
    void saveFavorites();
    juce::File getFavoritesFile() const;
    
    //==========================================================================
    // State
    //==========================================================================
    
    // Dependencies
    Engine* engine_ = nullptr;
    juce::String currentInstrumentId_;
    LoadPresetCallback loadPresetCallback_;
    CaptureStateCallback captureStateCallback_;
    
    // Mode
    PresetBrowserMode currentMode_ = PresetBrowserMode::Browser;
    
    // Data
    std::vector<DisplayPreset> allPresets_;
    std::vector<DisplayPreset> filteredPresets_;
    std::vector<CategoryInfo> categories_;
    std::vector<juce::String> allTags_;
    std::set<juce::String> favoriteIds_;
    
    // Selection and Filter
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    juce::String searchText_;
    juce::String categoryFilter_;
    juce::String tagFilter_;
    PresetSortCriteria sortCriteria_ = PresetSortCriteria::Name;
    bool showOnlyFavorites_ = false;
    bool showOnlyUserPresets_ = false;
    
    // Scroll
    float scrollOffset_ = 0.0f;
    float maxScrollOffset_ = 0.0f;
    bool isDraggingScrollBar_ = false;
    float scrollBarHoverProgress_ = 0.0f;
    
    // Hover states for UI elements
    int hoveredCategoryIndex_ = -1;
    int hoveredTagIndex_ = -1;
    int hoveredRating_ = -1;
    int hoveredFooterButton_ = -1;
    bool isSearchBarHovered_ = false;
    bool isSearchBarFocused_ = false;
    
    // Animation
    float previewPanelProgress_ = 0.0f;
    float searchProgress_ = 0.0f;
    
    // Text input
    std::unique_ptr<juce::TextEditor> searchEditor_;
    void setupSearchEditor();
    void onSearchTextChanged();
    
    // Async operations
    std::atomic<bool> isLoadingPresets_{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};

} // namespace zenith
