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

#include "ZenithLookAndFeel.h"


 * @file ZenithLookAndFeel.cpp
 * @brief Modern LookAndFeel implementation with micro-interactions
 * @author Fixed by Claude - December 2025



// Type aliases for cleaner access to ZenithDesignSystem/ZenithTheme
using Radius = ZenithTheme::Radius;
using Shadows = ZenithTheme::Shadows;
using Spacing = ZenithTheme::Spacing;
using Typography = ZenithTheme::Typography;
using ThemeColors = ZenithTheme::Colors;

// Static definitions for ZenithLookAndFeel::Colors (backwards compatibility)
const juce::Colour &ZenithLookAndFeel::Colors::background = design::toJuceColour(design::colors::BG_01);
const juce::Colour &ZenithLookAndFeel::Colors::backgroundPanel =
    design::toJuceColour(design::colors::BG_02);
const juce::Colour &ZenithLookAndFeel::Colors::panel = design::toJuceColour(design::colors::BG_03);
const juce::Colour &ZenithLookAndFeel::Colors::textPrimary =
    design::toJuceColour(design::colors::TEXT_PRIMARY);
const juce::Colour &ZenithLookAndFeel::Colors::textSecondary =
    design::toJuceColour(design::colors::TEXT_SECONDARY);
const juce::Colour &ZenithLookAndFeel::Colors::border =
    design::toJuceColour(design::colors::BORDER_DEFAULT);
const juce::Colour &ZenithLookAndFeel::Colors::accent =
    design::toJuceColour(design::colors::ACCENT_PRIMARY);

std::unique_ptr<ZenithLookAndFeel> ZenithLookAndFeel::instance_ = nullptr;

ZenithLookAndFeel::ZenithLookAndFeel() {
  auto getCol = [](SkColor c) { return design::toJuceColour(c); };

  // Window backgrounds
  setColour(juce::ResizableWindow::backgroundColourId, getCol(design::colors::BG_01));

  // Text colors
  setColour(juce::Label::textColourId, getCol(design::colors::TEXT_PRIMARY));
  setColour(juce::Label::textWhenEditingColourId, getCol(design::colors::TEXT_PRIMARY));

  // Button colors
  setColour(juce::TextButton::buttonColourId, getCol(design::colors::BG_03));
  setColour(juce::TextButton::buttonOnColourId, getCol(design::colors::ACCENT_PRIMARY));
  setColour(juce::TextButton::textColourOffId, getCol(design::colors::TEXT_PRIMARY));
  setColour(juce::TextButton::textColourOnId, getCol(design::colors::BG_01)); // inverse approx

  // ComboBox colors
  setColour(juce::ComboBox::backgroundColourId, getCol(design::colors::BG_02));
  setColour(juce::ComboBox::outlineColourId, getCol(design::colors::BORDER_DEFAULT));
  setColour(juce::ComboBox::textColourId, getCol(design::colors::TEXT_PRIMARY));
  setColour(juce::ComboBox::arrowColourId, getCol(design::colors::TEXT_SECONDARY));
  setColour(juce::ComboBox::focusedOutlineColourId, getCol(design::colors::ACCENT_PRIMARY));

  // Slider colors
  setColour(juce::Slider::thumbColourId, getCol(design::colors::TEXT_PRIMARY));
  setColour(juce::Slider::trackColourId, getCol(design::colors::ACCENT_PRIMARY));
  setColour(juce::Slider::backgroundColourId, getCol(design::colors::BG_02));
  setColour(juce::Slider::rotarySliderFillColourId, getCol(design::colors::ACCENT_PRIMARY));
  setColour(juce::Slider::rotarySliderOutlineColourId, getCol(design::colors::BORDER_DEFAULT));

  // TextEditor colors
  setColour(juce::TextEditor::backgroundColourId, getCol(design::colors::BG_02));
  setColour(juce::TextEditor::textColourId, getCol(design::colors::TEXT_PRIMARY));
  setColour(juce::TextEditor::outlineColourId, getCol(design::colors::BORDER_DEFAULT));
  setColour(juce::TextEditor::focusedOutlineColourId, getCol(design::colors::ACCENT_PRIMARY));
  setColour(juce::TextEditor::highlightColourId, getCol(design::colors::ACCENT_PRIMARY).withAlpha(0.3f));

  // ScrollBar colors
  setColour(juce::ScrollBar::thumbColourId, getCol(design::colors::TEXT_TERTIARY));
  setColour(juce::ScrollBar::backgroundColourId, getCol(design::colors::BG_01));

  // Tooltip colors
  setColour(juce::TooltipWindow::backgroundColourId, getCol(design::colors::BG_04));
  setColour(juce::TooltipWindow::textColourId, getCol(design::colors::TEXT_PRIMARY));
  setColour(juce::TooltipWindow::outlineColourId, getCol(design::colors::BORDER_SUBTLE));

  // PopupMenu colors
  setColour(juce::PopupMenu::backgroundColourId, getCol(design::colors::BG_03));
  setColour(juce::PopupMenu::textColourId, getCol(design::colors::TEXT_PRIMARY));
  setColour(juce::PopupMenu::highlightedBackgroundColourId,
            getCol(design::colors::ACCENT_PRIMARY).withAlpha(0.2f));
  setColour(juce::PopupMenu::highlightedTextColourId, getCol(design::colors::ACCENT_PRIMARY));
}

