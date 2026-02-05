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

AuxBus::AuxBus(const juce::String &name) : name_(name) {}

AuxBus::~AuxBus() { pluginChain.clearPlugins(); }

void AuxBus::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  currentSampleRate_ = sampleRate;

  currentBlockSize_ = samplesPerBlockExpected;
  emptyMidi_.ensureSize(65536);

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
  emptyMidi_.clear();
  pluginChain.process(inputBuffer_, emptyMidi_);

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
