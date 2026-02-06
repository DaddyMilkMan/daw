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

// Engine.h - Core audio engine for Zenith DAW

#include <atomic>
#include <functional>
#include <unordered_map>
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

#include "dsp/Dither.h"
#include "dsp/MasterLimiter.h"
#include "dsp/StereoAudioFifo.h"
#include "engine/EngineConstants.h"
#include "engine/MacroControl.h"
#include "engine/RoutingGraph.h"
#include "AudioRenderer.h"
#include "EngineEvent.h"
#include "WCETMonitor.h"
#include "utils/PowerManagement.h"

// Forward declarations
namespace zenith {
class ProjectState;
class TrackAutomationSynchronizer;
class Track;
class Clip;
class MixerChannel;
class AudioFilePool;
class PluginHost;
class Metronome;
class PluginEditorWindowManager;
class TempoMap;
class AuxBus;
class InstrumentRegistry;
class TrackFreezeManager;
class AudioRenderer;
class AudioExporter;
class RecordingManager;
class TransportController;
class MeteringSystem;
class MixerController;
class Midi2DiscoveryService;
class PropertyExchangeManager;

namespace ai {
class SessionDebuggerAgent;
class AIMasteringAgent;
} // namespace ai

//==============================================================================
/**
 * @class Engine
 // Brief: Core audio engine
 *
 * This class manages the entire audio processing chain:
 * 1. Audio device I/O
 * 2. Transport (play/stop/record)
 * 3. Audio routing and mixing
 * 4. MIDI input routing
 * 5. Audio input recording
 * 6. CPU usage monitoring
 *
 * ## Ownership Model (to prevent shared_ptr cycles):
 *
 * **Parent -> Child (shared_ptr/unique_ptr):**
 * - Engine owns Tracks via std::vector<std::shared_ptr<Track>>
 * - Engine owns AuxBuses via std::vector<std::shared_ptr<AuxBus>>
 * - Engine owns subsystems via std::unique_ptr (AudioRenderer, RecordingManager, etc.)
 *
 * **Child -> Parent (raw pointer/reference):**
 * - Subsystems hold Engine& references (TrackStateSynchronizer, RecordingManager, etc.)
 * - No child component holds std::shared_ptr<Engine>
 *
 * **RT-safe snapshots:**
 * - TrackSnapshot uses shared_ptr only for lifetime management (lifecycle vector)
 * - Audio thread accesses raw pointers extracted from the snapshot
 *
 // Note: To avoid memory leaks: NEVER store std::shared_ptr<Engine> in child components.
 *       Use Engine& or Engine* for back-references.
 */
class Engine : public juce::AudioIODeviceCallback,
               public juce::MidiInputCallback,
               public juce::ChangeListener {
public:
  //==========================================================================
  Engine();
  ~Engine() override;

  /**
   // Brief: Get the plugin format manager
   */
  juce::AudioPluginFormatManager &getPluginFormatManager();

  /**
   // Brief: Get the audio device manager
   */
  juce::AudioDeviceManager &getDeviceManager() { return deviceManager; }

  //==========================================================================
  // Initialization / Shutdown
  //==========================================================================

  /**
   // Brief: Set project state for automation synchronization and tempo
   * @param state Pointer to project state (can be nullptr to disable
   * automation)
   // Note: Must be called before initialize() or after automation is stopped
   */
  void setProjectState(ProjectState *state);

  /**
   // Brief: Get the associated ProjectState
   * @return Pointer to the ProjectState (may be null)
   */
  ProjectState *getProjectState() { return projectState_; }

  /**
   // Brief: Synchronize engine tracks with project state
   // Note: Message thread only - rebuilds track list from ProjectState
   */
  void syncWithProjectState();

  /**
   // Brief: Synchronize tempo map with project state
   // Note: Message thread only
   */
  void syncTempoMap();

  /**
   // Brief: Initialize the audio engine
   *
   * This:
   * 1. Opens the default audio device
   * 2. Sets up audio callback
   * 3. Starts audio processing
   *
   * @return true if initialization succeeded
   */
  bool initialize();

  /**
   // Brief: Shutdown the audio engine
   *
   * This:
   * 1. Stops audio processing
   * 2. Closes audio device
   * 3. Releases resources
   */
  void shutdown();

  //==========================================================================
  // Transport Controls
  //==========================================================================

  /**
   // Brief: Start playback
   */
  void play();

  /**
   // Brief: Stop playback
   */
  void stop();

  /**
   // Brief: Check if playing
   */
  bool isPlaying() const;

  /**
   // Brief: Start recording
   // Note: MESSAGE THREAD ONLY - Starts recording on armed tracks (both MIDI and
   * audio)
   */
  void record();

  /**
   // Brief: Stop recording and bake clips
   // Note: MESSAGE THREAD ONLY - Converts recordings to clips (both MIDI and
   * audio)
   */
  void stopRecording();

  /**
   // Brief: Check if recording
   */
  bool isRecording() const;

  /**
   // Brief: Toggle recording on/off
   // Note: Convenience method for record button
   */
  void toggleRecording();

  /**
   // Brief: Panic - Stop all sound immediately
   // Note: Stops transport, sends All Notes Off to all tracks, and clears
   * buffers.
   */
  void panic();

  /**
   // Brief: Suspend audio processing (e.g. for offline export)
   * @param shouldSuspend True to silence audio output/input
   // Note: Real-time safe (sets atomic flag)
   */
  void suspendProcessing(bool shouldSuspend) { isSuspended_.store(shouldSuspend); }

  /**
   // Brief: Check if processing is suspended
   */
  bool isSuspended() const { return isSuspended_.load(); }

  /**
   // Brief: Set sidechain source for a specific plugin on a track
   * @param destTrackIndex Index of the track containing the plugin
   * @param pluginIndex Index of the plugin to receive sidechain
   * @param sourceTrackIndex Index of the source track
   */
  void setSidechainSource(int destTrackIndex, int pluginIndex,
                          int sourceTrackIndex);

  //==========================================================================
  // Real-time Event Queue
  //==========================================================================

  /**
   // Brief: Queue an event for the audio thread
   * @param e Event to queue
   * @return true if queued successfully, false if full
   // Note: Lock-free, safe to call from any thread
   */
  bool queueEvent(const zenith::EngineEvent &e);

  //==========================================================================
  // Render Context (Live)
  //==========================================================================

  // REMOVED: getLiveContext() - AudioRenderer now manages its own internal state
  // The AudioRenderContext is owned by AudioRenderer, not Engine. 

  //==========================================================================
  // Transport Position & Looping
  //==========================================================================

  /**
   // Brief: Get current playback position in samples
   */
  juce::int64 getPlayheadSamples() const;

  /**
   // Brief: Get current playback position in samples (legacy accessor)
   */
  // [[deprecated("Use getPlayheadSamples() instead")]]
  juce::int64 getPlaybackPosition() const;

  /**
   // Brief: Get current playback position in beats
   */
  double getPlaybackPositionBeats() const;

  /**
   // Brief: Set playback position (MESSAGE THREAD ONLY)
   */
  void setPlayheadSamples(juce::int64 position);

  /**
   // Brief: Enable/disable looping
   */
  void setLooping(bool shouldLoop);

  /**
   // Brief: Check if looping is enabled
   */
  bool isLooping() const;

  /**
   // Brief: Set loop region in samples (MESSAGE THREAD ONLY)
   */
  void setLoopRegion(juce::int64 start, juce::int64 end);

  /**
   // Brief: Get loop start position in samples
   */
  juce::int64 getLoopStart() const;

  /**
   // Brief: Get loop end position in samples
   */
  juce::int64 getLoopEnd() const;

  //==========================================================================
  // Plugin Delay Compensation (PDC)
  //==========================================================================

  /**
   // Brief: Get total plugin latency for a track in samples
   * @param trackIndex Track index
   * @return Total latency from all plugins in the track's chain
   */
  int getTrackLatency(int trackIndex) const;

  /**
   // Brief: Get master bus total latency in samples
   * @return Total latency from all master bus plugins
   */
  int getMasterLatency() const;

  /**
   // Brief: Recalculate PDC for all tracks
   // Note: Call after adding/removing plugins or changing plugin latency
   */
  void recalculatePDC();

  /**
   // Brief: Check if PDC is enabled
   */
  bool isPDCEnabled() const;

  /**
   // Brief: Enable/disable PDC
   */
  void setPDCEnabled(bool enabled);

  /**
   // Brief: Get maximum track latency (for PDC compensation)
   */
  int getMaxTrackLatency() const;

  //==========================================================================
  // Test Tone (for calibration)
  //==========================================================================

  /**
   // Brief: Enable/disable the test tone generator
   * @param enabled true to enable 440Hz sine tone
   */
  void setEnableTestTone(bool enabled);

  /**
   // Brief: Check if test tone is enabled
   */
  bool isTestToneEnabled() const;

  //==========================================================================
  // Audio Device Management
  //==========================================================================

  /**
   // Brief: Get current audio device info
   * @return String describing current device and settings
   */
  juce::String getAudioDeviceInfo() const;

  /**
   // Brief: Get current sample rate
   */
  double getSampleRate() const { return currentSampleRate.load(); }

  /**
   // Brief: Get current buffer size
   */
  int getBufferSize() const { return currentBufferSize.load(); }

  //==========================================================================
  // CPU Monitoring
  //==========================================================================

  /**
   // Brief: Get current CPU usage percentage
   * @return CPU usage (0.0 - 100.0)
   */
  double getCpuUsage() const;

  //==========================================================================
  // Macro Controls
  //==========================================================================

  MacroControl &getMacro(int index) { return macroBank_[index]; }
  const MacroControl &getMacro(int index) const { return macroBank_[index]; }

  //==========================================================================
  // Instrument Registry
  //==========================================================================

  /**
   // Brief: Get the instrument registry
   * @return Reference to the instrument registry
   */
  InstrumentRegistry &getInstrumentRegistry() { return *instrumentRegistry_; }
  const InstrumentRegistry &getInstrumentRegistry() const {
    return *instrumentRegistry_;
  }

  //==========================================================================
  // Session Debugger Agent (AI Technical Integrity)
  //==========================================================================

  /**
   // Brief: Get the session debugger agent
   * @return Pointer to the session debugger agent (may be null before
   * initialization)
   */
  ai::SessionDebuggerAgent *getSessionDebugger() {
    return sessionDebugger_.get();
  }
  ai::SessionDebuggerAgent *getSessionDebugger() const {
    return sessionDebugger_.get();
  }

  /**
   // Brief: Get the AI Mastering Agent
   */
  ai::AIMasteringAgent *getMasteringAgent() const;

  //==========================================================================
  // Analysis (Visualizers)
  //==========================================================================

  /**
   // Brief: Get the analysis FIFO for visualizers
   * @return Pointer to the stereo audio FIFO
   */
  zenith::StereoAudioFifo *getAnalysisFifo() const {
    return analysisFifo_.get();
  }

  //==========================================================================
  // Track Management
  //==========================================================================

  /**
   // Brief: Get number of tracks in engine
   * @return Track count
   // Note: Thread-safe; can be called from any thread
   */
  int getNumTracks() const noexcept;

  /**
   // Brief: Get const reference to tracks container
   * @return Const reference to tracks vector
   // Note: Use only from message thread; do NOT iterate from audio thread
   */
  const std::vector<std::shared_ptr<Track>> &tracks() const noexcept;

  /**
   // Brief: Get a thread-safe snapshot of tracks (copy of shared_ptrs)
   // Note: Safe to iterate on any thread while tracks are being added/removed
   */
  std::vector<std::shared_ptr<Track>> getTracksSnapshot() const {
      // Always use lock for thread safety - the RCU optimization for message thread
      // was causing a potential data race (accessing currentSnapshotHolder_ without lock)
      const juce::ScopedReadLock lock(tracksLock_);
      return tracks_;
  }

  /**
   // Brief: Debug helper to create test tracks (message thread only)
   * @param count Number of tracks to create
   // Note: Does NOT attach tracks to audio graph; for compile/UI testing only
   */
  void addTestTracks(int count);

  /**
   // Brief: Create a new track in both Engine and ProjectState
   * @param name Track name
   * @param type Track type ("audio" or "midi")
   * @return Track ID from ProjectState
   // Note: Message thread only; use this instead of addTestTracks for real
   * tracks
   */
  juce::String createTrack(const juce::String &name, const juce::String &type);

  /**
   // Brief: Add a pre-created track to the engine
   * @param track Shared pointer to track
   // Note: Message thread only; used by TrackStateSynchronizer
   */
  void addTrack(std::shared_ptr<Track> track);

  /**
   // Brief: Remove a track from the engine
   * @param index Index of track to remove
   // Note: Message thread only; used by TrackStateSynchronizer
   */
  void removeTrack(int index);

  /**
   // Brief: Get a track by its unique ID
   * @param trackId The unique track ID string
   * @return Pointer to the track, or nullptr if not found
   // Note: Message thread only
   */
  Track* getTrackById(const juce::String& trackId);

  //==========================================================================
  // Aux Bus Management (MESSAGE THREAD ONLY)
  //==========================================================================

  /**
   // Brief: Create a new auxiliary send/return bus
   * @param name Name for the aux bus (e.g., "Reverb", "Delay")
   * @return Index of the created aux bus
   // Note: Message thread only
   */
  int createAuxBus(const juce::String &name);

  /**
   // Brief: Remove an auxiliary bus
   * @param auxIndex Index of aux bus to remove
   // Note: Message thread only
   */
  void removeAuxBus(int auxIndex);

  /**
   // Brief: Get number of aux buses
   * @return Number of aux buses
   */
  int getNumAuxBuses() const noexcept;

  /**
   // Brief: Get aux bus by index
   * @param auxIndex Index of aux bus
   * @return Pointer to aux bus, or nullptr if invalid
   // Note: Message thread only
   */
  AuxBus *getAuxBus(int auxIndex) noexcept;

  /**
   // Brief: Get aux bus meters
   * @param auxIndex Index of aux bus
   * @return Current level (0.0 - 1.0+), or 0.0 if invalid
   */
  float getAuxBusLevel(int auxIndex) const;
  float getAuxBusPeakLevel(int auxIndex) const;

  //==========================================================================
  // Mixer Control (MESSAGE THREAD ONLY)
  //==========================================================================

  /**
   // Brief: Set track mixer controls (message thread only)
   // Note: These methods update the engine Track objects directly
   */
  void setTrackVolume(int trackIndex, float volume);
  void setTrackPan(int trackIndex, float pan);
  void setTrackMute(int trackIndex, bool muted);
  void setTrackSolo(int trackIndex, bool solo);
  void setTrackArmed(int trackIndex, bool armed);
  void setTrackInputChannel(int trackIndex,
                            int channelIndex); // Configures audio input routing

  //==========================================================================
  // Metering (MESSAGE THREAD SAFE)
  //==========================================================================

  /**
   // Brief: Get current level for a track
   * @param trackIndex Track index
   * @return Current level (0.0 - 1.0+), or 0.0 if invalid
   // Note: Safe to call from message thread (reads from atomic)
   */
  float getTrackLevel(int trackIndex) const;

  /**
   // Brief: Get peak level for a track
   * @param trackIndex Track index
   * @return Peak level (0.0 - 1.0+), or 0.0 if invalid
   // Note: Safe to call from message thread (reads from atomic)
   */
  float getTrackPeakLevel(int trackIndex) const;

  /**
   // Brief: Get current master output level
   * @return Master level (0.0 - 1.0+)
   // Note: Safe to call from message thread (reads from atomic)
   */
  float getMasterLevel() const;

  /**
   // Brief: Get peak master output level
   * @return Master peak level (0.0 - 1.0+)
   // Note: Safe to call from message thread (reads from atomic)
   */
  float getMasterPeakLevel() const;

  /**
   // Brief: Reset all peak meters
   // Note: Safe to call from message thread
   */
  void resetPeakMeters();

  //==========================================================================
  // Master Limiter (prevents clipping on master bus)
  //==========================================================================

  /**
   // Brief: Enable/disable the master limiter
   * @param enabled true to enable limiting, false to bypass
   */
  void setMasterLimiterEnabled(bool enabled);

  /**
   // Brief: Add a plugin to the master bus
   * @param plugin Shared pointer to the plugin instance
   */
  void addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance> plugin);