ZenithLookAndFeel &ZenithLookAndFeel::getInstance() {
  if (!instance_) {
    instance_ = std::make_unique<ZenithLookAndFeel>();
  }
  return *instance_;
}

//==============================================================================
// Button Rendering with Hover/Press States
//==============================================================================

void ZenithLookAndFeel::drawButtonBackground(
    juce::Graphics &g, juce::Button &button,
    const juce::Colour &backgroundColour, bool isHighlighted, bool isDown) {

  auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
  auto cornerSize = Radius::sm;

  // Determine button state color
  juce::Colour buttonColor = ThemeColors::bg_03;

  if (button.getToggleState()) {
    buttonColor = ThemeColors::accent_primary;
  }

  if (isDown) {
    buttonColor =
        button.getToggleState() ? ThemeColors::accent_secondary : ThemeColors::bg_04;
  } else if (isHighlighted) {
    buttonColor =
        button.getToggleState() ? ThemeColors::accent_primary.brighter(0.1f) : ThemeColors::bg_04;
  }

  // Draw shadow for elevation (skip if pressed)
  if (!isDown && button.isEnabled()) {
    Shadows::drawShadow(g, bounds, Shadows::elevation_1, cornerSize);
  }

  // Draw button background
  g.setColour(buttonColor);
  g.fillRoundedRectangle(bounds, cornerSize);

  // Draw border
  if (!button.getToggleState()) {
    g.setColour(ThemeColors::border_default);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
  }

  // Draw focus ring if focused
  if (button.hasKeyboardFocus(true)) {
    drawFocusRing(g, bounds, cornerSize);
  }
}

void ZenithLookAndFeel::drawButtonText(juce::Graphics &g,
                                       juce::TextButton &button,
                                       bool isHighlighted, bool isDown) {

  juce::Font font = design::typography::getJuceFont(14.0f, design::FontWeight::Medium);
  g.setFont(font);

  // Text color based on button state
  juce::Colour textColor =
      button.getToggleState() ? ThemeColors::bg_01 : ThemeColors::text_primary;

  if (!button.isEnabled()) {
    textColor = ThemeColors::text_tertiary;
  }

  g.setColour(textColor);

  auto textBounds = button.getLocalBounds();
  g.drawText(button.getButtonText(), textBounds, juce::Justification::centred,
             true);
}

//==============================================================================
// Modern Rotary Slider (Knob)
//==============================================================================

void ZenithLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                         int width, int height, float sliderPos,
                                         const float rotaryStartAngle,
                                         const float rotaryEndAngle,
                                         juce::Slider &slider) {

  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
  auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 4.0f;
  auto centreX = bounds.getCentreX();
  auto centreY = bounds.getCentreY();
  auto angle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  // Draw shadow
  auto knobBounds = juce::Rectangle<float>(centreX - radius, centreY - radius,
                                           radius * 2.0f, radius * 2.0f);
  Shadows::drawShadow(g, knobBounds, Shadows::elevation_2, 9999.0f);

  // Draw background ring
  g.setColour(ThemeColors::bg_03);
  g.fillEllipse(knobBounds);

  // Draw track outline
  g.setColour(ThemeColors::border_default);
  g.drawEllipse(knobBounds.reduced(2.0f), 2.0f);

  // Draw value arc
  juce::Path valueArc;
  auto arcRadius = radius - 6.0f;
  valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, angle, true);

  juce::PathStrokeType strokeType(4.0f, juce::PathStrokeType::curved,
                                  juce::PathStrokeType::rounded);

  // Gradient for value arc
  juce::ColourGradient gradient(ThemeColors::accent_primary, centreX,
                                centreY - arcRadius, ThemeColors::accent_hover,
                                centreX, centreY + arcRadius, false);
  g.setGradientFill(gradient);
  g.strokePath(valueArc, strokeType);

  // Draw indicator line
  auto indicatorLength = radius * 0.6f;
  juce::Point<float> indicatorStart(centreX, centreY - 8.0f);
  juce::Point<float> indicatorEnd(centreX, centreY - indicatorLength);

  juce::AffineTransform rotation =
      juce::AffineTransform::rotation(angle, centreX, centreY);

  indicatorStart = indicatorStart.transformedBy(rotation);
  indicatorEnd = indicatorEnd.transformedBy(rotation);

  g.setColour(ThemeColors::text_primary);
  g.drawLine(indicatorStart.x, indicatorStart.y, indicatorEnd.x, indicatorEnd.y,
             3.0f);

  // Draw center dot
  auto dotRadius = 4.0f;
  g.fillEllipse(centreX - dotRadius, centreY - dotRadius, dotRadius * 2.0f,
                dotRadius * 2.0f);
}

//==============================================================================
// Modern Linear Slider (Fader)
//==============================================================================

void ZenithLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y,
                                         int width, int height, float sliderPos,
                                         float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style,
                                         juce::Slider &slider) {

  auto isVertical = (style == juce::Slider::LinearVertical);
  auto trackWidth = isVertical ? 4.0f : 4.0f;

  // Calculate track bounds
  juce::Rectangle<float> trackBounds;
  if (isVertical) {
    trackBounds = juce::Rectangle<float>(x + width * 0.5f - trackWidth * 0.5f,
                                         (float)y, trackWidth, (float)height);
  } else {
    trackBounds =
        juce::Rectangle<float>((float)x, y + height * 0.5f - trackWidth * 0.5f,
                               (float)width, trackWidth);
  }

  // Draw track background
  g.setColour(ThemeColors::bg_03);
  g.fillRoundedRectangle(trackBounds, trackWidth * 0.5f);

  // Draw filled portion
  juce::Rectangle<float> filledTrack;
  if (isVertical) {
    filledTrack = juce::Rectangle<float>(trackBounds.getX(), sliderPos,
                                         trackBounds.getWidth(),
                                         trackBounds.getBottom() - sliderPos);
  } else {
    filledTrack = juce::Rectangle<float>(trackBounds.getX(), trackBounds.getY(),
                                         sliderPos - trackBounds.getX(),
                                         trackBounds.getHeight());
  }

  g.setColour(ThemeColors::accent_primary);
  g.fillRoundedRectangle(filledTrack, trackWidth * 0.5f);

  // Draw thumb
  auto thumbSize = isVertical ? 12.0f : 12.0f;
  juce::Rectangle<float> thumbBounds;

  if (isVertical) {
    thumbBounds = juce::Rectangle<float>(x + width * 0.5f - thumbSize * 0.5f,
                                         sliderPos - thumbSize * 0.5f,
                                         thumbSize, thumbSize);
  } else {
    thumbBounds = juce::Rectangle<float>(sliderPos - thumbSize * 0.5f,
                                         y + height * 0.5f - thumbSize * 0.5f,
                                         thumbSize, thumbSize);
  }

  // Thumb shadow
  Shadows::drawShadow(g, thumbBounds, Shadows::elevation_2, thumbSize * 0.5f);

  // Thumb background
  g.setColour(ThemeColors::text_primary);
  g.fillEllipse(thumbBounds);

  // Thumb border
  g.setColour(ThemeColors::accent_primary);
  g.drawEllipse(thumbBounds.reduced(1.0f), 2.0f);
}

//==============================================================================
// ComboBox Rendering
//==============================================================================

