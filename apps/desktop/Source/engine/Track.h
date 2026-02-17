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
#include "AutomationManager.h"
#include "EngineEvent.h" // For MidiFifo
#include "MixerChannel.h"
#include "PluginChain.h"
#include "TrackProcessor.h"
#include <atomic>
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
class TakeFolder; // Forward declare TakeFolder


//==============================================================================
/**
    Represents an audio or MIDI track in the DAW.

    Each track can contain multiple clips, has its own plugin chain,
    and provides mixer controls (volume, pan, mute, solo).

    This class is designed to be used from both the audio thread and the
    message thread, so all controls use atomic operations for lock-free access.

    ## Ownership Model (to prevent shared_ptr cycles):

    **Engine -> Track:** Engine holds std::shared_ptr<Track> (parent owns child)
    **Track -> Engine:** No back-reference stored (engine passed by reference
   when needed)
    **ClipTrack -> Clip:** ClipTrack owns clips via std::unique_ptr (parent owns
   child)
    **Clip -> Track:** No back-reference stored

    @note Track should NEVER hold std::shared_ptr<Engine> to avoid cycles.
*/
class Track : public juce::AudioSource, public juce::ChangeBroadcaster {
public:
  friend class AudioRenderer; // Allow AudioRenderer to access private members

  void setSoloed(bool shouldBeSoloed);
  bool isSoloed() const;

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
  // Bug 25: Ensure destructor is virtual for proper cleanup of derived classes
  virtual ~Track() override;

  //==============================================================================
  // AudioSource interface
  virtual void prepareToPlay(int samplesPerBlockExpected,
                             double sampleRate) override;
  virtual void releaseResources() override;

  // Standard AudioSource override to avoid abstraction issue
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override {
    getNextAudioBlock(bufferToFill, 0, nullptr, {}, nullptr);
  }


  // Phase 1.3: Version that takes explicit playhead position and optional
  // incoming MIDI and aux buffers. Added optional TempoMap for automation.
  virtual void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      const std::vector<juce::AudioBuffer<float> *> &auxBuffers = {},
      const TempoMap *tempoMap = nullptr,
      const juce::AudioBuffer<float> *sidechainBuffer = nullptr) = 0;

  /**
   * @brief Update clip scheduling/positions based on playhead
   * @param playheadPosition Current playhead position in samples
   */
  virtual void updateClipPositions(juce::int64 playheadPosition);

  //==============================================================================
  // Track properties
  const juce::String &getName() const { return trackName; }
  void setName(const juce::String &newName);

  const juce::String &getTrackId() const { return trackId; }
  const juce::String &getId() const { return trackId; } // Alias for UI
  void setTrackId(const juce::String &id) { trackId = id; }

  Type getType() const { return trackType; }
  juce::String getTypeString() const;

  int getTrackIndex() const { return trackIndex; }
  void setTrackIndex(int index) { trackIndex = index; }

  //==============================================================================
  // Appearance & Routing
  void setColor(juce::Colour newColor);
  juce::Colour getColor() const { return trackColor; }

  void setOutputId(const juce::String& newOutputId);
  juce::String getOutputId() const { return outputId; }

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
  void setInputMonitorEnabled(bool enabled) { inputMonitor_.store(enabled); }
  bool isInputMonitorEnabled() const { return inputMonitor_.load(); }

  void setInputChannel(int channel) { inputChannelIndex.store(channel); }
  int getInputChannel() const { return inputChannelIndex.load(); }


  //==============================================================================
  // Freeze state (for CPU optimization)
  void setFrozen(bool shouldBeFrozen) { frozen.store(shouldBeFrozen); }
  bool isFrozen() const { return frozen.load(); }

  // Active freeze rendering flag (audio thread safety)
  void setBeingFrozen(bool shouldBeFrozen) { isBeingFrozen_.store(shouldBeFrozen); }
  bool isBeingFrozen() const { return isBeingFrozen_.load(); }

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
   * @return Pointer to buffer, or nullptr if not frozen
   * @note Audio thread safe - Lock-free
   */
  juce::AudioBuffer<float> *getFreezeBuffer() const {
    return activeFreezeBuffer_.load(std::memory_order_acquire);
  }

  juce::AudioBuffer<float> &getSidechainBuffer() { return processor->getSidechainBuffer(); }
  TrackProcessor* getProcessor() const { return processor.get(); }

  MixerChannel &getMixerChannel() { return mixerChannel; }
  const MixerChannel &getMixerChannel() const { return mixerChannel; }

  // Instrument management (moved to InstrumentTrack)

  /**
   * @brief Set the sidechain source for a specific plugin on this track
   * @param pluginIndex Index of the plugin
   * @param sourceTrack Pointer to the source track (can be nullptr to disable)
   * @note Message thread only
   */
  void setPluginSidechainSource(int pluginIndex, Track* sourceTrack);
  
  /**
   * @brief Get the sidechain source track (simplified: assumes one source per track for now)
   */
  Track* getSidechainSource() const { return sidechainSourceTrack_.load(); } 

  //==============================================================================
  // Live MIDI Injection (Thread-safe)
  //==============================================================================
  /**
   * @brief Inject a MIDI message from the message thread (e.g. virtual
   * keyboard)
   * @param message The MIDI message to inject
   */
  void injectLiveMidiMessage(const juce::MidiMessage &message);

  //==============================================================================
  // Plugin chain management (Phase 3: VST3 hosting MVP)
  // MESSAGE THREAD ONLY for add/remove/clear
  // Audio thread can process existing plugins safely (no modifications during
  // playback)
  void addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin);
  void insertPluginAt(int pluginIndex,
                      std::unique_ptr<juce::AudioPluginInstance> plugin);
  void removePlugin(int pluginIndex);
  void clearPlugins();
  int getNumPlugins() const;
  juce::AudioPluginInstance *getPlugin(int index) const;
  virtual int getNumClips() const { return 0; }
  virtual Clip *getClip(int index) const { return nullptr; }
  virtual void addClip(Clip *clip) { juce::ignoreUnused(clip); }
  virtual void addClip(std::unique_ptr<Clip> clip);
  
  // Take Folder Management
  virtual int getNumTakeFolders() const { return 0; }
  virtual TakeFolder *getTakeFolder(int index) const { return nullptr; }
  virtual TakeFolder *getTakeFolderAt(int64_t position) const { return nullptr; }
  virtual void addTakeFolder(std::shared_ptr<TakeFolder> folder) { juce::ignoreUnused(folder); }
  virtual void removeTakeFolder(TakeFolder *folder) { juce::ignoreUnused(folder); }

  virtual Instrument *getInstrument() const { return nullptr; }
  virtual bool hasInstrument() const { return getInstrument() != nullptr; }

  // Send management
  void setSendDestination(int sendIndex, int auxBusIndex);
  int getSendDestination(int sendIndex) const;
  void setSendLevel(int sendIndex, float level);
  float getSendLevel(int sendIndex) const;
  void setSendPreFader(int sendIndex, bool preFader);
  bool isSendPreFader(int sendIndex) const;

  // Default implementation does nothing - subclasses with clips override

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

  //==============================================================================
  // Plugin Parameter Automation
  //==============================================================================

  /**
   * @brief Get all automatable parameters for a plugin on this track
   * @param pluginIndex Index of the plugin in the chain
   * @return Vector of parameter info
   */
  std::vector<PluginChain::ParameterInfo> getPluginParameters(int pluginIndex) const {
    return pluginChain.getAutomatableParameters(pluginIndex);
  }

  /**
   * @brief Get all automatable parameters for all plugins on this track
   * @return Vector of parameter info for all plugins
   */
  std::vector<PluginChain::ParameterInfo> getAllPluginParameters() const {
    return pluginChain.getAllAutomatableParameters();
  }

  /**
   * @brief Set a plugin parameter value (from automation)
   * @param pluginIndex Index of the plugin in the chain
   * @param paramIndex Index of the parameter
   * @param normalizedValue Value in range [0.0, 1.0]
   * @note Message thread only - will be applied RT-safely via setValueNotifyingHost
   */
  void setPluginParameterValue(int pluginIndex, int paramIndex, float normalizedValue) {
    pluginChain.setParameterValue(pluginIndex, paramIndex, normalizedValue);
  }

  /**
   * @brief Get the number of parameters for a plugin
   * @param pluginIndex Index of the plugin
   * @return Number of parameters
   */
  int getPluginNumParameters(int pluginIndex) const {
    return pluginChain.getNumParameters(pluginIndex);
  }

  /**
   * @brief Get a parameter name
   * @param pluginIndex Index of the plugin
   * @param paramIndex Index of the parameter
   * @return Parameter name
   */
  juce::String getPluginParameterName(int pluginIndex, int paramIndex) const {
    return pluginChain.getParameterName(pluginIndex, paramIndex);
  }

