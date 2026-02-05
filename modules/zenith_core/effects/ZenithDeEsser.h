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
  // Note: listen is AudioParameterBool but getRawParameterValue returns float*
  // Check against 0.5f threshold for boolean semantics
  std::atomic<float> *listen = nullptr; // Listen to delta/sibilance

  // DSP
  // Crossover L-R 4th order
  juce::dsp::LinkwitzRileyFilter<float> crossoverLow;
  juce::dsp::LinkwitzRileyFilter<float> crossoverHigh;

  juce::dsp::Compressor<float> compressor;

  // Pre-allocated buffer for high band (real-time safe)
  juce::AudioBuffer<float> highBand;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithDeEsser)
};

} // namespace zenith
