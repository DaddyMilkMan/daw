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

#include "PresetBrowserComponent.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
#include <algorithm>
#include <cmath>

namespace zenith {

using namespace design;

//==============================================================================
// Construction / Destruction
//==============================================================================

PresetBrowserComponent::PresetBrowserComponent()
    : engine_(nullptr)
{
    setName("PresetBrowserComponent");
    setWantsKeyboardFocus(true);
    
    // Setup search editor
    setupSearchEditor();
    
    // Load favorites
    loadFavorites();
    
    // Start animation timer
    startTimerHz(60);
}

PresetBrowserComponent::~PresetBrowserComponent()
{
    saveFavorites();
    stopTimer();
}

void PresetBrowserComponent::setupSearchEditor()
{
    searchEditor_ = std::make_unique<juce::TextEditor>();
    searchEditor_->setMultiLine(false);
    searchEditor_->setReturnKeyStartsNewLine(false);
    searchEditor_->setEscapeAndReturnKeysConsumed(true);
    searchEditor_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchEditor_->setColour(juce::TextEditor::textColourId, toJuceColour(colors::TEXT_PRIMARY));
    searchEditor_->setColour(juce::TextEditor::highlightedTextColourId, toJuceColour(colors::TEXT_PRIMARY));
    searchEditor_->setColour(juce::TextEditor::highlightColourId, toJuceColour(colors::ACCENT_PRIMARY));
    searchEditor_->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    searchEditor_->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    searchEditor_->setFont(design::getFont(14.0f, FontWeight::Regular));
    searchEditor_->setTextToShowWhenEmpty("Search presets...", 
        toJuceColour(withAlpha(colors::TEXT_SECONDARY, 0.5f)));
    
    searchEditor_->onTextChange = [this]() {
        onSearchTextChanged();
    };
    
    searchEditor_->onEscapeKey = [this]() {
        searchEditor_->clear();
        onSearchTextChanged();
        isSearchBarFocused_ = false;
        grabKeyboardFocus();
    };
    
    searchEditor_->onReturnKey = [this]() {
        if (!filteredPresets_.empty()) {
            selectedIndex_ = 0;
            startSelectionAnimation(0);
            repaint();
        }
    };
    
    addAndMakeVisible(*searchEditor_);
}

void PresetBrowserComponent::onSearchTextChanged()
{
    searchText_ = searchEditor_->getText();
    applyFilters();
    scrollOffset_ = 0.0f;
    markDirty();
}

//==============================================================================
// Configuration
//==============================================================================

void PresetBrowserComponent::setEngine(Engine* engine)
{
    engine_ = engine;
    refreshPresetList();
}

void PresetBrowserComponent::setInstrumentId(const juce::String& instrumentId)
{
    if (currentInstrumentId_ != instrumentId) {
        currentInstrumentId_ = instrumentId;
        refreshPresetList();
    }
}

void PresetBrowserComponent::setLoadPresetCallback(LoadPresetCallback callback)
{
    loadPresetCallback_ = std::move(callback);
}

void PresetBrowserComponent::setCaptureStateCallback(CaptureStateCallback callback)
{
    captureStateCallback_ = std::move(callback);
}

//==============================================================================
// Mode Control
//==============================================================================

void PresetBrowserComponent::setMode(PresetBrowserMode mode)
{
    currentMode_ = mode;
    markDirty();
}

//==============================================================================
// Preset Operations
//==============================================================================

void PresetBrowserComponent::refreshPresetList()
{
    loadPresetsFromManager();
    extractCategories();
    extractTags();
    applyFilters();
    applySorting();
    updateScrollLimits();
    markDirty();
}

bool PresetBrowserComponent::loadSelectedPreset()
{
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(filteredPresets_.size()))
        return false;
    
    const auto& displayPreset = filteredPresets_[selectedIndex_];
    auto& manager = ZenithPresetManager::getInstance();
    
    Preset preset = manager.loadPreset(displayPreset.metadata.instrumentId, 
                                       displayPreset.metadata.id);
    
    if (preset.id.isEmpty()) {
        if (onError)
            onError("Failed to load preset: " + displayPreset.metadata.name);
        return false;
    }
    
    // Update play count and last played
    // (would be persisted in a real implementation)
    
    // Invoke callback
    if (loadPresetCallback_)
        loadPresetCallback_(preset);
    
    if (onPresetLoaded)
        onPresetLoaded(preset);
    
    return true;
}

void PresetBrowserComponent::saveCurrentAsPreset(const juce::String& name, 
                                                  const juce::String& category)
{
    if (!captureStateCallback_ || name.isEmpty())
        return;
    
    Preset captured = captureStateCallback_();
    captured.name = name;
    captured.category = category.isEmpty() ? "User" : category;
    captured.author = "User";
    captured.id = Preset::generateId(name);
    
    auto& manager = ZenithPresetManager::getInstance();
    
    if (manager.savePreset(captured, true)) {
        refreshPresetList();
        
        // Select the newly saved preset
        for (size_t i = 0; i < filteredPresets_.size(); ++i) {
            if (filteredPresets_[i].metadata.id == captured.id) {
                selectedIndex_ = static_cast<int>(i);
                startSelectionAnimation(static_cast<int>(i));
                break;
            }
        }
        
        if (onPresetSaved)
            onPresetSaved(captured);
    } else {
        if (onError)
            onError("Failed to save preset: " + name);
    }
}

bool PresetBrowserComponent::deleteSelectedPreset()
{
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(filteredPresets_.size()))
        return false;
    
    const auto& displayPreset = filteredPresets_[selectedIndex_];
    
    // Only allow deletion of user presets
    if (!displayPreset.isUserPreset) {
        if (onError)
            onError("Cannot delete factory presets");
        return false;
    }
    
    auto& manager = ZenithPresetManager::getInstance();
    
    if (manager.deletePreset(displayPreset.metadata.instrumentId, 
                             displayPreset.metadata.id, true)) {
        refreshPresetList();
        selectedIndex_ = -1;
        return true;
    }
    
    return false;
}

void PresetBrowserComponent::toggleFavoriteSelected()
{
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(filteredPresets_.size()))
        return;
    
    auto& preset = filteredPresets_[selectedIndex_];
    preset.isFavorite = !preset.isFavorite;
    
    if (preset.isFavorite)
        favoriteIds_.insert(preset.metadata.id);
    else
        favoriteIds_.erase(preset.metadata.id);
    
    saveFavorites();
    markDirty();
}

void PresetBrowserComponent::setRatingForSelected(int rating)
{
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(filteredPresets_.size()))
        return;
    
    // In a real implementation, this would persist to disk
    // For now, we just update the display
    filteredPresets_[selectedIndex_].metadata.userRating = juce::jlimit(0, 5, rating);
    markDirty();
}

//==============================================================================
// Search and Filter
//==============================================================================

void PresetBrowserComponent::setSearchText(const juce::String& text)
{
    searchText_ = text;
    if (searchEditor_)
        searchEditor_->setText(text, false);
    applyFilters();
    scrollOffset_ = 0.0f;
    markDirty();
}

