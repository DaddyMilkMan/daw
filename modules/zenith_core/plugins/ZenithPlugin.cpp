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

#include "ZenithPlugin.h"
#include "ZenithPluginEditor.h" // We'll create this next

namespace zenith {

ZenithPlugin::ZenithPlugin(
    juce::AudioProcessorValueTreeState::ParameterLayout layout)
    : juce::AudioPluginInstance(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", std::move(layout)) {}

ZenithPlugin::~ZenithPlugin() {}

//==============================================================================
void ZenithPlugin::prepareToPlay(double sampleRate, int samplesPerBlock) {
  // Common setup if needed
  juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void ZenithPlugin::releaseResources() {}

bool ZenithPlugin::isBusesLayoutSupported(const BusesLayout &layouts) const {
  // Simple stereo-in/stereo-out check
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

//==============================================================================
juce::AudioProcessorEditor *ZenithPlugin::createEditor() {
  return new ZenithPluginEditor(*this);
}

//==============================================================================
void ZenithPlugin::getStateInformation(juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void ZenithPlugin::setStateInformation(const void *data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(apvts.state.getType()))
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

} // namespace zenith
