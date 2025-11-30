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
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), 40), paint);
    
    SkFont headerFont;
    headerFont.setSize(16.0f);
    paint.setColor(SK_ColorWHITE);
    canvas->drawString("PRESETS", 10, 25, headerFont, paint);
    
    // Draw preset list
    SkFont itemFont;
    itemFont.setSize(14.0f);
    
    int visibleStart = scrollOffset_;
    int visibleEnd = juce::jmin(visibleStart + maxVisibleItems_, 
                                 (int)filteredPresets_.size());
    
    for (int i = visibleStart; i < visibleEnd; ++i) {
        auto itemBounds = getItemBounds(i - scrollOffset_);
        
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
                          itemBounds.getX() + 10,
                          itemBounds.getCentreY() + 5,
                          itemFont, paint);
    }
    
    // Scrollbar if needed
    if (filteredPresets_.size() > maxVisibleItems_) {
        float scrollbarHeight = (float)maxVisibleItems_ / filteredPresets_.size() * bounds.getHeight();
        float scrollbarY = ((float)scrollOffset_ / filteredPresets_.size()) * bounds.getHeight();
        
        paint.setColor(SkColorSetARGB(100, 255, 255, 255));
        canvas->drawRect(SkRect::MakeXYWH(bounds.getWidth() - 5, scrollbarY, 5, scrollbarHeight), paint);
    }
}

void BrowserPanel::setPresets(const juce::StringArray& presets) {
    allPresets_ = presets;
    updateFilteredList();
}

void BrowserPanel::setFilter(const juce::String& filter) {
    currentFilter_ = filter;
    updateFilteredList();
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