void PresetBrowserComponent::setCategoryFilter(const juce::String& category)
{
    categoryFilter_ = category;
    applyFilters();
    scrollOffset_ = 0.0f;
    markDirty();
}

void PresetBrowserComponent::setTagFilter(const juce::String& tag)
{
    tagFilter_ = tag;
    applyFilters();
    scrollOffset_ = 0.0f;
    markDirty();
}

void PresetBrowserComponent::setSortCriteria(PresetSortCriteria criteria)
{
    sortCriteria_ = criteria;
    applySorting();
    markDirty();
}

void PresetBrowserComponent::setShowOnlyFavorites(bool showOnly)
{
    showOnlyFavorites_ = showOnly;
    applyFilters();
    markDirty();
}

void PresetBrowserComponent::setShowOnlyUserPresets(bool showOnly)
{
    showOnlyUserPresets_ = showOnly;
    applyFilters();
    markDirty();
}

//==============================================================================
// Data Management
//==============================================================================

void PresetBrowserComponent::loadPresetsFromManager()
{
    allPresets_.clear();
    
    if (currentInstrumentId_.isEmpty())
        return;
    
    auto& manager = ZenithPresetManager::getInstance();
    
    // Load factory presets
    auto factoryPresets = manager.getPresetList(currentInstrumentId_);
    for (const auto& metadata : factoryPresets) {
        DisplayPreset dp;
        dp.metadata = metadata;
        dp.isFavorite = favoriteIds_.count(metadata.id) > 0;
        dp.isUserPreset = false;
        allPresets_.push_back(std::move(dp));
    }
    
    // Load user presets
    auto userPresets = manager.getPresetList(currentInstrumentId_);
    for (const auto& metadata : userPresets) {
        DisplayPreset dp;
        dp.metadata = metadata;
        dp.isFavorite = favoriteIds_.count(metadata.id) > 0;
        dp.isUserPreset = true;
        allPresets_.push_back(std::move(dp));
    }
}

void PresetBrowserComponent::applyFilters()
{
    filteredPresets_.clear();
    
    for (auto& preset : allPresets_) {
        // Apply favorites filter
        if (showOnlyFavorites_ && !preset.isFavorite)
            continue;
        
        // Apply user preset filter
        if (showOnlyUserPresets_ && !preset.isUserPreset)
            continue;
        
        // Apply category filter
        if (!categoryFilter_.isEmpty() && !matchesCategory(preset.metadata, categoryFilter_))
            continue;
        
        // Apply tag filter
        if (!tagFilter_.isEmpty() && !matchesTag(preset.metadata, tagFilter_))
            continue;
        
        // Apply search filter
        if (!searchText_.isEmpty() && !matchesSearch(preset.metadata, searchText_))
            continue;
        
        filteredPresets_.push_back(preset);
    }
    
    updateScrollLimits();
}

void PresetBrowserComponent::applySorting()
{
    std::sort(filteredPresets_.begin(), filteredPresets_.end(),
        [this](const DisplayPreset& a, const DisplayPreset& b) {
            switch (sortCriteria_) {
                case PresetSortCriteria::Name:
                    return a.metadata.name.compareNatural(b.metadata.name) < 0;
                case PresetSortCriteria::Category:
                    if (a.metadata.category != b.metadata.category)
                        return a.metadata.category < b.metadata.category;
                    return a.metadata.name < b.metadata.name;
                case PresetSortCriteria::Author:
                    if (a.metadata.author != b.metadata.author)
                        return a.metadata.author < b.metadata.author;
                    return a.metadata.name < b.metadata.name;
                case PresetSortCriteria::Rating:
                    return a.metadata.userRating > b.metadata.userRating;
                case PresetSortCriteria::Popularity:
                    return a.metadata.playCount > b.metadata.playCount;
                default:
                    return a.metadata.name < b.metadata.name;
            }
        });
}

void PresetBrowserComponent::extractCategories()
{
    categories_.clear();
    std::map<juce::String, int> categoryCounts;
    
    for (const auto& preset : allPresets_) {
        categoryCounts[preset.metadata.category]++;
    }
    
    // Predefined category colors
    std::map<juce::String, SkColor> categoryColors = {
        {"Bass", SkColorSetRGB(255, 100, 100)},
        {"Lead", SkColorSetRGB(100, 200, 255)},
        {"Pad", SkColorSetRGB(150, 100, 255)},
        {"Keys", SkColorSetRGB(255, 200, 100)},
        {"Pluck", SkColorSetRGB(100, 255, 150)},
        {"FX", SkColorSetRGB(255, 100, 200)},
        {"Drums", SkColorSetRGB(200, 200, 200)},
        {"User", SkColorSetRGB(100, 255, 255)}
    };
    
    for (const auto& [name, count] : categoryCounts) {
        CategoryInfo info;
        info.name = name;
        info.count = count;
        info.color = categoryColors.count(name) > 0 ? categoryColors[name] 
                                                     : SkColorSetRGB(150, 150, 150);
        categories_.push_back(info);
    }
    
    // Sort categories alphabetically
    std::sort(categories_.begin(), categories_.end(),
        [](const CategoryInfo& a, const CategoryInfo& b) {
            return a.name < b.name;
        });
}

void PresetBrowserComponent::extractTags()
{
    allTags_.clear();
    std::set<juce::String> uniqueTags;
    
    for (const auto& preset : allPresets_) {
        for (const auto& tag : preset.metadata.tags) {
            uniqueTags.insert(tag);
        }
    }
    
    allTags_.assign(uniqueTags.begin(), uniqueTags.end());
    std::sort(allTags_.begin(), allTags_.end());
}

void PresetBrowserComponent::updateScrollLimits()
{
    auto listRect = getPresetListRect();
    float totalHeight = filteredPresets_.size() * (kPresetItemHeight + kPresetItemPadding);
    maxScrollOffset_ = std::max(0.0f, totalHeight - listRect.height());
    scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_);
}

bool PresetBrowserComponent::matchesSearch(const PresetMetadata& preset, 
                                           const juce::String& search) const
{
    if (search.isEmpty())
        return true;
    
    juce::String lowerSearch = search.toLowerCase();
    
    // Check name
    if (preset.name.toLowerCase().contains(lowerSearch))
        return true;
    
    // Check author
    if (preset.author.toLowerCase().contains(lowerSearch))
        return true;
    
    // Check category
    if (preset.category.toLowerCase().contains(lowerSearch))
        return true;
    
    // Check tags
    for (const auto& tag : preset.tags) {
        if (tag.toLowerCase().contains(lowerSearch))
            return true;
    }
    
    return false;
}

bool PresetBrowserComponent::matchesCategory(const PresetMetadata& preset, 
                                             const juce::String& category) const
{
    return preset.category == category;
}

bool PresetBrowserComponent::matchesTag(const PresetMetadata& preset, 
                                        const juce::String& tag) const
{
    for (const auto& presetTag : preset.tags) {
        if (presetTag == tag)
            return true;
    }
    return false;
}

//==============================================================================
// Persistence
//==============================================================================

void PresetBrowserComponent::loadFavorites()
{
    auto file = getFavoritesFile();
    if (!file.existsAsFile())
        return;
    
    auto json = juce::JSON::parse(file.loadFileAsString());
    if (json.isObject()) {
        auto* array = json.getProperty("favorites", juce::var()).getArray();
        if (array) {
            for (const auto& item : *array) {
                favoriteIds_.insert(item.toString());
            }
        }
    }
}

