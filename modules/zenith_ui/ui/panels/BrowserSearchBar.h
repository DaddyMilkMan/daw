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

    BrowserSearchBar.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/


#pragma once

#include "SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#endif

namespace zenith {

class BrowserSearchBar : public SkiaComponent {
public:
  BrowserSearchBar();
  ~BrowserSearchBar() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  bool keyPressed(const juce::KeyPress &key) override;
  void resized() override;

  void setSearchText(const juce::String &text);
  juce::String getSearchText() const { return searchText_; }

  std::function<void(const juce::String&)> onSearchChanged;
  std::function<void()> onAddFolderRequested;
  std::function<void()> onBackRequested;

  void setBackButtonVisible(bool visible);

private:
  juce::String searchText_;
  bool backButtonVisible_ = false;

  juce::Rectangle<int> searchBoxBounds_;
  juce::Rectangle<int> backButtonBounds_;
  juce::Rectangle<int> addFolderButtonBounds_;

  static constexpr int searchBoxHeight_ = 30;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserSearchBar)
};

} // namespace zenith
