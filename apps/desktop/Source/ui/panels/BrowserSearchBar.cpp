/*
  ==============================================================================

    BrowserSearchBar.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserSearchBar.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include "../framework/GlassmorphicPanel.h"
#include <effects/SkGradientShader.h>

namespace zenith {

BrowserSearchBar::BrowserSearchBar() {
}

BrowserSearchBar::~BrowserSearchBar() {
}

void BrowserSearchBar::setSearchText(const juce::String &text) {
  if (searchText_ != text) {
    searchText_ = text;
    if (onSearchChanged) onSearchChanged(searchText_);
    repaint();
  }
}

void BrowserSearchBar::setBackButtonVisible(bool visible) {
  if (backButtonVisible_ != visible) {
    backButtonVisible_ = visible;
    repaint();
  }
}

void BrowserSearchBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  float h = (float)bounds.getHeight();
  float w = (float)bounds.getWidth();

  // Background
  SkPoint headerGradPoints[2] = {{0, 0}, {0, h}};
  juce::Colour bgToo = ZenithTheme::Colors::bg_02;
  juce::Colour bgThree = ZenithTheme::Colors::bg_03;
  SkColor headerGradColors[2] = {SkColorSetRGB(bgThree.getRed(), bgThree.getGreen(), bgThree.getBlue()),
                                 SkColorSetRGB(bgToo.getRed(), bgToo.getGreen(), bgToo.getBlue())};
  auto headerGradient = SkGradientShader::MakeLinear(headerGradPoints, headerGradColors, nullptr, 2, SkTileMode::kClamp);

  SkPaint headerBg;
  headerBg.setShader(headerGradient);
  canvas->drawRect(SkRect::MakeWH(w, h), headerBg);

  // Borders
  SkPaint highlightPaint;
  highlightPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
  canvas->drawLine(0, 0.5f, w, 0.5f, highlightPaint);

  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetRGB(50, 50, 58));
  canvas->drawLine(0, h - 0.5f, w, h - 0.5f, borderPaint);

  if (backButtonVisible_) {
    // Back Button
    GlassmorphicPanel::draw(canvas,
                            SkRect::MakeXYWH((float)backButtonBounds_.getX(), (float)backButtonBounds_.getY(),
                                             (float)backButtonBounds_.getWidth(), (float)backButtonBounds_.getHeight()),
                            GlassmorphicPanel::Style::Subtle);

    SkPath chevron;
    float cx = (float)backButtonBounds_.getCentreX();
    float cy = (float)backButtonBounds_.getCentreY();
    chevron.moveTo(cx + 3, cy - 6);
    chevron.lineTo(cx - 4, cy);
    chevron.lineTo(cx + 3, cy + 6);

    SkPaint chevronPaint;
    chevronPaint.setColor(SK_ColorWHITE);
    chevronPaint.setStyle(SkPaint::kStroke_Style);
    chevronPaint.setStrokeWidth(2.5f);
    chevronPaint.setStrokeCap(SkPaint::kRound_Cap);
    chevronPaint.setAntiAlias(true);
    canvas->drawPath(chevron, chevronPaint);
  } else {
    // Search Box
    SkRect searchRect = SkRect::MakeXYWH((float)searchBoxBounds_.getX(), (float)searchBoxBounds_.getY(),
                                       (float)searchBoxBounds_.getWidth(), (float)searchBoxBounds_.getHeight());

    SkPoint searchGradPoints[2] = {{0, searchRect.fTop}, {0, searchRect.fBottom}};
    SkColor searchGradColors[2] = {SkColorSetRGB(42, 42, 48), SkColorSetRGB(35, 35, 40)};
    auto searchGradient = SkGradientShader::MakeLinear(searchGradPoints, searchGradColors, nullptr, 2, SkTileMode::kClamp);

    SkPaint searchBgPaint;
    searchBgPaint.setShader(searchGradient);
    searchBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(searchRect, 8.0f, 8.0f, searchBgPaint);

    // Magnifying glass
    float iconX = (float)searchBoxBounds_.getX() + 18;
    float iconY = (float)searchBoxBounds_.getCentreY();
    SkPaint iconPaint;
    iconPaint.setColor(SkColorSetARGB(150, 255, 255, 255));
    iconPaint.setStyle(SkPaint::kStroke_Style);
    iconPaint.setStrokeWidth(1.8f);
    iconPaint.setAntiAlias(true);
    canvas->drawCircle(iconX, iconY - 1, 5, iconPaint);
    canvas->drawLine(iconX + 4, iconY + 3, iconX + 7, iconY + 6, iconPaint);

    // Search text
    SkFont searchFont = design::getSkFont(13.0f, design::FontWeight::Regular);
    SkPaint textPaint;
    textPaint.setColor(searchText_.isEmpty() ? SkColorSetARGB(90, 255, 255, 255) : SkColorSetRGB(230, 230, 240));
    textPaint.setAntiAlias(true);

    juce::String displayText = searchText_.isEmpty() ? "Search library..." : searchText_;
    canvas->drawString(displayText.toStdString().c_str(), (float)searchBoxBounds_.getX() + 32,
                       (float)searchBoxBounds_.getCentreY() + 4, searchFont, textPaint);
  }

  // Add Folder Button
  SkRect addBtnRect = SkRect::MakeXYWH((float)addFolderButtonBounds_.getX(), (float)addFolderButtonBounds_.getY(),
                                     (float)addFolderButtonBounds_.getWidth(), (float)addFolderButtonBounds_.getHeight());

  SkPoint btnGradPoints[2] = {{0, addBtnRect.fTop}, {0, addBtnRect.fBottom}};
  SkColor btnGradColors[2] = {SkColorSetRGB(55, 75, 55), SkColorSetRGB(40, 60, 40)};
  auto btnGradient = SkGradientShader::MakeLinear(btnGradPoints, btnGradColors, nullptr, 2, SkTileMode::kClamp);

  SkPaint addBtnPaint;
  addBtnPaint.setShader(btnGradient);
  addBtnPaint.setAntiAlias(true);
  canvas->drawRoundRect(addBtnRect, 6, 6, addBtnPaint);

  SkPaint btnBorder;
  btnBorder.setColor(SkColorSetRGB(80, 120, 80));
  btnBorder.setStyle(SkPaint::kStroke_Style);
  btnBorder.setStrokeWidth(1.0f);
  btnBorder.setAntiAlias(true);
  canvas->drawRoundRect(addBtnRect, 6, 6, btnBorder);

  // Plus icon
  float pcx = (float)addFolderButtonBounds_.getCentreX();
  float pcy = (float)addFolderButtonBounds_.getCentreY();
  SkPaint plusPaint;
  plusPaint.setColor(SkColorSetRGB(150, 255, 150));
  plusPaint.setStyle(SkPaint::kStroke_Style);
  plusPaint.setStrokeWidth(2.2f);
  plusPaint.setStrokeCap(SkPaint::kRound_Cap);
  plusPaint.setAntiAlias(true);
  canvas->drawLine(pcx - 5, pcy, pcx + 5, pcy, plusPaint);
  canvas->drawLine(pcx, pcy - 5, pcx, pcy + 5, plusPaint);
}

void BrowserSearchBar::mouseDown(const juce::MouseEvent &e) {
  if (backButtonVisible_ && backButtonBounds_.contains(e.getPosition())) {
    if (onBackRequested) onBackRequested();
    return;
  }

  if (addFolderButtonBounds_.contains(e.getPosition())) {
    if (onAddFolderRequested) onAddFolderRequested();
    return;
  }
}

bool BrowserSearchBar::keyPressed(const juce::KeyPress &key) {
  if (key.isKeyCode(juce::KeyPress::backspaceKey)) {
    if (searchText_.isNotEmpty()) {
      setSearchText(searchText_.dropLastCharacters(1));
      return true;
    }
  } else if (key.getTextCharacter() >= 32 && key.getTextCharacter() < 127) {
    setSearchText(searchText_ + juce::String::charToString(key.getTextCharacter()));
    return true;
  }
  return false;
}

void BrowserSearchBar::resized() {
  auto bounds = getLocalBounds();
  int w = bounds.getWidth();
  int h = bounds.getHeight();

  backButtonBounds_ = juce::Rectangle<int>(8, 10, 28, 28);
  addFolderButtonBounds_ = juce::Rectangle<int>(w - 40, 9, 32, searchBoxHeight_ - 2);
  searchBoxBounds_ = juce::Rectangle<int>(12, 9, w - 60, searchBoxHeight_ - 2);
}

} // namespace zenith
