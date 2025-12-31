/*
  ==============================================================================

    TitleBarComponent.cpp
    Created: 2025-12-30
    Author:  Zenith DAW

  ==============================================================================
*/

#include "TitleBarComponent.h"
#include "../design-system/ZenithTypography.h"
#include "../framework/GlassmorphicPanel.h"

namespace zenith {

TitleBarComponent::TitleBarComponent() 
    : transparentBackground_(false) {
  setOpaque(false);
  
  // Fonts
  titleFont_ = design::getSkFont(14.0f, design::FontWeight::SemiBold);
  
  textPaint_.setAntiAlias(true);
  textPaint_.setColor(design::colors::TEXT_PRIMARY);
}

TitleBarComponent::~TitleBarComponent() {}

// setupButtons removed

void TitleBarComponent::resized() {
  // Window buttons layout removed
}

void TitleBarComponent::drawSkia(SkCanvas* canvas) {
    using namespace design;

    SkRect bounds = SkRect::MakeWH(getWidth(), getHeight());
    
    // Draw simple background with subtle blur feel
    // If hub is open, we want NO background to avoid bleed.
    // Otherwise, we want a very subtle dark glass tint.
    if (!transparentBackground_) {
        SkPaint bgPaint;
        // Reduced from absolute white/gray to a very subtle dark tint (30% opacity)
        bgPaint.setColor(SkColorSetA(colors::BG_DARK, 80)); 
        canvas->drawRect(bounds, bgPaint);
        
        // Subtle 1px bottom divider
        SkPaint divPaint;
        divPaint.setAntiAlias(true);
        divPaint.setColor(SkColorSetA(SK_ColorWHITE, 15));
        canvas->drawLine(0, bounds.bottom() - 1, bounds.right(), bounds.bottom() - 1, divPaint);
    }
    
    // Draw Title
    if (showTitle_) {
        juce::String title = "Zenith DAW";
        if (auto* w = findParentComponentOfClass<juce::DocumentWindow>()) {
            title = w->getName();
        }
        
        SkString skTitle(title.toRawUTF8());
        SkRect titleBounds;
        titleFont_.measureText(skTitle.c_str(), skTitle.size(), SkTextEncoding::kUTF8, &titleBounds);
        
        canvas->drawSimpleText(skTitle.c_str(), skTitle.size(), SkTextEncoding::kUTF8, 
                               bounds.centerX() - titleBounds.width()/2, 
                               bounds.centerY() + titleBounds.height()/2, 
                               titleFont_, textPaint_);
    }
}

// drawWindowButton removed

void TitleBarComponent::mouseDown(const juce::MouseEvent& e) {
    if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>()) {
        dragger_.startDraggingComponent(dw, e);
    }
}

void TitleBarComponent::mouseDrag(const juce::MouseEvent& e) {
   if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>()) {
       dragger_.dragComponent(dw, e, nullptr);
   }
}

void TitleBarComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>()) {
        dw->setFullScreen(!dw->isFullScreen());
    }
}

// getButtonAt removed

void TitleBarComponent::mouseMove(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
}

void TitleBarComponent::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
}

void TitleBarComponent::mouseUp(const juce::MouseEvent& e) {
   juce::ignoreUnused(e);
}

} // namespace zenith
