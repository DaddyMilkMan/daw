#include "AuxBus.h"

namespace zenith {

AuxBus::AuxBus(const juce::String &name) : name_(name) {}

AuxBus::~AuxBus() { clearPlugins(); }

void AuxBus::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  currentSampleRate_ = sampleRate;
  currentBlockSize_ = samplesPerBlockExpected;

  // Prepare input buffer (stereo)
  inputBuffer_.setSize(2, samplesPerBlockExpected, false, true, true);
  inputBuffer_.clear();

  // Prepare mixer channel
  mixerChannel.prepareToPlay(samplesPerBlockExpected, sampleRate);

  // Prepare all plugins
  const juce::ScopedLock sl(pluginLock_);
  for (auto &plugin : plugins_) {
    if (plugin != nullptr) {
      plugin->prepareToPlay(sampleRate, samplesPerBlockExpected);
      plugin->setNonRealtime(false);
    }
  }
}

void AuxBus::releaseResources() {
  mixerChannel.releaseResources();

  const juce::ScopedLock sl(pluginLock_);
  for (auto &plugin : plugins_) {
    if (plugin != nullptr) {
      plugin->releaseResources();
    }
  }
}

void AuxBus::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  // The inputBuffer_ has already been filled by track sends
  // We just need to process it and copy to output

  // Clear output first
  bufferToFill.clearActiveBufferRegion();

  // Process through plugin chain (effect processors)
  juce::MidiBuffer emptyMidi; // Aux buses don't process MIDI
  for (auto &plugin : plugins_) {
    if (plugin != nullptr) {
      plugin->processBlock(inputBuffer_, emptyMidi);
    }
  }

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
  if (plugin == nullptr)
    return;

  const juce::ScopedLock sl(pluginLock_);

  // Prepare the plugin if we're already initialized
  if (currentSampleRate_ > 0) {
    plugin->prepareToPlay(currentSampleRate_, currentBlockSize_);
    plugin->setNonRealtime(false);
  }

  plugins_.push_back(std::move(plugin));
}

void AuxBus::removePlugin(int pluginIndex) {
  const juce::ScopedLock sl(pluginLock_);

  if (pluginIndex >= 0 && pluginIndex < static_cast<int>(plugins_.size())) {
    auto &plugin = plugins_[pluginIndex];
    if (plugin != nullptr) {
      plugin->releaseResources();
    }
    plugins_.erase(plugins_.begin() + pluginIndex);
  }
}

void AuxBus::clearPlugins() {
  const juce::ScopedLock sl(pluginLock_);

  for (auto &plugin : plugins_) {
    if (plugin != nullptr) {
      plugin->releaseResources();
    }
  }

  plugins_.clear();
}

int AuxBus::getNumPlugins() const {
  const juce::ScopedLock sl(pluginLock_);
  return static_cast<int>(plugins_.size());
}

juce::AudioPluginInstance *AuxBus::getPlugin(int index) const {
  const juce::ScopedLock sl(pluginLock_);
  if (index >= 0 && index < static_cast<int>(plugins_.size()))
    return plugins_[index].get();
  return nullptr;
}

} // namespace zenith
