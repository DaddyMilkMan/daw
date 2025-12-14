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
#include "../../ui/ZenithTheme.h"
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
