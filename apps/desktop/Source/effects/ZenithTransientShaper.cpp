/*
  ==============================================================================

    ZenithTransientShaper.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithTransientShaper.h"

namespace zenith {

ZenithTransientShaper::ZenithTransientShaper()
    : ZenithPlugin(createParameterLayout()) {
  attackGain = apvts.getRawParameterValue("attack");
  sustainGain = apvts.getRawParameterValue("sustain");
}

ZenithTransientShaper::~ZenithTransientShaper() {}

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithTransientShaper::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>("attack", "Attack",
                                                         -1.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain",
                                                         -1.0f, 1.0f, 0.0f));

  return layout;
}

void ZenithTransientShaper::prepareToPlay(double sampleRate,
                                          int samplesPerBlock) {
  sampleRate_ = static_cast<float>(sampleRate);

  for (int i = 0; i < 2; ++i) {
    fastEnvelope[i] = 0.0f;
    slowEnvelope[i] = 0.0f;
  }
}

void ZenithTransientShaper::releaseResources() {}

void ZenithTransientShaper::processBlock(juce::AudioBuffer<float> &buffer,
                                         juce::MidiBuffer &) {
  float att = attackGain->load();
  float sus = sustainGain->load();

  auto numChannels = buffer.getNumChannels();
  auto numSamples = buffer.getNumSamples();

  // Coefficients
  // Fast: 10ms approx
  // Slow: 100ms approx
  const float fastCoeff = std::exp(-1.0f / (sampleRate_ * 0.010f));
  const float slowCoeff = std::exp(-1.0f / (sampleRate_ * 0.100f));

  for (int ch = 0; ch < numChannels; ++ch) {
    if (ch >= 2)
      break; // Support stereo only for logic simplicity, or dup

    auto *data = buffer.getWritePointer(ch);
    float &fastEnv = fastEnvelope[ch];
    float &slowEnv = slowEnvelope[ch];

    for (int i = 0; i < numSamples; ++i) {
      float in = data[i];
      float absIn = std::abs(in);

      // Update envelopes
      fastEnv = fastCoeff * fastEnv + (1.0f - fastCoeff) * absIn;
      slowEnv = slowCoeff * slowEnv + (1.0f - slowCoeff) * absIn;

      // Transient = Fast - Slow
      float transient = fastEnv - slowEnv;

      // Gain calculation
      float gainFactor = 1.0f;

      // Apply Attack: Boost/Cut transients
      if (transient > 0.0f) {
        gainFactor += transient * att * 2.0f; // Scaling factor
      }

      // Apply Sustain: Boost/Cut body (Slow env)
      // Simpler approach: Sustain affects the whole signal but attack dominates
      // peaks Standard TS logic: gain += (process(transient) OR
      // process(sustain)) Here: Attack affects "transient" part Sustain affects
      // "slowEnv" part

      // Refined formula:
      // Signal = Input * (1 + att * transient_ratio + sus * sustain_ratio)

      gainFactor += slowEnv * sus;

      // Limit gain to avoid explosion
      gainFactor = juce::jlimit(0.1f, 4.0f, gainFactor);

      data[i] = in * gainFactor;
    }
  }
}

} // namespace zenith
