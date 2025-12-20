/*
  ==============================================================================

    ZenithDeEsser.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Split-band De-Esser for sibilance control.

  ==============================================================================
*/

#pragma once

#include "../plugins/ZenithPlugin.h"
#include <juce_dsp/juce_dsp.h>

namespace zenith {

class ZenithDeEsser : public ZenithPlugin {
public:
  ZenithDeEsser();
  ~ZenithDeEsser() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  const juce::String getName() const override { return "Zenith De-Esser"; }

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  // Parameters
  std::atomic<float> *threshold = nullptr;
  std::atomic<float> *frequency = nullptr;
  std::atomic<float> *amount = nullptr;
  std::atomic<float> *listen = nullptr; // Listen to delta/sibilance

  // DSP
  // Crossover L-R 4th order
  juce::dsp::LinkwitzRileyFilter<float> crossoverLow;
  juce::dsp::LinkwitzRileyFilter<float> crossoverHigh;

  juce::dsp::Compressor<float> compressor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithDeEsser)
};

} // namespace zenith
