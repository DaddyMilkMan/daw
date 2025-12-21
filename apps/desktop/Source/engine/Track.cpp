#include "Track.h" // Retry atomic fix
#include "PluginHost.h" // Required for PluginHost methods
#include "ProjectState.h"
#include "TempoMap.h"
#include "AudioTrack.h"
#include "MIDITrack.h"
#include "InstrumentTrack.h"
#include "AuxBusTrack.h"
#include "../instruments/Instrument.h"
#include "Clip.h"
#include <algorithm>

namespace zenith {

std::unique_ptr<Track> Track::create(const juce::String &name, Type type) {
    switch (type) {
        case Type::Audio:      return std::make_unique<AudioTrack>(name);
        case Type::MIDI:       return std::make_unique<MIDITrack>(name);
        case Type::Instrument: return std::make_unique<InstrumentTrack>(name);
        case Type::Bus:        return std::make_unique<AuxBusTrack>(name);
        default:
            jassertfalse; // Unknown track type!
            return nullptr;
    }
}

//==============================================================================
Track::Track(const juce::String &name, Type type)
    : trackName(name), trackType(type) {
}

Track::~Track() {
}

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

juce::String Track::getTypeString() const {
  switch (trackType) {
  case Type::Audio:      return "Audio";
  case Type::MIDI:       return "MIDI";
  case Type::Instrument: return "Instrument";
  case Type::Bus:        return "Bus";
  default:               return "Unknown";
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

    std::unique_ptr<juce::AudioFormatReader> reader(freezeFormatManager_.createReaderFor(file));
    if (reader != nullptr) {
        // Max 200 minutes at 48kHz (explicit int64 to avoid overflow)
        constexpr juce::int64 MAX_FREEZE_SAMPLES = 200LL * 60 * 48000;
        if (reader->lengthInSamples > 0 && reader->lengthInSamples < MAX_FREEZE_SAMPLES) {
            newBuffer = std::make_shared<juce::AudioBuffer<float>>(reader->numChannels, (int)reader->lengthInSamples);
            reader->read(newBuffer.get(), 0, (int)reader->lengthInSamples, 0, true, true);
        }
    }
  } // End if (file.existsAsFile())
  
  // FIX: RCU atomic update for lock-free audio thread access
  
  // 1. Keep old buffer alive in trash (simple garbage collection)
  if (freezeBufferOwner_)
      freezeTrash_.push_back(freezeBufferOwner_);
  
  // Limit trash size (keep last 4 updates alive to ensure audio thread safety)
  if (freezeTrash_.size() > 4)
      freezeTrash_.erase(freezeTrash_.begin());

  // 2. Take ownership of new buffer
  freezeBufferOwner_ = newBuffer;

  // 3. Atomically publish pointer to audio thread
  activeFreezeBuffer_.store(newBuffer.get(), std::memory_order_release);
}

//==============================================================================
void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> p) {
  pluginChain.addPlugin(std::move(p), currentSampleRate, currentBlockSize);
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
  state.setProperty("enabled", enabled.load(), nullptr);

  juce::ValueTree pluginsState("Plugins");
  for (int i = 0; i < pluginChain.getNumPlugins(); ++i) {
    auto* plugin = pluginChain.getPlugin(i);
    juce::ValueTree ps("Plugin");
    savePluginState(plugin, ps);
    pluginsState.appendChild(ps, nullptr);
  }
  state.appendChild(pluginsState, nullptr);
  return state;
}

void Track::loadState(const juce::ValueTree &state) {
  if (!state.hasType("Track")) return;
  
  trackName = state.getProperty("name", trackName);
  
  setVolume(state.getProperty("volume", 0.8f));
  setPan(state.getProperty("pan", 0.0f));
  setMuted(state.getProperty("muted", false));
  setSolo(state.getProperty("solo", false));
  
  armed.store(state.getProperty("armed", false));
  enabled.store(state.getProperty("enabled", true));
}

void Track::loadPluginStates(const juce::ValueTree &state, PluginHost &pluginHost) {
  juce::ValueTree plugins = state.getChildWithName("Plugins");
  for (int i = 0; i < plugins.getNumChildren(); ++i) {
    loadPluginState(plugins.getChild(i), pluginHost);
  }
}

//==============================================================================
void Track::processPluginChain(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi, int numSamples) {
  juce::ignoreUnused(numSamples); // process handles size via buffer
  pluginChain.process(buffer, midi);
}

void Track::applyGainAndPan(juce::AudioBuffer<float> &buffer, int numSamples) {
  juce::ignoreUnused(numSamples);
  mixerChannel.processOutput(buffer);
}


void Track::addClip(std::unique_ptr<Clip> clip) {
}

void Track::updateLevelMeters(const juce::AudioBuffer<float> &buffer, int numSamples) {
    juce::ignoreUnused(numSamples);
    // updateMeters(buffer, isInput)
    // We'll mark this as false (output) by default for the track level meters
    mixerChannel.updateMeters(buffer, false);
}

//==============================================================================
void Track::subscribeNote(const ActiveNote& note) {
    int start1, size1, start2, size2;
    noteFifo_.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0)
        noteBuffer_[start1] = note;
    else if (size2 > 0)
        noteBuffer_[start2] = note;
    noteFifo_.finishedWrite(size1 + size2);
}

void Track::processPendingNotes() {
    int start1, size1, start2, size2;
    noteFifo_.prepareToRead(noteFifo_.getNumReady(), start1, size1, start2, size2);
    
    // For now, we just consume the notes to show the pattern is implemented
    // In a real synth, these would be passed to the voice manager
    if (size1 > 0) {
        for (int i = 0; i < size1; ++i) {
            // Process noteBuffer_[start1 + i]
        }
    }
    if (size2 > 0) {
        for (int i = 0; i < size2; ++i) {
            // Process noteBuffer_[start2 + i]
        }
    }
    noteFifo_.finishedRead(size1 + size2);
}

} // namespace zenith
