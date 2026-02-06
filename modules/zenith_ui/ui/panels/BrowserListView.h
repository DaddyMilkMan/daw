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
#include "../../browser/BrowserModel.h"
#include "BrowserWaveformLoader.h"
#include "BrowserHoverPreview.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#endif

namespace zenith {

class BrowserListView : public SkiaComponent {
public:
  explicit BrowserListView(BrowserModel &model, BrowserWaveformLoader &waveformLoader);
  ~BrowserListView() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  bool keyPressed(const juce::KeyPress &key) override;
  void resized() override;
  void timerCallback() override;

  void updateDisplayItems();
  void setSearchText(const juce::String &text);
  void navigateTo(std::shared_ptr<BrowserItem> folder);
  void navigateUp();

  std::shared_ptr<BrowserItem> getSelectedItem() const;
  std::shared_ptr<BrowserItem> getCurrentRoot() const { return currentRoot_; }

  std::function<void(std::shared_ptr<BrowserItem>)> onItemSelected;
  std::function<void(std::shared_ptr<BrowserItem>)> onItemDoubleClicked;
  std::function<void(int, juce::Point<int>)> onItemRightClicked;

private:
  void drawBrowserItem(SkCanvas *canvas, int index, const juce::Rectangle<int> &bounds);
  void drawIcon(SkCanvas *canvas, BrowserItemType type, float x, float y, float size);
  void drawTags(SkCanvas *canvas, const std::vector<juce::String> &tags, float x, float y);
  int getItemIndexAt(int y) const;

  void triggerHoverPreview(int index);

  BrowserModel &model_;
  BrowserWaveformLoader &waveformLoader_;

  std::shared_ptr<BrowserItem> currentRoot_;
  std::vector<std::shared_ptr<BrowserItem>> displayItems_;

  juce::String searchText_;
  int selectedIndex_ = -1;
  int hoverIndex_ = -1;
  int scrollOffset_ = 0;

  // Hover Preview
  std::unique_ptr<BrowserHoverPreview> hoverPreview_;
  int hoverPreviewIndex_ = -1;
  static constexpr int hoverDelayMs_ = 350;

  static constexpr int itemHeight_ = 28;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserListView)
};

} // namespace zenith
