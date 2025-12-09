/*
  ==============================================================================

    Track.h
    Ported from: ZenithDAW-Native/Source/Audio/Track.h (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Audio/MIDI track with clip playback, plugin chain, and mixer controls

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - OwnedArray<Clip> → std::vector<std::unique_ptr<Clip>>
    - Plugin hosting stubbed for Phase 2

  ==============================================================================
*/

#pragma once

#include "MixerChannel.h"
#include "AutomationLane.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>
#include <unordered_map>

// Forward declarations
namespace zenith {
class Instrument;
class TempoMap;
}

namespace zenith {

// Forward declaration
class PluginHost;

//==============================================================================
/**
    Represents an audio or MIDI track in the DAW.

    Each track can contain multiple clips, has its own plugin chain,
    and provides mixer controls (volume, pan, mute, solo).

    This class is designed to be used from both the audio thread and the
    message thread, so all controls use atomic operations for lock-free access.
*/
class Track : public juce::AudioSource, public juce::ChangeBroadcaster {
public:
  //==============================================================================
  enum class Type {
    Audio,
    MIDI,
    Instrument // MIDI track with instrument plugin
  };

  //==============================================================================
  Track(const juce::String &name, Type type);
  ~Track() override;

  //==============================================================================
  // AudioSource interface
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void releaseResources() override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;

  // Phase 1.3: Version that takes explicit playhead position and optional
  // incoming MIDI and aux buffers. Added optional TempoMap for automation.
  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      const std::vector<juce::AudioBuffer<float> *> &auxBuffers = {},
      const TempoMap* tempoMap = nullptr);

  //==============================================================================
  // Track properties
  const juce::String &getName() const { return trackName; }
  void setName(const juce::String &newName);

  const juce::String &getTrackId() const { return trackId; }
  void setTrackId(const juce::String &id) { trackId = id; }

  Type getType() const { return trackType; }
  juce::String getTypeString() const;

  int getTrackIndex() const { return trackIndex; }
  void setTrackIndex(int index) { trackIndex = index; }

  //==============================================================================
  // Mixer controls (thread-safe using atomics)
  // Mixer controls (thread-safe using atomics)
  void setVolume(float newVolume) { mixerChannel.setVolume(newVolume); }
  float getVolume() const { return mixerChannel.getVolume(); }

  void setPan(float newPan) { mixerChannel.setPan(newPan); }
  float getPan() const { return mixerChannel.getPan(); }

  void setMuted(bool shouldBeMuted) { mixerChannel.setMuted(shouldBeMuted); }
  bool isMuted() const { return mixerChannel.isMuted(); }

  void setSolo(bool shouldBeSolo) { mixerChannel.setSolo(shouldBeSolo); }
  bool isSolo() const { return mixerChannel.isSolo(); }
  
  void setSilencedBySolo(bool silenced) { mixerChannel.setSilencedBySolo(silenced); }
  bool isSilencedBySolo() const { return mixerChannel.isSilencedBySolo(); }

  void setArmed(bool shouldBeArmed); // For recording
  bool isArmed() const { return armed.load(); }

  void setEnabled(bool shouldBeEnabled);
  bool isEnabled() const { return enabled.load(); }

  void setInputChannel(int channel) { inputChannelIndex.store(channel); }
  int getInputChannel() const { return inputChannelIndex.load(); }

  MixerChannel &getMixerChannel() { return mixerChannel; }
  const MixerChannel &getMixerChannel() const { return mixerChannel; }

  //==============================================================================
  // Instrument management (for Instrument tracks)
  /**
   * @brief Set the instrument for this track
   * @param instrument Instrument instance (must be non-null)
   * @note Message thread only
   */
  void setInstrument(std::unique_ptr<Instrument> instrument);

  /**
   * @brief Get the current instrument (if any)
   * @return Pointer to instrument, or nullptr if no instrument set
   * @note Message thread only
   */
  Instrument *getInstrument() const { return instrument_.get(); }