void PresetBrowserComponent::saveFavorites()
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    juce::Array<juce::var> favArray;
    
    for (const auto& id : favoriteIds_) {
        favArray.add(id.toString());
    }
    
    obj->setProperty("favorites", favArray);
    
    auto file = getFavoritesFile();
    file.create();
    file.replaceWithText(juce::JSON::toString(juce::var(obj.get())));
}

juce::File PresetBrowserComponent::getFavoritesFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("preset_favorites.json");
}

//==============================================================================
// Layout Calculation
//==============================================================================

SkRect PresetBrowserComponent::getHeaderRect() const
{
    return SkRect::MakeXYWH(kPanelPadding, kPanelPadding, 
                           getWidth() - 2 * kPanelPadding, kHeaderHeight);
}

SkRect PresetBrowserComponent::getSearchBarRect() const
{
    auto header = getHeaderRect();
    return SkRect::MakeXYWH(kPanelPadding, header.fBottom + 8.0f,
                           getWidth() - 2 * kPanelPadding, kSearchBarHeight);
}

SkRect PresetBrowserComponent::getFilterBarRect() const
{
    auto search = getSearchBarRect();
    return SkRect::MakeXYWH(kPanelPadding, search.fBottom + 8.0f,
                           getWidth() - 2 * kPanelPadding, kFilterBarHeight);
}

SkRect PresetBrowserComponent::getSidebarRect() const
{
    auto filter = getFilterBarRect();
    float sidebarTop = filter.fBottom + 8.0f;
    float sidebarBottom = getHeight() - kPanelPadding - kFooterHeight - 8.0f;
    
    return SkRect::MakeXYWH(kPanelPadding, sidebarTop,
                           kSidebarWidth, sidebarBottom - sidebarTop);
}

SkRect PresetBrowserComponent::getPresetListRect() const
{
    auto sidebar = getSidebarRect();
    auto filter = getFilterBarRect();
    float listTop = filter.fBottom + 8.0f;
    float listBottom = getHeight() - kPanelPadding - kFooterHeight - 8.0f;
    
    return SkRect::MakeXYWH(sidebar.fRight + 8.0f, listTop,
                           getWidth() - sidebar.width() - 3 * kPanelPadding - 8.0f,
                           listBottom - listTop);
}

SkRect PresetBrowserComponent::getPreviewPanelRect() const
{
    // Preview panel slides up from bottom when preset selected
    auto list = getPresetListRect();
    float previewHeight = kPreviewPanelHeight * previewPanelProgress_;
    
    return SkRect::MakeXYWH(list.fLeft, list.fBottom - previewHeight,
                           list.width(), previewHeight);
}

SkRect PresetBrowserComponent::getFooterRect() const
{
    return SkRect::MakeXYWH(kPanelPadding, getHeight() - kPanelPadding - kFooterHeight,
                           getWidth() - 2 * kPanelPadding, kFooterHeight);
}

float PresetBrowserComponent::getContentWidth() const
{
    return getWidth() - 2 * kPanelPadding;
}

float PresetBrowserComponent::getContentHeight() const
{
    return getHeight() - 2 * kPanelPadding;
}

//==============================================================================
// Skia Rendering
//==============================================================================

void PresetBrowserComponent::drawSkia(SkCanvas* canvas)
{
    // Background
    drawBackground(canvas);
    
    // Header
    drawHeader(canvas, getHeaderRect());
    
    // Search bar
    drawSearchBar(canvas, getSearchBarRect());
    
    // Filter bar
    drawFilterBar(canvas, getFilterBarRect());
    
    // Sidebar with categories
    drawSidebar(canvas, getSidebarRect());
    
    // Preset list
    drawPresetList(canvas, getPresetListRect());
    
    // Preview panel (if preset selected)
    if (selectedIndex_ >= 0 && previewPanelProgress_ > 0.01f) {
        drawPreviewPanel(canvas, getPreviewPanelRect());
    }
    
    // Footer
    drawFooter(canvas, getFooterRect());
    
    // Scrollbar
    if (maxScrollOffset_ > 0.0f) {
        drawScrollBar(canvas);
    }
}

void PresetBrowserComponent::drawBackground(SkCanvas* canvas)
{
    // Main background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(colors::BACKGROUND_SECONDARY));
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
    
    // Subtle grid pattern
    SkPaint gridPaint;
    gridPaint.setColor(SkColorSetA(toSkColor(colors::BORDER_DEFAULT), 30));
    gridPaint.setStrokeWidth(1.0f);
    
    const float gridSize = 40.0f;
    for (float x = 0; x < getWidth(); x += gridSize) {
        canvas->drawLine(x, 0, x, getHeight(), gridPaint);
    }
    for (float y = 0; y < getHeight(); y += gridSize) {
        canvas->drawLine(0, y, getWidth(), y, gridPaint);
    }
}

void PresetBrowserComponent::drawHeader(SkCanvas* canvas, const SkRect& bounds)
{
    // Title
    SkFont titleFont = getSkFont(24.0f, FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(toSkColor(colors::TEXT_PRIMARY));
    titlePaint.setAntiAlias(true);
    
    canvas->drawString("Preset Browser", bounds.fLeft, bounds.fTop + 28.0f, 
                      titleFont, titlePaint);
    
    // Subtitle with count
    SkFont subtitleFont = getSkFont(12.0f, FontWeight::Regular);
    SkPaint subtitlePaint;
    subtitlePaint.setColor(toSkColor(colors::TEXT_SECONDARY));
    subtitlePaint.setAntiAlias(true);
    
    juce::String subtitle = juce::String(filteredPresets_.size()) + " presets";
    if (!currentInstrumentId_.isEmpty()) {
        subtitle += " for " + currentInstrumentId_;
    }
    
    canvas->drawString(subtitle.toStdString().c_str(), 
                      bounds.fLeft, bounds.fTop + 48.0f,
                      subtitleFont, subtitlePaint);
    
    // Mode indicator (if not in browser mode)
    if (currentMode_ != PresetBrowserMode::Browser) {
        SkPaint modeBg;
        modeBg.setColor(toSkColor(colors::ACCENT_PRIMARY));
        modeBg.setAntiAlias(true);
        
        juce::String modeText = currentMode_ == PresetBrowserMode::Saver ? "Save Mode" : "Manage Mode";
        SkFont modeFont = getSkFont(11.0f, FontWeight::SemiBold);
        
        float textWidth = modeFont.measureText(modeText.toRawUTF8(), modeText.getNumBytesAsUTF8());
        float padding = 8.0f;
        SkRect modeRect = SkRect::MakeXYWH(bounds.fRight - textWidth - 2 * padding - 4.0f, 
                                           bounds.fTop + 8.0f,
                                           textWidth + 2 * padding, 24.0f);
        
        canvas->drawRoundRect(modeRect, 4.0f, 4.0f, modeBg);
        
        SkPaint modeTextPaint;
        modeTextPaint.setColor(SK_ColorWHITE);
        modeTextPaint.setAntiAlias(true);
        canvas->drawString(modeText.toStdString().c_str(),
                          modeRect.fLeft + padding, modeRect.fTop + 17.0f,
                          modeFont, modeTextPaint);
    }
}

void PresetBrowserComponent::drawSearchBar(SkCanvas* canvas, const SkRect& bounds)
{
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(withAlpha(colors::BACKGROUND_TERTIARY, 0.8f)));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);
    
    // Border (subtle glow when focused)
    if (isSearchBarFocused_) {
        SkPaint glowPaint;
        glowPaint.setColor(toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.5f)));
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(2.0f);
        glowPaint.setAntiAlias(true);
        canvas->drawRoundRect(bounds, 8.0f, 8.0f, glowPaint);
    } else {
        SkPaint borderPaint;
        borderPaint.setColor(toSkColor(withAlpha(colors::BORDER_DEFAULT, 0.3f)));
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(1.0f);
        borderPaint.setAntiAlias(true);
        canvas->drawRoundRect(bounds, 8.0f, 8.0f, borderPaint);
    }
    
    // Search icon
    SkPaint iconPaint;
    iconPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
    iconPaint.setAntiAlias(true);
    iconPaint.setStrokeWidth(2.0f);
    iconPaint.setStyle(SkPaint::kStroke_Style);
    
    float iconX = bounds.fLeft + 14.0f;
    float iconY = bounds.centerY();
    canvas->drawCircle(iconX, iconY, 6.0f, iconPaint);
    canvas->drawLine(iconX + 4.0f, iconY + 4.0f, iconX + 10.0f, iconY + 10.0f, iconPaint);
}

