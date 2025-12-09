/*
  ==============================================================================

    Track.h
    Created: 26 May 2024 1:35:46pm
    Author:  Administrator

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <map>
#include <memory>
#include <vector>

#include "MixerChannel.h"
#include "AudioFilePool.h"
#include "AutomationLane.h"
#include "Clip.h"
#include "TempoMap.h"

namespace zenith {

// Forward declarations
class Engine;
class PluginHost;

/**
 * @class Track
 * @brief Represents an audio or MIDI track in the DAW.
 *
 * Manages clips, plugins, volume, pan, mute, solo, and arm state.
 * Designed to be real-time safe for audio processing where possible.
 */
class Track {
public:
  enum class Type { Audio, MIDI, Instrument };

  //==============================================================================
  /**
   * @brief Constructor
   * @param name Initial name of the track
   * @param type Type of track (Audio, MIDI, Instrument)
   */
  Track(juce::String name, Type type);
  ~Track();

  //==============================================================================
  // Lifecycle
  //==============================================================================

  /**
   * @brief Prepares track for audio playback.
   * @param samplesPerBlockExpected The expected number of samples in each audio
   * block
   * @param sampleRate The sample rate of the audio context
   */
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate);

  /**
   * @brief Releases any resources used by the track.
   */
  void releaseResources();

  //==============================================================================
  // Audio Processing (Realtime Safe)
  //==============================================================================

  using AuxBufferList = const std::vector<juce::AudioBuffer<float> *>;

  /**
   * @brief Processes the next block of audio for this track.
   * @param bufferToFill The audio buffer to fill with processed audio.
   * @param playheadPosition The current playhead position in samples.
   * @param midiBuffer Optional: MIDI messages to process.
   * @param auxBuffers Optional: List of aux bus buffers to send audio to.
   * @param tempoMap Optional: Current tempo map for beat calculations.
   * @note This method must be real-time safe.
   */
  void getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill,
                         juce::int64 playheadPosition,
                         juce::MidiBuffer *midiBuffer = nullptr,
                         const AuxBufferList &auxBuffers = {},
                         const TempoMap *tempoMap = nullptr);

  /**
   * @brief Legacy overload for simple audio processing without advanced
   * context.
   * @param bufferToFill The audio buffer to fill.
   */
  void getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill);

  //==============================================================================
  // Properties (Thread-safe where appropriate)
  //==============================================================================

  void setTrackId(const juce::String &newId) { trackId = newId; }
  juce::String getTrackId() const { return trackId; }

  void setName(const juce::String &newName) { trackName = newName; }
  juce::String getName() const { return trackName; }

  void setType(Type newType) { trackType = newType; }
  Type getType() const { return trackType; }

  void setVolume(float newVolume) { mixerChannel.setVolume(newVolume); }
  float getVolume() const { return mixerChannel.getVolume(); }

  void setPan(float newPan) { mixerChannel.setPan(newPan); }
  float getPan() const { return mixerChannel.getPan(); }

  void setMuted(bool newMuted) { mixerChannel.setMuted(newMuted); }
  bool isMuted() const { return mixerChannel.isMuted(); }

  void setSolo(bool newSolo) { mixerChannel.setSolo(newSolo); }
  bool isSolo() const { return mixerChannel.isSolo(); }

  void setArmed(bool newArmed) { isArmed_ = newArmed; }
  bool isArmed() const { return isArmed_; }

  // Special setter for Engine's solo logic
  void setSilencedBySolo(bool silenced) { mixerChannel.setSilencedBySolo(silenced); }
  bool isSilencedBySolo() const { return mixerChannel.isSilencedBySolo(); }

  void setInputChannel(int channelIndex) { inputChannelIndex = channelIndex; }
  int getInputChannel() const { return inputChannelIndex; }

  void setMidiChannel(int channel) { midiChannel = channel; }
  int getMidiChannel() const { return midiChannel; }

  //==============================================================================
  // Clips
  //==============================================================================

  void addClip(std::unique_ptr<Clip> newClip);
  void removeClip(const juce::String &clipId);
  Clip *getClip(int index) const;
  int getNumClips() const { return static_cast<int>(clips_.size()); }
  void clearClips();

  //==============================================================================
  // Plugins / FX Chain
  //==============================================================================

  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin);
  void removePlugin(int index);
  juce::AudioPluginInstance *getPlugin(int index) const;
  int getNumPlugins() const;

  /**
   * @brief Get total latency of the track in samples (PDC)
   * Sums the latency of all plugins in the chain.
   */
  int getLatencySamples() const;

  //==============================================================================
  // MIDI Scheduling
  /**
   * @brief Sets the internal instrument for instrument tracks.
   * @param instrument The instrument to use (owned by the track)
   */
  void setInstrument(std::unique_ptr<juce::AudioPluginInstance> instrument);

  //==============================================================================
  // Metering (Thread-safe)
  //==============================================================================

  float getCurrentLevel() const { return mixerChannel.getOutputLevel(); }
  float getPeakLevel() const { return mixerChannel.getOutputPeak(); }
  void resetPeakLevel() { mixerChannel.resetPeaks(); }

  //==============================================================================
  // Automation
  //==============================================================================

  void setAutomationLane(const juce::String &paramId,
                         std::shared_ptr<AutomationLane> lane);
  void addAutomationLane(const juce::String& paramId, std::shared_ptr<AutomationLane> lane);
  void clearAutomationLanes();

  // PDC Support
  void setLatencyCompensation(int samples);
  int getLatencyCompensation() const { return latencyCompensationSamples.load(); }

  //==============================================================================
  // Persistence
  //==============================================================================
  void loadPluginState(const juce::ValueTree& pluginTree, PluginHost& host);
  static void savePluginState(juce::AudioPluginInstance* plugin, juce::ValueTree& pluginTree);

