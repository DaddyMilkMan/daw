/*
  ==============================================================================

    BrowserSearchBar.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserSearchBar.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
#include <algorithm>
#include <array>

namespace {
struct CommandEntry {
  const char *id;
  const char *label;
};

const std::vector<CommandEntry> kCommands{{
    {"add-folder", "Add Library Folder"},
    {"refresh-library", "Refresh Browser Library"},
    {"toggle-autoplay", "Toggle Preview Auto-Play"},
    {"clear-search", "Clear Search"},
    {"focus-list", "Focus Browser List"},
    // Global Commands
    {"play", "Transport: Play"},
    {"stop", "Transport: Stop"},
    {"record", "Transport: Record"},
    {"toggle-view", "View: Toggle Session/Arranger"},
    {"toggle-wingman", "View: Toggle AI Assistant"},
    {"toggle-browser", "View: Toggle Browser"},
    {"undo", "Edit: Undo"},
    {"redo", "Edit: Redo"},
}};
} // namespace

namespace zenith {

BrowserSearchBar::BrowserSearchBar() {
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
}

BrowserSearchBar::~BrowserSearchBar() {}

void BrowserSearchBar::setSearchText(const juce::String &text) {
  if (searchText_ != text) {
    searchText_ = text;
    if (commandMode_) {
      updateVisibleCommands();
    }
    if (onSearchChanged)
      onSearchChanged(searchText_);
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

  SkPaint headerBg;
  headerBg.setColor(design::colors::BG_02);
  canvas->drawRect(SkRect::MakeWH(w, h), headerBg);

  SkPaint borderPaint;
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  canvas->drawLine(0, h - 0.5f, w, h - 0.5f, borderPaint);

  if (backButtonVisible_) {
    const SkRect backRect = SkRect::MakeXYWH(
        (float)backButtonBounds_.getX(), (float)backButtonBounds_.getY(),
        (float)backButtonBounds_.getWidth(),
        (float)backButtonBounds_.getHeight());
    SkPaint backBg;
    backBg.setColor(design::colors::BG_03);
    backBg.setAntiAlias(true);
    canvas->drawRoundRect(backRect, design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, backBg);
    SkPaint backBorder;
    backBorder.setColor(design::colors::BORDER_DEFAULT);
    backBorder.setStyle(SkPaint::kStroke_Style);
    backBorder.setStrokeWidth(1.0f);
    backBorder.setAntiAlias(true);
    canvas->drawRoundRect(
        backRect.makeInset(0.5f, 0.5f), design::dimensions::RADIUS_SM,
        design::dimensions::RADIUS_SM, backBorder);

    icons::IconStyle backIconStyle;
    backIconStyle.color = design::colors::TEXT_PRIMARY;
    backIconStyle.strokeWidth = 1.8f;
    icons::drawIconCentered(
        canvas, icons::ChevronLeft(),
        SkRect::MakeXYWH((float)backButtonBounds_.getX(),
                         (float)backButtonBounds_.getY(),
                         (float)backButtonBounds_.getWidth(),
                         (float)backButtonBounds_.getHeight()),
        13.0f, backIconStyle);
  } else {
    SkRect searchRect = SkRect::MakeXYWH((float)searchBoxBounds_.getX(),
                                         (float)searchBoxBounds_.getY(),
                                         (float)searchBoxBounds_.getWidth(),
                                         (float)searchBoxBounds_.getHeight());

    SkPaint searchBgPaint;
    searchBgPaint.setColor(design::colors::BG_01);
    searchBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(searchRect, design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, searchBgPaint);
    SkPaint searchBorder;
    searchBorder.setColor(commandMode_ ? design::colors::BORDER_FOCUS
                                       : design::colors::BORDER_DEFAULT);
    searchBorder.setStyle(SkPaint::kStroke_Style);
    searchBorder.setStrokeWidth(1.0f);
    searchBorder.setAntiAlias(true);
    canvas->drawRoundRect(
        searchRect.makeInset(0.5f, 0.5f), design::dimensions::RADIUS_SM,
        design::dimensions::RADIUS_SM, searchBorder);

    icons::IconStyle searchIconStyle;
    searchIconStyle.color = design::withAlpha(design::colors::TEXT_SECONDARY, 0.9f);
    searchIconStyle.strokeWidth = 1.8f;
    icons::drawIconCentered(
        canvas, icons::Search(),
        SkRect::MakeXYWH((float)searchBoxBounds_.getX() + 10.0f,
                         (float)searchBoxBounds_.getY() + 7.0f, 14.0f, 14.0f),
        11.0f, searchIconStyle);

    SkFont searchFont = design::getSkFont(13.0f, design::FontWeight::Regular);
    SkPaint textPaint;
    textPaint.setColor(searchText_.isEmpty() ? design::colors::TEXT_TERTIARY
                                             : design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);

    juce::String displayText =
        searchText_.isEmpty()
            ? (commandMode_ ? "> command-palette" : "Search library...")
            : searchText_;
    canvas->drawString(displayText.toStdString().c_str(),
                       (float)searchBoxBounds_.getX() + 32,
                       (float)searchBoxBounds_.getCentreY() + 4, searchFont,
                       textPaint);
  }

  SkRect addBtnRect =
      SkRect::MakeXYWH((float)addFolderButtonBounds_.getX(),
                       (float)addFolderButtonBounds_.getY(),
                       (float)addFolderButtonBounds_.getWidth(),
                       (float)addFolderButtonBounds_.getHeight());

  SkPaint addBtnPaint;
  addBtnPaint.setColor(design::colors::BG_03);
  addBtnPaint.setAntiAlias(true);
  canvas->drawRoundRect(addBtnRect, design::dimensions::RADIUS_SM,
                        design::dimensions::RADIUS_SM, addBtnPaint);

  SkPaint btnBorder;
  btnBorder.setColor(design::colors::BORDER_DEFAULT);
  btnBorder.setStyle(SkPaint::kStroke_Style);
  btnBorder.setStrokeWidth(1.0f);
  btnBorder.setAntiAlias(true);
  canvas->drawRoundRect(addBtnRect, design::dimensions::RADIUS_SM,
                        design::dimensions::RADIUS_SM, btnBorder);

  icons::IconStyle plusIconStyle;
  plusIconStyle.color = design::colors::TEXT_PRIMARY;
  plusIconStyle.strokeWidth = 1.8f;
  icons::drawIconCentered(
      canvas, icons::Plus(),
      SkRect::MakeXYWH((float)addFolderButtonBounds_.getX(),
                       (float)addFolderButtonBounds_.getY(),
                       (float)addFolderButtonBounds_.getWidth(),
                       (float)addFolderButtonBounds_.getHeight()),
      12.0f, plusIconStyle);

  if (commandMode_) {
    const SkRect paletteRect =
        SkRect::MakeXYWH((float)commandPaletteBounds_.getX(),
                         (float)commandPaletteBounds_.getY(),
                         (float)commandPaletteBounds_.getWidth(),
                         (float)commandPaletteBounds_.getHeight());
    SkPaint palettePaint;
    palettePaint.setColor(design::colors::BG_02);
    palettePaint.setAntiAlias(true);
    canvas->drawRoundRect(paletteRect, design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, palettePaint);
    SkPaint paletteBorder;
    paletteBorder.setColor(design::colors::BORDER_DEFAULT);
    paletteBorder.setStyle(SkPaint::kStroke_Style);
    paletteBorder.setStrokeWidth(1.0f);
    paletteBorder.setAntiAlias(true);
    canvas->drawRoundRect(paletteRect.makeInset(0.5f, 0.5f),
                          design::dimensions::RADIUS_SM,
                          design::dimensions::RADIUS_SM, paletteBorder);

    if (visibleCommandIndices_.empty()) {
      SkFont emptyFont = design::getSkFont(11.0f, design::FontWeight::Regular);
      SkPaint emptyPaint;
      emptyPaint.setAntiAlias(true);
      emptyPaint.setColor(design::colors::TEXT_SECONDARY);
      canvas->drawString(
          "No matching commands", (float)commandPaletteBounds_.getX() + 10.0f,
          (float)commandPaletteBounds_.getY() + 18.0f, emptyFont, emptyPaint);
    } else {
      constexpr int rowH = 22;
      for (int i = 0; i < (int)visibleCommandIndices_.size(); ++i) {
        const int commandIndex = visibleCommandIndices_[(size_t)i];
        const auto &item = kCommands[(size_t)commandIndex];
        const int rowY = commandPaletteBounds_.getY() + i * rowH;

        if (i == selectedCommandIndex_) {
          SkPaint selPaint;
          selPaint.setAntiAlias(true);
          selPaint.setColor(
              design::withAlpha(design::colors::ACCENT_PRIMARY, 0.22f));
          canvas->drawRoundRect(
              SkRect::MakeXYWH((float)commandPaletteBounds_.getX() + 4.0f,
                               (float)rowY + 2.0f,
                               (float)commandPaletteBounds_.getWidth() - 8.0f,
                               (float)rowH - 3.0f),
              design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM,
              selPaint);
        }

        SkFont rowFont = design::getSkFont(11.0f, design::FontWeight::SemiBold);
        SkPaint rowPaint;
        rowPaint.setAntiAlias(true);
        rowPaint.setColor(i == selectedCommandIndex_ ? design::colors::TEXT_PRIMARY
                                                     : design::colors::TEXT_SECONDARY);
        canvas->drawString(item.label,
                           (float)commandPaletteBounds_.getX() + 10.0f,
                           (float)rowY + 16.0f, rowFont, rowPaint);
      }
    }
  }
}

void BrowserSearchBar::mouseDown(const juce::MouseEvent &e) {
  if (searchBoxBounds_.contains(e.getPosition())) {
    grabKeyboardFocus();
  }

  if (commandMode_ && commandPaletteBounds_.contains(e.getPosition())) {
    constexpr int rowH = 22;
    const int row = (e.y - commandPaletteBounds_.getY()) / rowH;
    if (row >= 0 && row < (int)visibleCommandIndices_.size()) {
      selectedCommandIndex_ = row;
      executeSelectedCommand();
      return;
    }
  }

  if (backButtonVisible_ && backButtonBounds_.contains(e.getPosition())) {
    if (onBackRequested)
      onBackRequested();
    return;
  }

  if (addFolderButtonBounds_.contains(e.getPosition())) {
    if (onAddFolderRequested)
      onAddFolderRequested();
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
            juce::jlimit(0, (int)visibleCommandIndices_.size() - 1,
                         selectedCommandIndex_ - 1);
        repaint();
      }
      return true;
    }
    if (key.isKeyCode(juce::KeyPress::downKey)) {
      if (!visibleCommandIndices_.empty()) {
        selectedCommandIndex_ =
            juce::jlimit(0, (int)visibleCommandIndices_.size() - 1,
                         selectedCommandIndex_ + 1);
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
    setSearchText(searchText_ +
                  juce::String::charToString(key.getTextCharacter()));
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

  backButtonBounds_ = juce::Rectangle<int>(8, 10, 30, 28);
  addFolderButtonBounds_ =
      juce::Rectangle<int>(w - 42, 9, 34, searchBoxHeight_ - 2);
  searchBoxBounds_ = juce::Rectangle<int>(12, 9, w - 64, searchBoxHeight_ - 2);
  commandPaletteBounds_ = juce::Rectangle<int>(
      searchBoxBounds_.getX(), searchBoxBounds_.getBottom() + 4,
      searchBoxBounds_.getWidth(), 5 * 22 + 8);
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
      kCommands[(size_t)visibleCommandIndices_[(size_t)selectedCommandIndex_]]
          .id);
  if (onCommandExecuted) {
    onCommandExecuted(commandId);
  }
  setCommandMode(false);
  setSearchText("");
}

} // namespace zenith
