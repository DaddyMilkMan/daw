#include "Track.h"
#include "../instruments/Instrument.h"
#include "AudioTrack.h"
#include "AuxBusTrack.h"
#include "Clip.h"
#include "InstrumentTrack.h"
#include "MIDITrack.h"
#include "PluginHost.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include <algorithm>

namespace zenith {

std::unique_ptr<Track> Track::create(const juce::String &name, Type type) {
  switch (type) {
  case Type::Audio:
    return std::make_unique<AudioTrack>(name);
  case Type::MIDI:
    return std::make_unique<MIDITrack>(name);
  case Type::Instrument:
    return std::make_unique<InstrumentTrack>(name);
  case Type::Bus:
    return std::make_unique<AuxBusTrack>(name);
  default:
    return nullptr;
  }
}

//==============================================================================
Track::Track(const juce::String &name, Type type)
    : trackName(name), trackType(type) {}

Track::~Track() {}

//==============================================================================
void Track::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlockExpected;

  pluginBuffer.setSize(2, samplesPerBlockExpected);

  pluginChain.prepareToPlay(sampleRate, samplesPerBlockExpected);
  mixerChannel.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Track::releaseResources() {
  pluginChain.releaseResources();
  mixerChannel.releaseResources();
}

//==============================================================================
void Track::setName(const juce::String &newName) {
  trackName = newName;
  sendChangeMessage();
}

void Track::setVolume(float newVolume) { mixerChannel.setVolume(newVolume); }
float Track::getVolume() const { return mixerChannel.getVolume(); }

void Track::setPan(float newPan) { mixerChannel.setPan(newPan); }
float Track::getPan() const { return mixerChannel.getPan(); }

void Track::setMuted(bool shouldBeMuted) {
  mixerChannel.setMuted(shouldBeMuted);
}
bool Track::isMuted() const { return mixerChannel.isMuted(); }

void Track::setSolo(bool shouldBeSolo) { mixerChannel.setSolo(shouldBeSolo); }
bool Track::isSolo() const { return mixerChannel.isSolo(); }

void Track::setSilencedBySolo(bool silenced) {
  mixerChannel.setSilencedBySolo(silenced);
}
bool Track::isSilencedBySolo() const { return mixerChannel.isSilencedBySolo(); }

float Track::getCurrentLevel() const { return mixerChannel.getOutputLevel(); }
float Track::getPeakLevel() const { return mixerChannel.getOutputPeak(); }
void Track::resetPeakLevel() { mixerChannel.resetPeaks(); }

// getType() is inline in Track.h

juce::String Track::getTypeString() const {
  switch (trackType) {
  case Type::Audio:
    return "Audio";
  case Type::MIDI:
    return "MIDI";
  case Type::Instrument:
    return "Instrument";
  case Type::Bus:
    return "Bus";
  default:
    return "Unknown";
  }
}

void Track::setArmed(bool shouldBeArmed) {
  armed.store(shouldBeArmed);
  sendChangeMessage();
}

void Track::setEnabled(bool shouldBeEnabled) {
  enabled.store(shouldBeEnabled);
  sendChangeMessage();
}

//==============================================================================
void Track::setFreezeFile(const juce::File &file) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  freezeFile_ = file;
  std::shared_ptr<juce::AudioBuffer<float>> newBuffer = nullptr;

  if (file.existsAsFile()) {
    if (freezeFormatManager_.getNumKnownFormats() == 0) {
      freezeFormatManager_.registerBasicFormats();
    }

    std::unique_ptr<juce::AudioFormatReader> reader(
        freezeFormatManager_.createReaderFor(file));
    if (reader != nullptr) {
      if (reader->lengthInSamples > 0 &&
          reader->lengthInSamples < 200 * 60 * 48000) {
        newBuffer = std::make_shared<juce::AudioBuffer<float>>(
            reader->numChannels, (int)reader->lengthInSamples);
        reader->read(newBuffer.get(), 0, (int)reader->lengthInSamples, 0, true,
                     true);
      }
    }
  }
  std::atomic_store_explicit(&freezeBuffer_, newBuffer,
                             std::memory_order_release);
}

//==============================================================================
void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
  pluginChain.addPlugin(std::move(plugin), currentSampleRate, currentBlockSize);
  sendChangeMessage();
}

void Track::removePlugin(int pluginIndex) {
  pluginChain.removePlugin(pluginIndex);
  sendChangeMessage();
}

void Track::clearPlugins() {
  pluginChain.clearPlugins();
  sendChangeMessage();
}

int Track::getNumPlugins() const { return pluginChain.getNumPlugins(); }
juce::AudioPluginInstance *Track::getPlugin(int index) const {
  return pluginChain.getPlugin(index);
}

//==============================================================================
// Automation delegated to automationManager in header

//==============================================================================
juce::ValueTree Track::getState() const {
  juce::ValueTree state("Track");
  state.setProperty("name", trackName, nullptr);
  state.setProperty("type", static_cast<int>(trackType), nullptr);
  state.setProperty("volume", mixerChannel.getVolume(), nullptr);
  state.setProperty("pan", mixerChannel.getPan(), nullptr);
  state.setProperty("muted", mixerChannel.isMuted(), nullptr);
  state.setProperty("solo", mixerChannel.isSolo(), nullptr);
  state.setProperty("armed", armed.load(), nullptr);
  // inputMonitor_ removed - not in header
  state.setProperty("enabled", enabled.load(), nullptr);

  juce::ValueTree pluginsState("Plugins");
  for (int i = 0; i < pluginChain.getNumPlugins(); ++i) {
    auto *plugin = pluginChain.getPlugin(i);
    juce::ValueTree ps("Plugin");
    savePluginState(plugin, ps);
    pluginsState.appendChild(ps, nullptr);
  }
  state.appendChild(pluginsState, nullptr);
  return state;
}

void Track::loadState(const juce::ValueTree &state) {
  if (!state.hasType("Track"))
    return;
  trackName = state.getProperty("name", "Untitled Track");
  mixerChannel.setVolume(state.getProperty("volume", 0.8f));
  mixerChannel.setPan(state.getProperty("pan", 0.0f));
  mixerChannel.setMuted(state.getProperty("muted", false));
  mixerChannel.setSolo(state.getProperty("solo", false));
  armed.store(static_cast<bool>(state.getProperty("armed", false)));
  enabled.store(static_cast<bool>(state.getProperty("enabled", true)));

  // Plugin states are loaded via loadPluginStates() from Engine
  sendChangeMessage();
}

void Track::loadPluginStates(const juce::ValueTree &state,
                             PluginHost &pluginHost) {
  auto pluginsState = state.getChildWithName("Plugins");
  if (!pluginsState.isValid())
    return;
  clearPlugins();
  for (auto ps : pluginsState) {
    if (ps.hasType("Plugin"))
      loadPluginState(ps, pluginHost);
  }
}

void Track::injectLiveMidiMessage(const juce::MidiMessage &message) {
  liveMidiFifo_.push(message);
}

//==============================================================================
void Track::processPluginChain(juce::AudioBuffer<float> &buffer,
                               juce::MidiBuffer &midi, int numSamples) {
  // Inject live MIDI messages
  liveMidiFifo_.drainTo(midi, numSamples);

  pluginChain.process(buffer, midi);
}

void Track::applyGainAndPan(juce::AudioBuffer<float> &buffer, int numSamples) {
  // MixerChannel handles gain and pan internally during getNextAudioBlock
  // This method is kept for API compatibility but is now a no-op
  juce::ignoreUnused(buffer, numSamples);
}

void Track::addClip(std::unique_ptr<Clip> /*clip*/) {
  // This track type does not support clips. The passed clip will be destroyed
  // on scope exit.
  jassertfalse;
}

void Track::updateLevelMeters(const juce::AudioBuffer<float> &buffer,
                              int numSamples) {
  juce::ignoreUnused(numSamples);
  mixerChannel.updateMeters(buffer, false); // false = output meters
}

void Track::updateClipPositions(juce::int64 playheadPosition) {
  for (int i = 0; i < getNumClips(); ++i) {
    if (auto *clip = getClip(i)) {
      clip->setTransportPosition(playheadPosition);
    }
  }
}

} // namespace zenith
