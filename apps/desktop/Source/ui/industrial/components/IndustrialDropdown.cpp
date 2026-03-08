#include "IndustrialDropdown.h"
#include <core/SkFont.h>

namespace zenith::industrial {

IndustrialDropdown::IndustrialDropdown(std::string label,
                                       std::vector<std::string> items,
                                       IndustrialTheme &theme)
    : label_(std::move(label)), items_(std::move(items)), theme_(theme) {}

SkRect IndustrialDropdown::listBounds() const {
  const float itemHeight = 24.0f;
  return SkRect::MakeXYWH(bounds_.x(), bounds_.bottom() + 1.0f, bounds_.width(),
                          itemHeight * static_cast<float>(items_.size()));
}

bool IndustrialDropdown::hitTest(float x, float y) const {
  return bounds_.contains(x, y) || (open_ && listBounds().contains(x, y));
}

void IndustrialDropdown::render(SkCanvas *canvas) {
  if (!isVisible()) {
    return;
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(IndustrialTheme::STEEL_DARK);
  canvas->drawRect(bounds_, paint);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setColor(isHovered_ ? IndustrialTheme::ALUMINUM
                            : IndustrialTheme::SILVER);
  canvas->drawRect(bounds_, paint);

  SkFont font(theme_.getTypeface(IndustrialTheme::FontWeight::Regular), 11.0f);
  font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(IndustrialTheme::LIGHT_GRAY);
  const juce::String upperLabel = juce::String(label_).toUpperCase();
  canvas->drawString(upperLabel.toRawUTF8(), bounds_.x() + 4.0f,
                     bounds_.y() - 2.0f, font, paint);
  if (!items_.empty()) {
    const juce::String currentItem =
        juce::String(items_[selectedIndex_]).toUpperCase();
    canvas->drawString(currentItem.toRawUTF8(), bounds_.x() + 4.0f,
                       bounds_.centerY() + 4.0f, font, paint);
  }
  canvas->drawString("V", bounds_.right() - 10.0f, bounds_.centerY() + 4.0f,
                     font, paint);

  if (!open_) {
    return;
  }

  const auto list = listBounds();
  paint.setColor(IndustrialTheme::GRAPHITE);
  canvas->drawRect(list, paint);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(IndustrialTheme::SILVER);
  canvas->drawRect(list, paint);

  paint.setStyle(SkPaint::kFill_Style);
  for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
    const float top = list.y() + static_cast<float>(i) * 24.0f;
    const SkRect itemRect =
        SkRect::MakeXYWH(list.x(), top, list.width(), 24.0f);
    if (i == selectedIndex_) {
      paint.setColor(IndustrialTheme::ALUMINUM);
      canvas->drawRect(itemRect, paint);
      paint.setColor(IndustrialTheme::WHITE);
    } else if (i == hoveredIndex_) {
      paint.setColor(IndustrialTheme::STEEL);
      canvas->drawRect(itemRect, paint);
      paint.setColor(IndustrialTheme::WHITE);
    } else {
      paint.setColor(IndustrialTheme::LIGHT_GRAY);
    }
    const juce::String itemUpper = juce::String(items_[i]).toUpperCase();
    canvas->drawString(itemUpper.toRawUTF8(), itemRect.x() + 4.0f,
                       itemRect.centerY() + 4.0f, font, paint);
  }
}

void IndustrialDropdown::handleMouseMove(const MouseEvent &e) {
  IndustrialComponent::handleMouseMove(e);
  hoveredIndex_ = -1;
  if (open_ && listBounds().contains(e.x, e.y)) {
    const int idx = static_cast<int>((e.y - listBounds().y()) / 24.0f);
    if (idx >= 0 && idx < static_cast<int>(items_.size())) {
      hoveredIndex_ = idx;
    }
  }
}

void IndustrialDropdown::handleMouseUp(const MouseEvent &e) {
  if (bounds_.contains(e.x, e.y)) {
    open_ = !open_;
    return;
  }

  if (open_ && listBounds().contains(e.x, e.y)) {
    const int idx = static_cast<int>((e.y - listBounds().y()) / 24.0f);
    if (idx >= 0 && idx < static_cast<int>(items_.size())) {
      selectedIndex_ = idx;
    }
    hoveredIndex_ = -1;
    open_ = false;
    return;
  }

  hoveredIndex_ = -1;
  open_ = false;
}

} // namespace zenith::industrial
