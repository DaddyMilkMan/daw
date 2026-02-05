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

    AIMixAssistantView.h
    Created: 2025-12-20
    Author:  Zenith DAW

    UI for the AI Mix Assistant and Mastering Agent.

  ==============================================================================

*/

#pragma once

#include "../../ai/AIMasteringAgent.h"
#include "../framework/GlassmorphicPanel.h"
#include "../controls/ZenithUIComponents.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

class AIMixAssistantView : public juce::Component {
public:
  AIMixAssistantView(Engine &engine);
  ~AIMixAssistantView() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  Engine &engine_;
  // We assume Engine owns the agent now, or we implement a local one if
  // isolated? Ideally Engine owns it. For now, we'll access it via Engine if
  // possible, or if not added to Engine yet, we might need to rely on a
  // temporary one BUT the mastering features need to persist. Let's assume we
  // will add getMasteringAgent() to Engine.

  // UI Controls
  juce::TextButton analyzeButton{"Analyze & Auto-Level Mix"};
  juce::TextButton applyMasteringButton{"Apply Mastering Chain"};

  juce::Slider targetLufsSlider;
  juce::Label targetLufsLabel;

  juce::ToggleButton enableEq{"Adaptive EQ"};
  juce::ToggleButton enableComp{"Glue Compressor"};
  juce::ToggleButton enableLimit{"Brickwall Limiter"};

  juce::Slider compAmountSlider;
  juce::Label compAmountLabel;

  // Visuals
  float mixHealthScore = 0.0f; // Mock score for now
  juce::String mixStatus = "Ready to analyze";

  void performAnalysis();
  void applyMastering();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIMixAssistantView)
};

} // namespace zenith
