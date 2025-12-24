/*
  ==============================================================================

    ZenithChannelStrip.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Professional Channel Strip Plugin.
    Features:
    - Noise Gate
    - 4-Band Parametric EQ (HPF, Low Shelf, Mid Peak, High Shelf)
    - VCA-style Compressor
    - Analog Saturation (Console Emulation)

  ==============================================================================
*/

#pragma once

#include "../plugins/ZenithPlugin.h"
#include <juce_dsp/juce_dsp.h>

namespace zenith {

class ZenithChannelStrip : public ZenithPlugin {
public:
  ZenithChannelStrip();
  ~ZenithChannelStrip() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  const juce::String getName() const override { return "Zenith Channel Strip"; }

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  // DSP Objects
  juce::dsp::NoiseGate<float> gate;

  // EQ Chain: HPF -> LowShelf -> Peak -> HighShelf
  using EQChain =
      juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>, // HPF
                                juce::dsp::IIR::Filter<float>, // Low Shelf
                                juce::dsp::IIR::Filter<float>, // Mid Peak
                                juce::dsp::IIR::Filter<float>  // High Shelf
                                >;
  EQChain eq;

  juce::dsp::Compressor<float> compressor;

  // Saturation
  juce::dsp::WaveShaper<float> saturation;

  // Parameters (cached for speed)
  // Gate
  std::atomic<float> *gateThresh = nullptr;

  // EQ
  std::atomic<float> *eqHpfFreq = nullptr;
  std::atomic<float> *eqLowFreq = nullptr;
  std::atomic<float> *eqLowGain = nullptr;
  std::atomic<float> *eqMidFreq = nullptr;
  std::atomic<float> *eqMidGain = nullptr;
  std::atomic<float> *eqMidQ = nullptr;
  std::atomic<float> *eqHighFreq = nullptr;
  std::atomic<float> *eqHighGain = nullptr;

  // Comp
  std::atomic<float> *compThresh = nullptr;
  std::atomic<float> *compRatio = nullptr;
  std::atomic<float> *compAttack = nullptr;
  std::atomic<float> *compRelease = nullptr;
  std::atomic<float> *compMakeup = nullptr;

  // Saturation
  std::atomic<float> *drive = nullptr;
  std::atomic<float> *outputGain = nullptr;

  void updateParameters();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithChannelStrip)
};

} // namespace zenith
