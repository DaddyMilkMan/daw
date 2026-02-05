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

    BrowserRecentSidebar.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/


#pragma once

#include "SkiaComponent.h"
#include "../../browser/BrowserModel.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class BrowserRecentSidebar : public SkiaComponent {
public:
  explicit BrowserRecentSidebar(BrowserModel &model);
  ~BrowserRecentSidebar() override;

  void drawSkia(SkCanvas *canvas) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  std::function<void(std::shared_ptr<BrowserItem>)> onItemSelected;

private:
  BrowserModel &model_;
  int hoverIndex_ = -1;
  static constexpr int itemHeight_ = 32;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserRecentSidebar)
};

} // namespace zenith
