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

    HelpViewPanel.h
    Created: 2025-12-25
    Author:  Zenith DAW

    Info View panel similar to Ableton Live's, displaying context-sensitive help.

  ==============================================================================

*/

#pragma once

#include "../framework/SkiaComponent.h"

namespace zenith {

class HelpViewPanel : public SkiaComponent {
public:
    HelpViewPanel();
    ~HelpViewPanel() override;

    void drawSkia(SkCanvas* canvas) override;
    void setContent(const juce::String& title, const juce::String& description);

private:
    juce::String title_ = "Info View";
    juce::String description_ = "Hover over controls to see details here.";
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpViewPanel)
};

} // namespace zenith
