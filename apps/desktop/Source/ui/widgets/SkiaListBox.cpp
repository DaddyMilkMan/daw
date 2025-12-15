/*
  ==============================================================================

    SkiaListBox.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based list box implementation

  ==============================================================================
*/

#include "SkiaListBox.h"
#include "ZenithDesignSystem.h"

namespace zenith {

SkiaListBox::SkiaListBox(const juce::String &componentName) {
  setName(componentName);

  // Set default appearance
  backgroundColour_ = design::colors::BG_DARKER;
  textColour_ = design::colors::TEXT_PRIMARY;
  selectedBackgroundColour_ = design::colors::CYAN;
  selectedTextColour_ = design::colors::BG_DARKEST;
  borderColour_ = design::colors::BORDER_DEFAULT;

  font_.setSize(design::typography::FONT_MD);
}

SkiaListBox::~SkiaListBox() { stopAllAnimations(); }

void SkiaListBox::setModel(Model *model) {
  if (model_ != model) {
    model_ = model;
    updateContent();
  }
}

void SkiaListBox::setSelectedRows(
    const juce::SparseSet<int> &setOfRowsToBeSelected, bool sendNotification) {
  selectedRows_ = setOfRowsToBeSelected;

  // Find the last selected row
  lastRowSelected_ = -1;
  for (int i = selectedRows_.getNumRanges() - 1; i >= 0; --i) {
    juce::Range<int> range = selectedRows_.getRange(i);
    if (!range.isEmpty()) {
      lastRowSelected_ = range.getEnd() - 1; // Last item in the range
      break;
    }
  }

  if (sendNotification && onSelectionChange) {
    onSelectionChange();
  }

  markDirty();
}

void SkiaListBox::selectRow(int rowNumber, bool sendNotification,
                            bool deselectOthersFirst) {
  if (deselectOthersFirst) {
    selectedRows_.clear();
  }

  if (rowNumber >= 0 && rowNumber < (model_ ? model_->getNumRows() : 0)) {
    selectedRows_.addRange(juce::Range<int>(rowNumber, rowNumber + 1));
    lastRowSelected_ = rowNumber;

    ensureRowIsVisible(rowNumber);

    if (sendNotification && onSelectionChange) {
      onSelectionChange();
    }
  }

  markDirty();
}

void SkiaListBox::deselectRow(int rowNumber) {
  selectedRows_.removeRange(juce::Range<int>(rowNumber, rowNumber + 1));
  markDirty();
}

void SkiaListBox::deselectAllRows() {
  selectedRows_.clear();
  lastRowSelected_ = -1;
  markDirty();
}

bool SkiaListBox::isRowSelected(int rowNumber) const {
  return selectedRows_.contains(rowNumber);
}

int SkiaListBox::getSelectedRow() const { return lastRowSelected_; }

void SkiaListBox::setRowHeight(int newHeight) {
  if (rowHeight_ != newHeight) {
    rowHeight_ = newHeight;
    markDirty();
  }
}

void SkiaListBox::setOutlineThickness(int thickness) {
  outlineThickness_ = thickness;
  markDirty();
}

void SkiaListBox::setColour(SkColor colour) {
  backgroundColour_ = colour;
  markDirty();
}

void SkiaListBox::scrollToEnsureRowIsOnscreen(int rowNumber) {
  ensureRowIsVisible(rowNumber);
}

int SkiaListBox::getRowContainingPosition(juce::Point<int> position) const {
  return getRowContainingPosition(position.x, position.y);
}

int SkiaListBox::getRowContainingPosition(int x, int y) const {
  juce::ignoreUnused(x);

  if (!model_ || rowHeight_ <= 0) {
    return -1;
  }

  int row = static_cast<int>((y + scrollY_) / rowHeight_);

  if (row >= 0 && row < model_->getNumRows()) {
    return row;
  }

  return -1;
}

void SkiaListBox::updateContent() {
  scrollY_ = 0.0f;
  targetScrollY_ = 0.0f;
  markDirty();
}

void SkiaListBox::repaintRow(int rowNumber) {
  juce::ignoreUnused(rowNumber);
  markDirty();
}

void SkiaListBox::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Draw background
  SkPaint bgPaint;
  bgPaint.setColor(backgroundColour_);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  // Draw border
  if (outlineThickness_ > 0) {
    SkPaint borderPaint;
    borderPaint.setColor(borderColour_);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(static_cast<float>(outlineThickness_));
    borderPaint.setAntiAlias(true);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                     borderPaint);
  }

  // Draw rows
  if (model_ && model_->getNumRows() > 0) {
    canvas->save();
    canvas->clipRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()));
    canvas->translate(0.0f, -scrollY_);

    int firstVisibleRow = static_cast<int>(scrollY_ / rowHeight_);
    int lastVisibleRow = juce::jmin(
        model_->getNumRows() - 1,
        static_cast<int>((scrollY_ + bounds.getHeight()) / rowHeight_));

    for (int row = firstVisibleRow; row <= lastVisibleRow; ++row) {
      auto rowBounds = getRowBounds(row);

      // Check if row is selected
      bool isSelected = isRowSelected(row);

      // Draw row background
      if (isSelected) {
        SkPaint selectedBgPaint;
        selectedBgPaint.setColor(selectedBackgroundColour_);
        canvas->drawRect(SkRect::MakeXYWH(rowBounds.getX(), rowBounds.getY(),
                                          rowBounds.getWidth(),
                                          rowBounds.getHeight()),
                         selectedBgPaint);
      }

      // Let model paint the item
      model_->paintListBoxItem(row, *canvas, rowBounds.getWidth(),
                               rowBounds.getHeight(), isSelected);
    }

    canvas->restore();
  }
}

