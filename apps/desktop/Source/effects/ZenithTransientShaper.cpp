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
  juce::ignoreUnused(samplesPerBlock);
  sampleRate_ = static_cast<float>(sampleRate);

  // Pre-compute envelope coefficients
  fastCoeff_ = std::exp(-1.0f / (sampleRate_ * 0.010f));
  slowCoeff_ = std::exp(-1.0f / (sampleRate_ * 0.100f));

  // Resize envelope vectors
  const int numChannels = getTotalNumOutputChannels();
  fastEnvelope.resize(static_cast<size_t>(numChannels), 0.0f);
  slowEnvelope.resize(static_cast<size_t>(numChannels), 0.0f);

  std::fill(fastEnvelope.begin(), fastEnvelope.end(), 0.0f);
  std::fill(slowEnvelope.begin(), slowEnvelope.end(), 0.0f);
}

void ZenithTransientShaper::releaseResources() {}

void ZenithTransientShaper::processBlock(juce::AudioBuffer<float> &buffer,
                                         juce::MidiBuffer &) {
  float att = attackGain->load();
  float sus = sustainGain->load();

  const int numChannels = buffer.getNumChannels();
  const int numSamples = buffer.getNumSamples();

  // Use pre-computed coefficients
  const float fastCoeff = fastCoeff_;
  const float slowCoeff = slowCoeff_;

  // Real-time safety: ensure vectors are large enough (should be sized in prepareToPlay)
  jassert(static_cast<int>(fastEnvelope.size()) >= numChannels);
  jassert(static_cast<int>(slowEnvelope.size()) >= numChannels);

  for (int ch = 0; ch < numChannels; ++ch) {
    auto *data = buffer.getWritePointer(ch);
    float &fastEnv = fastEnvelope[static_cast<size_t>(ch)];
    float &slowEnv = slowEnvelope[static_cast<size_t>(ch)];

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