private:
  // PDC State
  std::atomic<int> latencyCompensationSamples{0};
  juce::AudioBuffer<float> compensationBuffer;
  int compensationWritePos = 0;

  //==============================================================================
  // Track properties
  juce::String trackName;
  juce::String trackId;
  Type trackType;
  std::atomic<bool> isArmed_{false};
  int inputChannelIndex;
  int midiChannel = 1; // Default MIDI channel

  double lastSampleRate = 0.0;
  int lastBlockSize = 0;

  //==============================================================================
  // Mixer & FX Chain
  //==============================================================================

  MixerChannel mixerChannel; // Handles volume, pan, sends, metering

  std::vector<std::unique_ptr<juce::AudioPluginInstance>>
      pluginsOwned_; // Plugins for this track (owned)

  std::unique_ptr<juce::AudioPluginInstance>
      instrument_; // Special plugin for instrument tracks

  //==============================================================================
  // Clips
  //==============================================================================

  std::vector<std::unique_ptr<Clip>> clips_;

  //==============================================================================
  // Automation
  //==============================================================================

  std::map<juce::String, std::shared_ptr<AutomationLane>>
      automationLanes_; // Maps parameter ID to automation lane

  //==============================================================================
  // Real-time safe snapshots (lock-free)
  // The audio thread reads from these pointers, which are swapped atomically
  // when the message thread modifies the underlying vectors.
  //==============================================================================

  struct ClipSnapshot {
    std::vector<Clip *> clips;
    explicit ClipSnapshot(const std::vector<std::unique_ptr<Clip>> &c);
  };
  std::shared_ptr<ClipSnapshot> currentClipSnapshotHolder_;
  std::atomic<ClipSnapshot *> activeClipSnapshot_{nullptr};
  std::vector<std::shared_ptr<ClipSnapshot>> clipSnapshotTrash_;

  struct PluginSnapshot {
    std::vector<juce::AudioPluginInstance *> plugins;
    explicit PluginSnapshot(
        const std::vector<std::unique_ptr<juce::AudioPluginInstance>> &p);
  };
  std::shared_ptr<PluginSnapshot> currentPluginSnapshotHolder_;
  std::atomic<PluginSnapshot *> activePluginSnapshot_{nullptr};
  std::vector<std::shared_ptr<PluginSnapshot>> pluginSnapshotTrash_;

  struct AutomationSnapshot {
    std::map<juce::String, std::shared_ptr<AutomationLane>> lanes;
    explicit AutomationSnapshot(
        const std::map<juce::String, std::shared_ptr<AutomationLane>> &l);
  };
  std::shared_ptr<AutomationSnapshot> currentAutomationSnapshotHolder_;
  std::atomic<AutomationSnapshot *> activeAutomationSnapshot_{nullptr};
  std::vector<std::shared_ptr<AutomationSnapshot>> automationSnapshotTrash_;

  void updateClipSnapshot();
  void updatePluginSnapshot();
  void updateAutomationSnapshot();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith