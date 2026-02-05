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
    AlgorithmicToolsPanel.h
    Created: 2026-01-03
    Author:  Antigravity

    Left-side panel for Piano Roll power features (probability, harmony, etc.)
  ==============================================================================
*/


#pragma once

#include "../framework/SkiaComponent.h"
#include "../controls/SkiaButton.h"
#include "../../engine/ProjectState.h"

namespace zenith {

class PianoRollComponent;

class AlgorithmicToolsPanel : public SkiaComponent {
public:
    AlgorithmicToolsPanel(PianoRollComponent& owner, ProjectState& state);
    ~AlgorithmicToolsPanel() override;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;

private:
    PianoRollComponent& owner_;
    ProjectState& projectState_;

    // Tools
    std::unique_ptr<SkiaButton> humanizeBtn_;
    std::unique_ptr<SkiaButton> harmonyBtn_;
    std::unique_ptr<SkiaButton> probabilityBtn_;
    std::unique_ptr<SkiaButton> retrogradeBtn_;

    void applyHumanize();
    void applyHarmony();
    void applyProbability();
    void applyRetrograde();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmicToolsPanel)
};

} // namespace zenith
