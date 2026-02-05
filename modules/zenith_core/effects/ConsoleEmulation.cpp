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

    ConsoleEmulation.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/


#include "ConsoleEmulation.h"
#include <cmath>

namespace zenith {
namespace effects {

void ConsoleEmulation::prepare(juce::dsp::ProcessSpec &spec) {
  sampleRate = (float)spec.sampleRate;
  lowPass.prepare(spec);
  coefficientsDirty = true;
  reset();
}

void ConsoleEmulation::reset() {
  updateCoefficients();
  lowPass.reset();
}

void ConsoleEmulation::updateCoefficients() {
  if (!coefficientsDirty)
    return;

  // Guard: ensure sampleRate is valid before creating coefficients
  if (sampleRate <= 0.0f)
    return;

  // Guard: ensure coefficients pointer is valid (prepared)
  if (lowPass.coefficients == nullptr)
    return;

  if (mode == Mode::Vintage) {
    *lowPass.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 16000.0f);
  } else {
    *lowPass.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f);
  }

  coefficientsDirty = false;
}

void ConsoleEmulation::process(juce::AudioBuffer<float> &buffer) {
  if (mode == Mode::Clean && drive < 0.01f && character < 0.01f)
    return;

  updateCoefficients();

  const int numChannels = buffer.getNumChannels();
  const int numSamples = buffer.getNumSamples();

  float gainComp = 1.0f / (1.0f + drive * 0.5f);

  for (int ch = 0; ch < numChannels; ++ch) {
    auto *data = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      float in = data[i];

      float saturated = applySaturation(in, drive * 2.0f);

      float mixed = saturated;
      if (character < 1.0f) {
        mixed = in * (1.0f - character) + saturated * character;
      }

      data[i] = mixed * gainComp;
    }
  }

  if (mode == Mode::Vintage) {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPass.process(context);
  }
}

float ConsoleEmulation::applySaturation(float input, float driveAmount) {
  if (mode == Mode::Clean)
    return input;

  float x = input * (1.0f + driveAmount * 2.0f);

  if (mode == Mode::Tube) {
    // Asymmetric soft clipping
    if (x > 0)
      x = std::tanh(x);
    else
      x = std::tanh(x) / 1.1f; // Slight asymmetry
  } else {
    x = std::tanh(x);
  }

  return x;
}

} // namespace effects
} // namespace zenith
