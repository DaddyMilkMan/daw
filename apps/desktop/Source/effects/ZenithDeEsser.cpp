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
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = getTotalNumOutputChannels();

  crossoverLow.prepare(spec);
  crossoverHigh.prepare(spec);
  compressor.prepare(spec);

  // De-Esser settings: Fast attack, Fast release usually
  compressor.setAttack(1.0f);   // 1ms
  compressor.setRelease(50.0f); // 50ms
}

void ZenithDeEsser::releaseResources() {}

void ZenithDeEsser::processBlock(juce::AudioBuffer<float> &buffer,
                                 juce::MidiBuffer &) {
  juce::ScopedNoDenormals noDenormals;

  // Update DSP
  float freq = frequency->load();
  crossoverLow.setCutoffFrequency(freq);
  crossoverHigh.setCutoffFrequency(freq);

  compressor.setThreshold(threshold->load());

  // Approximate Ratio from Amount (1:1 to Inf:1)
  float amt = amount->load();
  float ratio = 1.0f + (amt * 19.0f); // Max 20:1
  compressor.setRatio(ratio);

  // Create copy for High Band
  juce::AudioBuffer<float> highBand(buffer);

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
    // Output only compressed high band
    buffer.allocate(buffer.getNumChannels(),
                    buffer.getNumSamples()); // Clear? No.
    buffer.makeCopyOf(highBand);
  } else {
    // Sum Low + High
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      buffer.addFrom(ch, 0, highBand, ch, 0, buffer.getNumSamples());
    }
  }
}

} // namespace zenith
