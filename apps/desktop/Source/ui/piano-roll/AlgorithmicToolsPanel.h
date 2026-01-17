/*
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