protected:
  //==============================================================================
  // Track properties

  juce::String trackName;
  juce::String trackId;
  Type trackType;
  int trackIndex = -1;
  juce::Colour trackColor = juce::Colours::grey;
  juce::String outputId = "master";

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
  std::atomic<bool> inputMonitor_{false};
  std::atomic<bool> frozen{false}; // Track freeze state for CPU optimization
  std::atomic<bool> isBeingFrozen_{false}; // Active freeze rendering flag

  // Freeze file storage (for CPU optimization)
  juce::File freezeFile_;

  // Freeze buffer storage (Lock-free RCU pattern)
  std::shared_ptr<juce::AudioBuffer<float>>
      freezeBufferOwner_; // Message thread owner
  std::atomic<juce::AudioBuffer<float> *> activeFreezeBuffer_{
      nullptr}; // Audio thread view

  juce::AudioFormatManager freezeFormatManager_;

  // Input routing
  std::atomic<int> inputChannelIndex{0};
  
  // Sidechaining
  std::atomic<Track*> sidechainSourceTrack_{nullptr};

  //==============================================================================
  // Level monitoring (delegated to MixerChannel)
  // We keep wrappers for compatibility but they read from MixerChannel

  //==============================================================================
  //==============================================================================
  // Processor - MUST be declared before mixerChannel and pluginChain references
  std::unique_ptr<TrackProcessor> processor;

  //==============================================================================
  // Mixer Channel Strip (EQ, Comp, Sends, Volume, Pan)
  // These are references to members within processor, so processor must be initialized first
  MixerChannel& mixerChannel;

  //==============================================================================
  // Plugin chain and Automation management (delegated)
  PluginChain& pluginChain;
  AutomationManager automationManager;

  // Thread-safe FIFO for live MIDI injection
  MidiFifo noteFifo_;

  //==============================================================================
  // Helper methods
  // Removed obsolete methods (processPluginChain, applyGainAndPan, updateLevelMeters)
  // as they are now handled by TrackProcessor::processBlock

  std::atomic<bool> soloed_{false};

  // Send destinations (indices into aux bus list)
  static constexpr int numSends = 4; // Should match MixerChannel::numSends
  std::atomic<int> sendDestinations[numSends];

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

} // namespace zenith
