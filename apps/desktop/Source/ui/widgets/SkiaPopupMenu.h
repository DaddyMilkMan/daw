/*
  ==============================================================================

    SkiaPopupMenu.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based popup menu component to replace juce::PopupMenu

  ==============================================================================
*/

#pragma once

#include "SkiaButton.h"
#include "SkiaComponent.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <utility>
#include <vector>

namespace zenith {

class SkiaPopupMenu : public SkiaComponent {
public:
  struct Item {
    juce::String text;
    int itemId = 0;
    bool isSeparator = false;
    bool isEnabled = true;
    bool isTicked = false;
    juce::String icon;
    std::function<void()> callback;
    std::unique_ptr<SkiaPopupMenu> subMenu;
  };

  SkiaPopupMenu();
  ~SkiaPopupMenu() override;

  // Item management
  int addItem(int itemId, const juce::String &text, bool isEnabled = true,
              bool isTicked = false, std::function<void()> callback = {});
  int addItem(const juce::String &text, std::function<void()> callback = {});
  void addSeparator();
  void addSubMenu(const juce::String &text,
                  std::unique_ptr<SkiaPopupMenu> subMenu);

  // Show menu
  void showAt(juce::Component *component, int x, int y);
  void showAt(juce::Point<int> screenPosition);

  // Appearance
  void setBackgroundColour(SkColor colour);
  void setTextColour(SkColor colour);
  void setHighlightColour(SkColor colour);
  void setBorderColour(SkColor colour);
  void setFont(const SkFont &font);

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

private:
  std::vector<Item> items_;
  std::unique_ptr<SkiaPopupMenu> activeSubMenu_;

  // Appearance
  SkColor backgroundColour_;
  SkColor textColour_;
  SkColor highlightColour_;
  SkColor borderColour_;
  SkFont font_;

  // State
  int hoveredItem_ = -1;
  int pressedItem_ = -1;
  bool isShowing_ = false;

  // Layout
  int itemHeight_ = 30;
  int iconWidth_ = 30;
  int separatorHeight_ = 8;

  // Internal methods
  void layoutItems();
  int getItemAtPosition(juce::Point<int> position) const;
  juce::Rectangle<int> getItemBounds(int itemIndex) const;
  void handleItemSelected(int itemIndex);
  void hideMenu();

  // Parent menu for submenus
  SkiaPopupMenu *parentMenu_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPopupMenu)
};

} // namespace zenith