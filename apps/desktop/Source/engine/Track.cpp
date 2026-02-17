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
#include "RealTimeGarbageCollector.h"
#include <algorithm>
#include "ZenithLogger.h"

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
    // Handle unknown track type gracefully - log error and return AudioTrack as fallback
    ZENITH_LOG_ERROR("ERROR: Unknown track type " + juce::String(static_cast<int>(type)) + " requested. Returning AudioTrack as fallback.");
    return std::make_unique<AudioTrack>(name);
  }
}

//==============================================================================
  Track::Track(const juce::String &name, Type type)
    : trackName(name), trackType(type),
      processor(std::make_unique<TrackProcessor>()),
      mixerChannel(processor->getMixerChannel()),
      pluginChain(processor->getPluginChain()) {
    for (int i = 0; i < numSends; ++i) {
      sendDestinations[i].store(-1); // -1 means no destination
    }
  }

Track::~Track() {}

//==============================================================================
void Track::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  // Validate input parameters
  jassert(samplesPerBlockExpected > 0 && samplesPerBlockExpected <= 8192);
  jassert(sampleRate > 0.0 && sampleRate <= 192000.0);

  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlockExpected;

  processor->prepareToPlay(sampleRate, samplesPerBlockExpected);
}

void Track::releaseResources() {
  processor->releaseResources();
}

//==============================================================================
void Track::setName(const juce::String &newName) {
  trackName = newName;
  // Ensure sendChangeMessage called from message thread (Bug 93)
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

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
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::setEnabled(bool shouldBeEnabled) {
  enabled.store(shouldBeEnabled);
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::setSoloed(bool shouldBeSoloed) {
  mixerChannel.setSolo(shouldBeSoloed);
  sendChangeMessage();
}

bool Track::isSoloed() const { return mixerChannel.isSolo(); }

void Track::setColor(juce::Colour newColor) {
  trackColor = newColor;
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::setOutputId(const juce::String &id) {
  outputId = id;
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
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

  // FIX: RCU atomic update for lock-free audio thread access

  // 1. Keep old buffer alive until safe (Garbage Collection)
  if (freezeBufferOwner_)
    RealTimeGarbageCollector::getInstance().deferDelete(freezeBufferOwner_);

  // 2. Take ownership of new buffer
  freezeBufferOwner_ = newBuffer;

  // 3. Atomically publish pointer to audio thread
  activeFreezeBuffer_.store(newBuffer.get(), std::memory_order_release);
}

//==============================================================================
//==============================================================================
void Track::addClip(std::unique_ptr<Clip> clip) {
  juce::ignoreUnused(clip);
  // Base Track class does not manage clips directly.
  // Subclasses (ClipTrack, AudioTrack, MIDITrack) should override this.
}

void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
  pluginChain.addPlugin(std::move(plugin), currentSampleRate, currentBlockSize);
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::insertPluginAt(int pluginIndex,
                           std::unique_ptr<juce::AudioPluginInstance> plugin) {
  pluginChain.insertPluginAt(pluginIndex, std::move(plugin), currentSampleRate,
                             currentBlockSize);
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::removePlugin(int pluginIndex) {
  pluginChain.removePlugin(pluginIndex);
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::clearPlugins() {
  pluginChain.clearPlugins();
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

int Track::getNumPlugins() const { return pluginChain.getNumPlugins(); }
juce::AudioPluginInstance *Track::getPlugin(int index) const {
  return pluginChain.getPlugin(index);
}

//==============================================================================
// Automation delegated to automationManager in header

//==============================================================================
void Track::setSendDestination(int sendIndex, int auxBusIndex) {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    sendDestinations[sendIndex].store(auxBusIndex);
    sendChangeMessage();
    // Routing changes will be handled by the Engine observing this track
  }
}

int Track::getSendDestination(int sendIndex) const {
  if (juce::isPositiveAndBelow(sendIndex, numSends)) {
    return sendDestinations[sendIndex].load();
  }
  return -1;
}

void Track::setSendLevel(int sendIndex, float level) {
  mixerChannel.setSendLevel(sendIndex, level);
}

float Track::getSendLevel(int sendIndex) const {
  return mixerChannel.getSendLevel(sendIndex);
}

void Track::setSendPreFader(int sendIndex, bool preFader) {
  mixerChannel.setSendPreFader(sendIndex, preFader);
}

bool Track::isSendPreFader(int sendIndex) const {
  return mixerChannel.isSendPreFader(sendIndex);
}

void Track::setPluginSidechainSource(int pluginIndex, Track* sourceTrack) {
    sidechainSourceTrack_.store(sourceTrack); // Simple storage
    if (processor) {
        processor->setSidechainSource(pluginIndex, sourceTrack);
    }
}

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
  state.setProperty("inputMonitor", inputMonitor_.load(), nullptr);
  state.setProperty("enabled", enabled.load(), nullptr);
  state.setProperty("color", trackColor.toString(), nullptr);
  state.setProperty("outputId", outputId, nullptr);

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
  armed.store(state.getProperty("armed", false));
  inputMonitor_.store(state.getProperty("inputMonitor", false));
  enabled.store(state.getProperty("enabled", true));
  trackColor = juce::Colour::fromString(state.getProperty("color", "FF808080").toString());
  outputId = state.getProperty("outputId", "master");

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

//==============================================================================
void Track::injectLiveMidiMessage(const juce::MidiMessage &message) {
  noteFifo_.push(message);
}

void Track::updateClipPositions(juce::int64 playheadPosition) {
  for (int i = 0; i < getNumClips(); ++i) {
    if (auto *clip = getClip(i)) {
      clip->setTransportPosition(playheadPosition);
    }
  }
}

//==============================================================================
} // namespace zenith