  /**
   * @brief Check if track has an instrument
   */
  bool hasInstrument() const { return instrument_ != nullptr; }

  //==============================================================================
  // Plugin chain management (Phase 3: VST3 hosting MVP)
  // MESSAGE THREAD ONLY for add/remove/clear
  // Audio thread can process existing plugins safely (no modifications during
  // playback)
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
   * @brief Generate MIDI events for the current audio block from NOTES in clips
   * @param trackState ValueTree for this track from ProjectState
   * @param tempo Current tempo in BPM
   * @param sampleRate Current sample rate
   * @param blockStartSample Transport position at start of block
   * @param blockSize Number of samples in block
   * @param midiOut MIDI buffer to fill with events
   */
  void generateMidiForBlock(const juce::ValueTree &trackState, double tempo,
                            double sampleRate, juce::int64 blockStartSample,
                            int blockSize, juce::MidiBuffer &midiOut);

  //==============================================================================
  // Clip management
  class Clip; // Forward declaration

  void addClip(std::unique_ptr<Clip> clip);
  void removeClip(int clipIndex);
  void removeClip(Clip *clip);
  void clearClips();
  int getNumClips() const;
  Clip *getClip(int index) const;
  const std::vector<std::unique_ptr<Clip>>& getClips() const { return clipsOwned_; }

  //==============================================================================
  // Monitoring
  // Monitoring
  float getCurrentLevel() const { return mixerChannel.getOutputLevel(); }
  float getPeakLevel() const { return mixerChannel.getOutputPeak(); }
  void resetPeakLevel() { mixerChannel.resetPeaks(); }

  //==============================================================================
  // State management
  juce::ValueTree getState() const;
  void loadState(const juce::ValueTree &state);

  /**
   * @brief Load plugin states from ValueTree
   *
   * This must be called AFTER loadState() and requires access to PluginHost
   * to recreate plugin instances.
   *
   * @param state The track state ValueTree
   * @param pluginHost Reference to PluginHost for plugin instantiation
   */
  void loadPluginStates(const juce::ValueTree &state, PluginHost &pluginHost);

  // Single plugin state helpers
  void loadPluginState(const juce::ValueTree& pluginTree, PluginHost& host);
  static void savePluginState(juce::AudioPluginInstance* plugin, juce::ValueTree& pluginTree);

  //==============================================================================
  // Automation Management
  //==============================================================================
  
  // Message thread only: update automation for a specific parameter
  void addAutomationLane(const juce::String& paramId, std::shared_ptr<AutomationLane> lane);
  void clearAutomationLanes();

  // PDC Support
  void setLatencyCompensation(int samples);
  int getLatencyCompensation() const { return latencyCompensationSamples.load(); }

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
  int trackIndex = -1;

  //==============================================================================
  // Audio processing state
  double currentSampleRate = 48000.0;
  int currentBlockSize = 512;

  //==============================================================================
  // Mixer controls (delegated to MixerChannel)
  // Note: armed and enabled are track-specific, not channel-strip specific
  std::atomic<bool> armed{false};
  std::atomic<bool> enabled{true};

  // Input routing
  std::atomic<int> inputChannelIndex{0};

  //==============================================================================
  // Level monitoring (delegated to MixerChannel)
  // We keep wrappers for compatibility but they read from MixerChannel

  //==============================================================================
  // Instrument (for Instrument tracks)
  std::unique_ptr<Instrument> instrument_;
  juce::AudioBuffer<float> instrumentBuffer_;

  //==============================================================================
  // Mixer Channel Strip (EQ, Comp, Sends, Volume, Pan)
  MixerChannel mixerChannel;

