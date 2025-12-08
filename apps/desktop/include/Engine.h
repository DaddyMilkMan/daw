
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
#include "../Source/engine/RoutingGraph.h"
#include "EngineEvent.h"

// Forward declarations
namespace zenith {
class ProjectState;
class TrackAutomationSynchronizer;
class Track;
class Clip;
class MixerChannel;
class AudioFilePool;
class PluginHost;
class PluginEditorWindowManager;
class TempoMap;
class AuxBus;
class InstrumentRegistry;

namespace ai {
class SessionDebuggerAgent;
}

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
 */
class Engine : public juce::AudioIODeviceCallback,
               public juce::MidiInputCallback {
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
  bool isPlaying() const { return isPlaying_.load(); }

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
  bool isRecording() const { return isRecording_.load(); }

  /**
   * @brief Toggle recording on/off
   * @note Convenience method for record button
   */
  void toggleRecording();

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
  // Transport Position & Looping
  //==========================================================================

  /**
   * @brief Get current playback position in samples
   */
  juce::int64 getPlayheadSamples() const { return playheadSamples_.load(); }

  /**
   * @brief Get current playback position in samples (legacy accessor)
   */
  juce::int64 getPlaybackPosition() const { return playheadSamples_.load(); }

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
  bool isLooping() const { return isLooping_.load(); }

  /**
   * @brief Set loop region in samples (MESSAGE THREAD ONLY)
   */
  void setLoopRegion(juce::int64 start, juce::int64 end);

  /**
   * @brief Get loop start position in samples
   */
  juce::int64 getLoopStart() const { return loopStartSamples_.load(); }

  /**
   * @brief Get loop end position in samples
   */
  juce::int64 getLoopEnd() const { return loopEndSamples_.load(); }

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
  bool isPDCEnabled() const { return pdcEnabled_.load(); }

  /**
   * @brief Enable/disable PDC
   */
  void setPDCEnabled(bool enabled);

  /**
   * @brief Get maximum track latency (for PDC compensation)
   */
  int getMaxTrackLatency() const { return maxTrackLatency_.load(); }

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
  const ai::SessionDebuggerAgent *getSessionDebugger() const {
    return sessionDebugger_.get();
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
  void setTrackInputChannel(int trackIndex, int channelIndex); // ROAST FIX #9

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
  // Project Export
  //==========================================================================

  /**
   * @brief Export project to WAV file
   * @param outputFile Output file path
   * @param sampleRate Sample rate for export
   * @param bitDepth Bit depth (16, 24, or 32)
   * @param durationInSeconds Duration to export
   * @return true if successful
   */
  bool exportProjectToWav(const juce::File &outputFile, double sampleRate,
                          int bitDepth, double durationInSeconds);

  enum class ExportFormat { WAV, FLAC, OGG };

  struct ExportOptions {
    juce::File outputFile;
    double sampleRate = 44100.0;
    int bitDepth = 24; // 8, 16, 24, 32
    ExportFormat format = ExportFormat::WAV;
    bool enableDither = true;
    bool normalize = false;
    double normalizeDb = -0.1;
    double duration = 0.0;
  };

  /**
   * @brief Advanced Project Export
   * Supports WAV/FLAC/OGG, Dithering, Normalization, and 8-bit.
   */
  bool exportProject(const ExportOptions &options);

private:
  //==========================================================================
  // Audio Processing (AUDIO THREAD)
  //==========================================================================

  /**
   * @brief Process audio when playing
   * @note AUDIO THREAD - real-time safe!
   */
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

  /**
   * @brief Process audio recording (AUDIO THREAD)
   * @note RT-safe: only writes to ThreadedWriter (lock-free FIFO)
   */
  /**
   * @brief Process audio recording (AUDIO THREAD)
   * @note RT-safe: only writes to ThreadedWriter (lock-free FIFO)
   */
  void captureAudioInput(const float *const *inputChannelData,
                         int numInputChannels, int numSamples) noexcept;

  //==========================================================================
  // Recording Helpers (MESSAGE THREAD)
  //==========================================================================

  /**
   * @brief Convert a completed audio recording into an AudioClip
   * @param track Track to add clip to
   * @param file Recorded audio file
   * @param recordingStartSamples Timeline position where recording started
   * @param sampleRate Sample rate of recording
   */
  void bakeAudioRecordingIntoTrack(zenith::Track &track, const juce::File &file,
                                   juce::int64 recordingStartSamples,
                                   double sampleRate);

  //==========================================================================
  // MIDI Recording Helpers (MESSAGE THREAD)
  //==========================================================================

  /**
   * @brief Convert recorded MIDI into clips on tracks
   * @param quantize If true, quantize events to 1/16 note grid
   * @note MESSAGE THREAD ONLY
   */
  void bakeMidiRecordingsIntoClips(bool quantize);

  /**
   * @brief Clear all MIDI recording buffers
   * @note MESSAGE THREAD ONLY
   */
  void clearMidiRecordings();

  /**
   * @brief Quantize MIDI sequence to grid
   * @param input Original sequence
   * @param tempo Project tempo in BPM
   * @param quantizeGrid Grid size (0.25 = 1/16, 0.5 = 1/8, etc.)
   * @return Quantized sequence
   */
  juce::MidiMessageSequence
  quantizeMidiSequence(const juce::MidiMessageSequence &input, double tempo,
                       double quantizeGrid);

  /**
   * @brief Drain MIDI record FIFO into recording buffers (lock-free)
   * @note MESSAGE THREAD ONLY - Call this before baking clips
   */
  void drainMidiRecordFifo();

  /**
   * @brief Auto-detect project duration from clip end positions
   * @return Duration in seconds (with 2s tail for reverb/delay)
   * @note MESSAGE THREAD ONLY
   */
  double autoDetectProjectDuration() const;

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

  // Transport state (std::atomic for thread-safe access)
  std::atomic<bool> isPlaying_{false};
  std::atomic<bool> isRecording_{false};

  // Audio settings (std::atomic for thread-safe access)
  std::atomic<double> currentSampleRate{44100.0};
  std::atomic<int> currentBufferSize{512};

  // CPU usage tracking
  mutable std::atomic<double> cpuUsage_{0.0};
  juce::int64 lastCpuCheckTime{0};

  // Transport position tracking (atomic for RT-safe access)
  std::atomic<juce::int64> playheadSamples_{0};
  std::atomic<bool> isLooping_{false};
  std::atomic<juce::int64> loopStartSamples_{0};
  std::atomic<juce::int64> loopEndSamples_{0}; // 0 = no loop end set

  // Test tone generator
  double phase{0.0};
  std::atomic<bool> enableTestTone_{false};

  // Track container (message thread for modification)
  // ROAST FIX #1: Use shared_ptr instead of unique_ptr to enable safe snapshot
  // sharing
  std::vector<std::shared_ptr<zenith::Track>> tracks_;

  // Routing Graph (Source of Truth for connections and processing order)
  RoutingGraph routingGraph_;

public:
  RoutingGraph &getRoutingGraph() { return routingGraph_; }
  const RoutingGraph &getRoutingGraph() const { return routingGraph_; }

private:
  // Thread-safe Track Snapshot (RCU-style)
  // Audio thread reads this snapshot without locking (wait-free iteration)
  // ROAST FIX #1: Use raw pointers for iteration (speed), shared_ptr for
  // lifetime (safety)
  struct TrackSnapshot {
    std::vector<zenith::Track *>
        tracks; // Raw pointers for fast, lock-free iteration
    std::vector<zenith::AuxBus *> auxBuses; // Raw pointers for buses

    std::vector<std::shared_ptr<zenith::Track>> lifecycle; // Keeps tracks alive
    std::vector<std::shared_ptr<zenith::AuxBus>>
        lifecycleAux; // Keeps buses alive

    TrackSnapshot() = default;
    TrackSnapshot(
        const std::vector<std::shared_ptr<zenith::Track>> &ownedTracks,
        const std::vector<std::shared_ptr<zenith::AuxBus>> &ownedBuses) {
      tracks.reserve(ownedTracks.size());
      lifecycle.reserve(ownedTracks.size());
      for (const auto &track : ownedTracks) {
        tracks.push_back(track.get());
        lifecycle.push_back(track); // Increment refcount
      }

      auxBuses.reserve(ownedBuses.size());
      lifecycleAux.reserve(ownedBuses.size());
      for (const auto &bus : ownedBuses) {
        auxBuses.push_back(bus.get());
        lifecycleAux.push_back(bus);
      }
    }
  };

  // Lock-free snapshot mechanism
  // Audio thread reads activeSnapshot_ (atomic raw pointer)
  // Main thread manages lifetime via currentSnapshotHolder_ and snapshotTrash_
  std::atomic<TrackSnapshot *> activeSnapshot_{nullptr};
  std::shared_ptr<TrackSnapshot> currentSnapshotHolder_;
  std::vector<std::shared_ptr<TrackSnapshot>> snapshotTrash_;

  void updateTrackSnapshot();

  // Phase 1.2: Audio file pool (message thread for load/unload, RT-safe for
  // access)
  std::unique_ptr<zenith::AudioFilePool> audioFilePool_;

  // Phase 3: Plugin hosting
  std::unique_ptr<zenith::PluginHost> pluginHost_;
  std::unique_ptr<zenith::PluginEditorWindowManager> pluginEditorWindowManager_;

  // Instrument Registry (Level 4: No Singleton)
  std::unique_ptr<zenith::InstrumentRegistry> instrumentRegistry_;

  // Session Debugger Agent (AI Technical Integrity)
  std::unique_ptr<ai::SessionDebuggerAgent> sessionDebugger_;

  // Project state reference (non-owning, for tempo/time sig/automation access)
  ProjectState *projectState_ = nullptr;

  // Automation synchronizer
  std::unique_ptr<TrackAutomationSynchronizer> automationSynchronizer;

  // Phase 15: Tempo map
  std::unique_ptr<zenith::TempoMap> tempoMap_;

  // Unified render path: Pre-allocated track buffers (avoid allocation in audio
  // thread)
  std::vector<juce::AudioBuffer<float>> trackBuffers_;
  juce::AudioBuffer<float> masterBuffer_;

  // Aux buses (send/return effects)
  std::vector<std::shared_ptr<zenith::AuxBus>> auxBuses_;
  std::vector<juce::AudioBuffer<float>>
      auxBusBuffers_; // Pre-allocated buffers for aux buses

  // Master bus plugins
  std::vector<std::unique_ptr<juce::AudioPluginInstance>> masterPlugins_;
  juce::CriticalSection masterPluginLock_;
  juce::AudioBuffer<float> masterPluginBuffer_;

  // Phase 11: Master metering (atomic for lock-free GUI access)
  std::atomic<float> masterLevel_{0.0f};
  std::atomic<float> masterPeakLevel_{0.0f};

  //==========================================================================
  // Plugin Delay Compensation (PDC)
  //==========================================================================

  std::atomic<bool> pdcEnabled_{true};
  std::atomic<int> maxTrackLatency_{0}; // Maximum latency across all tracks
  std::vector<int> trackLatencies_;     // Per-track latency values
  std::vector<juce::AudioBuffer<float>>
      pdcDelayBuffers_;               // Delay buffers for PDC
  std::vector<int> pdcDelayWritePos_; // Write positions for circular buffers
  int masterLatency_{0};              // Master bus total latency

  void applyPDCDelay(juce::AudioBuffer<float> &buffer, int trackIndex,
                     int delaySamples) noexcept;

  //==========================================================================
  // Sample-Accurate Looping State
  //==========================================================================

  // Sample-accurate loop: stores where in the buffer the loop wrap occurs
  // -1 means no loop wrap in current buffer
  std::atomic<int> loopWrapSampleOffset_{-1};

  // Phase 2A: MIDI input handling
  std::vector<std::unique_ptr<juce::MidiInput>> midiInputs_;
  zenith::MidiFifo midiFifo_; // Lock-free MIDI FIFO

  // Phase 2 Refactor: Lock-free Command Queue
  static constexpr int kCommandBufferSize = 1024;
  juce::AbstractFifo commandFifo_{kCommandBufferSize};
  std::vector<zenith::EngineEvent> commandBuffer_{kCommandBufferSize};

  // Phase 2A: MIDI recording state (per-track) - LOCK-FREE USING FIFO
  struct MidiRecordEvent {
    juce::MidiMessage message;
    int trackIndex;
    juce::int64 timestampSamples;
  };

  // Lock-free FIFO for MIDI recording events
  static constexpr int kMidiRecordFifoSize = 4096;
  juce::AbstractFifo midiRecordFifo_{kMidiRecordFifoSize};
  std::vector<MidiRecordEvent> midiRecordBuffer_{kMidiRecordFifoSize};

  // Baked recordings (message thread only, after stopRecording)
  struct MidiRecordingBuffer {
    std::vector<juce::MidiMessageSequence> trackRecordings; // One per track
    juce::int64 recordingStartSamples = 0; // Playhead when recording started
  };
  MidiRecordingBuffer midiRecording_;
  mutable juce::CriticalSection midiRecordingLock_;

  //==========================================================================
  // Phase 2D: Audio Recording Infrastructure
  //==========================================================================

  // Background thread for audio file writing
  std::unique_ptr<juce::TimeSliceThread> audioWriterThread_;

  // Audio recording session (per-track)
  struct AudioRecordingSession {
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
    juce::File file;
    int numChannels = 0;
    double sampleRate = 44100.0;
    juce::int64 recordingStartSamples = 0;
    int trackIndex = -1; // Which track this session belongs to
    int inputChannelIndex =
        0; // ROAST FIX #9: Which input channel to record from
  };

  // Active recording sessions (message thread creates, audio thread writes)
  std::vector<AudioRecordingSession> audioRecordingSessions_;

  // ROAST FIX #4: Pre-prepared sessions to avoid blocking I/O on record start
  std::vector<AudioRecordingSession> preppedSessions_;
  juce::CriticalSection preppedSessionsLock_;

  // Helper to prepare recording asynchronously
  void prepareRecordingForTrack(int trackIndex);

  // Helper to update SIP (Solo In Place) logic
  void updateSoloState();

  // ID Counter for Aux Busses
  std::atomic<int> auxBusIdCounter{0};

  // Flag to prevent use-after-free in async callbacks (CODEX FIX P2)
  std::atomic<bool> isShuttingDown_{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
} // namespace zenith
