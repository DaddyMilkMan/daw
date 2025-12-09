/*
  ==============================================================================

    Track.h
    Created: 26 May 2024 1:35:46pm
    Author:  Administrator

  ==============================================================================
*/

#pragma once

#include "AutomationLane.h"
#include "MixerChannel.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declarations
namespace zenith {
class Instrument;
class TempoMap;
class Clip; // Explicitly forward declare Clip
} // namespace zenith

namespace zenith {

// Forward declaration
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
  //==============================================================================
  enum class Type {
    Audio,
    MIDI,
    Instrument, // MIDI track with instrument plugin
    Bus,        // Audio bus (aux/submix)
    Master      // Master output track
  };

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
   * @param incomingMidi Optional: MIDI messages to process (from external controller).
   * @param auxBuffers Optional: List of aux bus buffers to send audio to.
   * @param tempoMap Optional: Current tempo map for beat calculations.
   * @note This method must be real-time safe.
   */
  void getNextAudioBlock(juce::AudioSourceChannelInfo &bufferToFill,
                         juce::int64 playheadPosition,
                         const juce::MidiBuffer *incomingMidi = nullptr,
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

  void setName(const juce::String &newName) { trackName = newName; sendChangeMessage(); }
  juce::String getName() const { return trackName; }

  void setType(Type newType) { trackType = newType; sendChangeMessage(); }
  Type getType() const { return trackType; }

  void setVolume(float newVolume) { mixerChannel.setVolume(newVolume); sendChangeMessage(); }
  float getVolume() const { return mixerChannel.getVolume(); }

  void setPan(float newPan) { mixerChannel.setPan(newPan); sendChangeMessage(); }
  float getPan() const { return mixerChannel.getPan(); }

  void setMuted(bool newMuted) { mixerChannel.setMuted(newMuted); sendChangeMessage(); }
  bool isMuted() const { return mixerChannel.isMuted(); }

  void setSolo(bool newSolo) { mixerChannel.setSolo(newSolo); sendChangeMessage(); }
  bool isSolo() const { return mixerChannel.isSolo(); }

  void setArmed(bool shouldBeArmed); // For recording
  bool isArmed() const { return armed.load(); }

  // Special setter for Engine's solo logic
  void setSilencedBySolo(bool silenced) { mixerChannel.setSilencedBySolo(silenced); }
  bool isSilencedBySolo() const { return mixerChannel.isSilencedBySolo(); }

  void setInputChannel(int channelIndex) { inputChannelIndex = channelIndex; sendChangeMessage(); }
  int getInputChannel() const { return inputChannelIndex; }

  void setMidiChannel(int channel) { midiChannel = channel; sendChangeMessage(); }
  int getMidiChannel() const { return midiChannel; }

  //==============================================================================
  // Clips
  //==============================================================================

  void addClip(std::unique_ptr<Clip> clip);
  void removeClip(int clipIndex);
  void removeClip(Clip *clip); // Overload for raw pointer
  void clearClips();
  int getNumClips() const;
  Clip *getClip(int index) const;
  const std::vector<std::unique_ptr<Clip>> &getClips() const {
    return clipsOwned_;
  }

  //==============================================================================
  // Plugins / FX Chain
  //==============================================================================

  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
  void removePlugin(int pluginIndex);
  void clearPlugins();
  int getNumPlugins() const;
  juce::AudioPluginInstance *getPlugin(int index) const;

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
  void setInstrument(std::unique_ptr<Instrument> instrument);
  Instrument* getInstrument() const { return instrument_.get(); } // Added getter

  //==============================================================================
  // Metering (Thread-safe)
  //==============================================================================

  float getCurrentLevel() const { return mixerChannel.getOutputLevel(); }
  float getPeakLevel() const { return mixerChannel.getOutputPeak(); }
  void resetPeakLevel() { mixerChannel.resetPeaks(); }

  //==============================================================================
  // Automation
  //==============================================================================

  void addAutomationLane(const juce::String &paramId,
                         std::shared_ptr<AutomationLane> lane);
  void clearAutomationLanes();

  // PDC Support
  void setLatencyCompensation(int samples);
  int getLatencyCompensation() const { return latencyCompensationSamples.load(); }

  //==============================================================================
  // Persistence
  //==============================================================================
  void loadPluginState(const juce::ValueTree& pluginTree, PluginHost& host);
  static void savePluginState(juce::AudioPluginInstance* plugin, juce::ValueTree& pluginTree);

  //==============================================================================
  // State management
  juce::ValueTree getState() const;
  void loadState(const juce::ValueTree &state);


  //==============================================================================
  // Expose MixerChannel for direct access (e.g. from MixerChannelComponent)
  MixerChannel& getMixerChannel() { return mixerChannel; }
  const MixerChannel& getMixerChannel() const { return mixerChannel; }

private:
  //==============================================================================
  // Track properties
  juce::String trackName;
  juce::String trackId;
  Type trackType;
  std::atomic<bool> armed{false}; // Changed from isArmed_ to armed to match new state
  std::atomic<bool> enabled{true}; // Added from origin/master
  int inputChannelIndex;
  int midiChannel = 1; // Default MIDI channel

  // PDC State
  std::atomic<int> latencyCompensationSamples{0};
  juce::AudioBuffer<float> compensationBuffer;
  int compensationWritePos = 0;

  // Current sample rate and block size (for plugin/clip preparation)
  double currentSampleRate;
  int currentBlockSize;

  //==============================================================================
  // Mixer & FX Chain
  //==============================================================================

  MixerChannel mixerChannel; // Handles volume, pan, sends, metering

  std::unique_ptr<Instrument> instrument_; // Special plugin for instrument tracks

  //==============================================================================
  // ROAST FIX #2: Plugin chain with RT-safe snapshot pattern (Phase 3: VST3
  // hosting MVP)
  //
  // Pattern (same as clips):
  // - Track owns plugins via std::vector<shared_ptr<Plugin>> (message thread
  // only)
  // - PluginSnapshot holds shared_ptr for audio thread to iterate safely
  // - Audio thread loads snapshot atomically, iterates without locking
  // - Message thread creates new snapshot when modifying plugins, swaps
  // atomically
  //
  // This eliminates the data race from the original code:
  // OLD: Audio thread reads std::vector while message thread modifies it (UB!)
  // NEW: Audio thread holds shared_ptr snapshot, ensuring plugins stay alive

  struct PluginSnapshot {
    std::vector<std::shared_ptr<juce::AudioPluginInstance>> plugins;

    PluginSnapshot() = default;
    explicit PluginSnapshot(
        const std::vector<std::shared_ptr<juce::AudioPluginInstance>>
            &ownedPlugins) {
      plugins.reserve(ownedPlugins.size());
      for (const auto &plugin : ownedPlugins)
        plugins.push_back(plugin); // Copy shared_ptr (increment refcount)
    }
  };

  // Plugin ownership (message thread only)
  std::vector<std::shared_ptr<juce::AudioPluginInstance>> pluginsOwned_;

  // Lock-free atomic snapshot for audio thread (truly RT-safe - no SpinLock!)
  // Audio thread reads this raw pointer atomically
  // Message thread manages lifetime via currentPluginSnapshot_ and
  // pluginSnapshotTrash_
  std::atomic<const PluginSnapshot *> activePluginSnapshot_{nullptr};
  std::shared_ptr<PluginSnapshot> currentPluginSnapshot_;
  std::vector<std::shared_ptr<PluginSnapshot>> pluginSnapshotTrash_;

  // Helper: Create new snapshot from current ownership
  void updatePluginSnapshot();

  juce::AudioBuffer<float> pluginBuffer; // From origin/master, not PDC

  //==============================================================================
  // Phase 2A: Lock-free clip list using RCU-style atomic snapshot
  //
  // Pattern:
  // - Track owns clips via std::vector<std::unique_ptr<Clip>> (message thread
  // only)
  // - ClipSnapshot holds raw Clip* pointers for audio thread to iterate
  // - Audio thread loads snapshot atomically, iterates without locking
  // - Message thread creates new snapshot when modifying clips, swaps
  // atomically
  //
  // This eliminates clipsLock from the audio thread (RT-safe).

  struct ClipSnapshot {
    std::vector<Clip *> clips;
    explicit ClipSnapshot(const std::vector<std::unique_ptr<Clip>> &c); // Definition in Track.cpp
  };

  // Clip ownership (message thread only)
  std::vector<std::unique_ptr<Clip>> clipsOwned_;

  // Lock-free atomic snapshot for audio thread (truly RT-safe - no SpinLock!)
  // Audio thread reads this raw pointer atomically
  // Message thread manages lifetime via currentClipSnapshot_ and
  // clipSnapshotTrash_
  std::atomic<const ClipSnapshot *> activeClipSnapshot_{nullptr};
  std::shared_ptr<ClipSnapshot> currentClipSnapshot_;
  std::vector<std::shared_ptr<ClipSnapshot>> clipSnapshotTrash_;

  // Helper: Create new snapshot from current ownership
  void updateClipSnapshot();

  // Phase 1: Pre-allocated clip buffer to avoid RT allocations
  juce::AudioBuffer<float> clipBuffer_;

  // Phase 2A: Pre-allocated MIDI buffer for MIDI clip playback and instruments
  juce::MidiBuffer midiBuffer_;

  //==============================================================================
  // Automation State (Lock-free RCU)
  //==============================================================================

  struct AutomationSnapshot {
    std::unordered_map<juce::String, std::shared_ptr<AutomationLane>> lanes;

    AutomationSnapshot() = default;
    explicit AutomationSnapshot(
        const std::unordered_map<juce::String, std::shared_ptr<AutomationLane>>
            &ownedLanes) {
      lanes = ownedLanes;
    }
  };

  // Ownership (Message thread)
  std::unordered_map<juce::String, std::shared_ptr<AutomationLane>>
      automationLanesOwned_;

  // RT Snapshot
  std::atomic<const AutomationSnapshot *> activeAutomationSnapshot_{nullptr};
  std::shared_ptr<AutomationSnapshot> currentAutomationSnapshot_;
  std::vector<std::shared_ptr<AutomationSnapshot>> automationSnapshotTrash_;

  void updateAutomationSnapshot();

  //==============================================================================
  // Helper methods
  void processPluginChain(juce::AudioBuffer<float> &buffer,
                          juce::MidiBuffer &midi, int numSamples);
  // These were from origin/master, not in HEAD. I will assume they are needed.
  void applyGainAndPan(juce::AudioBuffer<float> &buffer, int numSamples);
  void updateLevelMeters(const juce::AudioBuffer<float> &buffer,
                         int numSamples);

  // MIDI Scheduler state (from origin/master)
  struct ActiveNote {
    int pitch;
    int channel;
    juce::String noteId; // For tracking which ValueTree note this came from
  };
  juce::CriticalSection activeNotesLock; // Lock for activeNotes (from origin/master)
  std::vector<ActiveNote> activeNotes;   // Currently active notes (from origin/master)
  void generateMidiForBlock(const juce::ValueTree &trackState,
                            double tempo, double sampleRate,
                            juce::int64 blockStartSample, int blockSize,
                            juce::MidiBuffer &midiOut);


  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
