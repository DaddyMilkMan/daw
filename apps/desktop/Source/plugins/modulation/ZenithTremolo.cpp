/*
  ==============================================================================

    ZenithTremolo.cpp
    Created: 2025-12-19
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithTremolo.h"

namespace zenith {

ZenithTremolo::ZenithTremolo() : ZenithPlugin(createParameterLayout()) {
  rateParam = apvts.getRawParameterValue("rate");
  depthParam = apvts.getRawParameterValue("depth");
}

ZenithTremolo::~ZenithTremolo() {}

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithTremolo::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "rate", "Rate", juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f, 0.5f),
      5.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "depth", "Depth", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
      50.0f));

  return {params.begin(), params.end()};
}

void ZenithTremolo::processBlock(juce::AudioBuffer<float> &buffer,
                                 juce::MidiBuffer &midiMessages) {
  juce::ignoreUnused(midiMessages);

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear excess output channels
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  float rate = *rateParam;
  float depth = *depthParam / 100.0f; // 0..1
  double sampleRate = getSampleRate();
  if (sampleRate <= 0.0)
    return;

  float phaseIncrement = static_cast<float>(
      (rate * juce::MathConstants<double>::twoPi) / sampleRate);

  for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
    float lfo = (std::sin(currentPhase) + 1.0f) * 0.5f; // Unipolar 0..1
    float gain = 1.0f - (depth * lfo);

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
      auto *channelData = buffer.getWritePointer(channel);
      channelData[sample] *= gain;
    }

    currentPhase += phaseIncrement;
    if (currentPhase >= juce::MathConstants<float>::twoPi)
      currentPhase -= juce::MathConstants<float>::twoPi;
  }
}

} // namespace zenith
