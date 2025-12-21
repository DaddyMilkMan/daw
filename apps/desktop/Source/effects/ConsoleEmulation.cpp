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
<<<<<<< HEAD

  lowPass.prepare(spec);
  highPass.prepare(spec);

=======
>>>>>>> origin/master
  reset();
}

void ConsoleEmulation::reset() {
<<<<<<< HEAD
  lowPass.reset();
  highPass.reset();
  updateFilters();
}

void ConsoleEmulation::updateFilters() {
  if (sampleRate <= 0.0f)
    return;

=======
  auto spec = juce::dsp::ProcessSpec{(double)sampleRate, 512, 2};

  // Default filters
  *lowPass.coefficients =
      *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f);
  *highPass.coefficients =
      *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);

  lowPass.reset();
  highPass.reset();
}

void ConsoleEmulation::process(juce::AudioBuffer<float> &buffer) {
  if (mode == Mode::Clean && drive < 0.01f)
    return;

  // Update filters based on mode for each block (simple approx)
>>>>>>> origin/master
  if (mode == Mode::Vintage) {
    *lowPass.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 16000.0f);
  } else {
    *lowPass.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f);
  }

<<<<<<< HEAD
  *highPass.coefficients =
      *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
}

void ConsoleEmulation::process(juce::AudioBuffer<float> &buffer) {
  if (mode == Mode::Clean && drive < 0.01f && character < 0.01f)
    return;

  const int numChannels = buffer.getNumChannels();
  const int numSamples = buffer.getNumSamples();

=======
  const int numChannels = buffer.getNumChannels();
  const int numSamples = buffer.getNumSamples();

  // Drive gain compensation (rough)
>>>>>>> origin/master
  float gainComp = 1.0f / (1.0f + drive * 0.5f);

  for (int ch = 0; ch < numChannels; ++ch) {
    auto *data = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      float in = data[i];

<<<<<<< HEAD
      float saturated = applySaturation(in, drive * 2.0f);

      float mixed = saturated;
      if (character < 1.0f) {
        mixed = in * (1.0f - character) + saturated * character;
      }

      data[i] = mixed * gainComp;
    }
  }

=======
      // Saturation
      float out = applySaturation(in, drive * 2.0f); // Boost drive range

      // Mix with Dry if Character is used as Mix? Or used as tonal?
      // Let's assume Character is just intensity for now.

      data[i] = out * gainComp;
    }
  }

  // Apply filters if needed (Vintage roll-off)
>>>>>>> origin/master
  if (mode == Mode::Vintage) {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPass.process(context);
  }
}

float ConsoleEmulation::applySaturation(float input, float driveAmount) {
  if (mode == Mode::Clean)
    return input;

<<<<<<< HEAD
  float x = input * (1.0f + driveAmount * 2.0f);

  if (mode == Mode::Tube) {
    if (x > 0)
      x = std::tanh(x);
    else
      x = std::tanh(x) / 1.1f;
=======
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
>>>>>>> origin/master
  } else {
    x = std::tanh(x);
  }

  return x;
}

} // namespace effects
} // namespace zenith