  //==============================================================================
  // ROAST FIX #2: Plugin chain with RT-safe snapshot pattern (Phase 3: VST3 hosting MVP)
  //
  // Pattern (same as clips):
  // - Track owns plugins via std::vector<shared_ptr<Plugin>> (message thread only)
  // - PluginSnapshot holds shared_ptr for audio thread to iterate safely
  // - Audio thread loads snapshot atomically, iterates without locking
  // - Message thread creates new snapshot when modifying plugins, swaps atomically
  //
  // This eliminates the data race from the original code:
  // OLD: Audio thread reads std::vector while message thread modifies it (UB!)
  // NEW: Audio thread holds shared_ptr snapshot, ensuring plugins stay alive
  
  struct PluginSnapshot {
    std::vector<std::shared_ptr<juce::AudioPluginInstance>> plugins;
    
    PluginSnapshot() = default;
    explicit PluginSnapshot(const std::vector<std::shared_ptr<juce::AudioPluginInstance>>& ownedPlugins) {
      plugins.reserve(ownedPlugins.size());
      for (const auto& plugin : ownedPlugins)
        plugins.push_back(plugin); // Copy shared_ptr (increment refcount)
    }
  };

  // Plugin ownership (message thread only)
  std::vector<std::shared_ptr<juce::AudioPluginInstance>> pluginsOwned_;
  
  // Lock-free atomic snapshot for audio thread (truly RT-safe - no SpinLock!)
  // Audio thread reads this raw pointer atomically
  // Message thread manages lifetime via currentPluginSnapshot_ and pluginSnapshotTrash_
  std::atomic<const PluginSnapshot*> activePluginSnapshot_{nullptr};
  std::shared_ptr<PluginSnapshot> currentPluginSnapshot_;
  std::vector<std::shared_ptr<PluginSnapshot>> pluginSnapshotTrash_;
  
  // Helper: Create new snapshot from current ownership
  void updatePluginSnapshot();
  
  juce::AudioBuffer<float> pluginBuffer;

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
    std::vector<Clip *> clips; // Raw pointers (non-owning)

    ClipSnapshot() = default;
    explicit ClipSnapshot(
        const std::vector<std::unique_ptr<Clip>> &ownedClips) {
      clips.reserve(ownedClips.size());
      for (const auto &clip : ownedClips)
        clips.push_back(clip.get());
    }
  };

  // Clip ownership (message thread only)
  std::vector<std::unique_ptr<Clip>> clipsOwned_;

  // Lock-free atomic snapshot for audio thread (truly RT-safe - no SpinLock!)
  // Audio thread reads this raw pointer atomically
  // Message thread manages lifetime via currentClipSnapshot_ and clipSnapshotTrash_
  std::atomic<const ClipSnapshot*> activeClipSnapshot_{nullptr};
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
    explicit AutomationSnapshot(const std::unordered_map<juce::String, std::shared_ptr<AutomationLane>>& ownedLanes) {
        lanes = ownedLanes;
    }
  };

  // Ownership (Message thread)
  std::unordered_map<juce::String, std::shared_ptr<AutomationLane>> automationLanesOwned_;
  
  // RT Snapshot
  std::atomic<const AutomationSnapshot*> activeAutomationSnapshot_{nullptr};
  std::shared_ptr<AutomationSnapshot> currentAutomationSnapshot_;
  std::vector<std::shared_ptr<AutomationSnapshot>> automationSnapshotTrash_;

  void updateAutomationSnapshot();

  //==============================================================================
  // Helper methods
  void processPluginChain(juce::AudioBuffer<float> &buffer,
                          juce::MidiBuffer &midi, int numSamples);
  void applyGainAndPan(juce::AudioBuffer<float> &buffer, int numSamples);
  void updateLevelMeters(const juce::AudioBuffer<float> &buffer,
                         int numSamples);

  // MIDI Scheduler state
  struct ActiveNote {
    int pitch;
    int channel;
    juce::String noteId; // For tracking which ValueTree note this came from
  };
  std::vector<ActiveNote> activeNotes;
  juce::CriticalSection
      activeNotesLock; // Protects activeNotes vector for thread safety
  juce::int64 lastProcessedSample = 0;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