void PresetBrowserComponent::drawFilterBar(SkCanvas* canvas, const SkRect& bounds)
{
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(withAlpha(colors::BACKGROUND_TERTIARY, 0.4f)));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 6.0f, 6.0f, bgPaint);
    
    // Sort dropdown label
    SkFont labelFont = getSkFont(11.0f, FontWeight::Medium);
    SkPaint labelPaint;
    labelPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
    labelPaint.setAntiAlias(true);
    
    canvas->drawString("Sort by:", bounds.fLeft + 12.0f, bounds.centerY() + 4.0f,
                      labelFont, labelPaint);
    
    // Current sort criteria
    SkFont valueFont = getSkFont(11.0f, FontWeight::SemiBold);
    SkPaint valuePaint;
    valuePaint.setColor(toSkColor(colors::TEXT_PRIMARY));
    valuePaint.setAntiAlias(true);
    
    juce::String sortNames[] = {"Name", "Category", "Author", "Date", "Rating", "Popular"};
    canvas->drawString(sortNames[static_cast<int>(sortCriteria_)].toStdString().c_str(),
                      bounds.fLeft + 60.0f, bounds.centerY() + 4.0f,
                      valueFont, valuePaint);
    
    // Filter badges (right side)
    float badgeX = bounds.fRight - 8.0f;
    
    if (showOnlyFavorites_) {
        juce::String badge = "Favorites";
        float width = valueFont.measureText(badge.toRawUTF8(), badge.getNumBytesAsUTF8()) + 16.0f;
        badgeX -= width;
        
        SkPaint badgePaint;
        badgePaint.setColor(toSkColor(withAlpha(colors::ACCENT_SECONDARY, 0.3f)));
        badgePaint.setAntiAlias(true);
        canvas->drawRoundRect(SkRect::MakeXYWH(badgeX, bounds.centerY() - 10.0f, width, 20.0f),
                             10.0f, 10.0f, badgePaint);
        
        SkPaint badgeTextPaint;
        badgeTextPaint.setColor(toSkColor(colors::ACCENT_SECONDARY));
        badgeTextPaint.setAntiAlias(true);
        canvas->drawString(badge.toStdString().c_str(), badgeX + 8.0f, 
                          bounds.centerY() + 4.0f, valueFont, badgeTextPaint);
    }
    
    if (showOnlyUserPresets_) {
        juce::String badge = "User Only";
        float width = valueFont.measureText(badge.toRawUTF8(), badge.getNumBytesAsUTF8()) + 16.0f;
        badgeX -= (width + 8.0f);
        
        SkPaint badgePaint;
        badgePaint.setColor(toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.3f)));
        badgePaint.setAntiAlias(true);
        canvas->drawRoundRect(SkRect::MakeXYWH(badgeX, bounds.centerY() - 10.0f, width, 20.0f),
                             10.0f, 10.0f, badgePaint);
        
        SkPaint badgeTextPaint;
        badgeTextPaint.setColor(toSkColor(colors::ACCENT_PRIMARY));
        badgeTextPaint.setAntiAlias(true);
        canvas->drawString(badge.toStdString().c_str(), badgeX + 8.0f,
                          bounds.centerY() + 4.0f, valueFont, badgeTextPaint);
    }
}

void PresetBrowserComponent::drawSidebar(SkCanvas* canvas, const SkRect& bounds)
{
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(withAlpha(colors::BACKGROUND_TERTIARY, 0.3f)));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);
    
    // Categories header
    SkFont headerFont = getSkFont(12.0f, FontWeight::Bold);
    SkPaint headerPaint;
    headerPaint.setColor(toSkColor(colors::TEXT_PRIMARY));
    headerPaint.setAntiAlias(true);
    
    canvas->drawString("Categories", bounds.fLeft + 12.0f, bounds.fTop + 20.0f,
                      headerFont, headerPaint);
    
    // Category buttons
    float y = bounds.fTop + 36.0f;
    for (size_t i = 0; i < categories_.size() && y < bounds.fBottom - 20.0f; ++i) {
        SkRect catRect = SkRect::MakeXYWH(bounds.fLeft + 8.0f, y, 
                                          bounds.width() - 16.0f, 28.0f);
        bool isSelected = (categoryFilter_ == categories_[i].name);
        bool isHovered = (static_cast<int>(i) == hoveredCategoryIndex_);
        
        drawCategoryButton(canvas, catRect, categories_[i], isSelected);
        
        y += 32.0f;
    }
}

void PresetBrowserComponent::drawCategoryButton(SkCanvas* canvas, const SkRect& bounds,
                                               const CategoryInfo& category, bool isSelected)
{
    // Background
    SkPaint bgPaint;
    if (isSelected) {
        bgPaint.setColor(SkColorSetA(category.color, 80));
    } else if (static_cast<int>(&category - &categories_[0]) == hoveredCategoryIndex_) {
        bgPaint.setColor(toSkColor(withAlpha(colors::BACKGROUND_SECONDARY, 0.6f)));
    } else {
        bgPaint.setColor(toSkColor(withAlpha(colors::BACKGROUND_SECONDARY, 0.3f)));
    }
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 6.0f, 6.0f, bgPaint);
    
    // Color indicator
    SkPaint colorPaint;
    colorPaint.setColor(category.color);
    colorPaint.setAntiAlias(true);
    canvas->drawCircle(bounds.fLeft + 12.0f, bounds.centerY(), 5.0f, colorPaint);
    
    // Name
    SkFont nameFont = getSkFont(11.0f, isSelected ? FontWeight::SemiBold : FontWeight::Regular);
    SkPaint namePaint;
    namePaint.setColor(isSelected ? SK_ColorWHITE : toSkColor(colors::TEXT_PRIMARY));
    namePaint.setAntiAlias(true);
    
    canvas->drawString(category.name.toStdString().c_str(),
                      bounds.fLeft + 24.0f, bounds.centerY() + 4.0f,
                      nameFont, namePaint);
    
    // Count
    SkFont countFont = getSkFont(10.0f, FontWeight::Regular);
    SkPaint countPaint;
    countPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
    countPaint.setAntiAlias(true);
    
    juce::String countStr = juce::String(category.count);
    float countWidth = countFont.measureText(countStr.toRawUTF8(), countStr.getNumBytesAsUTF8());
    canvas->drawString(countStr.toStdString().c_str(),
                      bounds.fRight - countWidth - 8.0f, bounds.centerY() + 4.0f,
                      countFont, countPaint);
}

