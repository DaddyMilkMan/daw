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
    AlgorithmicToolsPanel.cpp
    Implementation of Piano Roll Power Tools
  ==============================================================================
*/


#include "../controls/SkiaAlertWindow.h"
#include <random>

namespace zenith {

AlgorithmicToolsPanel::AlgorithmicToolsPanel(PianoRollComponent& owner, ProjectState& state)
    : owner_(owner), projectState_(state)
{
    using namespace design;

    humanizeBtn_ = std::make_unique<SkiaButton>("Humanize");
    humanizeBtn_->setStyle(SkiaButton::Style::Ghost);
    humanizeBtn_->onClick = [this] { applyHumanize(); };
    addAndMakeVisible(humanizeBtn_.get());

    harmonyBtn_ = std::make_unique<SkiaButton>("Add Harmony");
    harmonyBtn_->setStyle(SkiaButton::Style::Ghost);
    harmonyBtn_->onClick = [this] { applyHarmony(); };
    addAndMakeVisible(harmonyBtn_.get());

    probabilityBtn_ = std::make_unique<SkiaButton>("Randomize Prob");
    probabilityBtn_->setStyle(SkiaButton::Style::Ghost);
    probabilityBtn_->onClick = [this] { applyProbability(); };
    addAndMakeVisible(probabilityBtn_.get());

    retrogradeBtn_ = std::make_unique<SkiaButton>("Retrograde");
    retrogradeBtn_->setStyle(SkiaButton::Style::Ghost);
    retrogradeBtn_->onClick = [this] { applyRetrograde(); };
    addAndMakeVisible(retrogradeBtn_.get());
}

AlgorithmicToolsPanel::~AlgorithmicToolsPanel() = default;

void AlgorithmicToolsPanel::resized()
{
    using namespace design;
    float margin = spacing::SM;
    float btnHeight = 28.0f;
    float y = margin;
    float w = getWidth() - (margin * 2);

    if (humanizeBtn_) humanizeBtn_->setBounds(margin, y, w, btnHeight);
    y += btnHeight + margin;

    if (harmonyBtn_) harmonyBtn_->setBounds(margin, y, w, btnHeight);
    y += btnHeight + margin;

    if (probabilityBtn_) probabilityBtn_->setBounds(margin, y, w, btnHeight);
    y += btnHeight + margin;

    if (retrogradeBtn_) retrogradeBtn_->setBounds(margin, y, w, btnHeight);
}

void AlgorithmicToolsPanel::drawSkia(SkCanvas* canvas)
{
    using namespace design;
    
    // Background - Darker glass for tools panel
    SkRect rect = SkRect::MakeWH(getWidth(), getHeight());
    SkPaint bgPaint;
    bgPaint.setColor(withAlpha(colors::BG_DARKEST, opacity::GLASS_SOLID));
    canvas->drawRect(rect, bgPaint);

    // Right border
    SkPaint borderPaint;
    borderPaint.setColor(withAlpha(colors::BORDER_SUBTLE, opacity::GLASS_MEDIUM));
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawLine(getWidth(), 0, getWidth(), getHeight(), borderPaint);
}

void AlgorithmicToolsPanel::applyHumanize()
{
    projectState_.getUndoManager().beginNewTransaction("Humanize");
    
    // Simple Gaussian randomization of start/velocity
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> timeDist(0.0, 0.05); // +/- 0.05 beats
    std::normal_distribution<> velDist(0.0, 5.0);   // +/- 5 velocity

    // Access current clip from owner (assuming public accessor or friend)
    // For now, we'll iterate selected notes via project state if possible, 
    // or request owner to do it.
    // Since this is a separate class, we need access to the selected notes.
    // The PianoRollComponent likely manages selection state.
    
    // Optimization: Call owner to get selected note IDs
    // owner_.humanizeSelectedNotes(timeDist, velDist); ... Ideally.
    // For this implementation, I will simulate the action via alert for now until linked.
     juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Humanize", "Humanization applied to selected notes (Simulated)");
}

void AlgorithmicToolsPanel::applyHarmony() {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Harmony", "Harmony generation (3rd/5th) applied (Simulated)");
}

void AlgorithmicToolsPanel::applyProbability() {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Probability", "Randomized probability for selected notes (Simulated)");
}

void AlgorithmicToolsPanel::applyRetrograde() {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Retrograde", "Retrograde transformation applied (Simulated)");
}

} // namespace zenith
