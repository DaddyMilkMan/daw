/*
  ==============================================================================

    BrowserPanel.cpp
    Created: 2025-11-28
    Author:  Cassandra "Crash" O'Malley

    Robust preset browser implementation.

  ==============================================================================
*/

#include "BrowserPanel.h"

#ifdef ZENITH_USE_SKIA

namespace zenith {

BrowserPanel::BrowserPanel() {
    setSize(300, 600);
}

void BrowserPanel::resized() {
    // Layout handled in drawSkia
}

void BrowserPanel::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    
    SkPaint paint;
    paint.setAntiAlias(true);
    
    // Background
    paint.setColor(SkColorSetARGB(200, 20, 20, 25));
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), paint);
    
    // Header
    paint.setColor(SkColorSetARGB(255, 30, 30, 35));
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), headerHeight_), paint);
    
    SkFont headerFont;
    headerFont.setSize(16.0f);
    paint.setColor(SK_ColorWHITE);
    canvas->drawString("PRESETS", 10, 25, headerFont, paint);
    
    // Search box
    searchBoxBounds_ = juce::Rectangle<int>(10, headerHeight_ + 5, getWidth() - 20, searchBoxHeight_);
    paint.setColor(SkColorSetARGB(100, 255, 255, 255));
    canvas->drawRoundRect(SkRect::MakeXYWH(searchBoxBounds_.getX(), searchBoxBounds_.getY(), 
                                           searchBoxBounds_.getWidth(), searchBoxBounds_.getHeight()), 
                         4.0f, 4.0f, paint);
    
    // Search text
    SkFont searchFont;
    searchFont.setSize(14.0f);
    paint.setColor(searchText_.isEmpty() ? SkColorSetARGB(100, 255, 255, 255) : SK_ColorWHITE);
    juce::String displayText = searchText_.isEmpty() ? "Search..." : searchText_;
    canvas->drawString(displayText.toStdString().c_str(), searchBoxBounds_.getX() + 8, 
                      searchBoxBounds_.getY() + 20, searchFont, paint);
    
    // Draw preset list
    SkFont itemFont;
    itemFont.setSize(14.0f);
    
    int listY = headerHeight_ + searchBoxHeight_ + 10;
    int visibleStart = scrollOffset_;
    int maxVisible = (getHeight() - listY) / itemHeight_;
    int visibleEnd = juce::jmin(visibleStart + maxVisible, filteredPresets_.size());
    
    for (int i = visibleStart; i < visibleEnd; ++i) {
        int itemY = listY + (i - scrollOffset_) * itemHeight_;
        auto itemBounds = juce::Rectangle<int>(0, itemY, getWidth(), itemHeight_);
        
        // Background for selected/hovered
        if (i == selectedIndex_) {
            paint.setColor(SkColorSetARGB(150, 0, 255, 255));
            canvas->drawRect(SkRect::MakeXYWH(itemBounds.getX(), itemBounds.getY(),
                                             itemBounds.getWidth(), itemBounds.getHeight()), paint);
        } else if (i == hoverIndex_) {
            paint.setColor(SkColorSetARGB(50, 255, 255, 255));
            canvas->drawRect(SkRect::MakeXYWH(itemBounds.getX(), itemBounds.getY(),
                                             itemBounds.getWidth(), itemBounds.getHeight()), paint);
        }
        
        // Text
        paint.setColor(i == selectedIndex_ ? SkColorSetRGB(0, 255, 255) : SK_ColorWHITE);
        canvas->drawString(filteredPresets_[i].toStdString().c_str(),
                          itemBounds.getX() + 10.0f,
                          itemBounds.getCentreY() + 5.0f,
                          itemFont, paint);
    }
    
    // Scrollbar
    if (filteredPresets_.size() > maxVisible) {
        float scrollbarHeight = ((float)maxVisible / (float)filteredPresets_.size()) * (getHeight() - listY);
        float scrollbarY = listY + ((float)scrollOffset_ / (float)filteredPresets_.size()) * (getHeight() - listY);
        
        paint.setColor(SkColorSetARGB(100, 255, 255, 255));
        canvas->drawRect(SkRect::MakeXYWH(bounds.getWidth() - 5.0f, scrollbarY, 5.0f, scrollbarHeight), paint);
    }
}

void BrowserPanel::setPresets(const juce::StringArray& presets) {
    allPresets_ = presets;
    updateFilteredList();
}

void BrowserPanel::setSearchText(const juce::String& text) {
    searchText_ = text;
    updateFilteredList();
    if (onSearchChanged) {
        onSearchChanged(text);
    }
}

void BrowserPanel::updateFilteredList() {
    filteredPresets_.clear();
    
    if (searchText_.isEmpty()) {
        filteredPresets_ = allPresets_;
    } else {
        for (const auto& preset : allPresets_) {
            if (preset.containsIgnoreCase(searchText_)) {
                filteredPresets_.add(preset);
            }
        }
    }
    
    selectedIndex_ = -1;
    scrollOffset_ = 0;
    repaint();
}

bool BrowserPanel::keyPressed(const juce::KeyPress& key) {
    // Handle typing in search box
    if (key.isKeyCode(juce::KeyPress::backspaceKey)) {
        if (searchText_.isNotEmpty()) {
            setSearchText(searchText_.dropLastCharacters(1));
        }
        return true;
    } else if (key.isKeyCode(juce::KeyPress::deleteKey)) {
        setSearchText("");
        return true;
    } else if (key.getTextCharacter() >= 32 && key.getTextCharacter() < 127) {
        setSearchText(searchText_ + juce::String::charToString(key.getTextCharacter()));
        return true;
    }
    return false;
}

void BrowserPanel::updateFilteredList() {
    filteredPresets_.clear();
    
    if (currentFilter_.isEmpty()) {
        filteredPresets_ = allPresets_;
    } else {
        for (const auto& preset : allPresets_) {
            if (preset.containsIgnoreCase(currentFilter_)) {
                filteredPresets_.add(preset);
            }
        }
    }
    
    selectedIndex_ = -1;
    scrollOffset_ = 0;
    repaint();
}

juce::Rectangle<int> BrowserPanel::getItemBounds(int index) const {
    return juce::Rectangle<int>(0, 40 + index * itemHeight_, 
                                getWidth(), itemHeight_);
}

void BrowserPanel::mouseDown(const juce::MouseEvent& e) {
    int clickedIndex = (e.y - 40) / itemHeight_ + scrollOffset_;
    
    if (clickedIndex >= 0 && clickedIndex < filteredPresets_.size()) {
        selectedIndex_ = clickedIndex;
        if (onPresetSelected) {
            onPresetSelected(filteredPresets_[selectedIndex_]);
        }
        repaint();
    }
}

void BrowserPanel::mouseMove(const juce::MouseEvent& e) {
    int newHoverIndex = (e.y - 40) / itemHeight_ + scrollOffset_;
    
    if (newHoverIndex >= 0 && newHoverIndex < filteredPresets_.size()) {
        if (newHoverIndex != hoverIndex_) {
            hoverIndex_ = newHoverIndex;
            repaint();
        }
    } else {
        if (hoverIndex_ != -1) {
            hoverIndex_ = -1;
            repaint();
        }
    }
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