void PresetBrowserComponent::drawPresetList(SkCanvas* canvas, const SkRect& bounds)
{
    if (filteredPresets_.empty()) {
        drawEmptyState(canvas, bounds);
        return;
    }
    
    // Clip to list bounds
    canvas->save();
    canvas->clipRect(bounds);
    
    // Draw presets
    float y = bounds.fTop - scrollOffset_;
    for (size_t i = 0; i < filteredPresets_.size(); ++i) {
        if (y + kPresetItemHeight < bounds.fTop) {
            y += kPresetItemHeight + kPresetItemPadding;
            continue;
        }
        if (y > bounds.fBottom)
            break;
        
        SkRect itemRect = SkRect::MakeXYWH(bounds.fLeft, y, 
                                          bounds.width() - kScrollBarWidth - 4.0f, 
                                          kPresetItemHeight);
        
        drawPresetItem(canvas, itemRect, filteredPresets_[i], static_cast<int>(i));
        
        y += kPresetItemHeight + kPresetItemPadding;
    }
    
    canvas->restore();
}

void PresetBrowserComponent::drawPresetItem(SkCanvas* canvas, const SkRect& bounds,
                                           const DisplayPreset& preset, int index)
{
    bool isSelected = (index == selectedIndex_);
    bool isHovered = (index == hoveredIndex_);
    
    // Background with animation
    float hoverAlpha = preset.hoverProgress;
    float selectionAlpha = preset.selectionProgress;
    
    SkPaint bgPaint;
    SkColor baseColor = toSkColor(colors::BACKGROUND_TERTIARY);
    SkColor hoverColor = toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.15f));
    SkColor selectColor = toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.3f));
    
    SkColor finalColor = baseColor;
    finalColor = SkColorSetA(hoverColor, 
        static_cast<uint8_t>(SkColorGetA(hoverColor) * hoverAlpha + SkColorGetA(finalColor) * (1.0f - hoverAlpha)));
    finalColor = SkColorSetA(selectColor,
        static_cast<uint8_t>(SkColorGetA(selectColor) * selectionAlpha + SkColorGetA(finalColor) * (1.0f - selectionAlpha)));
    
    bgPaint.setColor(finalColor);
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);
    
    // Border for selected
    if (selectionAlpha > 0.01f) {
        SkPaint borderPaint;
        borderPaint.setColor(SkColorSetA(toSkColor(colors::ACCENT_PRIMARY), 
                                        static_cast<uint8_t>(200 * selectionAlpha)));
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(2.0f);
        borderPaint.setAntiAlias(true);
        canvas->drawRoundRect(bounds, 8.0f, 8.0f, borderPaint);
    }
    
    // Favorite star
    if (preset.isFavorite) {
        SkPaint starPaint;
        starPaint.setColor(toSkColor(colors::ACCENT_SECONDARY));
        starPaint.setAntiAlias(true);
        
        float starX = bounds.fLeft + 14.0f;
        float starY = bounds.fTop + 16.0f;
        
        // Simple star shape
        canvas->drawCircle(starX, starY, 5.0f, starPaint);
    }
    
    // Preset name
    SkFont nameFont = getSkFont(13.0f, FontWeight::SemiBold);
    SkPaint namePaint;
    namePaint.setColor(isSelected ? toSkColor(colors::ACCENT_PRIMARY) : toSkColor(colors::TEXT_PRIMARY));
    namePaint.setAntiAlias(true);
    
    float nameX = preset.isFavorite ? bounds.fLeft + 28.0f : bounds.fLeft + 12.0f;
    canvas->drawString(preset.metadata.name.toStdString().c_str(),
                      nameX, bounds.fTop + 20.0f, nameFont, namePaint);
    
    // Category badge
    SkFont catFont = getSkFont(9.0f, FontWeight::Medium);
    SkPaint catPaint;
    catPaint.setAntiAlias(true);
    
    // Find category color
    SkColor catColor = SkColorSetRGB(150, 150, 150);
    for (const auto& cat : categories_) {
        if (cat.name == preset.metadata.category) {
            catColor = cat.color;
            break;
        }
    }
    
    catPaint.setColor(SkColorSetA(catColor, 60));
    
    juce::String catText = preset.metadata.category;
    float catWidth = catFont.measureText(catText.toRawUTF8(), catText.getNumBytesAsUTF8()) + 12.0f;
    SkRect catRect = SkRect::MakeXYWH(nameX, bounds.fTop + 26.0f, catWidth, 16.0f);
    canvas->drawRoundRect(catRect, 4.0f, 4.0f, catPaint);
    
    SkPaint catTextPaint;
    catTextPaint.setColor(catColor);
    catTextPaint.setAntiAlias(true);
    canvas->drawString(catText.toStdString().c_str(),
                      catRect.fLeft + 6.0f, catRect.fTop + 12.0f,
                      catFont, catTextPaint);
    
    // Author
    SkFont authorFont = getSkFont(10.0f, FontWeight::Regular);
    SkPaint authorPaint;
    authorPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
    authorPaint.setAntiAlias(true);
    
    juce::String authorText = "by " + preset.metadata.author;
    canvas->drawString(authorText.toStdString().c_str(),
                      nameX + catWidth + 12.0f, bounds.fTop + 38.0f,
                      authorFont, authorPaint);
    
    // Rating stars (mini)
    if (preset.metadata.userRating > 0) {
        SkPaint starPaint;
        starPaint.setColor(toSkColor(colors::ACCENT_SECONDARY));
        starPaint.setAntiAlias(true);
        
        float starX = bounds.fRight - 80.0f;
        float starY = bounds.centerY();
        
        for (int i = 0; i < 5; ++i) {
            if (i < preset.metadata.userRating) {
                canvas->drawCircle(starX + i * 10.0f, starY, 3.0f, starPaint);
            }
        }
    }
    
    // Mini waveform preview (placeholder visual)
    SkRect waveRect = SkRect::MakeXYWH(bounds.fRight - 140.0f, bounds.fTop + 12.0f,
                                       50.0f, kPresetItemHeight - 24.0f);
    std::vector<float> dummySamples(20);
    for (size_t i = 0; i < dummySamples.size(); ++i) {
        dummySamples[i] = 0.5f + 0.4f * std::sin(i * 0.8f + index);
    }
    drawMiniWaveform(canvas, waveRect, dummySamples);
}

