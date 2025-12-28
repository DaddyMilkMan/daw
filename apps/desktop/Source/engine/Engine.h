
/**
 * @file Engine.h
 * @brief Core audio engine for Zenith DAW
 *
 * This is the authoritative Engine for Zenith.
 *
 * Features:
 * - RT-safe track mixdown with unified render path
 * - Lock-free clip snapshots
 * - Atomic playhead tracking with loop support
 * - AudioFilePool integration for audio file caching
 * - MIDI input routing and recording
 * - Audio input recording
 * - Pre-allocated buffers (trackBuffers_, clipBuffer_)
 *
 * Manages:
 * - Audio device I/O
 * - Audio processing callback
 * - Transport state (play/stop/record)
 * - MIDI input routing
 * - Audio input recording
 * - CPU usage monitoring
 * - Sample rate and buffer size
 *
 * Thread Safety:
 * - audioDeviceIOCallback() runs on AUDIO THREAD (real-time safe!)
 * - All other methods run on MESSAGE THREAD
 * - Use std::atomic for cross-thread communication
 */

#pragma once

#include <atomic>
#include <functional>
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

#include "../Source/dsp/Dither.h"
#include "../Source/dsp/MasterLimiter.h"
#include "../Source/dsp/StereoAudioFifo.h"
#include "../Source/engine/EngineConstants.h"
#include "../Source/engine/MacroControl.h"
#include "../Source/engine/RoutingGraph.h"
#include "AudioRenderer.h"
#include "EngineEvent.h"
#include "ExportCommon.h"
#include "../audio/RealTimeAudioBuffer.h"

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
class RecordingManager;
class TransportController;
class MeteringSystem;
class MixerController;
class Midi2DiscoveryService;
class PropertyExchangeManager;
class AudioAnalysisService;

namespace ai {
class SessionDebuggerAgent;
class AIMasteringAgent;
} // namespace ai


//==============================================================================
/**
 * @class Engine
 * @brief Core audio engine
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
 * @note To avoid memory leaks: NEVER store std::shared_ptr<Engine> in child components.
 *       Use Engine& or Engine* for back-references.
 */
class ExportJob;