void ZenithLookAndFeel::drawComboBox(juce::Graphics &g, int width, int height,
                                     bool isButtonDown, int buttonX,
                                     int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox &box) {

  auto bounds =
      juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
  auto cornerSize = Radius::sm;

  // Background
  g.setColour(ThemeColors::bg_02);
  g.fillRoundedRectangle(bounds, cornerSize);

  // Border
  auto borderColor = box.hasKeyboardFocus(true) ? ThemeColors::accent_primary
                                                : ThemeColors::border_default;
  g.setColour(borderColor);
  g.drawRoundedRectangle(bounds, cornerSize, 1.5f);

  // Arrow
  auto arrowZone =
      juce::Rectangle<float>(buttonX, buttonY, buttonW, buttonH).toFloat();
  juce::Path arrow;
  arrow.addTriangle(
      arrowZone.getCentreX() - 4.0f, arrowZone.getCentreY() - 2.0f,
      arrowZone.getCentreX() + 4.0f, arrowZone.getCentreY() - 2.0f,
      arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);

  g.setColour(ThemeColors::text_secondary);
  g.fillPath(arrow);
}

//==============================================================================
// Popup Menu Rendering
//==============================================================================

void ZenithLookAndFeel::drawPopupMenuBackground(juce::Graphics &g, int width,
                                                int height) {

  auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);

  // Shadow
  Shadows::drawShadow(g, bounds, Shadows::elevation_3, Radius::md);

  // Background
  g.setColour(ThemeColors::bg_03);
  g.fillRoundedRectangle(bounds, Radius::md);

  // Border
  g.setColour(ThemeColors::border_strong);
  g.drawRoundedRectangle(bounds.reduced(0.5f), Radius::md, 1.0f);
}

void ZenithLookAndFeel::drawPopupMenuItem(
    juce::Graphics &g, const juce::Rectangle<int> &area, bool isSeparator,
    bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
    const juce::String &text, const juce::String &shortcutKeyText,
    const juce::Drawable *icon, const juce::Colour *textColour) {

  if (isSeparator) {
    auto separatorBounds = area.reduced(Spacing::md, 0).toFloat();
    separatorBounds =
        separatorBounds.withHeight(1.0f).withY(area.getCentreY() - 0.5f);
    g.setColour(ThemeColors::border_subtle);
    g.fillRect(separatorBounds);
    return;
  }

  auto textBounds = area.reduced(Spacing::md, 0);

  // Highlight background
  if (isHighlighted && isActive) {
    g.setColour(ThemeColors::accent_subtle);
    g.fillRoundedRectangle(area.toFloat().reduced(4.0f, 2.0f), Radius::sm);
  }

  // Text
  auto textColor =
      isActive ? (isHighlighted ? ThemeColors::accent_primary : ThemeColors::text_primary)
               : ThemeColors::text_tertiary;
  g.setColour(textColor);
  g.setFont(Typography::getBodyFont());

  auto textArea = textBounds;
  if (isTicked) {
    textArea = textArea.withTrimmedLeft(20);
  }

  g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);

  // Shortcut text
  if (shortcutKeyText.isNotEmpty()) {
    g.setColour(ThemeColors::text_secondary);
    g.setFont(Typography::getSmallFont());
    g.drawFittedText(shortcutKeyText, textBounds,
                     juce::Justification::centredRight, 1);
  }

  // Tick mark
  if (isTicked) {
    auto tickBounds = textBounds.removeFromLeft(20).toFloat();
    juce::Path tick;
    tick.addLineSegment(juce::Line<float>(tickBounds.getCentreX() - 4.0f,
                                          tickBounds.getCentreY(),
                                          tickBounds.getCentreX() - 1.0f,
                                          tickBounds.getCentreY() + 3.0f),
                        2.0f);
    tick.addLineSegment(juce::Line<float>(tickBounds.getCentreX() - 1.0f,
                                          tickBounds.getCentreY() + 3.0f,
                                          tickBounds.getCentreX() + 4.0f,
                                          tickBounds.getCentreY() - 3.0f),
                        2.0f);
    g.setColour(ThemeColors::accent_primary);
    g.fillPath(tick);
  }

  // Submenu arrow
  if (hasSubMenu) {
    auto arrowBounds = textBounds.removeFromRight(20).toFloat();
    juce::Path arrow;
    arrow.addTriangle(
        arrowBounds.getCentreX() - 2.0f, arrowBounds.getCentreY() - 4.0f,
        arrowBounds.getCentreX() - 2.0f, arrowBounds.getCentreY() + 4.0f,
        arrowBounds.getCentreX() + 3.0f, arrowBounds.getCentreY());
    g.setColour(ThemeColors::text_secondary);
    g.fillPath(arrow);
  }
}

