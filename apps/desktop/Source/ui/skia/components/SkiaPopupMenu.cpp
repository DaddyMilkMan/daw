/*
  ==============================================================================

    SkiaPopupMenu.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based popup menu implementation

  ==============================================================================
*/

#include "SkiaPopupMenu.h"
#include "../ZenithDesignSystem.h"

namespace zenith {

SkiaPopupMenu::SkiaPopupMenu() {
  // Set default appearance
  backgroundColour_ = design::colors::BG_DARKER;
  textColour_ = design::colors::TEXT_PRIMARY;
  highlightColour_ = design::colors::CYAN;
  borderColour_ = design::colors::BORDER_DEFAULT;
  font_.setSize(design::typography::FONT_MD);
}

SkiaPopupMenu::~SkiaPopupMenu() { hideMenu(); }

int SkiaPopupMenu::addItem(int itemId, const juce::String &text, bool isEnabled,
                           bool isTicked, std::function<void()> callback) {
  Item item;
  item.text = text;
  item.itemId = itemId;
  item.isEnabled = isEnabled;
  item.isTicked = isTicked;
  item.callback = callback;
  item.isSeparator = false;
  items_.add(item);
  return itemId;
}

int SkiaPopupMenu::addItem(const juce::String &text,
                           std::function<void()> callback) {
  static int autoId = 1000;
  return addItem(autoId++, text, true, false, callback);
}

void SkiaPopupMenu::addSeparator() {
  Item separator;
  separator.isSeparator = true;
  separator.itemId = -1;
  items_.add(separator);
}

void SkiaPopupMenu::addSubMenu(const juce::String &text,
                               std::unique_ptr<SkiaPopupMenu> subMenu) {
  Item item;
  item.text = text;
  item.itemId = -1;
  item.isEnabled = true;
  item.isSeparator = false;
  item.subMenu = std::move(subMenu);
  if (item.subMenu) {
    item.subMenu->parentMenu_ = this;
  }
  items_.add(std::move(item));
}

void SkiaPopupMenu::showAt(juce::Component *component, int x, int y) {
  if (!component)
    return;

  juce::Point<int> screenPos =
      component->localPointToGlobal(juce::Point<int>(x, y));
  showAt(screenPos);
}

void SkiaPopupMenu::showAt(juce::Point<int> screenPosition) {
  isShowing_ = true;
  layoutItems();

  // Calculate size
  int totalHeight = 0;
  int maxWidth = 150;

  for (int i = 0; i < items_.size(); ++i) {
    if (items_[i].isSeparator) {
      totalHeight += separatorHeight_;
    } else {
      totalHeight += itemHeight_;
      // Calculate text width
      float textWidth =
          font_.measureText(items_[i].text.toRawUTF8(), items_[i].text.length(),
                            SkTextEncoding::kUTF8);
      maxWidth = std::max(maxWidth, (int)textWidth + iconWidth_ + 40);
    }
  }

  setSize(maxWidth, totalHeight + 4); // +4 for padding

  // Position on screen
  setBounds(screenPosition.x, screenPosition.y, getWidth(), getHeight());
  setVisible(true);
  toFront(true);
}

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

void SkiaPopupMenu::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background with shadow
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));
  canvas->drawRoundRect(
      SkRect::MakeXYWH(2, 2, bounds.getWidth(), bounds.getHeight()), 4.0f, 4.0f,
      shadowPaint);

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(backgroundColour_);
  bgPaint.setAntiAlias(true);
  canvas->drawRoundRect(
      SkRect::MakeWH(bounds.getWidth() - 2, bounds.getHeight() - 2), 4.0f, 4.0f,
      bgPaint);

  // Draw border
  SkPaint borderPaint;
  borderPaint.setColor(borderColour_);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas->drawRoundRect(
      SkRect::MakeWH(bounds.getWidth() - 2, bounds.getHeight() - 2), 4.0f, 4.0f,
      borderPaint);

  // Draw items
  float y = 2.0f;

  for (int i = 0; i < items_.size(); ++i) {
    const auto &item = items_[i];

    if (item.isSeparator) {
      // Draw separator line
      SkPaint sepPaint;
      sepPaint.setColor(design::colors::BORDER_SUBTLE);
      canvas->drawLine(8.0f, y + separatorHeight_ * 0.5f,
                       bounds.getWidth() - 10.0f, y + separatorHeight_ * 0.5f,
                       sepPaint);
      y += separatorHeight_;
    } else {
      SkRect itemRect = SkRect::MakeXYWH(2.0f, y, bounds.getWidth() - 6.0f,
                                         (float)itemHeight_);

      // Highlight hovered item
      if (i == hoveredItem_ && item.isEnabled) {
        SkPaint highlightPaint;
        highlightPaint.setColor(design::withAlpha(highlightColour_, 0.3f));
        canvas->drawRoundRect(itemRect, 2.0f, 2.0f, highlightPaint);
      }

      // Draw tick mark if ticked
      if (item.isTicked) {
        SkPaint tickPaint;
        tickPaint.setColor(highlightColour_);
        tickPaint.setStrokeWidth(2.0f);
        tickPaint.setStyle(SkPaint::kStroke_Style);

        float tickX = 10.0f;
        float tickY = y + itemHeight_ * 0.5f;
        canvas->drawLine(tickX, tickY, tickX + 4, tickY + 4, tickPaint);
        canvas->drawLine(tickX + 4, tickY + 4, tickX + 12, tickY - 4,
                         tickPaint);
      }

      // Draw text
      SkPaint textPaint;
      textPaint.setColor(
          item.isEnabled ? (i == hoveredItem_ ? highlightColour_ : textColour_)
                         : design::colors::TEXT_DISABLED);
      textPaint.setAntiAlias(true);

      float textX = (float)iconWidth_;
      float textY = y + itemHeight_ * 0.5f + font_.getSize() * 0.35f;
      canvas->drawString(item.text.toRawUTF8(), textX, textY, font_, textPaint);

      // Draw submenu arrow if present
      if (item.subMenu) {
        SkPaint arrowPaint;
        arrowPaint.setColor(textColour_);
        arrowPaint.setAntiAlias(true);

        float arrowX = bounds.getWidth() - 16.0f;
        float arrowY = y + itemHeight_ * 0.5f;

        SkPath arrow;
        arrow.moveTo(arrowX, arrowY - 4);
        arrow.lineTo(arrowX + 6, arrowY);
        arrow.lineTo(arrowX, arrowY + 4);
        arrow.close();
        canvas->drawPath(arrow, arrowPaint);
      }

      y += itemHeight_;
    }
  }
}