class Engine : public juce::AudioIODeviceCallback,
               public juce::MidiInputCallback,
               public juce::ChangeListener {
public:
  //==========================================================================
  Engine();
  ~Engine() override;

  /**
   * @brief Get the plugin format manager
   */
  juce::AudioPluginFormatManager &getPluginFormatManager();

  /**
   * @brief Get the audio device manager
   */
  juce::AudioDeviceManager &getDeviceManager() { return deviceManager; }

  // New accessors
  MixerController& getMixerController();
  Midi2DiscoveryService* getMidi2DiscoveryService() const { return midi2DiscoveryService_.get(); }

  //==========================================================================
  // Global Access (Safety for Async Callbacks)
  //==========================================================================

  /**
   * @brief Get the global Engine instance (if valid)
   * @return Pointer to the engine, or nullptr if shutting down/not created
   * @note Use this in loose async callbacks to avoid dangling references
   */
  static Engine* getInstance();

  /**
   * @brief Cancel current offline export
   */
  void cancelExport();


  //==========================================================================
  // Initialization / Shutdown
  //==========================================================================

  /**
   * @brief Set project state for automation synchronization and tempo
   * @param state Pointer to project state (can be nullptr to disable
   * automation)
   * @note Must be called before initialize() or after automation is stopped
   */
  void setProjectState(ProjectState *state);

  /**
   * @brief Get the associated ProjectState
   * @return Pointer to the ProjectState (may be null)
   */
  ProjectState *getProjectState() { return projectState_; }

  /**
   * @brief Synchronize engine tracks with project state
   * @note Message thread only - rebuilds track list from ProjectState
   */
  void syncWithProjectState();

  /**
   * @brief Synchronize tempo map with project state
   * @note Message thread only
   */
  void syncTempoMap();

  /**
   * @brief Initialize the audio engine
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
   * @brief Shutdown the audio engine
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
   * @brief Start playback
   */
  void play();

  /**
   * @brief Stop playback
   */
  void stop();

  /**
   * @brief Check if playing
   */
  bool isPlaying() const;

  /**
   * @brief Start recording
   * @note MESSAGE THREAD ONLY - Starts recording on armed tracks (both MIDI and
   * audio)
   */
  void record();

  /**
   * @brief Stop recording and bake clips
   * @note MESSAGE THREAD ONLY - Converts recordings to clips (both MIDI and
   * audio)
   */
  void stopRecording();

  /**
   * @brief Check if recording
   */
  bool isRecording() const;

  /**
   * @brief Toggle recording on/off
   * @note Convenience method for record button
   */
  /**
   * @brief Toggle recording on/off
   * @note Convenience method for record button
   */
  void toggleRecording();

  /**
   * @brief Panic - Stop all sound immediately
   * @note Stops transport, sends All Notes Off to all tracks, and clears
   * buffers.
   */
  void panic();

  /**
   * @brief Suspend audio processing (e.g. for offline export)
   * @param shouldSuspend True to silence audio output/input
   * @note Real-time safe (sets atomic flag)
   */
  void suspendProcessing(bool shouldSuspend) { isSuspended_.store(shouldSuspend); }

  /**
   * @brief Check if processing is suspended
   */
  bool isSuspended() const { return isSuspended_.load(); }

  /**
   * @brief Set sidechain source for a specific plugin on a track
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
   * @brief Queue an event for the audio thread
   * @param e Event to queue
   * @return true if queued successfully, false if full
   * @note Lock-free, safe to call from any thread
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
   * @brief Get current playback position in samples
   */
  juce::int64 getPlayheadSamples() const;

  /**
   * @brief Get current playback position in samples (legacy accessor)
   */
  juce::int64 getPlaybackPosition() const;

  /**
   * @brief Get current playback position in beats
   */
  double getPlaybackPositionBeats() const;

  /**
   * @brief Set playback position (MESSAGE THREAD ONLY)
   */
  void setPlayheadSamples(juce::int64 position);

  /**
   * @brief Enable/disable looping
   */
  void setLooping(bool shouldLoop);

  /**
   * @brief Check if looping is enabled
   */
  bool isLooping() const;

  /**
   * @brief Set loop region in samples (MESSAGE THREAD ONLY)
   */
  void setLoopRegion(juce::int64 start, juce::int64 end);

  /**
   * @brief Get loop start position in samples
   */
  juce::int64 getLoopStart() const;

  /**
   * @brief Get loop end position in samples
   */
  juce::int64 getLoopEnd() const;

  //==========================================================================
  // Plugin Delay Compensation (PDC)
  //==========================================================================

  /**
   * @brief Get total plugin latency for a track in samples
   * @param trackIndex Track index
   * @return Total latency from all plugins in the track's chain
   */
  int getTrackLatency(int trackIndex) const;

  /**
   * @brief Get master bus total latency in samples
   * @return Total latency from all master bus plugins
   */
  int getMasterLatency() const;

  /**
   * @brief Recalculate PDC for all tracks
   * @note Call after adding/removing plugins or changing plugin latency
   */
  void recalculatePDC();

  /**
   * @brief Check if PDC is enabled
   */
  bool isPDCEnabled() const;

  /**
   * @brief Enable/disable PDC
   */
  void setPDCEnabled(bool enabled);

  /**
   * @brief Get maximum track latency (for PDC compensation)
   */
  int getMaxTrackLatency() const;

  //==========================================================================
  // Audio Device Management
  //==========================================================================

  /**
   * @brief Get current audio device info
   * @return String describing current device and settings
   */
  juce::String getAudioDeviceInfo() const;

  /**
   * @brief Get current sample rate
   */
  double getSampleRate() const { return currentSampleRate.load(); }

  /**
   * @brief Get current buffer size
   */
  int getBufferSize() const { return currentBufferSize.load(); }

  //==========================================================================
  // CPU Monitoring
  //==========================================================================

  /**
   * @brief Get current CPU usage percentage
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
   * @brief Get the instrument registry
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
   * @brief Get the session debugger agent
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
   * @brief Get the AI Mastering Agent
   */
  ai::AIMasteringAgent *getMasteringAgent() const;

  //==========================================================================
  // Analysis (Visualizers)
  //==========================================================================

  /**
   * @brief Get the analysis FIFO for visualizers
   * @return Pointer to the stereo audio FIFO
   */
  zenith::StereoAudioFifo *getAnalysisFifo() const {
    return analysisFifo_.get();
  }

  //==========================================================================
  // Track Management
  //==========================================================================

  /**
   * @brief Get number of tracks in engine
   * @return Track count
   * @note Thread-safe; can be called from any thread
   */
  int getNumTracks() const noexcept;

  /**
   * @brief Get const reference to tracks container
   * @return Const reference to tracks vector
   * @note Use only from message thread; do NOT iterate from audio thread
   */
  const std::vector<std::shared_ptr<Track>> &tracks() const noexcept;

  /**
   * @brief Get a thread-safe snapshot of tracks (copy of shared_ptrs)
   * @note Safe to iterate on any thread while tracks are being added/removed
   */
  std::vector<std::shared_ptr<Track>> getTracksSnapshot() const {
      const juce::ScopedReadLock lock(tracksLock_);
      return tracks_; // Implicit copy of shared_ptrs
  }

  /**
   * @brief Debug helper to create test tracks (message thread only)
   * @param count Number of tracks to create
   * @note Does NOT attach tracks to audio graph; for compile/UI testing only
   */
  void addTestTracks(int count);

  /**
   * @brief Create a new track in both Engine and ProjectState
   * @param name Track name
   * @param type Track type ("audio" or "midi")
   * @return Track ID from ProjectState
   * @note Message thread only; use this instead of addTestTracks for real
   * tracks
   */
  juce::String createTrack(const juce::String &name, const juce::String &type);

  /**
   * @brief Add a pre-created track to the engine
   * @param track Shared pointer to track
   * @note Message thread only; used by TrackStateSynchronizer
   */
  void addTrack(std::shared_ptr<Track> track);

  /**
   * @brief Remove a track from the engine
   * @param index Index of track to remove
   * @note Message thread only; used by TrackStateSynchronizer
   */
  void removeTrack(int index);

  /**
   * @brief Get a track by its unique ID
   * @param trackId The unique track ID string
   * @return Pointer to the track, or nullptr if not found
   * @note Message thread only
   */
  Track* getTrackById(const juce::String& trackId);

  //==========================================================================
  // Aux Bus Management (MESSAGE THREAD ONLY)
  //==========================================================================

  /**
   * @brief Create a new auxiliary send/return bus
   * @param name Name for the aux bus (e.g., "Reverb", "Delay")
   * @return Index of the created aux bus
   * @note Message thread only
   */
  int createAuxBus(const juce::String &name);

  /**
   * @brief Remove an auxiliary bus
   * @param auxIndex Index of aux bus to remove
   * @note Message thread only
   */
  void removeAuxBus(int auxIndex);

  /**
   * @brief Get number of aux buses
   * @return Number of aux buses
   */
  int getNumAuxBuses() const noexcept;

  /**
   * @brief Get aux bus by index
   * @param auxIndex Index of aux bus
   * @return Pointer to aux bus, or nullptr if invalid
   * @note Message thread only
   */
  AuxBus *getAuxBus(int auxIndex) noexcept;

  /**
   * @brief Get aux bus meters
   * @param auxIndex Index of aux bus
   * @return Current level (0.0 - 1.0+), or 0.0 if invalid
   */
  float getAuxBusLevel(int auxIndex) const;
  float getAuxBusPeakLevel(int auxIndex) const;

  //==========================================================================
  // Mixer Control (MESSAGE THREAD ONLY)
  //==========================================================================

  /**
   * @brief Set track mixer controls (message thread only)
   * @note These methods update the engine Track objects directly
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
   * @brief Get current level for a track
   * @param trackIndex Track index
   * @return Current level (0.0 - 1.0+), or 0.0 if invalid
   * @note Safe to call from message thread (reads from atomic)
   */
  float getTrackLevel(int trackIndex) const;

  /**
   * @brief Get peak level for a track
   * @param trackIndex Track index
   * @return Peak level (0.0 - 1.0+), or 0.0 if invalid
   * @note Safe to call from message thread (reads from atomic)
   */
  float getTrackPeakLevel(int trackIndex) const;

  /**
   * @brief Get current master output level
   * @return Master level (0.0 - 1.0+)
   * @note Safe to call from message thread (reads from atomic)
   */
  float getMasterLevel() const;

  /**
   * @brief Get peak master output level
   * @return Master peak level (0.0 - 1.0+)
   * @note Safe to call from message thread (reads from atomic)
   */
  float getMasterPeakLevel() const;

  /**
   * @brief Reset all peak meters
   * @note Safe to call from message thread
   */
  void resetPeakMeters();

  //==========================================================================
  // Master Limiter (prevents clipping on master bus)
  //==========================================================================

  /**
   * @brief Enable/disable the master limiter
   * @param enabled true to enable limiting, false to bypass
   */
  void setMasterLimiterEnabled(bool enabled);

  /**
   * @brief Add a plugin to the master bus
   * @param plugin Shared pointer to the plugin instance
   */
  void addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance> plugin);

  /**
   * @brief Remove a plugin from the master bus
   * @param index Index of the plugin to remove
   */
  void removeMasterPlugin(int index);

  /**
   * @brief Check if master limiter is enabled
   */
  bool isMasterLimiterEnabled() const;

  /**
   * @brief Set master limiter ceiling
   * @param ceilingDb Maximum output level in dB (typically -0.1 to -1.0)
   */
  void setMasterLimiterCeiling(float ceilingDb);

  /**
   * @brief Get current master limiter gain reduction
   * @return Gain reduction in dB (0.0 = no reduction)
   */
  float getMasterLimiterGainReduction() const;

  /**
   * @brief Get master limiter latency for PDC compensation
   * @return Latency in samples (includes lookahead and oversampling)
   */
  int getMasterLimiterLatency() const;

  /**
   * @brief Get the mixer controller
   */
  MixerController& getMixerController();
  TrackFreezeManager& getTrackFreezeManager() { return *freezeManager_; }

  //==========================================================================
  // Track Freeze (CPU optimization)
  //==========================================================================

  /**
   * @brief Freeze a track, rendering it to audio and disabling plugins
   * @param trackIndex Index of track to freeze
   * @param progress Optional progress callback
   * @return true if freeze started successfully
   * @note MESSAGE THREAD ONLY - rendering is async
   */
  bool freezeTrack(
      int trackIndex,
      std::function<void(float, const juce::String &)> progress = nullptr);

  /**
   * @brief Unfreeze a track, restoring original plugins
   * @param trackIndex Index of track to unfreeze
   * @return true if unfreeze succeeded
   * @note MESSAGE THREAD ONLY
   */
  bool unfreezeTrack(int trackIndex);

  /**
   * @brief Check if a track is frozen
   * @param trackIndex Index of track to check
   * @return true if track is frozen
   * @note Thread-safe
   */
  bool isTrackFrozen(int trackIndex) const;

  /**
   * @brief Cancel any active freeze operation
   * @note MESSAGE THREAD ONLY
   */
  void cancelFreeze();

  //==========================================================================
  // Audio File Pool
  //==========================================================================

  /**
   * @brief Get the audio file pool for loading/caching audio files
   * @return Reference to the audio file pool
   * @note Thread-safe; pool handles internal locking
   */
  AudioFilePool &getAudioFilePool();

  /**
   * @brief Get the tempo map for beat/time conversions
   * @return Reference to TempoMap
   * @note Thread-safe; uses lock-free snapshot mechanism
   */
  const TempoMap &getTempoMap() const noexcept;

  //==========================================================================
  // Plugin Hosting
  //==========================================================================

  /**
   * @brief Get the plugin host manager
   * @return Reference to PluginHost
   * @note Use only from message thread
   */
  PluginHost &getPluginHost() noexcept;

  /**
   * @brief Scan for plugins in default locations
   * @return Number of plugins found
   * @note MESSAGE THREAD ONLY - blocking operation
   */
  int scanForPlugins();

  /**
   * @brief Get the plugin editor window manager
   * @return Reference to PluginEditorWindowManager
   * @note Use only from message thread
   */
  PluginEditorWindowManager &getPluginEditorWindowManager() noexcept;

  // getInstrumentRegistry() is defined inline earlier in the file

  //==========================================================================
  // AudioIODeviceCallback interface (AUDIO THREAD)
  //==========================================================================

  /**
   * @brief Called when audio device is about to start
   * @note Runs on AUDIO THREAD - must be real-time safe!
   */
  void audioDeviceAboutToStart(juce::AudioIODevice *device) override;

  /**
   * @brief Called when audio device has stopped
   * @note Runs on MESSAGE THREAD
   */
  void audioDeviceStopped() override;

  /**
   * @brief Main audio processing callback (JUCE 8 version with context)
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
   * @brief Handle incoming MIDI messages from input devices
   * @note Runs on MIDI input thread, routes to armed tracks
   */
  void handleIncomingMidiMessage(juce::MidiInput *source,
                                 const juce::MidiMessage &message) override;

  //==========================================================================
  // ChangeListener interface
  //==========================================================================

  /**
   * @brief Handle callbacks from Track changes (e.g. plugin latency change)
   */
  void changeListenerCallback(juce::ChangeBroadcaster* source) override;

  //==========================================================================
  // Project Export
  //==========================================================================
  
  friend class AudioExporter;
  friend class AudioRecorder;
  friend class ExportJob;


  /**
   * @brief Render a specific block of audio for offline export
   * @param buffer Buffer to fill (must be sized correctly)
   * @param numSamples Number of samples to render
   * @param position Sample position in the project
   * @note Message thread only
   */
  void renderOfflineBlock(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 position);

  using ExportFormat = zenith::ExportFormat;

  /// Progress callback type for export operations
  using ExportProgressCallback = zenith::ExportProgressCallback;

  /**
   * @brief Export project to WAV file
   * @param outputFile Output file path
   * @param sampleRate Sample rate for export
   * @param bitDepth Bit depth (16, 24, or 32)
   * @param durationInSeconds Duration to export (0 = auto-detect)
   * @param startTimeSeconds Start time in seconds (default 0.0)
   * @param progressCallback Optional callback for progress updates
   * @return true if successful
   */
  bool exportProjectToWav(const juce::File &outputFile, double sampleRate,
                          int bitDepth, double durationInSeconds = 0.0,
                          double startTimeSeconds = 0.0,
                          ExportProgressCallback progressCallback = nullptr);

  /**
   * @brief Synchronous version of project export for background threads (AI)
   */
  bool exportProjectToWavSync(const juce::File &outputFile, double sampleRate,
                              int bitDepth, double durationInSeconds = 0.0,
                              double startTimeSeconds = 0.0);

