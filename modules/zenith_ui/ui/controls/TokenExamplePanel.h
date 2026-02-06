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

#pragma once

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "ZenithButton.h"
#include "ZenithSlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

/**
 * @class TokenExamplePanel
 * @brief Demonstration component showing proper design token usage.
 *
 * This panel contains:
 * - Tokenized panel background using design::panel::* tokens
 * - Button examples using design::button::* tokens
 * - Slider example using design::slider::* tokens
 *
 * All styling is derived from the centralized token system, ensuring
 * consistency and easy theme switching.
 */
class TokenExamplePanel : public SkiaComponent {
public:
  TokenExamplePanel() : title_("Token Example Panel") {
    // Create demo buttons with different styles
    primaryButton_ = std::make_unique<ZenithButton>("Primary");
    primaryButton_->setButtonStyle(ZenithButton::Style::Primary);
    primaryButton_->setButtonSize(ZenithButton::Size::Medium);
    addAndMakeVisible(primaryButton_.get());

    secondaryButton_ = std::make_unique<ZenithButton>("Secondary");
    secondaryButton_->setButtonStyle(ZenithButton::Style::Secondary);
    secondaryButton_->setButtonSize(ZenithButton::Size::Medium);
    addAndMakeVisible(secondaryButton_.get());

    dangerButton_ = std::make_unique<ZenithButton>("Danger");
    dangerButton_->setButtonStyle(ZenithButton::Style::Danger);
    dangerButton_->setButtonSize(ZenithButton::Size::Small);
    addAndMakeVisible(dangerButton_.get());

    // Create demo slider
    demoSlider_ = std::make_unique<ZenithSlider>("Demo");
    demoSlider_->setRange(0.0f, 1.0f, 0.5f);
    addAndMakeVisible(demoSlider_.get());
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(static_cast<int>(design::panel::getPadding()));
    
    // Header area
    auto headerHeight = static_cast<int>(design::panel::getHeaderHeight());
    bounds.removeFromTop(headerHeight + static_cast<int>(design::spacing::SM));

    // Button row
    auto buttonRow = bounds.removeFromTop(static_cast<int>(design::button::getHeightMd()));
    int buttonWidth = 100;
    int gap = static_cast<int>(design::spacing::SM);

    primaryButton_->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(gap);
    secondaryButton_->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(gap);
    dangerButton_->setBounds(buttonRow.removeFromLeft(80));

    bounds.removeFromTop(static_cast<int>(design::spacing::MD));

    // Slider row
    auto sliderRow = bounds.removeFromTop(static_cast<int>(design::slider::getHandleSize()) + 16);
    demoSlider_->setBounds(sliderRow.reduced(0, 4));
  }

  void drawSkia(SkCanvas *canvas) override {
#ifdef ZENITH_USE_SKIA
    if (canvas == nullptr)
      return;

    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // =========================================================================
    // Panel Background - Using panel::* tokens
    // =========================================================================
    float radius = design::panel::getRadius();
    SkRRect rrect = SkRRect::MakeRectXY(rect, radius, radius);

    // Main background (elevated surface)
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(design::panel::getBg2()); // Panels use BG_02
    canvas->drawRRect(rrect, bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(design::panel::getBorderDefault());
    canvas->drawRRect(rrect, borderPaint);

    // Glass highlight (top edge)
    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setStyle(SkPaint::kStroke_Style);
    highlightPaint.setStrokeWidth(1.0f);
    highlightPaint.setColor(design::panel::getGlassHighlight());
    
    SkRect topRect = SkRect::MakeXYWH(rect.fLeft + radius, rect.fTop,
                                       rect.width() - radius * 2, 1);
    canvas->drawRect(topRect, highlightPaint);

    // =========================================================================
    // Header - Using panel::* tokens
    // =========================================================================
    float padding = design::panel::getPadding();
    float headerHeight = design::panel::getHeaderHeight();
    
    SkRect headerRect = SkRect::MakeXYWH(padding, padding,
                                          rect.width() - padding * 2, headerHeight);

    // Header background
    SkPaint headerBgPaint;
    headerBgPaint.setAntiAlias(true);
    headerBgPaint.setColor(design::panel::getHeaderBg());
    SkRRect headerRRect = SkRRect::MakeRectXY(headerRect, 
                                               design::panel::getRadiusSm(),
                                               design::panel::getRadiusSm());
    canvas->drawRRect(headerRRect, headerBgPaint);

    // Header text - Using typography tokens
    SkFont font = design::typography::getSkFont(design::typography::FONT_MD,
                                                 design::FontWeight::SemiBold);
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_PRIMARY);

    std::string titleStr = title_.toStdString();
    canvas->drawSimpleText(titleStr.c_str(), titleStr.length(), SkTextEncoding::kUTF8,
                           headerRect.fLeft + design::spacing::SM,
                           headerRect.centerY() + design::typography::FONT_MD * 0.35f,
                           font, textPaint);

    // Note: Child components (buttons, slider) render themselves via their
    // own drawSkia() methods, which also use design tokens.
#else
    juce::ignoreUnused(canvas);
#endif
  }

  /**
   * Get token usage documentation for this component.
   * This demonstrates how tokens are applied.
   */
  static juce::String getTokenDocumentation() {
    return R"(
TokenExamplePanel Token Usage:
=============================

PANEL TOKENS (design::panel::*)
- getBg2()           -> Main panel background
- getBorderDefault() -> Panel border color
- getGlassHighlight()-> Top edge highlight
- getHeaderBg()      -> Header background
- getPadding()       -> Content padding (16px)
- getRadius()        -> Corner radius (16px)
- getRadiusSm()      -> Header corner radius (8px)
- getHeaderHeight()  -> Header height (32px)

BUTTON TOKENS (design::button::*)
- Primary style uses getBgPrimary() -> CYAN
- Secondary style uses getBgSecondary() -> BG_02
- Danger style uses getBgDanger() -> RED
- Height from getHeightMd() -> 32px

SLIDER TOKENS (design::slider::*)
- Track uses getTrackBg() -> BG_01
- Fill uses getTrackFillDefault() -> CYAN
- Handle uses getHandleDefault() -> TEXT_PRIMARY

TYPOGRAPHY TOKENS (design::typography::*)
- FONT_MD (14px) for header text
- getSkFont() with SemiBold weight
)";
  }

private:
  juce::String title_;

  std::unique_ptr<ZenithButton> primaryButton_;
  std::unique_ptr<ZenithButton> secondaryButton_;
  std::unique_ptr<ZenithButton> dangerButton_;
  std::unique_ptr<ZenithSlider> demoSlider_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TokenExamplePanel)
};

} // namespace zenith