void SkiaPopupMenu::resized() {
  // Layout is calculated in showAt
}

void SkiaPopupMenu::mouseDown(const juce::MouseEvent &e) {
  int item = getItemAtPosition(e.position.toInt());
  if (item >= 0 && items_[item].isEnabled && !items_[item].isSeparator) {
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
    markDirty();

    // Show submenu if hovering over item with submenu
    if (item >= 0 && items_[item].subMenu) {
      auto bounds = getItemBounds(item);
      items_[item].subMenu->showAt(this, getWidth(), bounds.getY());
    } else {
      // Hide any active submenu
      if (activeSubMenu_) {
        activeSubMenu_->hideMenu();
        activeSubMenu_.reset();
      }
    }
  }
}

void SkiaPopupMenu::layoutItems() {
  // Calculate positions - done dynamically in drawing
}

int SkiaPopupMenu::getItemAtPosition(juce::Point<int> position) const {
  float y = 2.0f;

  for (int i = 0; i < items_.size(); ++i) {
    float itemH =
        items_[i].isSeparator ? (float)separatorHeight_ : (float)itemHeight_;

    if (position.y >= y && position.y < y + itemH) {
      return items_[i].isSeparator ? -1 : i;
    }

    y += itemH;
  }

  return -1;
}

juce::Rectangle<int> SkiaPopupMenu::getItemBounds(int itemIndex) const {
  float y = 2.0f;

  for (int i = 0; i < itemIndex && i < items_.size(); ++i) {
    y += items_[i].isSeparator ? (float)separatorHeight_ : (float)itemHeight_;
  }

  return juce::Rectangle<int>(2, (int)y, getWidth() - 4, itemHeight_);
}

void SkiaPopupMenu::handleItemSelected(int itemIndex) {
  if (itemIndex < 0 || itemIndex >= items_.size())
    return;

  const auto &item = items_[itemIndex];

  if (item.subMenu) {
    // Show submenu
    auto bounds = getItemBounds(itemIndex);
    item.subMenu->showAt(this, getWidth(), bounds.getY());
  } else if (item.callback) {
    item.callback();
    hideMenu();
  } else {
    hideMenu();
  }
}

void SkiaPopupMenu::hideMenu() {
  isShowing_ = false;
  setVisible(false);

  // Hide any submenus
  for (int i = 0; i < items_.size(); ++i) {
    if (items_[i].subMenu) {
      items_[i].subMenu->hideMenu();
    }
  }

  // Notify parent to close if we're a submenu
  if (parentMenu_) {
    // Don't hide parent - let it manage itself
  }
}

} // namespace zenith