void PresetBrowserComponent::drawMiniWaveform(SkCanvas* canvas, const SkRect& bounds,
                                             const std::vector<float>& samples)
{
    if (samples.empty()) return;
    
    SkPaint wavePaint;
    wavePaint.setColor(toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.6f)));
    wavePaint.setAntiAlias(true);
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setStrokeWidth(1.5f);
    
    SkPath path;
    float centerY = bounds.centerY();
    float xStep = bounds.width() / (samples.size() - 1);
    
    path.moveTo(bounds.fLeft, centerY);
    
    for (size_t i = 0; i < samples.size(); ++i) {
        float x = bounds.fLeft + i * xStep;
        float y = centerY - (samples[i] - 0.5f) * bounds.height();
        path.lineTo(x, y);
    }
    
    canvas->drawPath(path, wavePaint);
    
    // Center line
    SkPaint centerPaint;
    centerPaint.setColor(toSkColor(withAlpha(colors::TEXT_SECONDARY, 0.2f)));
    centerPaint.setStrokeWidth(1.0f);
    canvas->drawLine(bounds.fLeft, centerY, bounds.fRight, centerY, centerPaint);
}

void PresetBrowserComponent::drawEmptyState(SkCanvas* canvas, const SkRect& bounds)
{
    // Icon (magnifying glass with X)
    SkPaint iconPaint;
    iconPaint.setColor(toSkColor(withAlpha(colors::TEXT_SECONDARY, 0.3f)));
    iconPaint.setAntiAlias(true);
    iconPaint.setStrokeWidth(3.0f);
    iconPaint.setStyle(SkPaint::kStroke_Style);
    
    float centerX = bounds.centerX();
    float centerY = bounds.centerY() - 20.0f;
    
    canvas->drawCircle(centerX, centerY, 25.0f, iconPaint);
    canvas->drawLine(centerX + 18.0f, centerY + 18.0f, 
                    centerX + 35.0f, centerY + 35.0f, iconPaint);
    
    // Text
    SkFont textFont = getSkFont(14.0f, FontWeight::Medium);
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
    textPaint.setAntiAlias(true);
    
    juce::String message = searchText_.isEmpty() ? "No presets found" : "No presets match your search";
    canvas->drawString(message.toStdString().c_str(), centerX - 80.0f, centerY + 50.0f,
                      textFont, textPaint);
    
    if (!searchText_.isEmpty()) {
        SkFont hintFont = getSkFont(12.0f, FontWeight::Regular);
        juce::String hint = "Try a different search term or clear filters";
        canvas->drawString(hint.toStdString().c_str(), centerX - 100.0f, centerY + 70.0f,
                          hintFont, textPaint);
    }
}

void PresetBrowserComponent::drawPreviewPanel(SkCanvas* canvas, const SkRect& bounds)
{
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(filteredPresets_.size()))
        return;
    
    const auto& preset = filteredPresets_[selectedIndex_];
    
    // Background (glassmorphic)
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(withAlpha(colors::BACKGROUND_TERTIARY, 0.95f)));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, bgPaint);
    
    // Border
    SkPaint borderPaint;
    borderPaint.setColor(toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.3f)));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, 8.0f, 8.0f, borderPaint);
    
    // Name
    SkFont nameFont = getSkFont(16.0f, FontWeight::Bold);
    SkPaint namePaint;
    namePaint.setColor(toSkColor(colors::TEXT_PRIMARY));
    namePaint.setAntiAlias(true);
    
    canvas->drawString(preset.metadata.name.toStdString().c_str(),
                      bounds.fLeft + 16.0f, bounds.fTop + 28.0f,
                      nameFont, namePaint);
    
    // Description (placeholder - would come from metadata)
    SkFont descFont = getSkFont(11.0f, FontWeight::Regular);
    SkPaint descPaint;
    descPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
    descPaint.setAntiAlias(true);
    
    juce::String desc = "Category: " + preset.metadata.category + 
                       " | Author: " + preset.metadata.author +
                       " | Plays: " + juce::String(preset.metadata.playCount);
    canvas->drawString(desc.toStdString().c_str(),
                      bounds.fLeft + 16.0f, bounds.fTop + 48.0f,
                      descFont, descPaint);
    
    // Tags
    float tagX = bounds.fLeft + 16.0f;
    float tagY = bounds.fTop + 70.0f;
    
    for (const auto& tag : preset.metadata.tags) {
        juce::String tagText = "#" + tag;
        float tagWidth = descFont.measureText(tagText.toRawUTF8(), tagText.getNumBytesAsUTF8()) + 12.0f;
        
        if (tagX + tagWidth > bounds.fRight - 16.0f) break;
        
        SkPaint tagBgPaint;
        tagBgPaint.setColor(toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.2f)));
        tagBgPaint.setAntiAlias(true);
        
        SkRect tagRect = SkRect::MakeXYWH(tagX, tagY, tagWidth, 20.0f);
        canvas->drawRoundRect(tagRect, 4.0f, 4.0f, tagBgPaint);
        
        SkPaint tagTextPaint;
        tagTextPaint.setColor(toSkColor(colors::ACCENT_PRIMARY));
        tagTextPaint.setAntiAlias(true);
        canvas->drawString(tagText.toStdString().c_str(),
                          tagX + 6.0f, tagY + 14.0f, descFont, tagTextPaint);
        
        tagX += tagWidth + 8.0f;
    }
}

