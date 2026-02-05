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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    SkiaPopupMenu.cpp
    Created: 2025-12-07
    Enhanced: 2025-12-25
    Author:  Zenith DAW Team

    Premium Skia-based popup menu implementation


  ==============================================================================
*/

#include "SkiaPopupMenu.h"
#include "ZenithDesignSystem.h"
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
// Construction
//==============================================================================

SkiaPopupMenu::SkiaPopupMenu() {
  // Set default appearance from design system
  backgroundColour_ = design::colors::BG_DARKER;
  textColour_ = design::colors::TEXT_PRIMARY;
  highlightColour_ = design::colors::CYAN;
  borderColour_ = design::colors::BORDER_DEFAULT;
  dangerColour_ = SkColorSetRGB(255, 80, 80);
  font_ = design::getSkFont(design::typography::FONT_MD, design::FontWeight::Regular);
  
  setWantsKeyboardFocus(true);
  addKeyListener(this);
}

SkiaPopupMenu::~SkiaPopupMenu() {
  removeKeyListener(this);
  hideMenu();
}

//==============================================================================
// Item Management - Basic
//==============================================================================

int SkiaPopupMenu::addItem(int itemId, const juce::String &text, bool isEnabled,
                           bool isTicked, std::function<void()> callback) {
  Item item;
  item.text = text;
  item.itemId = itemId;
  item.isEnabled = isEnabled;
  item.isTicked = isTicked;
  item.callback = std::move(callback);
  item.isSeparator = false;
  items_.push_back(std::move(item));
  return itemId;
}

int SkiaPopupMenu::addItem(const juce::String &text,
                           std::function<void()> callback) {
  static int autoId = 1000;
  return addItem(autoId++, text, true, false, std::move(callback));
}

//==============================================================================
// Item Management - With Icons
//==============================================================================

int SkiaPopupMenu::addItemWithIcon(int itemId, const juce::String &text,
                                   const SkPath &icon, bool isEnabled,
                                   std::function<void()> callback) {
  Item item;
  item.text = text;
  item.itemId = itemId;
  item.isEnabled = isEnabled;
  item.iconPath = icon;
  item.hasIcon = true;
  item.callback = std::move(callback);
  items_.push_back(std::move(item));
  return itemId;
}

int SkiaPopupMenu::addItemWithShortcut(int itemId, const juce::String &text,
                                       const juce::String &shortcut,
                                       bool isEnabled,
                                       std::function<void()> callback) {
  Item item;
  item.text = text;
  item.itemId = itemId;
  item.isEnabled = isEnabled;
  item.shortcutHint = shortcut;
  item.callback = std::move(callback);
  items_.push_back(std::move(item));
  return itemId;
}

int SkiaPopupMenu::addItemComplete(int itemId, const juce::String &text,
                                   const SkPath &icon,
                                   const juce::String &shortcut,
                                   bool isEnabled, bool isTicked,
                                   bool isDanger,
                                   std::function<void()> callback) {
  Item item;
  item.text = text;
  item.itemId = itemId;
  item.isEnabled = isEnabled;
  item.isTicked = isTicked;
  item.iconPath = icon;
  item.hasIcon = !icon.isEmpty();
  item.shortcutHint = shortcut;
  item.isDanger = isDanger;
  item.callback = std::move(callback);
  items_.push_back(std::move(item));
  return itemId;
}

void SkiaPopupMenu::addSeparator() {
  Item separator;
  separator.isSeparator = true;
  separator.itemId = -1;
  items_.push_back(std::move(separator));
}

void SkiaPopupMenu::addSectionHeader(const juce::String &text) {
  Item header;
  header.text = text;
  header.itemId = -2;  // Special ID for section headers
  header.isEnabled = false;
  items_.push_back(std::move(header));
}

void SkiaPopupMenu::addSubMenu(const juce::String &text,
                               std::unique_ptr<SkiaPopupMenu> subMenu,
                               const SkPath &icon) {
  Item item;
  item.text = text;
  item.itemId = -1;
  item.isEnabled = true;
  item.isSeparator = false;
  item.iconPath = icon;
  item.hasIcon = !icon.isEmpty();
  item.subMenu = std::move(subMenu);
  if (item.subMenu) {
    item.subMenu->parentMenu_ = this;
  }
  items_.push_back(std::move(item));
}

