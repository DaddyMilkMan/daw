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

#include "../../framework/SkiaComponent.h"
#include "../../design-system/ZenithTheme.h"
#include "../../../instruments/ZenithPresetManager.h"
#include <functional>
#include <vector>

namespace zenith::ui {

struct PresetListItem {
    PresetMetadata metadata;
    SkRect bounds;
    bool isHovered = false;
    bool isSelected = false;
    bool isLoaded = false;
};

struct CategoryItem {
    juce::String name;
    juce::String icon;
    int count = 0;
    SkRect bounds;
    bool isSelected = false;
};

class SkiaPresetBrowser : public SkiaComponent {
public:
    SkiaPresetBrowser();
    ~SkiaPresetBrowser() override;
    
    void setPresetManager(zenith::ZenithPresetManager* manager);
    void loadPresets();
    void refreshPresets();
    
    void setSearchQuery(const juce::String& query);
    void selectCategory(const juce::String& category);
    
    void selectPreset(int index);
    void loadSelectedPreset();
    void toggleFavorite(int index);
    
    std::function<void(const PresetMetadata&)> onPresetLoad;
    std::function<void(const PresetMetadata&)> onPresetSelected;
    std::function<void()> onClose;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
private:
    static constexpr float kSidebarWidth = 200.0f;
    static constexpr float kSearchHeight = 50.0f;
    static constexpr float kPresetItemHeight = 72.0f;
    static constexpr float kPadding = 16.0f;
    
    zenith::ZenithPresetManager* presetManager_ = nullptr;
    
    std::vector<PresetListItem> allPresets_;
    std::vector<PresetListItem> filteredPresets_;
    std::vector<CategoryItem> categories_;
    
    juce::String searchQuery_;
    juce::String selectedCategory_;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    
    float scrollOffset_ = 0.0f;
    float maxScroll_ = 0.0f;
    
    std::unique_ptr<juce::TextEditor> searchEditor_;
    
    void drawBackground(SkCanvas* canvas);
    void drawSidebar(SkCanvas* canvas);
    void drawSearchBar(SkCanvas* canvas);
    void drawPresetList(SkCanvas* canvas);
    void drawPresetItem(SkCanvas* canvas, const PresetListItem& item, int index);
    
    void filterPresets();
    void updateCategories();
    
    int hitTestPreset(float y) const;
    int hitTestCategory(float y) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPresetBrowser)
};

} // namespace zenith::ui