void PresetBrowserComponent::drawFooter(SkCanvas* canvas, const SkRect& bounds)
{
    // Separator line
    SkPaint linePaint;
    linePaint.setColor(toSkColor(withAlpha(colors::BORDER_DEFAULT, 0.3f)));
    linePaint.setStrokeWidth(1.0f);
    canvas->drawLine(bounds.fLeft, bounds.fTop, bounds.fRight, bounds.fTop, linePaint);
    
    // Action buttons based on mode
    SkFont btnFont = getSkFont(12.0f, FontWeight::SemiBold);
    
    if (currentMode_ == PresetBrowserMode::Browser) {
        // Load button
        bool hasSelection = (selectedIndex_ >= 0);
        
        SkRect loadBtn = SkRect::MakeXYWH(bounds.fRight - 100.0f, bounds.fTop + 12.0f, 90.0f, 32.0f);
        
        SkPaint loadBg;
        loadBg.setColor(hasSelection ? toSkColor(colors::ACCENT_PRIMARY) 
                                     : toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.3f)));
        loadBg.setAntiAlias(true);
        canvas->drawRoundRect(loadBtn, 6.0f, 6.0f, loadBg);
        
        SkPaint loadText;
        loadText.setColor(SK_ColorWHITE);
        loadText.setAntiAlias(true);
        canvas->drawString("Load", loadBtn.centerX() - 15.0f, loadBtn.centerY() + 4.0f,
                          btnFont, loadText);
        
        // Preview button (if available)
        SkRect previewBtn = SkRect::MakeXYWH(bounds.fRight - 200.0f, bounds.fTop + 12.0f, 90.0f, 32.0f);
        
        SkPaint previewBg;
        previewBg.setColor(toSkColor(withAlpha(colors::BACKGROUND_TERTIARY, 0.6f)));
        previewBg.setAntiAlias(true);
        canvas->drawRoundRect(previewBtn, 6.0f, 6.0f, previewBg);
        
        SkPaint previewText;
        previewText.setColor(toSkColor(colors::TEXT_PRIMARY));
        previewText.setAntiAlias(true);
        canvas->drawString("Preview", previewBtn.centerX() - 20.0f, previewBtn.centerY() + 4.0f,
                          btnFont, previewText);
    } else if (currentMode_ == PresetBrowserMode::Saver) {
        // Save button
        SkRect saveBtn = SkRect::MakeXYWH(bounds.fRight - 100.0f, bounds.fTop + 12.0f, 90.0f, 32.0f);
        
        SkPaint saveBg;
        saveBg.setColor(toSkColor(colors::ACCENT_PRIMARY));
        saveBg.setAntiAlias(true);
        canvas->drawRoundRect(saveBtn, 6.0f, 6.0f, saveBg);
        
        SkPaint saveText;
        saveText.setColor(SK_ColorWHITE);
        saveText.setAntiAlias(true);
        canvas->drawString("Save", saveBtn.centerX() - 15.0f, saveBtn.centerY() + 4.0f,
                          btnFont, saveText);
    }
    
    // Cancel/Close button
    SkRect cancelBtn = SkRect::MakeXYWH(bounds.fLeft, bounds.fTop + 12.0f, 80.0f, 32.0f);
    
    SkPaint cancelBg;
    cancelBg.setColor(toSkColor(withAlpha(colors::BACKGROUND_TERTIARY, 0.6f)));
    cancelBg.setAntiAlias(true);
    canvas->drawRoundRect(cancelBtn, 6.0f, 6.0f, cancelBg);
    
    SkPaint cancelText;
    cancelText.setColor(toSkColor(colors::TEXT_SECONDARY));
    cancelText.setAntiAlias(true);
    canvas->drawString("Close", cancelBtn.centerX() - 18.0f, cancelBtn.centerY() + 4.0f,
                      btnFont, cancelText);
}

void PresetBrowserComponent::drawScrollBar(SkCanvas* canvas)
{
    auto listRect = getPresetListRect();
    
    float scrollBarHeight = listRect.height() * (listRect.height() / (maxScrollOffset_ + listRect.height()));
    float maxScrollBarOffset = listRect.height() - scrollBarHeight;
    float scrollBarOffset = (scrollOffset_ / maxScrollOffset_) * maxScrollBarOffset;
    
    SkRect trackRect = SkRect::MakeXYWH(listRect.fRight - kScrollBarWidth - 2.0f,
                                        listRect.fTop,
                                        kScrollBarWidth,
                                        listRect.height());
    
    SkRect thumbRect = SkRect::MakeXYWH(trackRect.fLeft,
                                        trackRect.fTop + scrollBarOffset,
                                        kScrollBarWidth,
                                        std::max(30.0f, scrollBarHeight));
    
    // Track
    SkPaint trackPaint;
    trackPaint.setColor(toSkColor(withAlpha(colors::BACKGROUND_TERTIARY, 0.5f)));
    trackPaint.setAntiAlias(true);
    canvas->drawRoundRect(trackRect, kScrollBarWidth / 2.0f, kScrollBarWidth / 2.0f, trackPaint);
    
    // Thumb
    float alpha = 0.4f + (0.4f * scrollBarHoverProgress_);
    SkPaint thumbPaint;
    thumbPaint.setColor(toSkColor(withAlpha(colors::TEXT_SECONDARY, alpha)));
    thumbPaint.setAntiAlias(true);
    canvas->drawRoundRect(thumbRect, kScrollBarWidth / 2.0f, kScrollBarWidth / 2.0f, thumbPaint);
}

//==============================================================================
// Input Handling
//==============================================================================

void PresetBrowserComponent::resized()
{
    SkiaComponent::resized();
    
    // Update search editor position
    if (searchEditor_) {
        auto searchRect = getSearchBarRect();
        searchEditor_->setBounds(
            static_cast<int>(searchRect.fLeft + 40.0f),
            static_cast<int>(searchRect.fTop + 8.0f),
            static_cast<int>(searchRect.width() - 50.0f),
            static_cast<int>(searchRect.height() - 16.0f)
        );
    }
    
    updateScrollLimits();
}

void PresetBrowserComponent::mouseDown(const juce::MouseEvent& e)
{
    float x = e.x;
    float y = e.y;
    
    // Check search bar
    if (hitTestSearchBar(x, y)) {
        isSearchBarFocused_ = true;
        if (searchEditor_)
            searchEditor_->grabKeyboardFocus();
        markDirty();
        return;
    }
    isSearchBarFocused_ = false;
    
    // Check category buttons
    int catIndex = hitTestCategory(x, y);
    if (catIndex >= 0) {
        if (categoryFilter_ == categories_[catIndex].name)
            categoryFilter_ = juce::String(); // Toggle off
        else
            categoryFilter_ = categories_[catIndex].name;
        applyFilters();
        markDirty();
        return;
    }
    
    // Check preset items
    int presetIndex = hitTestPreset(x, y);
    if (presetIndex >= 0) {
        selectedIndex_ = presetIndex;
        startSelectionAnimation(presetIndex);
        
        // Animate preview panel
        previewPanelProgress_ = 0.0f;
        animateValue("previewPanel", 1.0f, 200);
        
        markDirty();
        return;
    }
    
    // Check scrollbar
    if (hitTestScrollBar(x, y)) {
        isDraggingScrollBar_ = true;
        return;
    }
    
    // Check footer buttons
    int footerBtn = hitTestFooterButton(x, y);
    if (footerBtn == 0) { // Close
        if (onClose)
            onClose();
    } else if (footerBtn == 1) { // Load/Preview
        if (selectedIndex_ >= 0)
            loadSelectedPreset();
    }
}

void PresetBrowserComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    int presetIndex = hitTestPreset(e.x, e.y);
    if (presetIndex >= 0) {
        selectedIndex_ = presetIndex;
        loadSelectedPreset();
    }
}

void PresetBrowserComponent::mouseMove(const juce::MouseEvent& e)
{
    float x = e.x;
    float y = e.y;
    
    // Update hover states
    int oldHover = hoveredIndex_;
    int oldCatHover = hoveredCategoryIndex_;
    
    hoveredIndex_ = hitTestPreset(x, y);
    hoveredCategoryIndex_ = hitTestCategory(x, y);
    
    bool oldSearchHover = isSearchBarHovered_;
    isSearchBarHovered_ = hitTestSearchBar(x, y);
    
    bool oldScrollHover = scrollBarHoverProgress_ > 0.5f;
    bool newScrollHover = hitTestScrollBar(x, y);
    scrollBarHoverProgress_ = newScrollHover ? 1.0f : 0.0f;
    
    // Trigger animations
    if (oldHover != hoveredIndex_) {
        if (oldHover >= 0)
            startHoverAnimation(oldHover, false);
        if (hoveredIndex_ >= 0)
            startHoverAnimation(hoveredIndex_, true);
        markDirty();
    }
    
    if (oldCatHover != hoveredCategoryIndex_ || 
        oldSearchHover != isSearchBarHovered_ ||
        oldScrollHover != newScrollHover) {
        markDirty();
    }
}

