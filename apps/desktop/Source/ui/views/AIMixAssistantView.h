/*
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
  zenith::ui::TextButton analyzeButton{"Analyze & Auto-Level Mix"};
  zenith::ui::TextButton applyMasteringButton{"Apply Mastering Chain"};

  zenith::ui::Slider targetLufsSlider;
  juce::Label targetLufsLabel;

  juce::ToggleButton enableEq{"Adaptive EQ"};
  juce::ToggleButton enableComp{"Glue Compressor"};
  juce::ToggleButton enableLimit{"Brickwall Limiter"};

  zenith::ui::Slider compAmountSlider;
  juce::Label compAmountLabel;

  // Visuals
  float mixHealthScore = 0.0f; // Mock score for now
  juce::String mixStatus = "Ready to analyze";

  void performAnalysis();
  void applyMastering();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIMixAssistantView)
};

} // namespace zenith