  /**
   // Brief: Remove a plugin from the master bus
   * @param index Index of the plugin to remove
   */
  void removeMasterPlugin(int index);

  /**
   // Brief: Check if master limiter is enabled
   */
  bool isMasterLimiterEnabled() const;

  /**
   // Brief: Set master limiter ceiling
   * @param ceilingDb Maximum output level in dB (typically -0.1 to -1.0)
   */
  void setMasterLimiterCeiling(float ceilingDb);

  /**
   // Brief: Get current master limiter gain reduction
   * @return Gain reduction in dB (0.0 = no reduction)
   */
  float getMasterLimiterGainReduction() const;

  /**
   // Brief: Get master limiter latency for PDC compensation
   * @return Latency in samples (includes lookahead and oversampling)
   */
  int getMasterLimiterLatency() const;

  /**
   // Brief: Get the mixer controller
   */
  MixerController& getMixerController();
  TrackFreezeManager& getTrackFreezeManager() { return *freezeManager_; }

  //==========================================================================
  // Track Freeze (CPU optimization)
  //==========================================================================

  /**
   // Brief: Freeze a track, rendering it to audio and disabling plugins
   * @param trackIndex Index of track to freeze
   * @param progress Optional progress callback
   * @return true if freeze started successfully
   // Note: MESSAGE THREAD ONLY - rendering is async
   */
  bool freezeTrack(
      int trackIndex,
      std::function<void(float, const juce::String &)> progress = nullptr);

