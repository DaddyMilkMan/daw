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

#include "TitleBarComponent.h"
#include "../design-system/ZenithTypography.h"
#include "../framework/GlassmorphicPanel.h"
#include "../../engine/ZenithLogger.h" // Added for logging

namespace zenith {

TitleBarComponent::TitleBarComponent() 
    : transparentBackground_(false) {
  setOpaque(false);
  
  // Initialize Menu Bar
  menuBar_ = std::make_unique<ZenithMenuBar>();
  addAndMakeVisible(menuBar_.get());
  
  // Fonts
  titleFont_ = design::getSkFont(14.0f, design::FontWeight::SemiBold);
  
  textPaint_.setAntiAlias(true);
  textPaint_.setColor(design::colors::TEXT_PRIMARY);
}

TitleBarComponent::~TitleBarComponent() {}

// setupButtons removed

void TitleBarComponent::resized() {
  auto bounds = getLocalBounds();
  int height = bounds.getHeight();
  
  // Window control buttons on the right - LARGER for easy clicking
  int buttonSize = 24;
  int buttonSpacing = 12;
  int buttonY = (height - buttonSize) / 2;
  int rightPadding = 16;
  
  closeButtonBounds_ = juce::Rectangle<int>(bounds.getWidth() - rightPadding - buttonSize, buttonY, buttonSize, buttonSize);
  maximizeButtonBounds_ = juce::Rectangle<int>(closeButtonBounds_.getX() - buttonSpacing - buttonSize, buttonY, buttonSize, buttonSize);
  minimizeButtonBounds_ = juce::Rectangle<int>(maximizeButtonBounds_.getX() - buttonSpacing - buttonSize, buttonY, buttonSize, buttonSize);
  
  // Menu bar starts from left edge (no logo), goes up to window buttons
  int menuBarX = 16;
  int menuBarWidth = minimizeButtonBounds_.getX() - menuBarX - 20;
  if (menuBar_ && menuBarWidth > 100) {
    menuBar_->setBounds(menuBarX, 0, menuBarWidth, height);
  }
}

void TitleBarComponent::drawSkia(SkCanvas* canvas) {
    using namespace design;

    SkRect bounds = SkRect::MakeWH(getWidth(), getHeight());
    
    // DEBUG LOGGING - Trace draw calls
    static uint32_t lastLogTime = 0;
    uint32_t now = juce::Time::getMillisecondCounter();
    if (now - lastLogTime > 200) { // Throttle logs
         lastLogTime = now;
         ZENITH_LOG_INFO(juce::String::formatted("TitleBar Draw: Bounds=[%.1f %.1f %.1f %.1f], Visible=%d, MenuVisible=%d, Opacity=%.1f", 
            bounds.fLeft, bounds.fTop, bounds.fRight, bounds.fBottom, 
            isVisible(),
            menuBar_ ? (int)menuBar_->isVisible() : -1,
            getAlpha()
         ));
    }
    
    // Solid dark background (sharp edges, fills entire top)
    SkPaint bgPaint;
    bgPaint.setAntiAlias(false); // Sharp edges
    bgPaint.setColor(SkColorSetARGB(255, 10, 10, 10)); // Force opaque near-black
    canvas->drawRect(bounds, bgPaint);
    
    // Subtle 1px bottom divider with cyan tint
    SkPaint divPaint;
    divPaint.setAntiAlias(true);
    divPaint.setColor(SkColorSetA(colors::CYAN, 60));
    canvas->drawLine(0, bounds.bottom() - 1, bounds.right(), bounds.bottom() - 1, divPaint);
    
    // Draw Menu Bar - ALWAYS draw when TitleBar is drawn
    if (menuBar_) {
        canvas->save();
        canvas->translate(static_cast<float>(menuBar_->getX()), static_cast<float>(menuBar_->getY()));
        // NO clipping here
        menuBar_->drawSkia(canvas);
        canvas->restore();
    }
    
    // 5. Draw Window Control Buttons (X, -, □) - Zenith Style
    auto drawWindowButton = [&](const juce::Rectangle<int>& rect, bool isHovered, SkColor baseColor, const SkPath& icon) {
        SkRect skRect = SkRect::MakeXYWH(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
        
        // Hover background circle
        if (isHovered) {
            SkPaint bgPaint;
            bgPaint.setAntiAlias(true);
            bgPaint.setColor(SkColorSetA(baseColor, 50));
            canvas->drawCircle(skRect.centerX(), skRect.centerY(), skRect.width() / 2.0f + 3, bgPaint);
        }
        
        // Icon
        icons::IconStyle style;
        style.color = isHovered ? baseColor : SkColorSetA(colors::TEXT_SECONDARY, 180);
        style.strokeWidth = icons::STROKE_LIGHT;
        if (isHovered) {
            style.glowColor = baseColor;
            style.glowRadius = 8.0f;
        }
        icons::drawIconCentered(canvas, icon, skRect, 14.0f, style);
    };
    
    // Close button (Red X)
    drawWindowButton(closeButtonBounds_, closeHovered_, colors::RED, icons::Close());
    
    // Maximize button (Yellow square)
    drawWindowButton(maximizeButtonBounds_, maximizeHovered_, colors::YELLOW, icons::Stop()); // Using Stop for square
    
    // Minimize button (Green dash)
    drawWindowButton(minimizeButtonBounds_, minimizeHovered_, colors::NEON_GREEN, icons::Minus());
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
    bool prevClose = closeHovered_, prevMin = minimizeHovered_, prevMax = maximizeHovered_;
    
    closeHovered_ = closeButtonBounds_.contains(e.getPosition());
    minimizeHovered_ = minimizeButtonBounds_.contains(e.getPosition());
    maximizeHovered_ = maximizeButtonBounds_.contains(e.getPosition());
    
    if (closeHovered_ != prevClose || minimizeHovered_ != prevMin || maximizeHovered_ != prevMax) {
        repaint();
    }
}

void TitleBarComponent::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    closeHovered_ = false;
    minimizeHovered_ = false;
    maximizeHovered_ = false;
    repaint();
}

void TitleBarComponent::mouseUp(const juce::MouseEvent& e) {
    if (closeButtonBounds_.contains(e.getPosition())) {
        if (onClose) onClose();
    } else if (minimizeButtonBounds_.contains(e.getPosition())) {
        if (onMinimize) onMinimize();
    } else if (maximizeButtonBounds_.contains(e.getPosition())) {
        if (onMaximize) onMaximize();
    }
}

} // namespace zenith