void PresetBrowserComponent::mouseExit(const juce::MouseEvent&)
{
    if (hoveredIndex_ >= 0) {
        startHoverAnimation(hoveredIndex_, false);
        hoveredIndex_ = -1;
        markDirty();
    }
    
    hoveredCategoryIndex_ = -1;
    isSearchBarHovered_ = false;
    scrollBarHoverProgress_ = 0.0f;
    markDirty();
}

void PresetBrowserComponent::mouseWheelMove(const juce::MouseEvent&, 
                                            const juce::MouseWheelDetails& wheel)
{
    if (maxScrollOffset_ <= 0.0f) return;
    
    float delta = wheel.deltaY * 50.0f;
    scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_ - delta);
    markDirty();
}

bool PresetBrowserComponent::keyPressed(const juce::KeyPress& key, juce::Component* origin)
{
    // Arrow navigation
    if (key.getKeyCode() == juce::KeyPress::downKey) {
        if (selectedIndex_ < static_cast<int>(filteredPresets_.size()) - 1) {
            selectedIndex_++;
            startSelectionAnimation(selectedIndex_);
            
            // Scroll into view
            float itemTop = selectedIndex_ * (kPresetItemHeight + kPresetItemPadding);
            float itemBottom = itemTop + kPresetItemHeight;
            auto listRect = getPresetListRect();
            
            if (itemBottom > scrollOffset_ + listRect.height()) {
                scrollOffset_ = itemBottom - listRect.height();
            } else if (itemTop < scrollOffset_) {
                scrollOffset_ = itemTop;
            }
            
            markDirty();
        }
        return true;
    }
    
    if (key.getKeyCode() == juce::KeyPress::upKey) {
        if (selectedIndex_ > 0) {
            selectedIndex_--;
            startSelectionAnimation(selectedIndex_);
            
            float itemTop = selectedIndex_ * (kPresetItemHeight + kPresetItemPadding);
            if (itemTop < scrollOffset_) {
                scrollOffset_ = itemTop;
            }
            
            markDirty();
        }
        return true;
    }
    
    if (key.getKeyCode() == juce::KeyPress::returnKey) {
        if (selectedIndex_ >= 0) {
            loadSelectedPreset();
        }
        return true;
    }
    
    if (key.getKeyCode() == juce::KeyPress::escapeKey) {
        if (onClose)
            onClose();
        return true;
    }
    
    // Favorite shortcut
    if (key.getKeyCode() == 'f' || key.getKeyCode() == 'F') {
        toggleFavoriteSelected();
        return true;
    }
    
    return SkiaComponent::keyPressed(key, origin);
}

//==============================================================================
// Hit Testing
//==============================================================================

int PresetBrowserComponent::hitTestPreset(float x, float y) const
{
    auto listRect = getPresetListRect();
    
    if (!listRect.contains(x, y))
        return -1;
    
    float relativeY = y - listRect.fTop + scrollOffset_;
    int index = static_cast<int>(relativeY / (kPresetItemHeight + kPresetItemPadding));
    
    if (index >= 0 && index < static_cast<int>(filteredPresets_.size()))
        return index;
    
    return -1;
}

int PresetBrowserComponent::hitTestCategory(float x, float y) const
{
    auto sidebar = getSidebarRect();
    
    if (!sidebar.contains(x, y))
        return -1;
    
    float startY = sidebar.fTop + 36.0f;
    float relativeY = y - startY;
    int index = static_cast<int>(relativeY / 32.0f);
    
    if (index >= 0 && index < static_cast<int>(categories_.size()))
        return index;
    
    return -1;
}

int PresetBrowserComponent::hitTestTag(float, float) const
{
    // TODO: Implement tag hit testing
    return -1;
}

bool PresetBrowserComponent::hitTestSearchBar(float x, float y) const
{
    return getSearchBarRect().contains(x, y);
}

bool PresetBrowserComponent::hitTestScrollBar(float x, float y) const
{
    auto listRect = getPresetListRect();
    SkRect scrollBarRect = SkRect::MakeXYWH(listRect.fRight - kScrollBarWidth - 2.0f,
                                            listRect.fTop,
                                            kScrollBarWidth,
                                            listRect.height());
    return scrollBarRect.contains(x, y);
}

int PresetBrowserComponent::hitTestRating(float, float) const
{
    // TODO: Implement rating hit testing
    return -1;
}

int PresetBrowserComponent::hitTestFooterButton(float x, float y) const
{
    auto footer = getFooterRect();
    
    if (!footer.contains(x, y))
        return -1;
    
    // Close button
    SkRect closeBtn = SkRect::MakeXYWH(footer.fLeft, footer.fTop + 12.0f, 80.0f, 32.0f);
    if (closeBtn.contains(x, y))
        return 0;
    
    // Load/Preview button
    SkRect loadBtn = SkRect::MakeXYWH(footer.fRight - 100.0f, footer.fTop + 12.0f, 90.0f, 32.0f);
    if (loadBtn.contains(x, y))
        return 1;
    
    return -1;
}

//==============================================================================
// Animation
//==============================================================================

void PresetBrowserComponent::startHoverAnimation(int index, bool hovering)
{
    if (index < 0 || index >= static_cast<int>(filteredPresets_.size()))
        return;
    
    // Simple interpolation - in a real implementation, use the animation system
    filteredPresets_[index].hoverProgress = hovering ? 1.0f : 0.0f;
}

void PresetBrowserComponent::startSelectionAnimation(int index)
{
    if (index < 0 || index >= static_cast<int>(filteredPresets_.size()))
        return;
    
    // Reset all selections
    for (auto& preset : filteredPresets_) {
        preset.selectionProgress = 0.0f;
    }
    
    filteredPresets_[index].selectionProgress = 1.0f;
}

void PresetBrowserComponent::onAnimationTick(float deltaMs)
{
    SkiaComponent::onAnimationTick(deltaMs);
    
    // Animate preview panel
    if (previewPanelProgress_ < 1.0f && selectedIndex_ >= 0) {
        previewPanelProgress_ = getSmoothValue(previewPanelProgress_, 1.0f, deltaMs * 0.005f);
        markDirty();
    } else if (previewPanelProgress_ > 0.0f && selectedIndex_ < 0) {
        previewPanelProgress_ = getSmoothValue(previewPanelProgress_, 0.0f, deltaMs * 0.005f);
        markDirty();
    }
    
    // Animate scrollbar hover
    float targetScrollHover = scrollBarHoverProgress_ > 0.5f ? 1.0f : 0.0f;
    float currentScrollHover = scrollBarHoverProgress_;
    if (std::abs(targetScrollHover - currentScrollHover) > 0.01f) {
        scrollBarHoverProgress_ = getSmoothValue(currentScrollHover, targetScrollHover, deltaMs * 0.01f);
        markDirty();
    }
}

float PresetBrowserComponent::getSmoothValue(float current, float target, float speed)
{
    return current + (target - current) * std::min(speed, 1.0f);
}

} // namespace zenith
