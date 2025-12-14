/*
  ==============================================================================

    SkiaComboBox.cpp
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based combo box implementation

  ==============================================================================
*/

#include "SkiaComboBox.h"
#include "../ZenithDesignSystem.h"

namespace zenith {

SkiaComboBox::SkiaComboBox(const juce::String &componentName) {
  setName(componentName);

  // Create trigger button
  triggerButton_ = std::make_unique<SkiaButton>();
  triggerButton_->setButtonStyle(SkiaButton::Style::Secondary);
  triggerButton_->onClick = [this]() {
    if (isPopupActive_) {
      hidePopup();
    } else {
      showPopup();
    }
  };
  addAndMakeVisible(triggerButton_.get());

  // Set default appearance
  backgroundColour_ = design::colors::BG_DARKER;
  textColour_ = design::colors::TEXT_PRIMARY;
  selectedTextColour_ = design::colors::BG_DARKEST;
  borderColour_ = design::colors::BORDER_DEFAULT;
  font_.setSize(design::typography::FONT_MD);

  textWhenNothingSelected_ = "Select an item...";
  textWhenNoChoicesAvailable_ = "(no choices)";
}

SkiaComboBox::~SkiaComboBox() { hidePopup(); }

void SkiaComboBox::addItem(const juce::String &itemText, int itemId) {
  Item newItem;
  newItem.text = itemText;
  newItem.id = itemId;
  newItem.isSeparator = false;
  items_.add(newItem);

  updateTriggerButtonText();
  markDirty();
}

void SkiaComboBox::addItemList(const juce::StringArray &items,
                               int firstItemId) {
  for (int i = 0; i < items.size(); ++i) {
    addItem(items[i], firstItemId + i);
  }
}

void SkiaComboBox::addSeparator() {
  Item separator;
  separator.text = "";
  separator.id = -1;
  separator.isSeparator = true;
  items_.add(separator);

  markDirty();
}

void SkiaComboBox::clear() {
  items_.clear();
  selectedId_ = 0;
  selectedItemIndex_ = -1;

  updateTriggerButtonText();
  markDirty();
}

void SkiaComboBox::setSelectedId(int itemId, bool sendNotification) {
  if (selectedId_ != itemId) {
    selectedId_ = itemId;

    // Find the item index
    selectedItemIndex_ = -1;
    for (int i = 0; i < items_.size(); ++i) {
      if (items_[i].id == itemId) {
        selectedItemIndex_ = i;
        break;
      }
    }

    updateTriggerButtonText();
    markDirty();

    if (sendNotification && onChange) {
      onChange();
    }
  }
}

void SkiaComboBox::setSelectedItemIndex(int index, bool sendNotification) {
  if (index >= 0 && index < items_.size() && selectedItemIndex_ != index) {
    selectedItemIndex_ = index;
    selectedId_ = items_[index].id;

    updateTriggerButtonText();
    markDirty();

    if (sendNotification && onChange) {
      onChange();
    }
  }
}

juce::String SkiaComboBox::getText() const {
  if (selectedItemIndex_ >= 0 && selectedItemIndex_ < items_.size()) {
    return items_[selectedItemIndex_].text;
  }
  return {};
}

void SkiaComboBox::setTextWhenNothingSelected(const juce::String &text) {
  textWhenNothingSelected_ = text;
  updateTriggerButtonText();
  markDirty();
}

void SkiaComboBox::setTextWhenNoChoicesAvailable(const juce::String &text) {
  textWhenNoChoicesAvailable_ = text;
  updateTriggerButtonText();
  markDirty();
}

void SkiaComboBox::drawSkia(SkCanvas *canvas) {
  // The combo box is primarily drawn by its trigger button
  // This method can be extended for custom drawing if needed
  juce::ignoreUnused(canvas);
}

void SkiaComboBox::resized() {
  if (triggerButton_) {
    triggerButton_->setBounds(getLocalBounds());
  }
}

void SkiaComboBox::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Handled by trigger button
}

void SkiaComboBox::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Handled by trigger button
}

void SkiaComboBox::showPopup() {
  if (items_.isEmpty() || isPopupActive_) {
    return;
  }

  isPopupActive_ = true;

  // Create popup list if needed
  if (!popupList_) {
    popupList_ = std::make_unique<SkiaListBox>();
    popupList_->setRowHeight(30);
    popupList_->setColour(backgroundColour_);

    // Create model for popup list
    class PopupModel : public SkiaListBox::Model {
    public:
      PopupModel(SkiaComboBox *combo) : comboBox(combo) {}

      int getNumRows() override { return comboBox->items_.size(); }

      void paintListBoxItem(int rowNumber, SkCanvas &canvas, int width,
                            int height, bool rowIsSelected) override {
        if (rowNumber < 0 || rowNumber >= comboBox->items_.size()) {
          return;
        }

        const auto &item = comboBox->items_[rowNumber];

        if (item.isSeparator) {
          // Draw separator
          SkPaint separatorPaint;
          separatorPaint.setColor(design::colors::BORDER_SUBTLE);
          separatorPaint.setStrokeWidth(1.0f);

          float y = height * 0.5f;
          canvas.drawLine(4.0f, y, width - 4.0f, y, separatorPaint);
        } else {
          // Draw item text
          SkPaint textPaint;
          textPaint.setColor(rowIsSelected ? comboBox->selectedTextColour_
                                           : comboBox->textColour_);
          textPaint.setAntiAlias(true);

          SkFont font;
          font.setSize(design::typography::FONT_MD);

          // Add padding
          float textX = 8.0f;
          float textY = height * 0.5f + font.getSize() * 0.3f;

          canvas.drawString(item.text.toRawUTF8(), textX, textY, font,
                            textPaint);
        }
      }

      void listBoxItemClicked(int rowNumber,
                              const juce::MouseEvent &e) override {
        comboBox->handleItemSelected(rowNumber);
      }

    private:
      SkiaComboBox *comboBox;
    };

    popupModel_.reset(new PopupModel(this));
    popupList_->setModel(popupModel_.get());
  }

  // Configure popup list
  popupList_->deselectAllRows();
  if (selectedItemIndex_ >= 0) {
    popupList_->selectRow(selectedItemIndex_, false);
  }

  // Show popup (this is a simplified version - in a real implementation,
  // you'd want to position this as a popup window/modal)
  // For now, we'll just make it visible in a predefined area
  addAndMakeVisible(popupList_.get());

  // Position popup below the combo box
  int popupHeight = juce::jmin(200, items_.size() * 30 + 4); // Max 200px height
  popupList_->setBounds(0, getHeight(), getWidth(), popupHeight);
}

void SkiaComboBox::hidePopup() {
  if (!isPopupActive_) {
    return;
  }

  isPopupActive_ = false;

  if (popupList_) {
    removeChildComponent(popupList_.get());
  }
}

void SkiaComboBox::handleItemSelected(int itemIndex) {
  if (itemIndex >= 0 && itemIndex < items_.size() &&
      !items_[itemIndex].isSeparator) {
    setSelectedItemIndex(itemIndex, true);
  }
  hidePopup();
}

void SkiaComboBox::updateTriggerButtonText() {
  if (!triggerButton_) {
    return;
  }

  juce::String buttonText;

  if (items_.isEmpty()) {
    buttonText = textWhenNoChoicesAvailable_;
  } else if (selectedItemIndex_ >= 0) {
    buttonText = items_[selectedItemIndex_].text;
  } else {
    buttonText = textWhenNothingSelected_;
  }

  triggerButton_->setButtonText(buttonText);
}

} // namespace zenith