  /**
   // Brief: Unfreeze a track, restoring original plugins
   * @param trackIndex Index of track to unfreeze
   * @return true if unfreeze succeeded
   // Note: MESSAGE THREAD ONLY
   */
  bool unfreezeTrack(int trackIndex);

  /**
   // Brief: Check if a track is frozen
   * @param trackIndex Index of track to check
   * @return true if track is frozen
   // Note: Thread-safe
   */
  bool isTrackFrozen(int trackIndex) const;

  /**
   // Brief: Cancel any active freeze operation
   // Note: MESSAGE THREAD ONLY
   */
  void cancelFreeze();

  //==========================================================================
  // Audio File Pool
  //==========================================================================

  /**
   // Brief: Get the audio file pool for loading/caching audio files
   * @return Reference to the audio file pool
   // Note: Thread-safe; pool handles internal locking
   */
  AudioFilePool &getAudioFilePool();

  /**
   // Brief: Get the tempo map for beat/time conversions
   * @return Reference to TempoMap
   // Note: Thread-safe; uses lock-free snapshot mechanism
   */
  const TempoMap &getTempoMap() const noexcept;

  //==========================================================================
  // Plugin Hosting
  //==========================================================================

  /**
   // Brief: Get the plugin host manager
   * @return Reference to PluginHost
   // Note: Use only from message thread
   */
  PluginHost &getPluginHost() noexcept;