void SkiaPopupMenu::clear() {
  items_.clear();
  hoveredItem_ = -1;
  keyboardFocusedItem_ = -1;
}

juce::String SkiaPopupMenu::getItemText(int index) const {
  if (index >= 0 && index < static_cast<int>(items_.size())) {
    return items_[index].text;
  }
  return {};
}

//==============================================================================
// Show/Hide
//==============================================================================

void SkiaPopupMenu::showAt(juce::Component *component, int x, int y) {
  if (!component)
    return;

  juce::Point<int> screenPos = component->localPointToGlobal(juce::Point<int>(x, y));
  showAt(screenPos);
}

void SkiaPopupMenu::showAt(juce::Point<int> screenPosition) {
  isShowing_ = true;
  hoveredItem_ = -1;
  keyboardFocusedItem_ = -1;
  showAnimation_ = 0.0f;

  calculateSize();
  
  // Position on screen
  setBounds(screenPosition.x, screenPosition.y, getWidth(), getHeight());
  
  // Ensure menu stays on screen
  ensureOnScreen();
  
  setVisible(true);
  toFront(true);
  grabKeyboardFocus();
  
  // Start show animation
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60);
}

void SkiaPopupMenu::showAtMouse() {
  auto mousePos = juce::Desktop::getInstance().getMousePosition();
  showAt(mousePos);
}

void SkiaPopupMenu::hideMenu() {
  isShowing_ = false;
  setVisible(false);
  stopTimer();

  // Hide any submenus
  hideAllSubMenus();

  // Notify parent or call dismiss callback
  if (onDismiss) {
    onDismiss();
  }
}

void SkiaPopupMenu::hideAllSubMenus() {
  for (auto &item : items_) {
    if (item.subMenu) {
      item.subMenu->hideMenu();
    }
  }
  activeSubMenu_.reset();
}

void SkiaPopupMenu::ensureOnScreen() {
  auto displays = juce::Desktop::getInstance().getDisplays();
  auto displayBounds = displays.getPrimaryDisplay()->userArea;
  
  auto bounds = getBounds();
  
  // Adjust horizontal position
  if (bounds.getRight() > displayBounds.getRight()) {
    bounds.setX(displayBounds.getRight() - bounds.getWidth() - 4);
  }
  if (bounds.getX() < displayBounds.getX()) {
    bounds.setX(displayBounds.getX() + 4);
  }
  
  // Adjust vertical position
  if (bounds.getBottom() > displayBounds.getBottom()) {
    bounds.setY(displayBounds.getBottom() - bounds.getHeight() - 4);
  }
  if (bounds.getY() < displayBounds.getY()) {
    bounds.setY(displayBounds.getY() + 4);
  }
  
  setBounds(bounds);
}

//==============================================================================
// Appearance
//==============================================================================

void SkiaPopupMenu::setBackgroundColour(SkColor colour) {
  backgroundColour_ = colour;
  markDirty();
}

void SkiaPopupMenu::setTextColour(SkColor colour) {
  textColour_ = colour;
  markDirty();
}

void SkiaPopupMenu::setHighlightColour(SkColor colour) {
  highlightColour_ = colour;
  markDirty();
}

void SkiaPopupMenu::setBorderColour(SkColor colour) {
  borderColour_ = colour;
  markDirty();
}

void SkiaPopupMenu::setFont(const SkFont &font) {
  font_ = font;
  markDirty();
}

//==============================================================================
// Size Calculation
//==============================================================================

void SkiaPopupMenu::calculateSize() {
  int totalHeight = horizontalPadding_;
  int maxWidth = 180;

  for (const auto &item : items_) {
    if (item.isSeparator) {
      totalHeight += separatorHeight_;
    } else if (item.itemId == -2) {  // Section header
      totalHeight += sectionHeaderHeight_;
    } else {
      totalHeight += itemHeight_;
      
      // Calculate text width
      float textWidth = font_.measureText(item.text.toRawUTF8(),
                                          item.text.length(),
                                          SkTextEncoding::kUTF8);
      
      // Add shortcut width if present
      float shortcutWidth = 0;
      if (item.shortcutHint.isNotEmpty()) {
        shortcutWidth = font_.measureText(item.shortcutHint.toRawUTF8(),
                                          item.shortcutHint.length(),
                                          SkTextEncoding::kUTF8) + shortcutPadding_;
      }
      
      int itemWidth = iconWidth_ + static_cast<int>(textWidth) + 
                      static_cast<int>(shortcutWidth) + horizontalPadding_ * 3;
      
      // Add space for submenu arrow
      if (item.subMenu) {
        itemWidth += 20;
      }
      
      maxWidth = std::max(maxWidth, itemWidth);
    }
  }

  totalHeight += horizontalPadding_;
  setSize(maxWidth, totalHeight);
}

