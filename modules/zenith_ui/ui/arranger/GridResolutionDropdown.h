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

    GridResolutionDropdown.h
    Created: 2025-12-26
    Author:  Zenith DAW

    A premium glassmorphic dropdown for selecting grid resolution.

  ==============================================================================

*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaPopupMenu.h"
#include "ArrangerTypes.h"

namespace zenith {

class GridResolutionDropdown : public SkiaComponent {
public:
    GridResolutionDropdown();
    ~GridResolutionDropdown() override;

    void setResolution(GridResolution res);
    GridResolution getResolution() const { return currentResolution; }

    std::function<void(GridResolution)> onResolutionChanged;

    void drawSkia(SkCanvas* canvas) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    GridResolution currentResolution = GridResolution::Beat_1;
    bool isHovered = false;
    std::unique_ptr<SkiaPopupMenu> menu;

    juce::String getResolutionText(GridResolution res) const;
    void showMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridResolutionDropdown)
};

} // namespace zenith
