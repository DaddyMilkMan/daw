/*
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
        title_ = title;
        description_ = description;
        markDirty();
    }
}

void HelpViewPanel::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(design::colors::BG_DARKEST);
    canvas->drawRect(rect, bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setColor(design::colors::BORDER_DEFAULT);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    canvas->drawRect(rect, borderPaint);

    // Title
    SkFont titleFont = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    
    canvas->drawString(title_.toRawUTF8(), 10, 24, titleFont, textPaint);

    // Description (Word wrapped)
    SkFont descFont = design::getSkFont(12.0f, design::FontWeight::Regular);
    textPaint.setColor(design::colors::TEXT_SECONDARY);
    
    float x = 10.0f;
    float y = 45.0f;
    float maxWidth = bounds.getWidth() - 20.0f;
    float lineHeight = 16.0f;

    juce::StringArray words;
    words.addTokens(description_, " ", "");
    
    juce::String currentLine;
    for (const auto& word : words) {
        juce::String testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
        float width = descFont.measureText(testLine.toRawUTF8(), testLine.length(), SkTextEncoding::kUTF8);
        
        if (width > maxWidth) {
            canvas->drawString(currentLine.toRawUTF8(), x, y, descFont, textPaint);
            y += lineHeight;
            currentLine = word;
        } else {
            currentLine = testLine;
        }
    }
    if (currentLine.isNotEmpty()) {
        canvas->drawString(currentLine.toRawUTF8(), x, y, descFont, textPaint);
    }
}

} // namespace zenith