//==============================================================================
// Drawing
//==============================================================================

void SkiaPopupMenu::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float animScale = showAnimation_;
  
  // Apply animation scale
  if (animScale < 1.0f) {
    canvas->save();
    float scale = 0.95f + 0.05f * animScale;
    canvas->translate(bounds.getCentreX(), 0);
    canvas->scale(scale, scale);
    canvas->translate(-bounds.getCentreX(), 0);
  }
  
  // Draw shadow
  drawShadow(canvas);
  
  // Draw backdrop with blur effect
  drawBackdrop(canvas);
  
  // Draw items
  float y = static_cast<float>(horizontalPadding_);

  for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
    const auto &item = items_[i];

    if (item.isSeparator) {
      // Draw separator line
      SkPaint sepPaint;
      sepPaint.setColor(design::colors::BORDER_SUBTLE);
      canvas->drawLine(12.0f, y + separatorHeight_ * 0.5f,
                       bounds.getWidth() - 12.0f, y + separatorHeight_ * 0.5f,
                       sepPaint);
      y += separatorHeight_;
    } else if (item.itemId == -2) {
      // Section header
      SkPaint headerPaint;
      headerPaint.setColor(design::colors::TEXT_TERTIARY);
      headerPaint.setAntiAlias(true);
      
      SkFont headerFont = design::getSkFont(design::typography::FONT_XS, 
                                            design::FontWeight::SemiBold);
      canvas->drawString(item.text.toUpperCase().toRawUTF8(),
                         static_cast<float>(horizontalPadding_ + 4),
                         y + sectionHeaderHeight_ * 0.7f,
                         headerFont, headerPaint);
      y += sectionHeaderHeight_;
    } else {
      SkRect itemRect = SkRect::MakeXYWH(static_cast<float>(horizontalPadding_), y,
                                         bounds.getWidth() - horizontalPadding_ * 2.0f,
                                         static_cast<float>(itemHeight_));
      
      drawItem(canvas, i, item, itemRect);
      y += itemHeight_;
    }
  }
  
  if (animScale < 1.0f) {
    canvas->restore();
  }
}

void SkiaPopupMenu::drawShadow(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 12.0f));
  
  SkRect shadowRect = SkRect::MakeXYWH(4, 6, bounds.getWidth() - 4,
                                       bounds.getHeight() - 4);
  canvas->drawRoundRect(shadowRect, static_cast<float>(cornerRadius_),
                        static_cast<float>(cornerRadius_), shadowPaint);
}

void SkiaPopupMenu::drawBackdrop(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect menuRect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  
  // Glassmorphic background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  
  // Gradient background
  SkPoint gradPoints[2] = {{0, 0}, {0, bounds.getHeight()}};
  SkColor gradColors[2] = {
    SkColorSetARGB(230, 25, 25, 30),
    SkColorSetARGB(245, 18, 18, 22)
  };
  bgPaint.setShader(SkGradientShader::MakeLinear(
      gradPoints, gradColors, nullptr, 2, SkTileMode::kClamp));
  
  canvas->drawRoundRect(menuRect, static_cast<float>(cornerRadius_),
                        static_cast<float>(cornerRadius_), bgPaint);
  
  // Glass highlight at top
  SkPaint highlightPaint;
  highlightPaint.setColor(design::colors::GLASS_HIGHLIGHT);
  highlightPaint.setAntiAlias(true);
  
  SkRect highlightRect = SkRect::MakeXYWH(1, 1, bounds.getWidth() - 2, 1);
  canvas->drawRoundRect(highlightRect, static_cast<float>(cornerRadius_) - 1,
                        static_cast<float>(cornerRadius_) - 1, highlightPaint);
  
  // Border
  SkPaint borderPaint;
  borderPaint.setColor(borderColour_);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas->drawRoundRect(menuRect.makeInset(0.5f, 0.5f),
                        static_cast<float>(cornerRadius_) - 0.5f,
                        static_cast<float>(cornerRadius_) - 0.5f, borderPaint);
}

