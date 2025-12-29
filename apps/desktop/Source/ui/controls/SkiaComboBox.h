/*
  ==============================================================================

    SkiaComboBox.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based combo box component to replace juce::ComboBox

  ==============================================================================
*/

#pragma once

#include "SkiaButton.h"
#include "SkiaComponent.h"
#include "SkiaListBox.h"
#include <juce_core/juce_core.h>

namespace zenith {

class SkiaComboBox : public SkiaComponent {
public:
  SkiaComboBox(const juce::String &componentName = {});
  ~SkiaComboBox() override;

  // Item management
  void addItem(const juce::String &itemText, int itemId);
  void addItemList(const juce::StringArray &items, int firstItemId);
  void addSeparator();
  void clear();

  int getNumItems() const { return items_.size(); }

  // Selection
  void setSelectedId(int itemId, bool sendNotification = true);
  int getSelectedId() const { return selectedId_; }

  void setSelectedItemIndex(int index, bool sendNotification = true);
  int getSelectedItemIndex() const { return selectedItemIndex_; }

  juce::String getText() const;
  juce::String getItemText(int index) const;

  // Appearance
  void setTextWhenNothingSelected(const juce::String &text);
  void setTextWhenNoChoicesAvailable(const juce::String &text);

  // Event callbacks
  std::function<void()> onChange;

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

private:
  struct Item {
    juce::String text;
    int id;
    bool isSeparator;
  };

  juce::Array<Item> items_;
  int selectedId_ = 0;
  int selectedItemIndex_ = -1;

  juce::String textWhenNothingSelected_;
  juce::String textWhenNoChoicesAvailable_;

  bool isPopupActive_ = false;
  std::unique_ptr<SkiaListBox::Model> popupModel_;
  std::unique_ptr<SkiaListBox> popupList_;

  // Button for triggering popup
  std::unique_ptr<SkiaButton> triggerButton_;

  // Appearance
  SkColor backgroundColour_;
  SkColor textColour_;
  SkColor selectedTextColour_;
  SkColor borderColour_;
  SkFont font_;

  void showPopup();
  void hidePopup();
  void handleItemSelected(int itemIndex);
  void updateTriggerButtonText();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaComboBox)
};

} // namespace zenith