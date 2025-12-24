/*
  ==============================================================================

    ZenithDropdown.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the premium dropdown.

  ==============================================================================
*/

#include "ZenithDropdown.h"

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithDropdown::ZenithDropdown() : label_("") {}

ZenithDropdown::ZenithDropdown(const juce::String &label) : label_(label) {}

void ZenithDropdown::addItem(const juce::String &item, int itemId) {
  items_.add({item, itemId != 0 ? itemId : items_.size() + 1});
  repaint();
}

void ZenithDropdown::addItems(const juce::StringArray &items) {
  for (const auto &item : items) {
    addItem(item);
  }
}

void ZenithDropdown::clear() {
  items_.clear();
  selectedIndex_ = -1;
  repaint();
}

void ZenithDropdown::setSelectedIndex(int index, bool sendNotification) {
  if (index >= -1 && index < items_.size() && index != selectedIndex_) {
    selectedIndex_ = index;
    repaint();

    if (sendNotification) {
      if (onSelectionChanged) {
        onSelectionChanged(selectedIndex_);
      }
      if (onChange) {
        onChange();
      }
    }
  }
}

void ZenithDropdown::setSelectedId(int id, bool sendNotification) {
  for (int i = 0; i < items_.size(); ++i) {
    if (items_[i].id == id) {
      setSelectedIndex(i, sendNotification);
      return;
    }
  }
}

int ZenithDropdown::getSelectedId() const {
  if (selectedIndex_ >= 0 && selectedIndex_ < items_.size()) {
    return items_[selectedIndex_].id;
  }
  return 0;
}

juce::String ZenithDropdown::getSelectedText() const {
  if (selectedIndex_ >= 0 && selectedIndex_ < items_.size()) {
    return items_[selectedIndex_].text;
  }
  return placeholder_;
}

void ZenithDropdown::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  showPopupMenu();
}

void ZenithDropdown::mouseEnter(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = true;
  repaint();
}

void ZenithDropdown::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hovered_ = false;
  repaint();
}

void ZenithDropdown::showPopupMenu() {
  juce::PopupMenu menu;

  for (int i = 0; i < items_.size(); ++i) {
    menu.addItem(i + 1, items_[i].text, true, i == selectedIndex_);
  }

  isOpen_ = true;
  repaint();

  menu.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(this).withMinimumWidth(
          getWidth()),
      [this](int result) {
        isOpen_ = false;
        repaint();

        if (result > 0) {
          setSelectedIndex(result - 1, true);
        }
      });
}

void ZenithDropdown::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  drawBackground(canvas);
  drawText(canvas);
  drawChevron(canvas);
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

void ZenithDropdown::drawBackground(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  SkRRect rrect = SkRRect::MakeRectXY(rect, 6.0f, 6.0f);

  SkPaint paint;
  paint.setAntiAlias(true);

  // Background gradient (glass effect)
  SkPoint pts[2] = {{0, 0}, {0, bounds.getHeight()}};
  SkColor colors[2] = {SkColorSetARGB(hovered_ ? 160 : 140, 40, 40, 50),
                       SkColorSetARGB(hovered_ ? 120 : 100, 30, 30, 40)};

  paint.setStyle(SkPaint::kFill_Style);
  paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                               SkTileMode::kClamp));
  canvas->drawRRect(rrect, paint);
  paint.setShader(nullptr);

  // Hover/open glow
  if (hovered_ || isOpen_) {
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(SkColorSetA(accentColor_, 150));
    paint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kNormal, 4.0f));
    canvas->drawRRect(rrect, paint);
    paint.setMaskFilter(nullptr);
  }

  // Border
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(SkColorSetARGB(80, 255, 255, 255));
  canvas->drawRRect(rrect, paint);
}

void ZenithDropdown::drawText(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  SkFont font;
  font.setSize(12.0f);
  font.setSubpixel(true);

  SkPaint paint;
  paint.setAntiAlias(true);

  juce::String displayText = getSelectedText();
  bool isPlaceholder = (selectedIndex_ < 0);

  paint.setColor(isPlaceholder ? SkColorSetARGB(120, 200, 200, 220)
                               : SkColorSetARGB(220, 255, 255, 255));

  std::string str = displayText.toStdString();
  float textY = bounds.getHeight() / 2 + 4.0f;

  canvas->drawSimpleText(str.c_str(), str.length(), SkTextEncoding::kUTF8,
                         12.0f, textY, font, paint);
}

void ZenithDropdown::drawChevron(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  float chevronSize = 6.0f;
  float chevronX = bounds.getWidth() - 16.0f;
  float chevronY = bounds.getHeight() / 2.0f;

  SkPath chevronPath;
  if (isOpen_) {
    // Up arrow
    chevronPath.moveTo(chevronX - chevronSize, chevronY + chevronSize / 2);
    chevronPath.lineTo(chevronX, chevronY - chevronSize / 2);
    chevronPath.lineTo(chevronX + chevronSize, chevronY + chevronSize / 2);
  } else {
    // Down arrow
    chevronPath.moveTo(chevronX - chevronSize, chevronY - chevronSize / 2);
    chevronPath.lineTo(chevronX, chevronY + chevronSize / 2);
    chevronPath.lineTo(chevronX + chevronSize, chevronY - chevronSize / 2);
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(2.0f);
  paint.setStrokeCap(SkPaint::kRound_Cap);
  paint.setStrokeJoin(SkPaint::kRound_Join);
  paint.setColor(SkColorSetARGB(180, 200, 200, 220));

  canvas->drawPath(chevronPath, paint);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