void SkiaPopupMenu::drawItem(SkCanvas *canvas, int index, const Item &item,
                             const SkRect &bounds) {
  bool isHovered = (index == hoveredItem_);
  bool isFocused = (index == keyboardFocusedItem_);
  bool isHighlighted = (isHovered || isFocused) && item.isEnabled;
  
  // Highlight background
  if (isHighlighted) {
    SkPaint highlightPaint;
    highlightPaint.setColor(design::withAlpha(highlightColour_, 0.2f));
    highlightPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, highlightPaint);
    
    // Left accent bar
    SkPaint accentPaint;
    accentPaint.setColor(highlightColour_);
    canvas->drawRoundRect(SkRect::MakeXYWH(bounds.fLeft, bounds.fTop + 4,
                                           2, bounds.height() - 8),
                          1, 1, accentPaint);
  }
  
  float iconCenterX = bounds.fLeft + iconWidth_ * 0.5f;
  float textY = bounds.centerY() + font_.getSize() * 0.35f;
  
  // Determine text color
  SkColor textColor;
  if (!item.isEnabled) {
    textColor = design::colors::TEXT_DISABLED;
  } else if (item.isDanger) {
    textColor = isHighlighted ? SkColorSetRGB(255, 100, 100) : dangerColour_;
  } else if (isHighlighted) {
    textColor = highlightColour_;
  } else {
    textColor = textColour_;
  }
  
  // Draw icon or checkmark
  SkRect iconBounds = SkRect::MakeXYWH(bounds.fLeft + 4, bounds.fTop + 6,
                                       iconWidth_ - 8, bounds.height() - 12);
  
  if (item.isTicked) {
    drawCheckmark(canvas, iconBounds, textColor);
  } else if (item.hasIcon) {
    drawIcon(canvas, item.iconPath, iconBounds, textColor);
  }
  
  // Draw text
  SkPaint textPaint;
  textPaint.setColor(textColor);
  textPaint.setAntiAlias(true);
  
  float textX = bounds.fLeft + iconWidth_;
  canvas->drawString(item.text.toRawUTF8(), textX, textY, font_, textPaint);
  
  // Draw shortcut hint (right-aligned)
  if (item.shortcutHint.isNotEmpty()) {
    SkPaint shortcutPaint;
    shortcutPaint.setColor(design::colors::TEXT_TERTIARY);
    shortcutPaint.setAntiAlias(true);
    
    SkFont shortcutFont = design::getSkFont(design::typography::FONT_SM,
                                            design::FontWeight::Regular);
    float shortcutWidth = shortcutFont.measureText(item.shortcutHint.toRawUTF8(),
                                                   item.shortcutHint.length(),
                                                   SkTextEncoding::kUTF8);
    
    canvas->drawString(item.shortcutHint.toRawUTF8(),
                       bounds.fRight - shortcutWidth - 8,
                       textY, shortcutFont, shortcutPaint);
  }
  
  // Draw submenu arrow
  if (item.subMenu) {
    SkRect arrowBounds = SkRect::MakeXYWH(bounds.fRight - 20, bounds.fTop,
                                          16, bounds.height());
    drawSubmenuArrow(canvas, arrowBounds, textColor);
  }
}

void SkiaPopupMenu::drawIcon(SkCanvas *canvas, const SkPath &icon,
                             const SkRect &bounds, SkColor color) {
  if (icon.isEmpty()) return;
  
  SkPaint iconPaint;
  iconPaint.setColor(color);
  iconPaint.setAntiAlias(true);
  iconPaint.setStyle(SkPaint::kStroke_Style);
  iconPaint.setStrokeWidth(1.5f);
  iconPaint.setStrokeCap(SkPaint::kRound_Cap);
  iconPaint.setStrokeJoin(SkPaint::kRound_Join);
  
  // Scale and center icon
  SkRect iconBounds = icon.getBounds();
  float scale = std::min(bounds.width() / iconBounds.width(),
                         bounds.height() / iconBounds.height()) * 0.7f;
  
  canvas->save();
  canvas->translate(bounds.centerX(), bounds.centerY());
  canvas->scale(scale, scale);
  canvas->translate(-iconBounds.centerX(), -iconBounds.centerY());
  canvas->drawPath(icon, iconPaint);
  canvas->restore();
}

