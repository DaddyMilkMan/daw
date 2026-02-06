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

#pragma once

// Track.h - Audio/MIDI track with clip playback, plugin chain, and mixer controls

#include "AutomationManager.h"
#include "EngineEvent.h" // For MidiFifo
#include "PluginChain.h"
#include "TrackProcessor.h"
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <memory>
#include <vector>

namespace zenith {
class AutomationLane;
class Clip;
class Instrument;
class MixerChannel;
class PluginHost;
class TempoMap;
class TrackFreezeState;
class TrackPluginManager;
class TrackSendManager;
class TrackSidechain;

class Track : public juce::AudioSource, public juce::ChangeBroadcaster {
public:
  enum class Type { Audio, MIDI, Instrument, Bus, Master };

  static std::unique_ptr<Track> create(const juce::String &name, Type type);

  Track(const juce::String &name, Type type);
  ~Track() override;

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void releaseResources() override;
  void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;
  virtual void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      std::span<juce::AudioBuffer<float>* const> auxBuffers = {},
      const TempoMap *tempoMap = nullptr,
      const juce::AudioBuffer<float> *sidechainBuffer = nullptr) = 0;
  virtual void updateClipPositions(juce::int64 playheadPosition);

  const juce::String &getName() const;
  void setName(const juce::String &newName);
  const juce::String &getTrackId() const;
  const juce::String &getId() const;
  void setTrackId(const juce::String &id);
  Type getType() const;
  juce::String getTypeString() const;
  int getTrackIndex() const;
  void setTrackIndex(int index);
  void setColor(juce::Colour newColor);
  juce::Colour getColor() const;
  void setOutputId(const juce::String &newOutputId);
  juce::String getOutputId() const;

  void setVolume(float newVolume);
  float getVolume() const;
  void setPan(float newPan);
  float getPan() const;
  void setMuted(bool shouldBeMuted);
  bool isMuted() const;
  void setSolo(bool shouldBeSolo);
  bool isSolo() const;
  void setSilencedBySolo(bool silenced);
  bool isSilencedBySolo() const;
  void setSoloed(bool shouldBeSoloed);
  bool isSoloed() const;
  void setArmed(bool shouldBeArmed);
  bool isArmed() const;
  void setEnabled(bool shouldBeEnabled);
  bool isEnabled() const;
  void setInputMonitorEnabled(bool enabled);
  bool isInputMonitorEnabled() const;
  void setInputChannel(int channel);
  int getInputChannel() const;

  void setFrozen(bool shouldBeFrozen);
  bool isFrozen() const;
  void setBeingFrozen(bool shouldBeFrozen);
  bool isBeingFrozen() const;
  void setFreezeFile(const juce::File &file);
  const juce::File &getFreezeFile() const;
  juce::AudioBuffer<float> *getFreezeBuffer() const;

  juce::AudioBuffer<float> &getSidechainBuffer();
  TrackProcessor *getProcessor() const;
  MixerChannel &getMixerChannel();
  const MixerChannel &getMixerChannel() const;
  void setPluginSidechainSource(int pluginIndex, std::shared_ptr<Track> sourceTrack);
  std::shared_ptr<Track> getSidechainSource() const;

  void injectLiveMidiMessage(const juce::MidiMessage &message);

  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
  void removePlugin(int pluginIndex);
  void clearPlugins();
  int getNumPlugins() const;
  juce::AudioPluginInstance *getPlugin(int index) const;

  virtual int getNumClips() const;
  virtual Clip *getClip(int index) const;
  virtual void addClip(Clip *clip);
  virtual void addClip(std::unique_ptr<Clip> clip);
  virtual Instrument *getInstrument() const;
  virtual bool hasInstrument() const;

  void setSendDestination(int sendIndex, int auxBusIndex);
  int getSendDestination(int sendIndex) const;
  void setSendLevel(int sendIndex, float level);
  float getSendLevel(int sendIndex) const;
  void setSendPreFader(int sendIndex, bool preFader);
  bool isSendPreFader(int sendIndex) const;

  float getCurrentLevel() const;
  float getPeakLevel() const;
  void resetPeakLevel();

  virtual juce::ValueTree getState() const;
  virtual void loadState(const juce::ValueTree &state);
  void loadPluginStates(const juce::ValueTree &state, PluginHost &pluginHost);
  void loadPluginState(const juce::ValueTree &pluginTree, PluginHost &host);
  static void savePluginState(juce::AudioPluginInstance *plugin,
                              juce::ValueTree &pluginTree);

  void addAutomationLane(const juce::String &paramId,
                         std::shared_ptr<AutomationLane> lane);
  void clearAutomationLanes();

  std::vector<PluginChain::ParameterInfo>
  getPluginParameters(int pluginIndex) const;
  std::vector<PluginChain::ParameterInfo> getAllPluginParameters() const;
  void setPluginParameterValue(int pluginIndex, int paramIndex,
                               float normalizedValue);
  int getPluginNumParameters(int pluginIndex) const;
  juce::String getPluginParameterName(int pluginIndex, int paramIndex) const;

protected:
  juce::String trackName;
  juce::String trackId;
  Type trackType;
  int trackIndex = -1;
  juce::Colour trackColor = juce::Colours::grey;
  juce::String outputId = "master";
  double currentSampleRate = 48000.0;
  int currentBlockSize = 512;
  std::atomic<bool> armed{false};
  std::atomic<bool> enabled{true};
  std::atomic<bool> inputMonitor_{false};
  std::atomic<int> inputChannelIndex{0};
  std::unique_ptr<TrackProcessor> processor;
  AutomationManager automationManager;
  MidiFifo noteFifo_;
  std::unique_ptr<TrackFreezeState> freezeState_;
  std::unique_ptr<TrackSidechain> sidechain_;
  std::unique_ptr<TrackSendManager> sendManager_;
  std::unique_ptr<TrackPluginManager> pluginManager_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
