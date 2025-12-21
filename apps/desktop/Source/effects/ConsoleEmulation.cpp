/*
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
  reset();
}

void ConsoleEmulation::reset() {
  // Default filters
  *lowPass.coefficients =
      *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f);

  lowPass.reset();
  highPass.reset();
  filtersDirty = true;
}

void ConsoleEmulation::process(juce::AudioBuffer<float> &buffer) {
  if (mode == Mode::Clean && drive < 0.01f)
    return;

  // Update filters only when mode changes (performance optimization)
  if (filtersDirty) {
    if (mode == Mode::Vintage) {
      *lowPass.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
          sampleRate, 16000.0f);
    } else {
      *lowPass.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
          sampleRate, 20000.0f);
    }
    filtersDirty = false;
  }

  const int numChannels = buffer.getNumChannels();
  const int numSamples = buffer.getNumSamples();

  // Drive gain compensation (rough)
  float gainComp = 1.0f / (1.0f + drive * 0.5f);

  for (int ch = 0; ch < numChannels; ++ch) {
    auto *data = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      float in = data[i];

      // Saturation
      float out = applySaturation(in, drive * 2.0f); // Boost drive range

      // Mix with Dry if Character is used as Mix? Or used as tonal?
      // Let's assume Character is just intensity for now.

      data[i] = out * gainComp;
    }
  }

  // Apply filters if needed (Vintage roll-off)
  if (mode == Mode::Vintage) {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPass.process(context);
  }
}

float ConsoleEmulation::applySaturation(float input, float driveAmount) {
  if (mode == Mode::Clean)
    return input;

  // Simple tanh saturation
  // x = input * (1 + drive)
  // f(x) = tanh(x)

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