void SkiaPopupMenu::drawCheckmark(SkCanvas *canvas, const SkRect &bounds,
                                  SkColor color) {
  SkPaint tickPaint;
  tickPaint.setColor(color);
  tickPaint.setStrokeWidth(2.0f);
  tickPaint.setStyle(SkPaint::kStroke_Style);
  tickPaint.setStrokeCap(SkPaint::kRound_Cap);
  tickPaint.setAntiAlias(true);

  float cx = bounds.centerX();
  float cy = bounds.centerY();
  
  SkPath checkPath;
  checkPath.moveTo(cx - 5, cy);
  checkPath.lineTo(cx - 1, cy + 4);
  checkPath.lineTo(cx + 6, cy - 4);
  
  canvas->drawPath(checkPath, tickPaint);
}

void SkiaPopupMenu::drawSubmenuArrow(SkCanvas *canvas, const SkRect &bounds,
                                     SkColor color) {
  SkPaint arrowPaint;
  arrowPaint.setColor(color);
  arrowPaint.setAntiAlias(true);
  
  float cx = bounds.centerX();
  float cy = bounds.centerY();

  SkPath arrow;
  arrow.moveTo(cx - 2, cy - 4);
  arrow.lineTo(cx + 3, cy);
  arrow.lineTo(cx - 2, cy + 4);
  arrow.close();
  
  canvas->drawPath(arrow, arrowPaint);
}

void SkiaPopupMenu::resized() {
  // Layout is calculated in calculateSize()
}

//==============================================================================
// Mouse Events
//==============================================================================

void SkiaPopupMenu::mouseDown(const juce::MouseEvent &e) {
  int item = getItemAtPosition(e.position.toInt());
  if (item >= 0 && items_[item].isEnabled && !items_[item].isSeparator &&
      items_[item].itemId != -2) {
    pressedItem_ = item;
    markDirty();
  }
}

void SkiaPopupMenu::mouseUp(const juce::MouseEvent &e) {
  int item = getItemAtPosition(e.position.toInt());
  if (item >= 0 && item == pressedItem_) {
    handleItemSelected(item);
  }
  pressedItem_ = -1;
  markDirty();
}

void SkiaPopupMenu::mouseMove(const juce::MouseEvent &e) {
  int item = getItemAtPosition(e.position.toInt());
  if (item != hoveredItem_) {
    hoveredItem_ = item;
    keyboardFocusedItem_ = -1;  // Clear keyboard focus on mouse movement
    markDirty();

    // Show submenu if hovering over item with submenu
    if (item >= 0 && items_[item].subMenu && items_[item].isEnabled) {
      showSubMenuForItem(item);
    } else {
      // Hide any active submenu when moving to non-submenu item
      hideAllSubMenus();
    }
  }
}

void SkiaPopupMenu::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  
  // Keep hover if we moved to a submenu
  if (!activeSubMenu_ || !activeSubMenu_->isMenuVisible()) {
    hoveredItem_ = -1;
    markDirty();
  }
}

//==============================================================================
// Keyboard Navigation
//==============================================================================

bool SkiaPopupMenu::keyPressed(const juce::KeyPress &key,
                               juce::Component *originatingComponent) {
  juce::ignoreUnused(originatingComponent);
  
  if (!isShowing_)
    return false;
  
  if (key == juce::KeyPress::escapeKey) {
    hideMenu();
    return true;
  }
  
  if (key == juce::KeyPress::downKey) {
    moveKeyboardFocus(1);
    return true;
  }
  
  if (key == juce::KeyPress::upKey) {
    moveKeyboardFocus(-1);
    return true;
  }
  
  if (key == juce::KeyPress::returnKey) {
    selectKeyboardFocusedItem();
    return true;
  }
  
  if (key == juce::KeyPress::rightKey) {
    // Open submenu if focused item has one
    if (keyboardFocusedItem_ >= 0 &&
        keyboardFocusedItem_ < static_cast<int>(items_.size())) {
      auto &item = items_[keyboardFocusedItem_];
      if (item.subMenu && item.isEnabled) {
        showSubMenuForItem(keyboardFocusedItem_);
        item.subMenu->grabKeyboardFocus();
        item.subMenu->moveKeyboardFocus(1);
        return true;
      }
    }
  }
  
  if (key == juce::KeyPress::leftKey) {
    // Close submenu and return to parent
    if (parentMenu_) {
      hideMenu();
      parentMenu_->grabKeyboardFocus();
      return true;
    }
  }
  
  return false;
}

