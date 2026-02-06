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

#include "SkiaPresetBrowser.h"
#include "../../design-system/ZenithTheme.h"

namespace zenith::ui {

SkiaPresetBrowser::SkiaPresetBrowser() {
    setWantsKeyboardFocus(true);
    
    // Setup search editor
    searchEditor_ = std::make_unique<juce::TextEditor>();
    searchEditor_->setMultiLine(false);
    searchEditor_->setReturnKeyStartsNewLine(false);
    searchEditor_->setTextToShowWhenEmpty("Search presets...", 
        juce::Colour(ZenithTheme::Colors::text_tertiary.getARGB()));
    searchEditor_->onTextChange = [this]() {
        setSearchQuery(searchEditor_->getText());
    };
    searchEditor_->onEscapeKey = [this]() {
        if (onClose) onClose();
    };
    addAndMakeVisible(searchEditor_.get());
    
    // Initialize with demo categories
    categories_.push_back({"All", "🎹", 0, SkRect::MakeEmpty(), true});
    categories_.push_back({"Bass", "🎸", 0, SkRect::MakeEmpty(), false});
    categories_.push_back({"Lead", "🎺", 0, SkRect::MakeEmpty(), false});
    categories_.push_back({"Pad", "🎵", 0, SkRect::MakeEmpty(), false});
    categories_.push_back({"Pluck", "🎻", 0, SkRect::MakeEmpty(), false});
    categories_.push_back({"FX", "✨", 0, SkRect::MakeEmpty(), false});
    categories_.push_back({"Drums", "🥁", 0, SkRect::MakeEmpty(), false});
    categories_.push_back({"Favorites", "⭐", 0, SkRect::MakeEmpty(), false});
}

SkiaPresetBrowser::~SkiaPresetBrowser() = default;

void SkiaPresetBrowser::setPresetManager(zenith::ZenithPresetManager* manager) {
    presetManager_ = manager;
    loadPresets();
}

void SkiaPresetBrowser::loadPresets() {
    allPresets_.clear();
    
    if (presetManager_) {
        auto presets = presetManager_->getAllPresets();
        for (const auto& preset : presets) {
            PresetListItem item;
            item.metadata = preset;
            allPresets_.push_back(item);
        }
    } else {
        // Demo presets when no manager
        for (int i = 0; i < 20; ++i) {
            PresetListItem item;
            item.metadata.name = "Preset " + juce::String(i + 1);
            item.metadata.author = "Zenith";
            item.metadata.category = (i % 2 == 0) ? "Bass" : "Lead";
            item.metadata.isFavorite = (i % 3 == 0);
            allPresets_.push_back(item);
        }
    }
    
    filterPresets();
    updateCategories();
}

void SkiaPresetBrowser::refreshPresets() {
    loadPresets();
}

void SkiaPresetBrowser::setSearchQuery(const juce::String& query) {
    searchQuery_ = query.toLowerCase();
    filterPresets();
    repaint();
}

void SkiaPresetBrowser::selectCategory(const juce::String& category) {
    selectedCategory_ = category;
    
    for (auto& cat : categories_) {
        cat.isSelected = (cat.name == category);
    }
    
    filterPresets();
    repaint();
}

void SkiaPresetBrowser::filterPresets() {
    filteredPresets_.clear();
    
    for (auto& preset : allPresets_) {
        bool matchesSearch = searchQuery_.isEmpty() || 
            preset.metadata.name.toLowerCase().contains(searchQuery_) ||
            preset.metadata.author.toLowerCase().contains(searchQuery_);
        
        bool matchesCategory = selectedCategory_.isEmpty() || 
            selectedCategory_ == "All" ||
            preset.metadata.category == selectedCategory_ ||
            (selectedCategory_ == "Favorites" && preset.metadata.isFavorite);
        
        if (matchesSearch && matchesCategory) {
            filteredPresets_.push_back(preset);
        }
    }
    
    // Update max scroll
    float contentHeight = filteredPresets_.size() * kPresetItemHeight;
    float viewHeight = getHeight() - kSearchHeight;
    maxScroll_ = std::max(0.0f, contentHeight - viewHeight);
    scrollOffset_ = std::min(scrollOffset_, maxScroll_);
}

void SkiaPresetBrowser::updateCategories() {
    // Update counts
    for (auto& cat : categories_) {
        if (cat.name == "All") {
            cat.count = static_cast<int>(allPresets_.size());
        } else if (cat.name == "Favorites") {
            cat.count = 0;
            for (const auto& p : allPresets_) {
                if (p.metadata.isFavorite) cat.count++;
            }
        } else {
            cat.count = 0;
            for (const auto& p : allPresets_) {
                if (p.metadata.category == cat.name) cat.count++;
            }
        }
    }
}

void SkiaPresetBrowser::selectPreset(int index) {
    if (index >= 0 && index < static_cast<int>(filteredPresets_.size())) {
        // Deselect previous
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(filteredPresets_.size())) {
            filteredPresets_[selectedIndex_].isSelected = false;
        }
        
        selectedIndex_ = index;
        filteredPresets_[index].isSelected = true;
        
        if (onPresetSelected) {
            onPresetSelected(filteredPresets_[index].metadata);
        }
        
        repaint();
    }
}