//==============================================================================
// ScrollBar Rendering
//==============================================================================

void ZenithLookAndFeel::drawScrollbar(juce::Graphics &g,
                                      juce::ScrollBar &scrollbar, int x, int y,
                                      int width, int height,
                                      bool isScrollbarVertical,
                                      int thumbStartPosition, int thumbSize,
                                      bool isMouseOver, bool isMouseDown) {

  // Track background (subtle)
  g.setColour(ThemeColors::bg_01);
  g.fillRect(x, y, width, height);

  // Thumb
  juce::Rectangle<int> thumbBounds;
  if (isScrollbarVertical) {
    thumbBounds =
        juce::Rectangle<int>(x + 2, thumbStartPosition, width - 4, thumbSize);
  } else {
    thumbBounds =
        juce::Rectangle<int>(thumbStartPosition, y + 2, thumbSize, height - 4);
  }

  auto thumbColor = isMouseDown
                        ? ThemeColors::text_secondary
                        : (isMouseOver ? ThemeColors::text_tertiary.brighter(0.2f)
                                       : ThemeColors::text_tertiary);

  g.setColour(thumbColor);
  g.fillRoundedRectangle(thumbBounds.toFloat(), 4.0f);
}

//==============================================================================
// Label Rendering
//==============================================================================

void ZenithLookAndFeel::drawLabel(juce::Graphics &g, juce::Label &label) {

  g.fillAll(label.findColour(juce::Label::backgroundColourId));

  if (!label.isBeingEdited()) {
    auto alpha = label.isEnabled() ? 1.0f : 0.5f;
    auto font = Typography::getBodyFont();

    g.setColour(
        label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
    g.setFont(font);

    auto textBounds = label.getLocalBounds().reduced(2);
    g.drawFittedText(
        label.getText(), textBounds, label.getJustificationType(),
        juce::jmax(1, (int)((float)textBounds.getHeight() / font.getHeight())),
        label.getMinimumHorizontalScale());

    g.setColour(label.findColour(juce::Label::outlineColourId)
                    .withMultipliedAlpha(alpha));
    g.drawRect(label.getLocalBounds());
  }
}

//==============================================================================
// TextEditor Rendering
//==============================================================================

void ZenithLookAndFeel::fillTextEditorBackground(juce::Graphics &g, int width,
                                                 int height,
                                                 juce::TextEditor &textEditor) {

  g.setColour(ThemeColors::bg_02);
  g.fillRoundedRectangle(0, 0, (float)width, (float)height, Radius::sm);
}

void ZenithLookAndFeel::drawTextEditorOutline(juce::Graphics &g, int width,
                                              int height,
                                              juce::TextEditor &textEditor) {

  auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
  auto borderColor = textEditor.hasKeyboardFocus(true) ? ThemeColors::accent_primary
                                                       : ThemeColors::border_default;

  g.setColour(borderColor);
  g.drawRoundedRectangle(bounds.reduced(0.5f), Radius::sm, 1.5f);
}

//==============================================================================
// ToggleButton Rendering
//==============================================================================

void ZenithLookAndFeel::drawToggleButton(juce::Graphics &g,
                                         juce::ToggleButton &button,
                                         bool isHighlighted, bool isDown) {

  auto bounds = button.getLocalBounds().toFloat();
  auto toggleSize = 20.0f;
  auto toggleBounds = bounds.removeFromLeft(toggleSize + Spacing::sm)
                          .withSizeKeepingCentre(toggleSize, toggleSize);

  // Toggle background
  auto bgColor =
      button.getToggleState() ? ThemeColors::accent_primary : ThemeColors::bg_03;
  if (isHighlighted) {
    bgColor = bgColor.brighter(0.1f);
  }

  g.setColour(bgColor);
  g.fillRoundedRectangle(toggleBounds, Radius::sm);

  // Border
  g.setColour(button.getToggleState() ? ThemeColors::accent_primary
                                      : ThemeColors::border_default);
  g.drawRoundedRectangle(toggleBounds, Radius::sm, 1.5f);

  // Checkmark
  if (button.getToggleState()) {
    juce::Path tick;
    tick.addLineSegment(juce::Line<float>(toggleBounds.getCentreX() - 4.0f,
                                          toggleBounds.getCentreY(),
                                          toggleBounds.getCentreX() - 1.0f,
                                          toggleBounds.getCentreY() + 3.0f),
                        2.0f);
    tick.addLineSegment(juce::Line<float>(toggleBounds.getCentreX() - 1.0f,
                                          toggleBounds.getCentreY() + 3.0f,
                                          toggleBounds.getCentreX() + 5.0f,
                                          toggleBounds.getCentreY() - 4.0f),
                        2.0f);
    g.setColour(ThemeColors::text_inverse);
    g.fillPath(tick);
  }

  // Label text
  g.setColour(button.isEnabled() ? ThemeColors::text_primary
                                 : ThemeColors::text_tertiary);
  g.setFont(Typography::getBodyFont());
  g.drawFittedText(button.getButtonText(), bounds.toNearestInt(),
                   juce::Justification::centredLeft, 1);
}

//==============================================================================
// TabBar Rendering
//==============================================================================

void ZenithLookAndFeel::drawTabButton(juce::TabBarButton &button,
                                      juce::Graphics &g, bool isMouseOver,
                                      bool isMouseDown) {

  auto bounds = button.getActiveArea().toFloat();
  auto isActive = button.isFrontTab();

  // Background
  if (isActive) {
    g.setColour(ThemeColors::bg_03);
    g.fillRect(bounds);

    // Active indicator line
    auto indicatorBounds = bounds.removeFromBottom(2.0f);
    g.setColour(ThemeColors::accent_primary);
    g.fillRect(indicatorBounds);
  } else if (isMouseOver) {
    g.setColour(ThemeColors::bg_02);
    g.fillRect(bounds);
  }

  // Text
  auto textColor = isActive ? ThemeColors::text_primary : ThemeColors::text_secondary;
  g.setColour(textColor);
  g.setFont(Typography::getBodyFont(isActive ? Typography::Weight::Medium
                                             : Typography::Weight::Regular));
  g.drawText(button.getButtonText(), bounds.reduced(Spacing::md, 0),
             juce::Justification::centred, true);
}

//==============================================================================
// Tooltip Rendering
//==============================================================================

void ZenithLookAndFeel::drawTooltip(juce::Graphics &g, const juce::String &text,
                                    int width, int height) {

  auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);

  // Shadow
  Shadows::drawShadow(g, bounds, Shadows::elevation_3, Radius::sm);

  // Background
  g.setColour(ThemeColors::bg_04);
  g.fillRoundedRectangle(bounds, Radius::sm);

  // Border
  g.setColour(ThemeColors::border_strong);
  g.drawRoundedRectangle(bounds.reduced(0.5f), Radius::sm, 1.0f);

  // Text
  g.setColour(ThemeColors::text_primary);
  g.setFont(Typography::getSmallFont());
  g.drawFittedText(text, bounds.reduced(Spacing::sm).toNearestInt(),
                   juce::Justification::centred, 2);
}