  /**
   // Brief: Scan for plugins in default locations
   * @return Number of plugins found
   // Note: MESSAGE THREAD ONLY - blocking operation
   */
  int scanForPlugins();

  /**
   // Brief: Get the plugin editor window manager
   * @return Reference to PluginEditorWindowManager
   // Note: Use only from message thread
   */
  PluginEditorWindowManager &getPluginEditorWindowManager() noexcept;

  // getInstrumentRegistry() is defined inline earlier in the file

  //==========================================================================
  // AudioIODeviceCallback interface (AUDIO THREAD)
  //==========================================================================

  /**
   // Brief: Called when audio device is about to start
   // Note: Runs on AUDIO THREAD - must be real-time safe!
   */
  void audioDeviceAboutToStart(juce::AudioIODevice *device) override;

  /**
   // Brief: Called when audio device has stopped
   // Note: Runs on MESSAGE THREAD
   */
  void audioDeviceStopped() override;

  /**
   // Brief: Main audio processing callback (JUCE 8 version with context)
   *
   * ⚠️ CRITICAL: This runs on the AUDIO THREAD!
   *
   * NEVER do these things here:
   * - Allocate memory (malloc, new, std::vector::push_back)
   * - Lock mutexes (std::mutex, std::lock_guard)
   * - Make system calls (file I/O, logging, network)
   * - Call UI methods
   *
   * ONLY do these things:
   * - Process audio samples
   * - Read std::atomic values
   * - Use lock-free data structures
   * - Use pre-allocated buffers
   *
   * @param inputChannelData Input audio samples
   * @param numInputChannels Number of input channels
   * @param outputChannelData Output audio samples (WRITE HERE)
   * @param numOutputChannels Number of output channels
   * @param numSamples Number of samples per channel
   * @param context Callback context with timing info
   */
  void audioDeviceIOCallbackWithContext(
      const float *const *inputChannelData, int numInputChannels,
      float *const *outputChannelData, int numOutputChannels, int numSamples,
      const juce::AudioIODeviceCallbackContext &context) noexcept override;

