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
