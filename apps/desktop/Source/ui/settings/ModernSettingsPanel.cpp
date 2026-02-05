/*
  ==============================================================================

    ModernSettingsPanel.cpp
    Created: 2026-01-17
    Author:  Zenith Team

    Minimal implementation to keep the settings panel functional while
    design iterations continue.

  ==============================================================================
*/

#include "ModernSettingsPanel.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

ModernSettingsPanel::ModernSettingsPanel() {
    setName("ModernSettingsPanel");
    setSize(PANEL_WIDTH, PANEL_HEIGHT);
    setVisible(false);
    setOpaque(false);
    setWantsKeyboardFocus(true);
}

ModernSettingsPanel::~ModernSettingsPanel() = default;

void ModernSettingsPanel::drawSkia(SkCanvas* canvas) {
    if (canvas == nullptr)
        return;

    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());

    SkPaint background;
    background.setAntiAlias(true);
    background.setColor(design::withAlpha(design::colors::BG_DARKER, 0.92f));
    canvas->drawRRect(SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS), background);

    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(design::withAlpha(design::colors::BORDER_DEFAULT, 0.6f));
    canvas->drawRRect(SkRRect::MakeRectXY(rect, CORNER_RADIUS, CORNER_RADIUS), border);

    SkFont titleFont = design::getSkFont(18.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setAntiAlias(true);
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    canvas->drawString("Settings", bounds.getX() + CARD_PADDING,
                       bounds.getY() + CARD_PADDING + titleFont.getSize(),
                       titleFont, titlePaint);
}

void ModernSettingsPanel::resized() {
    // Placeholder layout; real controls are introduced in future iterations.
}

void ModernSettingsPanel::mouseMove(const juce::MouseEvent& e) {
    updateHoverState(e.getPosition());
}

void ModernSettingsPanel::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    hoveredItemIndex_ = -1;
    markDirty();
}

void ModernSettingsPanel::mouseDown(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
}

void ModernSettingsPanel::refreshFromSettings() {
    // Stub: real settings sync will be wired in later.
}

void ModernSettingsPanel::applySettings() {
    // Stub: real settings application will be wired in later.
}

void ModernSettingsPanel::startShowAnimation() {
    isAnimatingIn_ = true;
    animationProgress_ = 0.0f;
    setVisible(true);
    markDirty();
}

void ModernSettingsPanel::startHideAnimation() {
    isAnimatingIn_ = false;
    animationProgress_ = 0.0f;
    setVisible(false);
    markDirty();
}

void ModernSettingsPanel::updateHoverState(const juce::Point<int>& mousePos) {
    juce::ignoreUnused(mousePos);
}

} // namespace zenith