  //==========================================================================
  // MidiInputCallback interface
  //==========================================================================

  /**
   // Brief: Handle incoming MIDI messages from input devices
   // Note: Runs on MIDI input thread, routes to armed tracks
   */
  void handleIncomingMidiMessage(juce::MidiInput *source,
                                 const juce::MidiMessage &message) override;

  //==========================================================================
  // ChangeListener interface
  //==========================================================================

  /**
   // Brief: Handle callbacks from Track changes (e.g. plugin latency change)
   */
  void changeListenerCallback(juce::ChangeBroadcaster* source) override;

  //==========================================================================
  // Project Export
  //==========================================================================
  
  friend class AudioExporter;
  friend class AudioRecorder;

  /**
   // Brief: Render a specific block of audio for offline export
   * @param buffer Buffer to fill (must be sized correctly)
   * @param numSamples Number of samples to render
   * @param position Sample position in the project
   // Note: Message thread only
   */
  void renderOfflineBlock(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 position);

  /**
   // Brief: Export project to WAV file
   * @param outputFile Output file path
   * @param sampleRate Sample rate for export
   * @param bitDepth Bit depth (16, 24, or 32)
   * @param durationInSeconds Duration to export
   * @return true if successful
   */
  bool exportProjectToWav(const juce::File &outputFile, double sampleRate,
                          int bitDepth, double durationInSeconds);

