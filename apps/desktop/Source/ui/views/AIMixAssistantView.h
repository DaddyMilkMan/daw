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
#include "../framework/SkiaComponent.h"
#include "../controls/ZenithButton.h"
#include "../controls/ZenithSlider.h"
#include "../controls/SkiaLabel.h"


namespace zenith {

class AIMixAssistantView : public SkiaComponent {
public:
  AIMixAssistantView(Engine &engine);
  ~AIMixAssistantView() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

private:
  Engine &engine_;
  // We assume Engine owns the agent now, or we implement a local one if
  // isolated? Ideally Engine owns it. For now, we'll access it via Engine if
  // possible, or if not added to Engine yet, we might need to rely on a
  // temporary one BUT the mastering features need to persist. Let's assume we
  // will add getMasteringAgent() to Engine.

  // UI Controls
  ZenithButton analyzeButton{"Analyze & Auto-Level Mix"};
  ZenithButton applyMasteringButton{"Apply Mastering Chain"};

  ZenithSlider targetLufsSlider;
  std::unique_ptr<SkiaLabel> targetLufsLabel;

  ZenithButton enableEq{"Adaptive EQ"};
  ZenithButton enableComp{"Glue Compressor"};
  ZenithButton enableLimit{"Brickwall Limiter"};

  ZenithSlider compAmountSlider;
  std::unique_ptr<SkiaLabel> compAmountLabel;

  // Visuals
  float mixHealthScore = 0.0f; // Mock score for now
  juce::String mixStatus = "Ready to analyze";

  void performAnalysis();
  void applyMastering();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIMixAssistantView)
};

} // namespace zenith