void SkiaPopupMenu::moveKeyboardFocus(int delta) {
  if (items_.empty())
    return;
  
  int newFocus = keyboardFocusedItem_;
  int attempts = static_cast<int>(items_.size());
  
  do {
    newFocus += delta;
    
    // Wrap around
    if (newFocus < 0)
      newFocus = static_cast<int>(items_.size()) - 1;
    if (newFocus >= static_cast<int>(items_.size()))
      newFocus = 0;
    
    // Skip separators and section headers
    if (!items_[newFocus].isSeparator && items_[newFocus].itemId != -2 &&
        items_[newFocus].isEnabled) {
      keyboardFocusedItem_ = newFocus;
      hoveredItem_ = -1;
      markDirty();
      return;
    }
    
    --attempts;
  } while (attempts > 0);
}

void SkiaPopupMenu::selectKeyboardFocusedItem() {
  if (keyboardFocusedItem_ >= 0 &&
      keyboardFocusedItem_ < static_cast<int>(items_.size())) {
    handleItemSelected(keyboardFocusedItem_);
  }
}

//==============================================================================
// Item Selection
//==============================================================================

int SkiaPopupMenu::getItemAtPosition(juce::Point<int> position) const {
  float y = static_cast<float>(horizontalPadding_);

  for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
    const auto &item = items_[i];
    float itemH;
    
    if (item.isSeparator) {
      itemH = static_cast<float>(separatorHeight_);
    } else if (item.itemId == -2) {
      itemH = static_cast<float>(sectionHeaderHeight_);
    } else {
      itemH = static_cast<float>(itemHeight_);
    }

    if (position.y >= y && position.y < y + itemH) {
      if (item.isSeparator || item.itemId == -2)
        return -1;
      return i;
    }

    y += itemH;
  }

  return -1;
}

juce::Rectangle<int> SkiaPopupMenu::getItemBounds(int itemIndex) const {
  float y = static_cast<float>(horizontalPadding_);

  for (int i = 0; i < itemIndex && i < static_cast<int>(items_.size()); ++i) {
    if (items_[i].isSeparator) {
      y += separatorHeight_;
    } else if (items_[i].itemId == -2) {
      y += sectionHeaderHeight_;
    } else {
      y += itemHeight_;
    }
  }

  return juce::Rectangle<int>(horizontalPadding_, static_cast<int>(y),
                              getWidth() - horizontalPadding_ * 2, itemHeight_);
}

void SkiaPopupMenu::handleItemSelected(int itemIndex) {
  if (itemIndex < 0 || itemIndex >= static_cast<int>(items_.size()))
    return;

  const auto &item = items_[itemIndex];

  if (!item.isEnabled)
    return;

  if (item.subMenu) {
    // Show submenu
    showSubMenuForItem(itemIndex);
  } else if (item.callback) {
    item.callback();
    hideMenu();
    
    // Also hide parent menus
    auto *parent = parentMenu_;
    while (parent) {
      parent->hideMenu();
      parent = parent->parentMenu_;
    }
  } else {
    hideMenu();
  }
}

void SkiaPopupMenu::showSubMenuForItem(int itemIndex) {
  if (itemIndex < 0 || itemIndex >= static_cast<int>(items_.size()))
    return;
  
  auto &item = items_[itemIndex];
  if (!item.subMenu)
    return;
  
  // Hide other submenus first
  for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
    if (i != itemIndex && items_[i].subMenu) {
      items_[i].subMenu->hideMenu();
    }
  }
  
  auto bounds = getItemBounds(itemIndex);
  auto screenPos = localPointToGlobal(juce::Point<int>(getWidth() - 4, bounds.getY()));
  item.subMenu->showAt(screenPos);
}

//==============================================================================
// Animation
//==============================================================================

void SkiaPopupMenu::timerCallback() {
  if (isShowing_ && showAnimation_ < 1.0f) {
    showAnimation_ += 0.15f;
    if (showAnimation_ >= 1.0f) {
      showAnimation_ = 1.0f;
      stopTimer();
    }
    markDirty();
  }
}

} // namespace zenith