  /**
   // Brief: Synchronous version of WAV export with start time support
   // Note: Thread-safe (can be called from background threads)
   */
  bool exportProjectToWavSync(const juce::File &outputFile, double sampleRate,
                              int bitDepth, double duration, double startTime = 0.0);

  enum class ExportFormat { WAV, FLAC, OGG, AIFF };

  /// Progress callback type for export operations
  using ExportProgressCallback = std::function<void(float progress, const juce::String& status)>;

  struct ExportOptions {
    juce::File outputFile;
    double sampleRate = 44100.0;
    int bitDepth = 24; // 8, 16, 24, 32
    ExportFormat format = ExportFormat::WAV;
    bool enableDither = true;
    bool normalize = false;
    double normalizeDb = -0.1;
    double duration = 0.0;
    
    // Stem export options
    bool exportStems = false;
    std::vector<int> stemTrackIndices; // Empty = all tracks
    
    // Progress callback (optional)
    ExportProgressCallback progressCallback = nullptr;
  };

  /**
   // Brief: Advanced Project Export
   * Supports WAV/FLAC/OGG/AIFF, Dithering, Normalization, and 8-bit.
   */
  bool exportProject(const ExportOptions &options);



  //==========================================================================
  // Metronome
  //==========================================================================

  void toggleMetronome();
  bool isMetronomeEnabled() const;
  void setMetronomeLevel(float level);

  //==========================================================================
  // Application Thread Pool (Background Tasks)
  //==========================================================================

  /**
   // Brief: Get the shared thread pool for background tasks
   */
  juce::ThreadPool &getThreadPool();

  Midi2DiscoveryService* getMidi2DiscoveryService() const { return midi2DiscoveryService_.get(); }

  //==========================================================================
  // WCET Monitoring (Performance Analysis)
  //==========================================================================
  
  /**
   // Brief: Get the WCET monitor for performance analysis
   * @return Reference to the WCET monitor
   */
  profiling::WCETMonitor& getWCETMonitor() { return wcetMonitor_; }
  const profiling::WCETMonitor& getWCETMonitor() const { return wcetMonitor_; }

private:
  //==========================================================================
  // Audio Processing (AUDIO THREAD)
  //==========================================================================

  /**
   // Brief: Process audio when playing
   // Note: AUDIO THREAD - real-time safe!
   */
  void processAudioBlock(const float *const *inputChannelData,
                         int numInputChannels, float *const *outputChannelData,
                         int numOutputChannels, int numSamples) noexcept;

  /**
   // Brief: Process pending events
   // Note: AUDIO THREAD - Lock-free
   */
  void processEvents() noexcept;

  // Track management (message thread only)
  void prepareTracks(int samplesPerBlockExpected, double sampleRate);

  // MIDI input management (message thread only)
  void enableMidiInput();
  void disableMidiInput();

  //==========================================================================
  // Offline Rendering Helpers (MESSAGE THREAD)
  //==========================================================================

  /**
   // Brief: Prepare per-track buffers for offline rendering
   * @param blockSize Block size for offline rendering (e.g., 4096 samples)
   * @param numChannels Number of channels per track
   // Note: Must be called before renderBlock() during offline export
   */
  void prepareBuffersForOfflineRender(int blockSize, int numChannels);

