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
#include "Track.h"
#include "AudioTrack.h"
#include "MIDITrack.h"
#include "InstrumentTrack.h"
#include "AuxBusTrack.h"
#include "TrackProcessor.h"
#include "TrackPluginManager.h"
#include "TrackFreeze.h"
#include "TrackSidechain.h"
#include "TrackSendManager.h"
#include "ZenithLogger.h"
#include <memory>

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
      freezeState_(std::make_unique<TrackFreezeState>()),
      sidechain_(std::make_unique<TrackSidechain>(*processor)),
      sendManager_(std::make_unique<TrackSendManager>(processor->getMixerChannel())),
      pluginManager_(std::make_unique<TrackPluginManager>(*processor,
                                                          currentSampleRate,
                                                          currentBlockSize)) {}

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

void Track::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill) {
  getNextAudioBlock(bufferToFill, 0, nullptr, {}, nullptr, nullptr);
}

//==============================================================================
const juce::String &Track::getName() const { return trackName; }

void Track::setName(const juce::String &newName) {
  trackName = newName;
  // Ensure sendChangeMessage called from message thread (Bug 93)
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

const juce::String &Track::getTrackId() const { return trackId; }
const juce::String &Track::getId() const { return trackId; }
void Track::setTrackId(const juce::String &id) { trackId = id; }

Track::Type Track::getType() const { return trackType; }

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

int Track::getTrackIndex() const { return trackIndex; }
void Track::setTrackIndex(int index) { trackIndex = index; }

void Track::setColor(juce::Colour newColor) {
  trackColor = newColor;
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

juce::Colour Track::getColor() const { return trackColor; }

void Track::setOutputId(const juce::String &id) {
  outputId = id;
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

juce::String Track::getOutputId() const { return outputId; }

void Track::setVolume(float newVolume) {
  processor->getMixerChannel().setVolume(newVolume);
}

float Track::getVolume() const {
  return processor->getMixerChannel().getVolume();
}

void Track::setPan(float newPan) { processor->getMixerChannel().setPan(newPan); }

float Track::getPan() const { return processor->getMixerChannel().getPan(); }

void Track::setMuted(bool shouldBeMuted) {
  processor->getMixerChannel().setMuted(shouldBeMuted);
}

bool Track::isMuted() const { return processor->getMixerChannel().isMuted(); }

void Track::setSolo(bool shouldBeSolo) {
  processor->getMixerChannel().setSolo(shouldBeSolo);
}

bool Track::isSolo() const { return processor->getMixerChannel().isSolo(); }

void Track::setSilencedBySolo(bool silenced) {
  processor->getMixerChannel().setSilencedBySolo(silenced);
}

bool Track::isSilencedBySolo() const {
  return processor->getMixerChannel().isSilencedBySolo();
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
  processor->getMixerChannel().setSolo(shouldBeSoloed);
  // BUG FIX #11: Consistent thread safety check like other setters
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

bool Track::isSoloed() const { return processor->getMixerChannel().isSolo(); }

//==============================================================================
void Track::setInputMonitorEnabled(bool enabled) {
  inputMonitor_.store(enabled);
}

bool Track::isInputMonitorEnabled() const { return inputMonitor_.load(); }

void Track::setInputChannel(int channel) { inputChannelIndex.store(channel); }

int Track::getInputChannel() const { return inputChannelIndex.load(); }

void Track::setFrozen(bool shouldBeFrozen) {
  freezeState_->setFrozen(shouldBeFrozen);
}

bool Track::isFrozen() const { return freezeState_->isFrozen(); }

void Track::setBeingFrozen(bool shouldBeFrozen) {
  freezeState_->setBeingFrozen(shouldBeFrozen);
}

bool Track::isBeingFrozen() const { return freezeState_->isBeingFrozen(); }

void Track::setFreezeFile(const juce::File &file) {
  freezeState_->setFreezeFile(file);
}

const juce::File &Track::getFreezeFile() const {
  return freezeState_->getFreezeFile();
}

juce::AudioBuffer<float> *Track::getFreezeBuffer() const {
  return freezeState_->getFreezeBuffer();
}

juce::AudioBuffer<float> &Track::getSidechainBuffer() {
  return processor->getSidechainBuffer();
}

TrackProcessor *Track::getProcessor() const { return processor.get(); }

MixerChannel &Track::getMixerChannel() { return processor->getMixerChannel(); }

const MixerChannel &Track::getMixerChannel() const {
  return processor->getMixerChannel();
}

//==============================================================================
void Track::addClip(Clip *clip) {
  juce::ignoreUnused(clip);
  // Base Track class does not manage clips directly.
  // Subclasses (ClipTrack, AudioTrack, MIDITrack) should override this.
}

void Track::addClip(std::unique_ptr<Clip> clip) {
  juce::ignoreUnused(clip);
  // Base Track class does not manage clips directly.
  // Subclasses (ClipTrack, AudioTrack, MIDITrack) should override this.
}

void Track::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
  pluginManager_->addPlugin(std::move(plugin));
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::removePlugin(int pluginIndex) {
  pluginManager_->removePlugin(pluginIndex);
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

void Track::clearPlugins() {
  pluginManager_->clearPlugins();
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
    sendChangeMessage();
  } else {
    juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
  }
}

int Track::getNumPlugins() const { return pluginManager_->getNumPlugins(); }
juce::AudioPluginInstance *Track::getPlugin(int index) const {
  return pluginManager_->getPlugin(index);
}

//==============================================================================
// Automation delegated to automationManager in header

//==============================================================================
void Track::setSendDestination(int sendIndex, int auxBusIndex) {
  sendManager_->setSendDestination(sendIndex, auxBusIndex);
  sendChangeMessage();
  // Routing changes will be handled by the Engine observing this track
}

int Track::getSendDestination(int sendIndex) const {
  return sendManager_->getSendDestination(sendIndex);
}

void Track::setSendLevel(int sendIndex, float level) {
  sendManager_->setSendLevel(sendIndex, level);
}

float Track::getSendLevel(int sendIndex) const {
  return sendManager_->getSendLevel(sendIndex);
}

void Track::setSendPreFader(int sendIndex, bool preFader) {
  sendManager_->setSendPreFader(sendIndex, preFader);
}

bool Track::isSendPreFader(int sendIndex) const {
  return sendManager_->isSendPreFader(sendIndex);
}

void Track::setPluginSidechainSource(int pluginIndex, std::shared_ptr<Track> sourceTrack) {
  sidechain_->setPluginSidechainSource(pluginIndex, sourceTrack);
  if (sourceTrack != nullptr) {
    DBG("Track " + trackName + ": Set sidechain source to " + sourceTrack->getName());
  } else {
    DBG("Track " + trackName + ": Cleared sidechain source");
  }
}

std::shared_ptr<Track> Track::getSidechainSource() const {
  return sidechain_->getSidechainSource();
}

//==============================================================================
juce::ValueTree Track::getState() const {
  juce::ValueTree state("Track");
  state.setProperty("name", trackName, nullptr);
  state.setProperty("type", static_cast<int>(trackType), nullptr);
  state.setProperty("volume", processor->getMixerChannel().getVolume(), nullptr);
  state.setProperty("pan", processor->getMixerChannel().getPan(), nullptr);
  state.setProperty("muted", processor->getMixerChannel().isMuted(), nullptr);
  state.setProperty("solo", processor->getMixerChannel().isSolo(), nullptr);
  state.setProperty("armed", armed.load(), nullptr);
  state.setProperty("inputMonitor", inputMonitor_.load(), nullptr);
  state.setProperty("enabled", enabled.load(), nullptr);
  state.setProperty("color", trackColor.toString(), nullptr);
  state.setProperty("outputId", outputId, nullptr);

  juce::ValueTree pluginsState("Plugins");
  for (int i = 0; i < processor->getPluginChain().getNumPlugins(); ++i) {
    auto *plugin = processor->getPluginChain().getPlugin(i);
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
  processor->getMixerChannel().setVolume(state.getProperty("volume", 0.8f));
  processor->getMixerChannel().setPan(state.getProperty("pan", 0.0f));
  processor->getMixerChannel().setMuted(state.getProperty("muted", false));
  processor->getMixerChannel().setSolo(state.getProperty("solo", false));
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

int Track::getNumClips() const { return 0; }

Clip *Track::getClip(int index) const {
  juce::ignoreUnused(index);
  return nullptr;
}

// Note: addClip() implementations are at lines 242+ (not stubbed here)

Instrument *Track::getInstrument() const { return nullptr; }

bool Track::hasInstrument() const { return getInstrument() != nullptr; }

float Track::getCurrentLevel() const {
  return processor->getMixerChannel().getOutputLevel();
}

float Track::getPeakLevel() const {
  return processor->getMixerChannel().getOutputPeak();
}

void Track::resetPeakLevel() { processor->getMixerChannel().resetPeaks(); }

void Track::addAutomationLane(const juce::String &paramId,
                              std::shared_ptr<AutomationLane> lane) {
  automationManager.addLane(paramId, lane);
}

void Track::clearAutomationLanes() { automationManager.clearLanes(); }

std::vector<PluginChain::ParameterInfo>
Track::getPluginParameters(int pluginIndex) const {
  return pluginManager_->getPluginParameters(pluginIndex);
}

std::vector<PluginChain::ParameterInfo>
Track::getAllPluginParameters() const {
  return pluginManager_->getAllPluginParameters();
}

void Track::setPluginParameterValue(int pluginIndex, int paramIndex,
                                    float normalizedValue) {
  pluginManager_->setPluginParameterValue(pluginIndex, paramIndex,
                                          normalizedValue);
}

int Track::getPluginNumParameters(int pluginIndex) const {
  return pluginManager_->getPluginNumParameters(pluginIndex);
}

juce::String Track::getPluginParameterName(int pluginIndex,
                                           int paramIndex) const {
  return pluginManager_->getPluginParameterName(pluginIndex, paramIndex);
}

void Track::updateClipPositions(juce::int64 playheadPosition) {
  for (int i = 0; i < getNumClips(); ++i) {
    if (auto *clip = getClip(i)) {
      clip->setTransportPosition(playheadPosition);
    }
  }
}

//==============================================================================
bool Track::isArmed() const { return armed.load(); }

bool Track::isEnabled() const { return enabled.load(); }

} // namespace zenith