//==============================================================================
// Utility Methods
//==============================================================================

void ZenithLookAndFeel::drawCard(juce::Graphics &g,
                                 juce::Rectangle<float> bounds,
                                 float cornerRadius, float elevation) {

  // Shadow
  Shadows::drawShadow(g, bounds, elevation, cornerRadius);

  // Background
  g.setColour(ThemeColors::bg_02);
  g.fillRoundedRectangle(bounds, cornerRadius);

  // Border
  g.setColour(ThemeColors::border_default);
  g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
}

void ZenithLookAndFeel::drawFocusRing(juce::Graphics &g,
                                      juce::Rectangle<float> bounds,
                                      float cornerRadius) {

  g.setColour(ThemeColors::accent_primary.withAlpha(0.3f));
  g.drawRoundedRectangle(bounds.expanded(2.0f), cornerRadius + 2.0f, 2.0f);
}

void ZenithLookAndFeel::drawSeparator(juce::Graphics &g,
                                      juce::Rectangle<float> bounds,
                                      bool vertical) {

  g.setColour(ThemeColors::border_subtle);
  if (vertical) {
    auto line = bounds.withWidth(1.0f).withX(bounds.getCentreX() - 0.5f);
    g.fillRect(line);
  } else {
    auto line = bounds.withHeight(1.0f).withY(bounds.getCentreY() - 0.5f);
    g.fillRect(line);
  }
}

} // namespace zenith