  /**
   // Brief: Auto-detect project duration based on clips
   * @return Duration in seconds
   */
  double autoDetectProjectDuration() const;

  //==========================================================================
  // Plugin Management
  //==========================================================================

  /**
   // Brief: Render a block of audio into the output buffer
   * @param outputBuffer Buffer to render into
   * @param numSamples Number of samples to render
   * @param playheadPosition Current playhead position in samples
   // Note: MESSAGE THREAD - used for offline rendering only
   */
  void renderAudioGraph(juce::AudioBuffer<float> &outputBuffer, int numSamples,
                        juce::int64 playheadPosition,
                        const std::vector<zenith::Track *> &tracks,
                        const std::vector<zenith::AuxBus *> &auxBuses,
                        const juce::MidiBuffer *incomingMidi = nullptr);

  juce::AudioFormatManager formatManager;
  zenith::dsp::Dither dither;

  void registerFormats();

  // Helper to apply normalization gain to a buffer
  void applyNormalization(juce::AudioBuffer<float> &buffer, float maxPeak,
                          float targetDb);

  //==========================================================================
  // Member Variables
  //==========================================================================

  // Audio device manager
  juce::AudioDeviceManager deviceManager;

  // Audio settings (std::atomic for thread-safe access)
  std::atomic<double> currentSampleRate{44100.0};
  std::atomic<int> currentBufferSize{512};

  // CPU usage tracking
  mutable std::atomic<double> cpuUsage_{0.0};
  juce::int64 lastCpuCheckTime{0};

  // Test tone generator
  double phase{0.0};
  std::atomic<bool> enableTestTone_{false};

  // Track container (message thread for modification)
  // Use shared_ptr instead of unique_ptr to enable RT-safe snapshot sharing
  mutable juce::ReadWriteLock tracksLock_;
  std::vector<std::shared_ptr<zenith::Track>> tracks_;

  // Routing Graph (Source of Truth for connections and processing order)
  RoutingGraph routingGraph_;

public:
  RoutingGraph &getRoutingGraph() { return routingGraph_; }
  const RoutingGraph &getRoutingGraph() const { return routingGraph_; }

private:
  // Thread-safe Track Snapshot (RCU-style)
  // Audio thread reads this snapshot without locking (wait-free iteration)
  // Use raw pointers for iteration speed, shared_ptr for lifetime management
  struct TrackSnapshot {
    std::vector<zenith::Track *>
        tracks; // Raw pointers for fast, lock-free iteration
    std::vector<zenith::AuxBus *> auxBuses; // Raw pointers for buses

    std::vector<std::shared_ptr<zenith::Track>> lifecycle; // Keeps tracks alive
    std::vector<std::shared_ptr<zenith::AuxBus>>
        lifecycleAux; // Keeps buses alive

    // Fast lookup maps (ID -> Pointer)
    // Audio thread usage: Read-only access to find tracks by ID from
    // RoutingGraph
    std::unordered_map<std::string, zenith::Track *> trackMap;
    std::unordered_map<std::string, zenith::AuxBus *> auxBusMap;

    TrackSnapshot() = default;

    // Constructor defined in .cpp to avoid circular includes
    TrackSnapshot(
        const std::vector<std::shared_ptr<zenith::Track>> &ownedTracks,
        const std::vector<std::shared_ptr<zenith::AuxBus>> &ownedBuses);
  };

  // Lock-free snapshot mechanism
  // Audio thread reads activeSnapshot_ (atomic raw pointer)
  // Main thread manages lifetime via currentSnapshotHolder_ and snapshotTrash_
  std::atomic<TrackSnapshot *> activeSnapshot_{nullptr};
  std::shared_ptr<TrackSnapshot> currentSnapshotHolder_;

  void updateTrackSnapshot();

  // RT-safe event applicator to deduplicate processEvents logic
  void applyEvent(const zenith::EngineEvent &e,
                  TrackSnapshot *snapshot) noexcept;

  // Master Plugin RCU
  struct MasterPluginSnapshot {
    std::vector<std::shared_ptr<juce::AudioPluginInstance>> plugins;
  };

  std::atomic<MasterPluginSnapshot *> activeMasterPluginsSnapshot_{nullptr};
  std::shared_ptr<MasterPluginSnapshot> currentMasterPluginsSnapshotHolder_;

