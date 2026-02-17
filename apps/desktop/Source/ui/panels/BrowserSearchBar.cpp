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
#include <array>
#include <effects/SkGradientShader.h>
#include <algorithm>

namespace {
struct CommandEntry {
  const char* id;
  const char* label;
};

const std::array<CommandEntry, 5> kCommands{{
    {"add-folder", "Add Library Folder"},
    {"refresh-library", "Refresh Browser Library"},
    {"toggle-autoplay", "Toggle Preview Auto-Play"},
    {"clear-search", "Clear Search"},
    {"focus-list", "Focus Browser List"},
}};
}

namespace zenith {

BrowserSearchBar::BrowserSearchBar() {
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
}

BrowserSearchBar::~BrowserSearchBar() {
}

void BrowserSearchBar::setSearchText(const juce::String &text) {
  if (searchText_ != text) {
    searchText_ = text;
    if (commandMode_) {
      updateVisibleCommands();
    }
    if (onSearchChanged) onSearchChanged(searchText_);
    repaint();
  }
}

void BrowserSearchBar::setCommandMode(bool enabled) {
  if (commandMode_ == enabled) {
    return;
  }
  commandMode_ = enabled;
  if (commandMode_) {
    if (!searchText_.startsWithChar('>')) {
      searchText_ = ">";
    }
    updateVisibleCommands();
    selectedCommandIndex_ = 0;
    grabKeyboardFocus();
  } else if (searchText_ == ">") {
    searchText_.clear();
  }
  repaint();
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

    juce::String displayText = searchText_.isEmpty()
                                   ? (commandMode_ ? "> command-palette" : "Search library...")
                                   : searchText_;
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

  if (commandMode_) {
    GlassmorphicPanel::draw(canvas,
                            SkRect::MakeXYWH((float)commandPaletteBounds_.getX(),
                                             (float)commandPaletteBounds_.getY(),
                                             (float)commandPaletteBounds_.getWidth(),
                                             (float)commandPaletteBounds_.getHeight()),
                            GlassmorphicPanel::Style::Subtle);

    if (visibleCommandIndices_.empty()) {
      SkFont emptyFont = design::getSkFont(11.0f, design::FontWeight::Regular);
      SkPaint emptyPaint;
      emptyPaint.setAntiAlias(true);
      emptyPaint.setColor(SkColorSetARGB(165, 230, 236, 246));
      canvas->drawString("No matching commands", (float)commandPaletteBounds_.getX() + 10.0f,
                         (float)commandPaletteBounds_.getY() + 18.0f, emptyFont, emptyPaint);
    } else {
      constexpr int rowH = 22;
      for (int i = 0; i < (int)visibleCommandIndices_.size(); ++i) {
        const int commandIndex = visibleCommandIndices_[(size_t)i];
        const auto& item = kCommands[(size_t)commandIndex];
        const int rowY = commandPaletteBounds_.getY() + i * rowH;

        if (i == selectedCommandIndex_) {
          SkPaint selPaint;
          selPaint.setAntiAlias(true);
          selPaint.setColor(SkColorSetARGB(76, 95, 145, 230));
          canvas->drawRoundRect(
              SkRect::MakeXYWH((float)commandPaletteBounds_.getX() + 4.0f, (float)rowY + 2.0f,
                               (float)commandPaletteBounds_.getWidth() - 8.0f, (float)rowH - 3.0f),
              5.0f, 5.0f, selPaint);
        }

        SkFont rowFont = design::getSkFont(11.0f, design::FontWeight::SemiBold);
        SkPaint rowPaint;
        rowPaint.setAntiAlias(true);
        rowPaint.setColor(i == selectedCommandIndex_
                              ? SkColorSetRGB(238, 246, 255)
                              : SkColorSetARGB(210, 220, 232, 248));
        canvas->drawString(item.label, (float)commandPaletteBounds_.getX() + 10.0f,
                           (float)rowY + 16.0f, rowFont, rowPaint);
      }
    }
  }
}

void BrowserSearchBar::mouseDown(const juce::MouseEvent &e) {
  if (searchBoxBounds_.contains(e.getPosition())) {
    grabKeyboardFocus();
  }

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
  if (key == juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0) ||
      key == juce::KeyPress('k', juce::ModifierKeys::ctrlModifier, 0)) {
    setCommandMode(true);
    return true;
  }

  if (key.isKeyCode(juce::KeyPress::escapeKey)) {
    if (commandMode_) {
      setCommandMode(false);
      return true;
    }
  }

  if (commandMode_) {
    if (key.isKeyCode(juce::KeyPress::upKey)) {
      if (!visibleCommandIndices_.empty()) {
        selectedCommandIndex_ =
            juce::jlimit(0, (int)visibleCommandIndices_.size() - 1, selectedCommandIndex_ - 1);
        repaint();
      }
      return true;
    }
    if (key.isKeyCode(juce::KeyPress::downKey)) {
      if (!visibleCommandIndices_.empty()) {
        selectedCommandIndex_ =
            juce::jlimit(0, (int)visibleCommandIndices_.size() - 1, selectedCommandIndex_ + 1);
        repaint();
      }
      return true;
    }
    if (key.isKeyCode(juce::KeyPress::returnKey)) {
      executeSelectedCommand();
      return true;
    }
  }

  if (key.isKeyCode(juce::KeyPress::backspaceKey)) {
    if (searchText_.isNotEmpty()) {
      auto nextText = searchText_.dropLastCharacters(1);
      if (commandMode_ && nextText.isEmpty()) {
        nextText = ">";
      }
      setSearchText(nextText);
      return true;
    }
  } else if (key.getTextCharacter() >= 32 && key.getTextCharacter() < 127) {
    setSearchText(searchText_ + juce::String::charToString(key.getTextCharacter()));
    if (searchText_.startsWithChar('>')) {
      setCommandMode(true);
    }
    return true;
  } else if (key.isKeyCode(juce::KeyPress::returnKey)) {
    if (!commandMode_ && searchText_.startsWithChar('>')) {
      setCommandMode(true);
      return true;
    }
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
  commandPaletteBounds_ = juce::Rectangle<int>(searchBoxBounds_.getX(),
                                               searchBoxBounds_.getBottom() + 4,
                                               searchBoxBounds_.getWidth(),
                                               5 * 22 + 8);
}

void BrowserSearchBar::updateVisibleCommands() {
  visibleCommandIndices_.clear();
  juce::String query = searchText_;
  if (query.startsWithChar('>')) {
    query = query.substring(1);
  }
  query = query.trim().toLowerCase();

  for (int i = 0; i < (int)kCommands.size(); ++i) {
    const juce::String id(kCommands[(size_t)i].id);
    const juce::String label(kCommands[(size_t)i].label);
    if (query.isEmpty() || id.toLowerCase().contains(query) ||
        label.toLowerCase().contains(query)) {
      visibleCommandIndices_.push_back(i);
    }
  }
  selectedCommandIndex_ =
      juce::jlimit(0, juce::jmax(0, (int)visibleCommandIndices_.size() - 1),
                   selectedCommandIndex_);
}

void BrowserSearchBar::executeSelectedCommand() {
  if (visibleCommandIndices_.empty()) {
    return;
  }
  const auto commandId = juce::String(
      kCommands[(size_t)visibleCommandIndices_[(size_t)selectedCommandIndex_]].id);
  if (onCommandExecuted) {
    onCommandExecuted(commandId);
  }
  setCommandMode(false);
  setSearchText("");
}

} // namespace zenith
