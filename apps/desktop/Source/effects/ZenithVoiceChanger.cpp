/*
  ==============================================================================

    ZenithVoiceChanger.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithVoiceChanger.h"

namespace zenith {

ZenithVoiceChanger::ZenithVoiceChanger()
    : ZenithPlugin(createParameterLayout()) {
  characterParam = apvts.getRawParameterValue("character");
}

ZenithVoiceChanger::~ZenithVoiceChanger() {}

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithVoiceChanger::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  juce::StringArray characters;
  characters.add("Deep Male");
  characters.add("Chipmunk");
  characters.add("Robot");
  characters.add("Ethereal");

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "character", "Character", characters, 0));

  return layout;
}

void ZenithVoiceChanger::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = getTotalNumOutputChannels();

  voiceChanger.prepare(spec);
}

void ZenithVoiceChanger::releaseResources() { voiceChanger.reset(); }

void ZenithVoiceChanger::processBlock(juce::AudioBuffer<float> &buffer,
                                      juce::MidiBuffer &) {
  juce::ScopedNoDenormals noDenormals;

  int charIndex = static_cast<int>(characterParam->load());
  auto character = static_cast<DSPVoiceChanger::VoiceCharacter>(charIndex);

  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::AudioBlock<float> outputBlock(buffer); // In-place processing

  // Cast to const block for input? DSPVoiceChanger process signature:
  // process(const AudioBlock<const float>& input, AudioBlock<float>& output,
  // VoiceCharacter) We can cast block to const.

  juce::dsp::AudioBlock<const float> inputBlock(buffer);

  voiceChanger.process(inputBlock, outputBlock, character);
}

} // namespace zenith
