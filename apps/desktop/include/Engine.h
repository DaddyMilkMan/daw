/**
 * @file Engine.h
 * @brief Core audio engine for Zenith DAW
 *
 * CANONICAL IMPLEMENTATION: This is the authoritative Engine for Zenith.
 * Supersedes: src/audio/AudioEngine.*, src/juce-engine/,
 * VexelDAW-Native/Source/Audio/AudioEngine.*
 *
 * Phase 1-2 Complete:
 * - RT-safe track mixdown with unified render path
 * - Lock-free clip snapshots (Phase 2A)
 * - Atomic playhead tracking with loop support
 * - AudioFilePool integration for audio file caching
 * - MIDI input routing and recording (Phase 2A/2C)
 * - Audio input recording (Phase 2D)
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

#include "EngineEvent.h"

// Flecs ECS Integration (optional but recommended)
#include "ECSIntegrationExample.h"

// Forward declarations
class ProjectState;
class TrackAutomationSynchronizer;

// Forward declarations for engine primitives
namespace zenith {
class Track;
class Clip;
class MixerChannel;
class AudioFilePool;
class PluginHost;
class PluginEditorWindowManager;
class TempoMap;
class AuxBus;
} // namespace zenith

//==============================================================================
/**
 * @class Engine
 * @brief Core audio engine
 *
 * This class manages the entire audio processing chain:
 * 1. Audio device I/O
 * 2. Transport (play/stop/record)
 * 3. Audio routing and mixing
 * 4. MIDI input routing (Phase 2A)
 * 5. Audio input recording (Phase 2D)
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
  juce::AudioPluginFormatManager& getPluginFormatManager();

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
  // Real-time Event Queue (Phase 2 Refactor)
  //==========================================================================

  /**
   * @brief Queue an event for the audio thread
   * @param e Event to queue
   * @return true if queued successfully, false if full
   * @note Lock-free, safe to call from any thread
   */
  bool queueEvent(const zenith::EngineEvent& e);

  //==========================================================================
  // Phase 1.3: Transport Position & Looping
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
  const std::vector<std::unique_ptr<zenith::Track>> &tracks() const noexcept;

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
   * @param track Unique pointer to track
   * @note Message thread only; used by TrackStateSynchronizer
   */
  void addTrack(std::unique_ptr<zenith::Track> track);

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
  zenith::AuxBus *getAuxBus(int auxIndex) noexcept;

  /**
   * @brief Get aux bus meters
   * @param auxIndex Index of aux bus
   * @return Current level (0.0 - 1.0+), or 0.0 if invalid
   */
  float getAuxBusLevel(int auxIndex) const;
  float getAuxBusPeakLevel(int auxIndex) const;

  //==========================================================================
  // Phase 11: Mixer Control (MESSAGE THREAD ONLY)
  //==========================================================================

  /**
   * @brief Set track mixer controls (message thread only)
   * @note These methods update the engine Track objects directly
   * @note In Phase 11, these are called by TrackStateSynchronizer
   */
  void setTrackVolume(int trackIndex, float volume);
  void setTrackPan(int trackIndex, float pan);
  void setTrackMute(int trackIndex, bool muted);
  void setTrackSolo(int trackIndex, bool solo);
  void setTrackArmed(int trackIndex, bool armed);

  //==========================================================================
  // Phase 11: Metering (MESSAGE THREAD SAFE)
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
  // Phase 1.2: Audio File Pool
  //==========================================================================

  /**
   * @brief Get the audio file pool for loading/caching audio files
   * @return Reference to the audio file pool
   * @note Thread-safe; pool handles internal locking
   */
  zenith::AudioFilePool &getAudioFilePool();

  /**
   * @brief Get the tempo map for beat/time conversions
   * @return Reference to TempoMap
   * @note Thread-safe; uses lock-free snapshot mechanism
   */
  const zenith::TempoMap &getTempoMap() const noexcept;

  //==========================================================================
  // Plugin Hosting (Phase 3: VST3 hosting MVP)
  //==========================================================================

  /**
   * @brief Get the plugin host manager
   * @return Reference to PluginHost
   * @note Use only from message thread
   */
  zenith::PluginHost &getPluginHost() noexcept;

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
  zenith::PluginEditorWindowManager &getPluginEditorWindowManager() noexcept;

  //==========================================================================
  // Flecs ECS Integration (Optional, Coexists with Legacy Track/Clip)
  //==========================================================================

  /**
   * @brief Get the ECS engine for entity-component system features
   * @return Pointer to ECS engine, or nullptr if not initialized
   * @note Use only from message thread for entity creation
   * @note Audio thread can use cached queries (see ECSIntegrationExample.h)
   * 
   * Example usage:
   *   auto* ecs = engine.getECSEngine();
   *   if (ecs) {
   *       auto track = ecs->createTrack("Piano", "track-1");
   *   }
   */
  zenith::ECSEngine* getECSEngine() noexcept { return ecsEngine_.get(); }
  
  /**
   * @brief Enable ECS integration (creates ECS world)
   * @note Call this before using ECS features
   * @note Safe to call multiple times (idempotent)
   */
  void enableECS();

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
  // Phase 2A: MidiInputCallback interface
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

