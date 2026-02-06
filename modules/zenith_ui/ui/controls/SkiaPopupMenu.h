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

#pragma once

#include "SkiaComponent.h"
#include <core/SkPath.h>
#include <juce_core/juce_core.h>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace zenith {

/**
 * @class SkiaPopupMenu
 * @brief Premium Skia-rendered context menu with full DAW integration
 *
 * Features:
 * - Glassmorphic appearance with backdrop blur
 * - SVG/Path icons for each item
 * - Keyboard shortcut display (right-aligned)
 * - Full keyboard navigation
 * - Smooth animated show/hide
 * - Submenu cascading support
 * - Auto-dismiss on click outside
 */
class SkiaPopupMenu : public SkiaComponent {
public:
  //============================================================================
  // Item Structure
  //============================================================================
  
  struct Item {
    juce::String text;
    int itemId = 0;
    bool isSeparator = false;
    bool isEnabled = true;
    bool isTicked = false;
    
    // Icon as SkPath (optional)
    SkPath iconPath;
    bool hasIcon = false;
    
    // Keyboard shortcut hint (e.g., "Ctrl+D")
    juce::String shortcutHint;
    
    // Callback on selection
    std::function<void()> callback;
    
    // Submenu (optional)
    std::unique_ptr<SkiaPopupMenu> subMenu;
    
    // Danger/destructive action (renders in red)
    bool isDanger = false;
  };

  //============================================================================
  // Construction
  //============================================================================
  
  SkiaPopupMenu();
  ~SkiaPopupMenu() override;

  //============================================================================
  // Item Management
  //============================================================================
  
  /**
   * @brief Add a menu item with full options
   * @param itemId Unique identifier for this item
   * @param text Display text
   * @param isEnabled Whether the item is clickable
   * @param isTicked Whether to show a checkmark
   * @param callback Function to call when selected
   * @return The item ID
   */
  int addItem(int itemId, const juce::String &text, bool isEnabled = true,
              bool isTicked = false, std::function<void()> callback = {});
  
  /**
   * @brief Add a simple menu item with auto-generated ID
   */
  int addItem(const juce::String &text, std::function<void()> callback = {});
  
  /**
   * @brief Add item with icon
   */
  int addItemWithIcon(int itemId, const juce::String &text, const SkPath &icon,
                      bool isEnabled = true, std::function<void()> callback = {});
  
  /**
   * @brief Add item with shortcut hint
   */
  int addItemWithShortcut(int itemId, const juce::String &text,
                          const juce::String &shortcut, bool isEnabled = true,
                          std::function<void()> callback = {});
  
  /**
   * @brief Add item with both icon and shortcut
   */
  int addItemComplete(int itemId, const juce::String &text, const SkPath &icon,
                      const juce::String &shortcut, bool isEnabled = true,
                      bool isTicked = false, bool isDanger = false,
                      std::function<void()> callback = {});
  
  /**
   * @brief Add a visual separator line
   */
  void addSeparator();
  
  /**
   * @brief Add a section header (non-clickable label)
   */
  void addSectionHeader(const juce::String &text);
  
  /**
   * @brief Add a submenu item
   */
  void addSubMenu(const juce::String &text,
                  std::unique_ptr<SkiaPopupMenu> subMenu,
                  const SkPath &icon = {});
  
  /**
   * @brief Clear all items
   */
  void clear();
  
  /**
   * @brief Get number of items (including separators)
   */
  int getNumItems() const { return static_cast<int>(items_.size()); }
  
  /**
   * @brief Get item text by index
   */
  juce::String getItemText(int index) const;

  //============================================================================
  // Show/Hide
  //============================================================================
  
  /**
   * @brief Show menu at position relative to component
   */
  void showAt(juce::Component *component, int x, int y);
  
  /**
   * @brief Show menu at screen position
   */
  void showAt(juce::Point<int> screenPosition);
  
  /**
   * @brief Show menu at mouse position
   */
  void showAtMouse();
  
  /**
   * @brief Hide the menu (and any submenus)
   */
  void hideMenu();
  
  /**
   * @brief Check if menu is currently visible
   */
  bool isMenuVisible() const { return isShowing_; }
  
  /**
   * @brief Dismiss callback (called when menu closes for any reason)
   */
  std::function<void()> onDismiss;

  //============================================================================
  // Appearance
  //============================================================================
  
  void setBackgroundColour(SkColor colour);
  void setTextColour(SkColor colour);
  void setHighlightColour(SkColor colour);
  void setBorderColour(SkColor colour);
  void setFont(const SkFont &font);
  
  /**
   * @brief Enable/disable backdrop blur effect
   */
  void setBackdropBlurEnabled(bool enabled) { backdropBlur_ = enabled; }
  
  /**
   * @brief Set animation duration in milliseconds
   */
  void setAnimationDuration(int ms) { animationDurationMs_ = ms; }

  //============================================================================
  // Component Interface
  //============================================================================
  
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  
  //============================================================================
  // KeyListener Interface
  //============================================================================
  
  bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;

private:
  std::vector<Item> items_;
  std::unique_ptr<SkiaPopupMenu> activeSubMenu_;

  // Appearance
  SkColor backgroundColour_;
  SkColor textColour_;
  SkColor highlightColour_;
  SkColor borderColour_;
  SkColor dangerColour_;
  SkFont font_;
  bool backdropBlur_ = true;

  // State
  int hoveredItem_ = -1;
  int pressedItem_ = -1;
  int keyboardFocusedItem_ = -1;
  bool isShowing_ = false;
  
  // Animation
  float showAnimation_ = 0.0f;  // 0.0 = hidden, 1.0 = fully visible
  int animationDurationMs_ = 150;

  // Layout
  int itemHeight_ = 32;
  int iconWidth_ = 28;
  int shortcutPadding_ = 40;
  int separatorHeight_ = 9;
  int sectionHeaderHeight_ = 24;
  int horizontalPadding_ = 8;
  int cornerRadius_ = static_cast<int>(design::dimensions::RADIUS_SM);

  // Internal methods
  void calculateSize();
  int getItemAtPosition(juce::Point<int> position) const;
  juce::Rectangle<int> getItemBounds(int itemIndex) const;
  void handleItemSelected(int itemIndex);
  void moveKeyboardFocus(int delta);
  void selectKeyboardFocusedItem();
  void showSubMenuForItem(int itemIndex);
  void hideAllSubMenus();
  void ensureOnScreen();
  
  // Drawing helpers
  void drawBackdrop(SkCanvas *canvas);
  void drawShadow(SkCanvas *canvas);
  void drawItem(SkCanvas *canvas, int index, const Item &item, const SkRect &bounds);
  void drawIcon(SkCanvas *canvas, const SkPath &icon, const SkRect &bounds, SkColor color);
  void drawCheckmark(SkCanvas *canvas, const SkRect &bounds, SkColor color);
  void drawSubmenuArrow(SkCanvas *canvas, const SkRect &bounds, SkColor color);

  // Parent menu for submenus
  SkiaPopupMenu *parentMenu_ = nullptr;

  // Timer for animation
  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPopupMenu)
};

} // namespace zenith