void SkiaListBox::resized() {
  // Layout logic can be added here
}

void SkiaListBox::mouseDown(const juce::MouseEvent &e) {
  if (!model_)
    return;

  int row = getRowContainingPosition(e.position.toInt());

  if (row >= 0 && row < model_->getNumRows()) {
    updateSelection(row, !e.mods.isCommandDown(), true);

    if (onRowClicked) {
      onRowClicked(row);
    }

    model_->listBoxItemClicked(row, e);
  } else {
    deselectAllRows();
  }
}

void SkiaListBox::mouseDoubleClick(const juce::MouseEvent &e) {
  if (!model_)
    return;

  int row = getRowContainingPosition(e.position.toInt());

  if (row >= 0 && row < model_->getNumRows()) {
    if (onRowDoubleClicked) {
      onRowDoubleClicked(row);
    }

    model_->listBoxItemDoubleClicked(row, e);
  }
}

void SkiaListBox::mouseWheelMove(const juce::MouseEvent &e,
                                 const juce::MouseWheelDetails &wheel) {
  juce::ignoreUnused(e);

  if (model_ && model_->getNumRows() > 0) {
    float scrollAmount =
        wheel.deltaY * rowHeight_ * 3.0f; // 3 rows per wheel tick
    targetScrollY_ = juce::jlimit(
        0.0f, static_cast<float>(getTotalContentHeight() - getHeight()),
        targetScrollY_ - scrollAmount);
  }
}

int SkiaListBox::getTotalContentHeight() const {
  if (!model_)
    return 0;
  return model_->getNumRows() * rowHeight_;
}

int SkiaListBox::getVisibleRowCount() const {
  if (rowHeight_ <= 0)
    return 0;
  return getHeight() / rowHeight_ + 2; // +2 for partial rows at top/bottom
}

juce::Rectangle<int> SkiaListBox::getRowBounds(int rowNumber) const {
  return juce::Rectangle<int>(0, rowNumber * rowHeight_, getWidth(),
                              rowHeight_);
}

void SkiaListBox::updateSelection(int rowNumber, bool deselectOthers,
                                  bool sendNotification) {
  if (deselectOthers) {
    selectedRows_.clear();
  }

  selectedRows_.addRange(juce::Range<int>(rowNumber, rowNumber + 1));
  lastRowSelected_ = rowNumber;

  if (sendNotification && onSelectionChange) {
    onSelectionChange();
  }

  markDirty();
}

void SkiaListBox::ensureRowIsVisible(int rowNumber) {
  if (rowNumber < 0 || !model_ || rowNumber >= model_->getNumRows()) {
    return;
  }

  auto rowBounds = getRowBounds(rowNumber);

  // Scroll up if row is above visible area
  if (rowBounds.getY() < scrollY_) {
    targetScrollY_ = static_cast<float>(rowBounds.getY());
  }
  // Scroll down if row is below visible area
  else if (rowBounds.getBottom() > scrollY_ + getHeight()) {
    targetScrollY_ = static_cast<float>(rowBounds.getBottom() - getHeight());
  }
}

} // namespace zenith