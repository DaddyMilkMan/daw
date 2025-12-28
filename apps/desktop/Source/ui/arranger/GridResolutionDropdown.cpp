/*
  ==============================================================================

    GridResolutionDropdown.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "GridResolutionDropdown.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

GridResolutionDropdown::GridResolutionDropdown() {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

GridResolutionDropdown::~GridResolutionDropdown() {
    if (menu) {
        menu->hideMenu(); 
    }
}

void GridResolutionDropdown::setResolution(GridResolution res) {
    if (currentResolution != res) {
        currentResolution = res;
        repaint();
    }
}

juce::String GridResolutionDropdown::getResolutionText(GridResolution res) const {
    switch (res) {
        case GridResolution::Bar_1: return "1 Bar";
        case GridResolution::Beat_1: return "1 Beat";
        case GridResolution::Beat_1_2: return "1/2";
        case GridResolution::Beat_1_4: return "1/4";
        case GridResolution::Beat_1_8: return "1/8";
        case GridResolution::Beat_1_3: return "1/3";
        case GridResolution::Beat_1_6: return "1/6";
        case GridResolution::Off: return "Off";
        default: return "1 Beat";
    }
}

void GridResolutionDropdown::drawSkia(SkCanvas* canvas) {
    using namespace zenith::design;
    
    auto bounds = getLocalBounds();
    float width = static_cast<float>(bounds.getWidth());
    float height = static_cast<float>(bounds.getHeight());
    
    // Background: Glassmorphic panel effect
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    
    if (isHovered) {
        bgPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
    } else {
        bgPaint.setColor(SkColorSetARGB(20, 255, 255, 255));
    }
    
    SkRRect rrect = SkRRect::MakeRectXY(SkRect::MakeWH(width, height), 4.0f, 4.0f);
    canvas->drawRRect(rrect, bgPaint);
    
    // Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    
    if (isHovered) {
        borderPaint.setColor(colors::BORDER_FOCUS);
    } else {
        borderPaint.setColor(colors::BORDER_SUBTLE);
    }
    canvas->drawRRect(rrect, borderPaint);
    
    // Text
    SkFont font = typography::getSkFont(typography::FONT_XS, FontWeight::Medium);
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors::TEXT_PRIMARY);
    
    juce::String text = getResolutionText(currentResolution);
    
    SkRect textBounds;
    font.measureText(text.toRawUTF8(), text.length(), SkTextEncoding::kUTF8, &textBounds);
    
    float arrowSpace = 12.0f;
    float contentWidth = textBounds.width() + arrowSpace;
    float startX = (width - contentWidth) * 0.5f;
    float textY = height * 0.5f + typography::FONT_XS * 0.35f;
    
    canvas->drawString(text.toRawUTF8(), startX, textY, font, textPaint);
    
    // Dropdown Arrow
    SkPath arrow;
    float arrowX = startX + textBounds.width() + 6.0f;
    float arrowY = height * 0.5f - 2.0f;
    arrow.moveTo(arrowX, arrowY);
    arrow.lineTo(arrowX + 8.0f, arrowY);
    arrow.lineTo(arrowX + 4.0f, arrowY + 5.0f);
    arrow.close();
    
    SkPaint arrowPaint;
    arrowPaint.setAntiAlias(true);
    arrowPaint.setColor(colors::TEXT_SECONDARY);
    arrowPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawPath(arrow, arrowPaint);
}

void GridResolutionDropdown::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        showMenu();
    }
}

void GridResolutionDropdown::mouseEnter(const juce::MouseEvent&) {
    isHovered = true;
    repaint();
}

void GridResolutionDropdown::mouseExit(const juce::MouseEvent&) {
    isHovered = false;
    repaint();
}

void GridResolutionDropdown::showMenu() {
    menu = std::make_unique<SkiaPopupMenu>();
    
    auto addOption = [&](const juce::String& name, GridResolution res) {
        int id = static_cast<int>(res) + 1; // 1-based ID
        menu->addItem(id, name, true, false, [this, res]() {
             setResolution(res);
             if (onResolutionChanged) {
                 onResolutionChanged(res);
             }
        });
    };
    
    addOption("1 Bar", GridResolution::Bar_1);
    addOption("1 Beat", GridResolution::Beat_1);
    addOption("1/2 Beat", GridResolution::Beat_1_2);
    addOption("1/4 Beat", GridResolution::Beat_1_4);
    addOption("1/8 Beat", GridResolution::Beat_1_8);
    addOption("1/3 Beat (Triplet)", GridResolution::Beat_1_3);
    addOption("1/6 Beat (Triplet)", GridResolution::Beat_1_6);
    menu->addSeparator();
    addOption("Off", GridResolution::Off);
    
    menu->showAt(this, 0, getHeight() + 2);
}

} // namespace zenith