void SkiaPresetBrowser::loadSelectedPreset() {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(filteredPresets_.size())) {
        if (onPresetLoad) {
            onPresetLoad(filteredPresets_[selectedIndex_].metadata);
        }
    }
}

void SkiaPresetBrowser::toggleFavorite(int index) {
    if (index >= 0 && index < static_cast<int>(filteredPresets_.size())) {
        filteredPresets_[index].metadata.isFavorite = !filteredPresets_[index].metadata.isFavorite;
        updateCategories();
        repaint();
    }
}

void SkiaPresetBrowser::resized() {
    auto bounds = getLocalBounds();
    
    // Search bar at top
    if (searchEditor_) {
        searchEditor_->setBounds(
            static_cast<int>(kSidebarWidth + kPadding),
            static_cast<int>(kPadding),
            bounds.getWidth() - static_cast<int>(kSidebarWidth + kPadding * 2),
            30
        );
    }
}

void SkiaPresetBrowser::drawSkia(SkCanvas* canvas) {
    drawBackground(canvas);
    drawSidebar(canvas);
    drawSearchBar(canvas);
    drawPresetList(canvas);
}

void SkiaPresetBrowser::drawBackground(SkCanvas* canvas) {
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
}

void SkiaPresetBrowser::drawSidebar(SkCanvas* canvas) {
    SkRect sidebarRect = SkRect::MakeXYWH(0, 0, kSidebarWidth, getHeight());
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_01.getARGB());
    canvas->drawRect(sidebarRect, bgPaint);
    
    // Border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    canvas->drawLine(kSidebarWidth, 0, kSidebarWidth, getHeight(), borderPaint);
    
    // Title
    SkPaint titlePaint;
    titlePaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
    titlePaint.setAntiAlias(true);
    SkFont titleFont = design::typography::getSkFont(14.0f, design::FontWeight::Bold);
    canvas->drawString("Categories", 16, 30, titleFont, titlePaint);
    
    // Categories
    float y = 60;
    for (size_t i = 0; i < categories_.size(); ++i) {
        auto& cat = categories_[i];
        
        SkRect catRect = SkRect::MakeXYWH(8, y - 18, kSidebarWidth - 16, 32);
        cat.bounds = catRect;
        
        // Background
        if (cat.isSelected) {
            SkPaint selectedPaint;
            selectedPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.2f).getARGB());
            selectedPaint.setAntiAlias(true);
            canvas->drawRoundRect(catRect, 6, 6, selectedPaint);
        }
        
        // Icon
        SkFont iconFont = design::typography::getSkFont(16.0f);
        SkPaint iconPaint;
        iconPaint.setColor(ZenithTheme::Colors::text_secondary.getARGB());
        canvas->drawString(cat.icon.toRawUTF8(), 16, y, iconFont, iconPaint);
        
        // Name
        SkFont nameFont = design::typography::getSkFont(12.0f);
        SkPaint namePaint;
        namePaint.setColor(cat.isSelected ? 
            ZenithTheme::Colors::accent_primary.getARGB() : 
            ZenithTheme::Colors::text_secondary.getARGB());
        namePaint.setAntiAlias(true);
        canvas->drawString(cat.name.toRawUTF8(), 40, y, nameFont, namePaint);
        
        // Count
        juce::String countStr = juce::String(cat.count);
        SkFont countFont = design::typography::getSkFont(10.0f);
        SkPaint countPaint;
        countPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
        countPaint.setAntiAlias(true);
        canvas->drawString(countStr.toRawUTF8(), kSidebarWidth - 40, y, countFont, countPaint);
        
        y += 40;
    }
}

