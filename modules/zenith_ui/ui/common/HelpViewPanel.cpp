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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    HelpViewPanel.cpp
    Created: 2025-12-25
    Author:  Zenith DAW

  ==============================================================================
*/


#include "HelpViewPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include <core/SkBlurTypes.h>

namespace zenith {

HelpViewPanel::HelpViewPanel() {
    // Set default content
    title_ = "Quick Tips";
    description_ = "Double-click clips to edit • Drag to rearrange • Right-click for options";
    
    // Register global callback
    SkiaComponent::globalHelpCallback = [this](const juce::String& t, const juce::String& d) {
        setContent(t, d);
    };
}

HelpViewPanel::~HelpViewPanel() {
    SkiaComponent::globalHelpCallback = nullptr;
}

void HelpViewPanel::setContent(const juce::String& title, const juce::String& description) {
    if (title_ != title || description_ != description) {
        title_ = title.isEmpty() ? "Quick Tips" : title;
        description_ = description.isEmpty() ? "Double-click clips to edit • Drag to rearrange • Right-click for options" : description;
        markDirty();
    }
}

void HelpViewPanel::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Background - subtle gradient
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_DARKER);
    canvas->drawRect(rect, bgPaint);

    // Title
    SkFont titleFont = design::getSkFont(13.0f, design::FontWeight::SemiBold);
    
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    
    // Description (Word wrapped)
    SkFont descFont = design::getSkFont(11.0f, design::FontWeight::Regular);
    textPaint.setColor(design::colors::TEXT_SECONDARY);
    
    float x = 12.0f;
    float y = 42.0f;
    float maxWidth = bounds.getWidth() - 24.0f;
    float lineHeight = 15.0f;

    // Safety check: ensure we have space to draw
    if (maxWidth <= 0 || description_.isEmpty()) {
        return;
    }

    juce::StringArray words;
    words.addTokens(description_, " ", "");
    
    juce::String currentLine;
    for (const auto& word : words) {
        // If a single word is wider than maxWidth, we must print it anyway or clip it.
        // For simplicity, we just flow it.
        juce::String testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
        
        // Measure text safely - use getNumBytesAsUTF8() not length() for UTF-8 encoding
        float width = descFont.measureText(testLine.toRawUTF8(), testLine.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
        
        if (width > maxWidth) {
            if (currentLine.isNotEmpty()) {
                canvas->drawString(currentLine.toRawUTF8(), x, y, descFont, textPaint);
                y += lineHeight;
            }
            currentLine = word;
            
            // Stop drawing if we exceed bounds significantly to avoid wasted cycles
            if (y > bounds.getHeight()) break;
        } else {
            currentLine = testLine;
        }
    }
    
    if (currentLine.isNotEmpty() && y <= bounds.getHeight()) {
        canvas->drawString(currentLine.toRawUTF8(), x, y, descFont, textPaint);
    }
}
} // namespace zenith
