/*
  ==============================================================================

    ZenithTremolo.h
    Created: 2025-12-19
    Author:  Zenith DAW

    Simple Tremolo effect to verify ZenithPlugin architecture.

  ==============================================================================
*/

#pragma once

#include "../ZenithPlugin.h"

namespace zenith {

class ZenithTremolo : public ZenithPlugin {
public:
  ZenithTremolo();
  ~ZenithTremolo() override;

  const juce::String getName() const override { return "Zenith Tremolo"; }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  // Parameters
  // Note: We use raw pointers for speed in processBlock, updated from APVTS
  std::atomic<float> *rateParam = nullptr;
  std::atomic<float> *depthParam = nullptr;

  // State
  float currentPhase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTremolo)
};

} // namespace zenith
