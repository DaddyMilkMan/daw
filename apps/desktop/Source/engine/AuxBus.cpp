#include "AuxBus.h"

namespace zenith {

AuxBus::AuxBus(const juce::String &name) : name_(name) {}

AuxBus::~AuxBus() { pluginChain.clearPlugins(); }

void AuxBus::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  currentSampleRate_ = sampleRate;
  currentBlockSize_ = samplesPerBlockExpected;

  // Prepare input buffer (stereo)
  inputBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);
  inputBuffer_.clear();

  // Prepare mixer channel
  mixerChannel.prepareToPlay(samplesPerBlockExpected, sampleRate);

  // Prepare all plugins
  pluginChain.prepareToPlay(sampleRate, samplesPerBlockExpected);
}

void AuxBus::releaseResources() {
  mixerChannel.releaseResources();

  pluginChain.releaseResources();
}

void AuxBus::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  // The inputBuffer_ has already been filled by track sends
  // We just need to process it and copy to output

  // Clear output first
  bufferToFill.clearActiveBufferRegion();

  // Process through plugin chain (effect processors)
  juce::MidiBuffer emptyMidi; // Aux buses don't process MIDI
  pluginChain.process(inputBuffer_, emptyMidi);

  // Process through mixer channel (volume, pan, metering, etc.)
  juce::AudioSourceChannelInfo mixerInfo(&inputBuffer_, 0,
                                         bufferToFill.numSamples);
  mixerChannel.getNextAudioBlock(mixerInfo);

  // Copy processed audio to output
  for (int ch = 0; ch < juce::jmin(bufferToFill.buffer->getNumChannels(),
                                   inputBuffer_.getNumChannels());
       ++ch) {
    bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample, inputBuffer_,
                                  ch, 0, bufferToFill.numSamples);
  }

  // Clear input buffer for next block
  inputBuffer_.clear();
}

//==============================================================================
// Plugin Management
//==============================================================================

void AuxBus::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
  pluginChain.addPlugin(std::move(plugin), currentSampleRate_,
                        currentBlockSize_);
}

void AuxBus::removePlugin(int pluginIndex) {
  pluginChain.removePlugin(pluginIndex);
}

void AuxBus::clearPlugins() { pluginChain.clearPlugins(); }

int AuxBus::getNumPlugins() const { return pluginChain.getNumPlugins(); }

juce::AudioPluginInstance *AuxBus::getPlugin(int index) const {
  return pluginChain.getPlugin(index);
}

} // namespace zenith
