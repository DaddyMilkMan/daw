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

    PresetGeneticistView.h
    Created: 2025-12-09
    Author:  Zenith DAW AI Team

    Visualizer for the AI Sound Designer.
    Shows the target spectrum vs the current evolved spectrum.


  ==============================================================================
*/

#pragma once

#include "../../ai/PresetGeneticistAgent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {
namespace ui {
namespace views {

class PresetGeneticistView : public juce::Component, public juce::Timer {
public:
  //==============================================================================
  explicit PresetGeneticistView(zenith::ai::PresetGeneticistAgent &agent);
  ~PresetGeneticistView() override;

  //==============================================================================
  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

private:
  //==============================================================================
  // Reference to the AI Agent
  zenith::ai::PresetGeneticistAgent &agent_;

  // UI Controls
  juce::TextButton startButton_{"START EVOLUTION"};
  juce::TextButton loadTargetButton_{"LOAD TARGET"};

  // Visualization Data
  juce::Path currentSpectrumPath_;
  juce::Path targetSpectrumPath_;

  // Path Generation Helper
  void generatePathFromSpectrum(const std::vector<float> &spectrum,
                                juce::Path &path,
                                juce::Rectangle<float> bounds);

  // File Chooser
  std::unique_ptr<juce::FileChooser> fileChooser_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetGeneticistView)
};

} // namespace views
} // namespace ui
} // namespace zenith
