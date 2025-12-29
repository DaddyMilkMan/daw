/*
  ==============================================================================

    ZenithPlugin.cpp
    Created: 2025-12-19
    Author:  Zenith DAW

  ==============================================================================
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