private:
  //==========================================================================
  // Audio Processing (AUDIO THREAD)
  //==========================================================================

  /**
   * @brief Process audio when playing
   * @note AUDIO THREAD - real-time safe!
   */
  void processAudio(const float *const *inputChannelData, int numInputChannels,
                    float *const *outputChannelData, int numOutputChannels,
                    int numSamples) noexcept;

  /**
   * @brief Process pending events
   * @note AUDIO THREAD - Lock-free
   */
  void processEvents() noexcept;

  /**
   * @brief Process audio recording (AUDIO THREAD)
   * @note RT-safe: only writes to ThreadedWriter (lock-free FIFO)
   */
  void processAudioRecording(const float *const *inputChannelData,
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
  // Phase 2C: MIDI Recording Helpers (MESSAGE THREAD)
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

  // Track management (message thread only)
  void prepareTracks(int samplesPerBlockExpected, double sampleRate);

  // Phase 2A: MIDI input management (message thread only)
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
  void renderBlock(juce::AudioBuffer<float> &outputBuffer, int numSamples,
                   juce::int64 playheadPosition,
                   const juce::MidiBuffer *incomingMidi = nullptr);

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

  // Phase 1.3: Transport position tracking (atomic for RT-safe access)
  std::atomic<juce::int64> playheadSamples_{0};
  std::atomic<bool> isLooping_{false};
  std::atomic<juce::int64> loopStartSamples_{0};
  std::atomic<juce::int64> loopEndSamples_{0}; // 0 = no loop end set

  // Test tone generator (Phase 0 testing)
  double phase{0.0};
  std::atomic<bool> enableTestTone_{false};

  // Track container (message thread for modification)
  std::vector<std::unique_ptr<zenith::Track>> tracks_;

  // Thread-safe Track Snapshot (RCU-style)
  // Audio thread reads this snapshot without locking (wait-free iteration)
  struct TrackSnapshot {
    std::vector<zenith::Track *> tracks; // Raw pointers (non-owning)
    TrackSnapshot() = default;
    explicit TrackSnapshot(const std::vector<std::unique_ptr<zenith::Track>> &ownedTracks) {
      tracks.reserve(ownedTracks.size());
      for (const auto &track : ownedTracks)
        tracks.push_back(track.get());
    }
  };

  std::shared_ptr<const TrackSnapshot> tracksSnapshot_;
  juce::SpinLock snapshotLock_; // Protects the swap of the shared_ptr

  void updateTrackSnapshot() {
    auto newSnapshot = std::make_shared<TrackSnapshot>(tracks_);
    const juce::SpinLock::ScopedLockType sl(snapshotLock_);
    tracksSnapshot_ = newSnapshot;
  }

  // Phase 1.2: Audio file pool (message thread for load/unload, RT-safe for
  // access)
  std::unique_ptr<zenith::AudioFilePool> audioFilePool_;

  // Phase 3: Plugin hosting
  std::unique_ptr<zenith::PluginHost> pluginHost_;
  std::unique_ptr<zenith::PluginEditorWindowManager> pluginEditorWindowManager_;

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
  std::vector<std::unique_ptr<zenith::AuxBus>> auxBuses_;
  std::vector<juce::AudioBuffer<float>>
      auxBusBuffers_; // Pre-allocated buffers for aux buses

  // Master bus plugins
  std::vector<std::unique_ptr<juce::AudioPluginInstance>> masterPlugins_;
  juce::CriticalSection masterPluginLock_;
  juce::AudioBuffer<float> masterPluginBuffer_;

  // Phase 11: Master metering (atomic for lock-free GUI access)
  std::atomic<float> masterLevel_{0.0f};
  std::atomic<float> masterPeakLevel_{0.0f};

  // Phase 2A: MIDI input handling
  std::vector<std::unique_ptr<juce::MidiInput>> midiInputs_;
  zenith::MidiFifo midiFifo_; // Lock-free MIDI FIFO

  // Phase 2 Refactor: Lock-free Command Queue
  static constexpr int kCommandBufferSize = 1024;
  juce::AbstractFifo commandFifo_{kCommandBufferSize};
  std::vector<zenith::EngineEvent> commandBuffer_{kCommandBufferSize};

  // Phase 2A: MIDI recording state (per-track)
  struct MidiRecordingBuffer {
    std::vector<juce::MidiMessageSequence> trackRecordings; // One per track
    juce::int64 recordingStartSamples = 0; // Playhead when recording started
  };
  MidiRecordingBuffer midiRecording_;
  juce::CriticalSection midiRecordingLock_;

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
  };

  // Active recording sessions (message thread creates, audio thread writes)
  std::vector<AudioRecordingSession> audioRecordingSessions_;

  // Flag to prevent use-after-free in async callbacks (CODEX FIX P2)
  std::atomic<bool> isShuttingDown_{false};

  // Flecs ECS Integration (optional, nullptr if not enabled)
  std::unique_ptr<zenith::ECSEngine> ecsEngine_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
