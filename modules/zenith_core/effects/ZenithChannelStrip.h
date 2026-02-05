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

  // Cached EQ param values for dirty checking
  float cachedHpfFreq = 0.0f;
  float cachedLowFreq = 0.0f;
  float cachedLowGain = 0.0f;
  float cachedMidFreq = 0.0f;
  float cachedMidGain = 0.0f;
  float cachedMidQ = 0.0f;
  float cachedHighFreq = 0.0f;
  float cachedHighGain = 0.0f;

  void updateEqCoefficientsIfNeeded(double sampleRate);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithChannelStrip)
};

} // namespace zenith
