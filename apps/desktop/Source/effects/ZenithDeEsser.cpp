/*
  ==============================================================================

    ZenithDeEsser.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithDeEsser.h"

namespace zenith {

ZenithDeEsser::ZenithDeEsser() : ZenithPlugin(createParameterLayout()) {
  threshold = apvts.getRawParameterValue("threshold");
  frequency = apvts.getRawParameterValue("frequency");
  amount = apvts.getRawParameterValue("amount");
  listen = apvts.getRawParameterValue("listen");

  crossoverLow.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
  crossoverHigh.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
}

ZenithDeEsser::~ZenithDeEsser() {}

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithDeEsser::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "threshold", "Threshold", -60.0f, 0.0f, -20.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "frequency", "Frequency", 2000.0f, 10000.0f, 5000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "amount", "Amount", 0.0f, 1.0f, 0.5f)); // Ratio/Mix control
  layout.add(std::make_unique<juce::AudioParameterBool>("listen", "Listen Mode",
                                                        false));

  return layout;
}

void ZenithDeEsser::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
  spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

  crossoverLow.prepare(spec);
  crossoverHigh.prepare(spec);
  compressor.prepare(spec);

  // De-Esser settings: Fast attack, Fast release usually
  compressor.setAttack(1.0f);   // 1ms
  compressor.setRelease(50.0f); // 50ms

  // Pre-allocate highBand buffer for real-time safety
  // (avoid heap allocation in processBlock)
  highBand.setSize(static_cast<int>(spec.numChannels), samplesPerBlock);
}

void ZenithDeEsser::releaseResources() {}

void ZenithDeEsser::processBlock(juce::AudioBuffer<float> &buffer,
                                 juce::MidiBuffer &) {
  juce::ScopedNoDenormals noDenormals;

  // Assert that buffer size is within pre-allocated capacity for RT-safety
  jassert(buffer.getNumSamples() <= highBand.getNumSamples());
  jassert(buffer.getNumChannels() <= highBand.getNumChannels());

  // Update DSP
  float freq = frequency->load();
  crossoverLow.setCutoffFrequency(freq);
  crossoverHigh.setCutoffFrequency(freq);

  compressor.setThreshold(threshold->load());

  // Approximate Ratio from Amount (1:1 to Inf:1)
  float amt = amount->load();
  float ratio = 1.0f + (amt * 19.0f); // Max 20:1
  compressor.setRatio(ratio);

  // Resize highBand if needed (shouldn't happen often, but handle gracefully)
  if (highBand.getNumChannels() < buffer.getNumChannels() ||
      highBand.getNumSamples() < buffer.getNumSamples()) {
    highBand.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
  }
  // Copy input to highBand buffer (real-time safe copy)
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    highBand.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
  }

  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::AudioBlock<float> highBlock(highBand);

  juce::dsp::ProcessContextReplacing<float> contextLow(block);
  juce::dsp::ProcessContextReplacing<float> contextHigh(highBlock);

  // Split
  crossoverLow.process(contextLow);   // buffer becomes Low Band
  crossoverHigh.process(contextHigh); // highBand becomes High Band

  // Compress High Band
  compressor.process(contextHigh);

  // Sum
  if (listen->load() > 0.5f) {
    // Output only compressed high band (real-time safe copy)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      buffer.copyFrom(ch, 0, highBand, ch, 0, buffer.getNumSamples());
    }
  } else {
    // Sum Low + High
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      buffer.addFrom(ch, 0, highBand, ch, 0, buffer.getNumSamples());
    }
  }
}

} // namespace zenith