using ExportOptions = zenith::ExportOptions;

  /**
   * @brief Advanced Project Export
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
   * @brief Get the shared thread pool for background tasks
   */
  juce::ThreadPool &getThreadPool();

  Midi2DiscoveryService* getMidi2DiscoveryService() const { return midi2DiscoveryService_.get(); }
  AudioAnalysisService* getAnalysisService() const { return analysisService_.get(); }


private:
  //==========================================================================
  // Audio Processing (AUDIO THREAD)
  //==========================================================================

  /**
   * @brief Process audio when playing
   * @note AUDIO THREAD - real-time safe!
   */
  void processAudioBlock(const float *const *inputChannelData,
                         int numInputChannels, float *const *outputChannelData,
                         int numOutputChannels, int numSamples) noexcept;

  /**
   * @brief Process pending events
   * @note AUDIO THREAD - Lock-free
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
   * @brief Prepare per-track buffers for offline rendering
   * @param blockSize Block size for offline rendering (e.g., 4096 samples)
   * @param numChannels Number of channels per track
   * @note Must be called before renderBlock() during offline export
   */
  void prepareBuffersForOfflineRender(int blockSize, int numChannels);

  /**
   * @brief Auto-detect project duration based on clips
   * @return Duration in seconds
   */
  double autoDetectProjectDuration() const;

  //==========================================================================
  // Plugin Management
  //==========================================================================

  /**
   * @brief Render a block of audio into the output buffer
   * @param outputBuffer Buffer to render into
   * @param numSamples Number of samples to render
   * @param playheadPosition Current playhead position in samples
   * @note MESSAGE THREAD - used for offline rendering only
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
  std::unique_ptr<AudioAnalysisService> analysisService_;
  std::unique_ptr<Metronome> metronome_;
  std::unique_ptr<Midi2DiscoveryService> midi2DiscoveryService_;

  // Analysis FIFO (Stereo)
  std::unique_ptr<zenith::StereoAudioFifo> analysisFifo_;

  // Project state reference
  ProjectState *projectState_ = nullptr;

  // Automation synchronizer
  std::unique_ptr<TrackAutomationSynchronizer> automationSynchronizer;

  // Thread Pool (Shared)
  juce::ThreadPool threadPool{
      1}; // Start with 1 thread to be safe, or default constructor

  //==========================================================================
  // Modular Engine Components (Refactor 2025-12-09)
  //==========================================================================

  std::unique_ptr<AudioRenderer> audioRenderer_;
  // Render Contexts (State for AudioRenderer)
  AudioRenderContext liveContext_;      // For real-time playback
  AudioRenderContext renderContext_;    // For offline export/rendering
  std::atomic<bool> isSuspended_{false}; // Suspend flag
  std::unique_ptr<RecordingManager> recordingManager_;
  std::unique_ptr<TransportController> transportController_;
  std::unique_ptr<MeteringSystem> meteringSystem_;
  std::unique_ptr<MixerController> mixerController_;
  std::unique_ptr<Midi2DiscoveryService> midi2DiscoveryService_;
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

  // Real-time audio processor (Real-time safety and monitoring)
  audio::RealTimeAudioProcessor rtProcessor_;

  // MIDI input handling
  std::vector<std::unique_ptr<juce::MidiInput>> midiInputs_;
  zenith::MidiFifo midiFifo_; // Lock-free MIDI FIFO for input routing

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

  // Async Export Job Tracking
  std::atomic<ExportJob*> currentExportJob_{nullptr};


  // Global singleton instance pointer (for async callback safety)
  static inline Engine* instance = nullptr;

  // Weak reference support for God Mode
  juce::WeakReference<Engine>::Master masterReference;
  friend class juce::WeakReference<Engine>;


  // Export Job (Async Legacy)
  std::unique_ptr<juce::Thread> exportThread_;

  friend class LegacyExportThread;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
} // namespace zenith