  void updateMasterPluginSnapshot();

  // Phase 1.2: Audio file pool
  std::unique_ptr<zenith::AudioFilePool> audioFilePool_;

  // Phase 3: Plugin hosting
  std::unique_ptr<zenith::PluginHost> pluginHost_;
  std::unique_ptr<zenith::PluginEditorWindowManager> pluginEditorWindowManager_;

  // Instrument Registry
  std::unique_ptr<zenith::InstrumentRegistry> instrumentRegistry_;

  // Session Debugger Agent
  std::unique_ptr<ai::SessionDebuggerAgent> sessionDebugger_;
  std::unique_ptr<ai::AIMasteringAgent> masteringAgent_;
  std::unique_ptr<Metronome> metronome_;
  std::unique_ptr<Midi2DiscoveryService> midi2DiscoveryService_;

  // Analysis FIFO (Stereo)
  std::unique_ptr<zenith::StereoAudioFifo> analysisFifo_;

  // Project state reference
  ProjectState *projectState_ = nullptr;

  // Session clip tracking (for session view clip launching)
  // Maps track index -> active clip ID for session playback
  std::unordered_map<int, juce::String> activeSessionClips_;
  juce::CriticalSection sessionClipsLock_;

  // Automation synchronizer
  std::unique_ptr<TrackAutomationSynchronizer> automationSynchronizer;

  // Thread Pool (Shared)
  juce::ThreadPool threadPool{
      1}; // Start with 1 thread to be safe, or default constructor

  //==========================================================================
  // Modular Engine Components (Refactor 2025-12-09)
  //==========================================================================

  std::unique_ptr<AudioRenderer> audioRenderer_;
  std::unique_ptr<AudioExporter> audioExporter_;
  // Context for live playback
  AudioRenderContext renderContext_;
  std::atomic<bool> isSuspended_{false}; // Suspend flag
  std::unique_ptr<RecordingManager> recordingManager_;
  std::unique_ptr<TransportController> transportController_;
  std::unique_ptr<MeteringSystem> meteringSystem_;
  std::unique_ptr<MixerController> mixerController_;
  std::unique_ptr<zenith::TempoMap>
      tempoMap_; // Kept for now, shared with controllers

  // Aux buses (Managed by Engine, rendered by AudioRenderer)
  std::vector<std::shared_ptr<zenith::AuxBus>> auxBuses_;

  // Master bus plugins (Managed by Engine, rendered by AudioRenderer)
  std::vector<std::shared_ptr<juce::AudioPluginInstance>> masterPlugins_;
  juce::CriticalSection masterPluginLock_;

  // Master Limiter (Used by AudioRenderer)
  MasterLimiter masterLimiter_;

  // Track Freeze Manager
  std::unique_ptr<TrackFreezeManager> freezeManager_;

  // MIDI input handling
  std::vector<std::unique_ptr<juce::MidiInput>> midiInputs_;
  zenith::MidiFifo midiFifo_; // Lock-free MIDI FIFO for input routing
  juce::MidiBuffer liveMidiPass1_;
  juce::MidiBuffer liveMidiPass2_;
  juce::MidiBuffer liveMidiScratch_; // Avoid per-block temporaries in callback
  double lastLiveMidiCallbackTimeSeconds_ = 0.0;

  // Lock-free Command Queue
  static constexpr int kCommandBufferSize = 1024;
  juce::AbstractFifo commandFifo_{kCommandBufferSize};
  std::vector<zenith::EngineEvent> commandBuffer_{kCommandBufferSize};

  // Helper to update SIP (Solo In Place) logic
  void updateSoloState();

  // ID Counter for Aux Busses
  std::atomic<int> auxBusIdCounter{0};

  // ID Counter for Tracks
  std::atomic<uint64_t> nextTrackId_{0};

  // Flag to prevent use-after-free in async callbacks
  std::atomic<bool> isShuttingDown_{false};

  // Macro Bank
  MacroBank macroBank_;
  
  // WCET Monitor (Performance analysis)
  profiling::WCETMonitor wcetMonitor_;

  juce::WeakReference<Engine>::Master masterReference;
  friend class juce::WeakReference<Engine>;

  // Power Management
  PowerManagement powerManagement_;
  void updatePowerManagement();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
} // namespace zenith
