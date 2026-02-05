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

    ZenithDropdown.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Premium dropdown/combo box for waveform selection etc:
    - Glass background
    - Chevron indicator

    - Popup menu styling

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "SkiaPopupMenu.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#endif

namespace zenith {

class ZenithDropdown : public SkiaComponent {
public:
  // ----- Constructors -----
  ZenithDropdown();
  explicit ZenithDropdown(const juce::String &label);
  ~ZenithDropdown() override = default;

  // ----- Items -----
  void addItem(const juce::String &item, int itemId = 0);
  void addItems(const juce::StringArray &items);
  void clear();
  int getNumItems() const { return items_.size(); }

  // ----- Selection -----
  void setSelectedIndex(int index, bool sendNotification = true);
  int getSelectedIndex() const { return selectedIndex_; }
  void setSelectedId(int id, bool sendNotification = true);
  int getSelectedId() const;
  juce::String getSelectedText() const;

  // ----- Appearance -----
  void setLabel(const juce::String &label) {
    label_ = label;
    repaint();
  }
  juce::String getLabel() const { return label_; }

  void setPlaceholder(const juce::String &text) {
    placeholder_ = text;
    repaint();
  }
  void setAccentColor(SkColor color) {
    accentColor_ = color;
    repaint();
  }

  // ----- Callbacks -----
  std::function<void(int)> onSelectionChanged; // Index
  std::function<void()> onChange;              // Legacy

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

protected:
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

private:
  void showPopupMenu();

#ifdef ZENITH_USE_SKIA
  void drawBackground(SkCanvas *canvas);
  void drawText(SkCanvas *canvas);
  void drawChevron(SkCanvas *canvas);
#endif

  struct Item {
    juce::String text;
    int id;
  };

  juce::Array<Item> items_;
  int selectedIndex_ = -1;

  juce::String label_;
  juce::String placeholder_ = "Select...";

  bool hovered_ = false;
  bool isOpen_ = false;
  
  std::unique_ptr<SkiaPopupMenu> activeMenu_;

  SkColor accentColor_ = SkColorSetRGB(0, 255, 255);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithDropdown)
};

} // namespace zenith