void SkiaPresetBrowser::drawSearchBar(SkCanvas* canvas) {
    float x = kSidebarWidth + kPadding;
    float y = kPadding;
    float w = getWidth() - x - kPadding;
    float h = 30;
    
    // Background
    SkRect searchRect = SkRect::MakeXYWH(x, y, w, h);
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_01.getARGB());
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(searchRect, 6, 6, bgPaint);
    
    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1);
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(searchRect, 6, 6, borderPaint);
    
    // Search icon
    SkFont iconFont = design::typography::getSkFont(14.0f);
    SkPaint iconPaint;
    iconPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    iconPaint.setAntiAlias(true);
    canvas->drawString("🔍", x + 10, y + 20, iconFont, iconPaint);
    
    // Result count
    juce::String resultStr = juce::String(filteredPresets_.size()) + " presets";
    SkFont countFont = design::typography::getSkFont(10.0f);
    SkPaint countPaint;
    countPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    countPaint.setAntiAlias(true);
    canvas->drawString(resultStr.toRawUTF8(), x + w - 80, y + 20, countFont, countPaint);
}

void SkiaPresetBrowser::drawPresetList(SkCanvas* canvas) {
    float listX = kSidebarWidth + kPadding;
    float listY = kSearchHeight;
    float listW = getWidth() - listX - kPadding;
    float listH = getHeight() - listY;
    
    // Clip to list area
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(listX, listY, listW, listH));
    
    // Draw presets
    float y = listY - scrollOffset_;
    
    for (size_t i = 0; i < filteredPresets_.size(); ++i) {
        if (y + kPresetItemHeight > listY && y < listY + listH) {
            drawPresetItem(canvas, filteredPresets_[i], static_cast<int>(i), 
                          listX, y, listW);
        }
        y += kPresetItemHeight;
    }
    
    // Empty state
    if (filteredPresets_.empty()) {
        SkPaint emptyPaint;
        emptyPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
        emptyPaint.setAntiAlias(true);
        SkFont emptyFont = design::typography::getSkFont(14.0f);
        canvas->drawString("No presets found", listX + 20, listY + 50, emptyFont, emptyPaint);
    }
    
    canvas->restore();
    
    // Scrollbar
    if (maxScroll_ > 0) {
        float scrollbarHeight = (listH / (listH + maxScroll_)) * listH;
        float scrollbarY = listY + (scrollOffset_ / maxScroll_) * (listH - scrollbarHeight);
        
        SkPaint scrollbarPaint;
        scrollbarPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
        scrollbarPaint.setAntiAlias(true);
        canvas->drawRoundRect(
            SkRect::MakeXYWH(getWidth() - 8, scrollbarY, 4, scrollbarHeight),
            2, 2, scrollbarPaint
        );
    }
}

