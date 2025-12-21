/*
  ==============================================================================

    Track.h
    Ported from: ZenithDAW-Native/Source/Audio/Track.h (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Audio/MIDI track with clip playback, plugin chain, and mixer controls

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - OwnedArray<Clip> → std::vector<std::unique_ptr<Clip>>
    Audio/MIDI track with clip playback, plugin chain, and mixer controls
  ==============================================================================
*/

#pragma once

#include "AutomationLane.h"
#include "MixerChannel.h"
#include "PluginChain.h"
#include "AutomationManager.h"
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
#include <unordered_map>
#include <vector>

// Forward declarations
namespace zenith {
class Instrument;
class TempoMap;
class Clip; // Move outside
} // namespace zenith

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
  friend class AudioRenderer; // Allow AudioRenderer to access private members

  void setSoloed(bool shouldBeSoloed);
  bool isSoloed() const;

public:
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
   * @brief Track Factory - Create the appropriate track subclass based on type
   */
  static std::unique_ptr<Track> create(const juce::String &name, Type type);

  Track(const juce::String &name, Type type);
  ~Track() override;

  //==============================================================================
  // AudioSource interface
  virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  virtual void releaseResources() override;


  // Standard AudioSource override to avoid abstraction issue
  void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override {
    getNextAudioBlock(bufferToFill, 0, nullptr, {}, nullptr);
  }

  // Phase 1.3: Version that takes explicit playhead position and optional
  // incoming MIDI and aux buffers. Added optional TempoMap for automation.
  virtual void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      const std::vector<juce::AudioBuffer<float> *> &auxBuffers = {},
      const TempoMap *tempoMap = nullptr) = 0;

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

  void setSilencedBySolo(bool silenced) {
    mixerChannel.setSilencedBySolo(silenced);
  }
  bool isSilencedBySolo() const { return mixerChannel.isSilencedBySolo(); }

  void setArmed(bool shouldBeArmed); // For recording
  bool isArmed() const { return armed.load(); }

  void setEnabled(bool shouldBeEnabled);
  bool isEnabled() const { return enabled.load(); }

  void setInputChannel(int channel) { inputChannelIndex.store(channel); }
  int getInputChannel() const { return inputChannelIndex.load(); }

  //==============================================================================
  // Freeze state (for CPU optimization)
  void setFrozen(bool shouldBeFrozen) { frozen.store(shouldBeFrozen); }
  bool isFrozen() const { return frozen.load(); }

  /**
   * @brief Set the freeze file for this track
   * @param file The pre-rendered audio file
   * @note Message thread only
   */
  void setFreezeFile(const juce::File &file);

  /**
   * @brief Get the freeze file for this track
   * @return The freeze file, or invalid file if not frozen
   */
  const juce::File &getFreezeFile() const { return freezeFile_; }

  /**
   * @brief Get the freeze audio buffer
   * @return Shared pointer to buffer, or nullptr if not frozen
   * @note Audio thread safe - RCU pattern
   */
  std::shared_ptr<juce::AudioBuffer<float>> getFreezeBuffer() const {
    return std::atomic_load_explicit(&freezeBuffer_, std::memory_order_acquire);
  }

  MixerChannel &getMixerChannel() { return mixerChannel; }
  const MixerChannel &getMixerChannel() const { return mixerChannel; }

  // Instrument management (moved to InstrumentTrack)

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
  virtual int getNumClips() const { return 0; }
  virtual Clip* getClip(int index) const { return nullptr; }
  virtual void addClip(Clip* clip) { juce::ignoreUnused(clip); }
  virtual void addClip(std::unique_ptr<Clip> clip);
  virtual Instrument* getInstrument() const { return nullptr; }
  virtual bool hasInstrument() const { return getInstrument() != nullptr; }

  // MIDI Scheduling (moved to MIDITrack)

  // Clip management (moved to subclasses)

  //==============================================================================
  // Monitoring
  // Monitoring
  float getCurrentLevel() const { return mixerChannel.getOutputLevel(); }
  float getPeakLevel() const { return mixerChannel.getOutputPeak(); }
  void resetPeakLevel() { mixerChannel.resetPeaks(); }

  //==============================================================================
  // State management
  virtual juce::ValueTree getState() const;
  virtual void loadState(const juce::ValueTree &state);

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
  void loadPluginState(const juce::ValueTree &pluginTree, PluginHost &host);
  static void savePluginState(juce::AudioPluginInstance *plugin,
                              juce::ValueTree &pluginTree);

  //==============================================================================
  // Automation Management
  //==============================================================================

  // Message thread only: update automation for a specific parameter
  void addAutomationLane(const juce::String &paramId,
                         std::shared_ptr<AutomationLane> lane) {
    automationManager.addLane(paramId, lane);
  }
  void clearAutomationLanes() { automationManager.clearLanes(); }

protected:
  //==============================================================================
  // Track properties
  juce::String trackName;
  juce::String trackId;
  Type trackType;
  int trackIndex = -1;

protected:
  //==============================================================================
  // Audio processing state
  double currentSampleRate = 48000.0;
  int currentBlockSize = 512;

  //==============================================================================
  // Mixer controls (delegated to MixerChannel)
  // Note: armed and enabled are track-specific, not channel-strip specific
  std::atomic<bool> armed{false};
  std::atomic<bool> enabled{true};
  std::atomic<bool> frozen{false}; // Track freeze state for CPU optimization

  // Freeze file storage (for CPU optimization)
  juce::File freezeFile_;
  // Freeze buffer storage (RT-safe access via shared_ptr atomic load)
  std::shared_ptr<juce::AudioBuffer<float>> freezeBuffer_;
  juce::AudioFormatManager freezeFormatManager_;

  // Input routing
  std::atomic<int> inputChannelIndex{0};

  //==============================================================================
  // Level monitoring (delegated to MixerChannel)
  // We keep wrappers for compatibility but they read from MixerChannel

  //==============================================================================
  // Mixer Channel Strip (EQ, Comp, Sends, Volume, Pan)
  MixerChannel mixerChannel;

  //==============================================================================
  // Plugin chain and Automation management (delegated)
  PluginChain pluginChain;
  AutomationManager automationManager;

  juce::AudioBuffer<float> pluginBuffer;


  //==============================================================================
  // Helper methods
  void processPluginChain(juce::AudioBuffer<float> &buffer,
                          juce::MidiBuffer &midi, int numSamples);
  void applyGainAndPan(juce::AudioBuffer<float> &buffer, int numSamples);
  void updateLevelMeters(const juce::AudioBuffer<float> &buffer,
                         int numSamples);

  std::atomic<bool> soloed_{false};

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
