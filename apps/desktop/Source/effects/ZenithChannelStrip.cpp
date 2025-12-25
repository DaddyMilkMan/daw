/*
  ==============================================================================

    ZenithChannelStrip.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithChannelStrip.h"

namespace zenith {

// Parameter IDs
namespace IDs {
// Gate
const juce::String gateThresh = "gate_thresh";

// EQ
const juce::String eqHpfFreq = "eq_hpf_freq";
const juce::String eqLowFreq = "eq_low_freq";
const juce::String eqLowGain = "eq_low_gain";
const juce::String eqMidFreq = "eq_mid_freq";
const juce::String eqMidGain = "eq_mid_gain";
const juce::String eqMidQ = "eq_mid_q";
const juce::String eqHighFreq = "eq_high_freq";
const juce::String eqHighGain = "eq_high_gain";

// Comp
const juce::String compThresh = "comp_thresh";
const juce::String compRatio = "comp_ratio";
const juce::String compAttack = "comp_attack";
const juce::String compRelease = "comp_release";
const juce::String compMakeup = "comp_makeup";

// Master
const juce::String drive = "drive";
const juce::String output = "output";
} // namespace IDs

ZenithChannelStrip::ZenithChannelStrip()
    : ZenithPlugin(createParameterLayout()) {
  // Cache parameters
  gateThresh = apvts.getRawParameterValue(IDs::gateThresh);

  eqHpfFreq = apvts.getRawParameterValue(IDs::eqHpfFreq);
  eqLowFreq = apvts.getRawParameterValue(IDs::eqLowFreq);
  eqLowGain = apvts.getRawParameterValue(IDs::eqLowGain);
  eqMidFreq = apvts.getRawParameterValue(IDs::eqMidFreq);
  eqMidGain = apvts.getRawParameterValue(IDs::eqMidGain);
  eqMidQ = apvts.getRawParameterValue(IDs::eqMidQ);
  eqHighFreq = apvts.getRawParameterValue(IDs::eqHighFreq);
  eqHighGain = apvts.getRawParameterValue(IDs::eqHighGain);

  compThresh = apvts.getRawParameterValue(IDs::compThresh);
  compRatio = apvts.getRawParameterValue(IDs::compRatio);
  compAttack = apvts.getRawParameterValue(IDs::compAttack);
  compRelease = apvts.getRawParameterValue(IDs::compRelease);
  compMakeup = apvts.getRawParameterValue(IDs::compMakeup);

  drive = apvts.getRawParameterValue(IDs::drive);
  outputGain = apvts.getRawParameterValue(IDs::output);

  // Initialize Saturation function (Soft Clip)
  saturation.functionToUse = [](float x) { return std::tanh(x); };
}

ZenithChannelStrip::~ZenithChannelStrip() {}

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithChannelStrip::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // Gate
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::gateThresh, "Gate Thresh", -100.0f, 0.0f, -100.0f));

  // EQ
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::eqHpfFreq, "HPF Freq",
      juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f), 20.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::eqLowFreq, "Low Freq",
      juce::NormalisableRange<float>(50.0f, 800.0f, 1.0f, 0.5f), 100.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::eqLowGain, "Low Gain", -12.0f, 12.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::eqMidFreq, "Mid Freq",
      juce::NormalisableRange<float>(200.0f, 4000.0f, 1.0f, 0.5f), 1000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::eqMidGain, "Mid Gain", -12.0f, 12.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(IDs::eqMidQ, "Mid Q",
                                                         0.1f, 10.0f, 0.707f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::eqHighFreq, "High Freq",
      juce::NormalisableRange<float>(1000.0f, 16000.0f, 1.0f, 0.5f), 5000.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::eqHighGain, "High Gain", -12.0f, 12.0f, 0.0f));

  // Comp
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::compThresh, "Comp Thresh", -60.0f, 0.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::compRatio, "Comp Ratio", 1.0f, 20.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::compAttack, "Comp Attack", 0.1f, 100.0f, 10.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::compRelease, "Comp Release", 10.0f, 1000.0f, 100.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      IDs::compMakeup, "Comp Makeup", 0.0f, 24.0f, 0.0f));

  // Saturation
  layout.add(std::make_unique<juce::AudioParameterFloat>(IDs::drive, "Drive",
                                                         0.0f, 24.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(IDs::output, "Output",
                                                         -24.0f, 24.0f, 0.0f));

  return layout;
}

void ZenithChannelStrip::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = getTotalNumOutputChannels();

  gate.prepare(spec);
  eq.prepare(spec);
  compressor.prepare(spec);
  saturation.prepare(spec);

  // Force initial EQ coefficient update
  cachedHpfFreq = -1.0f; // Reset to force update
  updateEqCoefficientsIfNeeded(sampleRate);
}

void ZenithChannelStrip::releaseResources() {}

void ZenithChannelStrip::updateEqCoefficientsIfNeeded(double sr) {
  float hpf = eqHpfFreq->load();
  float lowF = eqLowFreq->load();
  float lowG = eqLowGain->load();
  float midF = eqMidFreq->load();
  float midG = eqMidGain->load();
  float midQ = eqMidQ->load();
  float highF = eqHighFreq->load();
  float highG = eqHighGain->load();

  bool needsUpdate = (hpf != cachedHpfFreq) || (lowF != cachedLowFreq) ||
                     (lowG != cachedLowGain) || (midF != cachedMidFreq) ||
                     (midG != cachedMidGain) || (midQ != cachedMidQ) ||
                     (highF != cachedHighFreq) || (highG != cachedHighGain);

  if (!needsUpdate)
    return;

  cachedHpfFreq = hpf;
  cachedLowFreq = lowF;
  cachedLowGain = lowG;
  cachedMidFreq = midF;
  cachedMidGain = midG;
  cachedMidQ = midQ;
  cachedHighFreq = highF;
  cachedHighGain = highG;

  *eq.get<0>().coefficients =
      *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hpf);
  *eq.get<1>().coefficients =
      *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
          sr, lowF, 0.707f, juce::Decibels::decibelsToGain(lowG));
  *eq.get<2>().coefficients =
      *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
          sr, midF, midQ, juce::Decibels::decibelsToGain(midG));
  *eq.get<3>().coefficients =
      *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
          sr, highF, 0.707f, juce::Decibels::decibelsToGain(highG));
}

void ZenithChannelStrip::processBlock(juce::AudioBuffer<float> &buffer,
                                      juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear side channels
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);

  double sr = getSampleRate();
  if (sr <= 0)
    sr = 44100.0; // Safety

  // Update EQ coefficients only when parameters change
  updateEqCoefficientsIfNeeded(sr);

  // Compressor
  compressor.setThreshold(compThresh->load());
  compressor.setRatio(compRatio->load());
  compressor.setAttack(compAttack->load());
  compressor.setRelease(compRelease->load());

  // Gate
  gate.setThreshold(gateThresh->load());

  // --- Processing Chain ---

  // 1. Gate (Clean up before processing)
  gate.process(context);

  // 2. EQ (Tone shaping)
  eq.process(context);

  // 3. Compressor (Dynamics)
  compressor.process(context);

  // 4. Makeup Gain (Post compressor)
  float makeup = juce::Decibels::decibelsToGain(compMakeup->load());
  if (std::abs(makeup - 1.0f) > 0.001f) {
    buffer.applyGain(makeup);
  }

  // 5. Saturation (Console Color)
  // Apply drive gain
  float drv = juce::Decibels::decibelsToGain(drive->load());
  if (drv > 1.0f) {
    // Boost into saturator
    buffer.applyGain(drv);
    saturation.process(context);
  }

  // 6. Output Trim
  float out = juce::Decibels::decibelsToGain(outputGain->load());
  buffer.applyGain(out);
}

} // namespace zenith