void SkiaPresetBrowser::drawPresetItem(SkCanvas* canvas, const PresetListItem& item, 
                                        int index, float x, float y, float w) {
    SkRect itemRect = SkRect::MakeXYWH(x, y, w, kPresetItemHeight - 8);
    
    // Background
    if (item.isSelected) {
        SkPaint selectedPaint;
        selectedPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.15f).getARGB());
        selectedPaint.setAntiAlias(true);
        canvas->drawRoundRect(itemRect, 8, 8, selectedPaint);
        
        // Border
        SkPaint borderPaint;
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(1.5f);
        borderPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        borderPaint.setAntiAlias(true);
        canvas->drawRoundRect(itemRect, 8, 8, borderPaint);
    } else if (index == hoveredIndex_) {
        SkPaint hoverPaint;
        hoverPaint.setColor(ZenithTheme::Colors::hover_overlay.getARGB());
        hoverPaint.setAntiAlias(true);
        canvas->drawRoundRect(itemRect, 8, 8, hoverPaint);
    }
    
    // Icon
    SkFont iconFont = design::typography::getSkFont(24.0f);
    SkPaint iconPaint;
    iconPaint.setColor(ZenithTheme::Colors::text_secondary.getARGB());
    canvas->drawString("🎹", x + 12, y + 38, iconFont, iconPaint);
    
    // Name
    SkFont nameFont = design::typography::getSkFont(13.0f, design::FontWeight::Medium);
    SkPaint namePaint;
    namePaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
    namePaint.setAntiAlias(true);
    canvas->drawString(item.metadata.name.toRawUTF8(), x + 50, y + 28, nameFont, namePaint);
    
    // Author & Category
    juce::String infoStr = item.metadata.author + " • " + item.metadata.category;
    SkFont infoFont = design::typography::getSkFont(11.0f);
    SkPaint infoPaint;
    infoPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    infoPaint.setAntiAlias(true);
    canvas->drawString(infoStr.toRawUTF8(), x + 50, y + 48, infoFont, infoPaint);
    
    // Favorite star
    if (item.metadata.isFavorite) {
        SkFont favFont = design::typography::getSkFont(14.0f);
        SkPaint favPaint;
        favPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        favPaint.setAntiAlias(true);
        canvas->drawString("⭐", x + w - 30, y + 35, favFont, favPaint);
    }
}

void SkiaPresetBrowser::mouseDown(const juce::MouseEvent& e) {
    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);
    
    // Check category click
    int catIndex = hitTestCategory(y);
    if (catIndex >= 0) {
        selectCategory(categories_[catIndex].name);
        return;
    }
    
    // Check preset click
    int presetIndex = hitTestPreset(y);
    if (presetIndex >= 0) {
        if (e.mods.isDoubleClick()) {
            selectPreset(presetIndex);
            loadSelectedPreset();
        } else {
            selectPreset(presetIndex);
        }
        return;
    }
}

void SkiaPresetBrowser::mouseMove(const juce::MouseEvent& e) {
    float y = static_cast<float>(e.y);
    
    int oldHover = hoveredIndex_;
    hoveredIndex_ = hitTestPreset(y);
    
    if (oldHover != hoveredIndex_) {
        repaint();
    }
}

void SkiaPresetBrowser::mouseWheelMove(const juce::MouseEvent& e, 
                                        const juce::MouseWheelDetails& wheel) {
    juce::ignoreUnused(e);
    
    float delta = wheel.deltaY * 50.0f;
    scrollOffset_ = std::clamp(scrollOffset_ - delta, 0.0f, maxScroll_);
    repaint();
}

bool SkiaPresetBrowser::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    juce::ignoreUnused(origin);
    
    if (key == juce::KeyPress::upKey) {
        if (selectedIndex_ > 0) {
            selectPreset(selectedIndex_ - 1);
        }
        return true;
    }
    
    if (key == juce::KeyPress::downKey) {
        if (selectedIndex_ < static_cast<int>(filteredPresets_.size()) - 1) {
            selectPreset(selectedIndex_ + 1);
        }
        return true;
    }
    
    if (key == juce::KeyPress::returnKey) {
        loadSelectedPreset();
        return true;
    }
    
    if (key == juce::KeyPress::escapeKey) {
        if (onClose) onClose();
        return true;
    }
    
    return false;
}

int SkiaPresetBrowser::hitTestPreset(float y) const {
    float listY = kSearchHeight;
    float itemY = y - listY + scrollOffset_;
    int index = static_cast<int>(itemY / kPresetItemHeight);
    
    if (index >= 0 && index < static_cast<int>(filteredPresets_.size())) {
        return index;
    }
    return -1;
}

int SkiaPresetBrowser::hitTestCategory(float y) const {
    if (x > kSidebarWidth) return -1;
    
    float catY = 60;
    for (size_t i = 0; i < categories_.size(); ++i) {
        if (y >= catY - 18 && y < catY + 14) {
            return static_cast<int>(i);
        }
        catY += 40;
    }
    return -1;
}

} // namespace zenith::ui
