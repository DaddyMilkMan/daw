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

    SkiaListBox.h
    Created: 2025-12-07
    Author:  AI Assistant

    Pure Skia-based list box component to replace juce::ListBox

  ==============================================================================

*/

#pragma once

#include "SkiaComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {

class SkiaListBox : public SkiaComponent {
public:
  SkiaListBox(const juce::String &componentName = {});
  ~SkiaListBox() override;

  // Model interface (similar to JUCE ListBoxModel)
  class Model {
  public:
    virtual ~Model() = default;
    virtual int getNumRows() = 0;
    virtual void paintListBoxItem(int rowNumber, SkCanvas &canvas, int width,
                                  int height, bool rowIsSelected) = 0;
    virtual void listBoxItemClicked(int rowNumber, const juce::MouseEvent &e) {
      juce::ignoreUnused(rowNumber, e);
    }
    virtual void listBoxItemDoubleClicked(int rowNumber,
                                          const juce::MouseEvent &e) {
      juce::ignoreUnused(rowNumber, e);
    }
    virtual juce::String getTooltipForRow(int rowNumber) { return {}; }
  };

  void setModel(Model *model);
  Model *getModel() const { return model_; }

  // Selection
  void setSelectedRows(const juce::SparseSet<int> &setOfRowsToBeSelected,
                       bool sendNotification = true);
  void selectRow(int rowNumber, bool sendNotification = true,
                 bool deselectOthersFirst = true);
  void deselectRow(int rowNumber);
  void deselectAllRows();
  bool isRowSelected(int rowNumber) const;
  int getSelectedRow() const;
  juce::SparseSet<int> getSelectedRows() const { return selectedRows_; }

  // Appearance
  void setRowHeight(int newHeight);
  int getRowHeight() const { return rowHeight_; }

  void setOutlineThickness(int thickness);
  void setColour(SkColor colour);

  // Scrolling
  void scrollToEnsureRowIsOnscreen(int rowNumber);
  int getRowContainingPosition(juce::Point<int> position) const;
  int getRowContainingPosition(int x, int y) const;

  // Update
  void updateContent();
  void repaintRow(int rowNumber);

  // Event callbacks
  std::function<void()> onSelectionChange;
  std::function<void(int)> onRowClicked;
  std::function<void(int)> onRowDoubleClicked;

  // Component interface
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

private:
  Model *model_ = nullptr;
  int rowHeight_ = 30;
  int outlineThickness_ = 1;

  juce::SparseSet<int> selectedRows_;
  int lastRowSelected_ = -1;

  // Scroll offset
  float scrollY_ = 0.0f;
  float targetScrollY_ = 0.0f;

  // Appearance
  SkColor backgroundColour_;
  SkColor textColour_;
  SkColor selectedBackgroundColour_;
  SkColor selectedTextColour_;
  SkColor borderColour_;
  SkFont font_;

  // Internal methods
  int getTotalContentHeight() const;
  int getVisibleRowCount() const;
  juce::Rectangle<int> getRowBounds(int rowNumber) const;
  void updateSelection(int rowNumber, bool deselectOthers,
                       bool sendNotification);
  void ensureRowIsVisible(int rowNumber);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaListBox)
};

} // namespace zenith