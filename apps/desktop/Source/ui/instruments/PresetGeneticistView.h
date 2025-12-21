/*
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
#include "../framework/SkiaComponent.h"
#include "../widgets/SkiaButton.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {
namespace ui {
namespace views {

class PresetGeneticistView : public SkiaComponent {
public:
  //==============================================================================
  explicit PresetGeneticistView(zenith::ai::PresetGeneticistAgent &agent);
  ~PresetGeneticistView() override;

  //==============================================================================
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void timerCallback() override;

private:
  //==============================================================================
  // Reference to the AI Agent
  zenith::ai::PresetGeneticistAgent &agent_;

  // UI Controls
  SkiaButton startButton_{"START EVOLUTION"};
  SkiaButton loadTargetButton_{"LOAD TARGET"};

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
