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
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRect.h>
#include <include/core/SkFont.h>
#include <include/core/SkColor.h>
#include <include/effects/SkGradientShader.h>

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
    // Header Background with Gradient
    SkPoint pts[2] = { {0.0f, 0.0f}, {0.0f, 40.0f} };
    SkColor colors[2] = { SkColorSetRGB(45, 45, 50), SkColorSetRGB(30, 30, 35) };
    paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), 40), paint);
    paint.setShader(nullptr); // Reset shader
    
    // Header Text
    SkFont headerFont;
    headerFont.setSize(16.0f);
    headerFont.setEmbolden(true);
    headerFont.setEdging(SkFont::Edging::kAntiAlias);
    
    paint.setColor(SK_ColorWHITE);
    canvas->drawString("PRESETS", 15, 26, headerFont, paint);
    
    // Header Separator
    paint.setColor(SkColorSetARGB(100, 0, 255, 255)); // Cyan accent
    canvas->drawLine(0, 40, bounds.getWidth(), 40, paint);
    
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
        canvas->drawString(filteredPresets_[i].toRawUTF8(), (float)itemBounds.getX() + 10, (float)itemBounds.getCentreY() + 5, itemFont, paint);
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
