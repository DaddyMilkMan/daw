/**
 * @file Engine.h
 * @brief Core audio engine for Zenith DAW
 *
 * CANONICAL IMPLEMENTATION: This is the authoritative Engine for Zenith.
 * Supersedes: src/audio/AudioEngine.*, src/juce-engine/*, VexelDAW-Native/Source/Audio/AudioEngine.*
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

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <memory>

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
 * 4. MIDI input routing (Phase 2A)
 * 5. Audio input recording (Phase 2D)
 * 6. CPU usage monitoring
 */
class Engine : public juce::AudioIODeviceCallback,
               public juce::MidiInputCallback
{
public:
    //==========================================================================
    Engine();
    ~Engine() override;

    //==========================================================================
    // Initialization / Shutdown
    //==========================================================================

    /**
     * @brief Set project state for automation synchronization and tempo
     * @param state Pointer to project state (can be nullptr to disable automation)
     * @note Must be called before initialize() or after automation is stopped
     */
    void setProjectState(ProjectState* state);

    /**
     * @brief Synchronize engine tracks with project state
     * @note Message thread only - rebuilds track list from ProjectState
     */
    void syncWithProjectState();

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
     * @note MESSAGE THREAD ONLY - Starts recording on armed tracks (both MIDI and audio)
     */
    void record();

    /**
     * @brief Stop recording and bake clips
     * @note MESSAGE THREAD ONLY - Converts recordings to clips (both MIDI and audio)
     */
    void stopRecording();

    /**
     * @brief Check if recording
     */
    bool isRecording() const { return isRecording_.load(); }

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
    const std::vector<std::unique_ptr<zenith::Track>>& tracks() const noexcept;

    /**
     * @brief Debug helper to create test tracks (message thread only)
     * @param count Number of tracks to create
     * @note Does NOT attach tracks to audio graph; for compile/UI testing only
     */
    void addTestTracks(int count);

    //==========================================================================
    // Phase 1.2: Audio File Pool
    //==========================================================================

    /**
     * @brief Get the audio file pool for loading/caching audio files
     * @return Reference to the audio file pool
     * @note Thread-safe; pool handles internal locking
     */
    zenith::AudioFilePool& getAudioFilePool();

    //==========================================================================
    // Plugin Hosting (Phase 3: VST3 hosting MVP)
    //==========================================================================

    /**
     * @brief Get the plugin host manager
     * @return Reference to PluginHost
     * @note Use only from message thread
     */
    zenith::PluginHost& getPluginHost() noexcept;

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
    zenith::PluginEditorWindowManager& getPluginEditorWindowManager() noexcept;

    //==========================================================================
    // Unified Render Path
    //==========================================================================

    /**
     * @brief Unified render function used by both realtime and offline paths
     *
     * This is the single source of truth for audio rendering:
     * - Realtime callback calls this with live transport position
     * - Export calls this with offline transport position
     *
     * @param outputBuffer Pre-allocated stereo buffer to fill
     * @param transportPosition Current playback position in samples
     * @param numSamples Number of samples to render
     * @note Can run on AUDIO THREAD - must be real-time safe!
     * @note Uses pre-allocated track buffers to avoid allocation
     */
    void renderBlock(juce::AudioBuffer<float>& outputBuffer,
                     juce::int64 transportPosition,
                     int numSamples);

    /**
     * @brief Export project to WAV file using unified render path
     *
     * This uses the SAME renderBlock() function as realtime playback,
     * ensuring bit-identical output.
     *
     * @param outputFilePath Path to output WAV file
     * @param durationSeconds Duration to export (0 = auto-detect from project)
     * @param sampleRate Sample rate for export (0 = use current engine rate)
     * @return true if export succeeded
     * @note Runs on MESSAGE THREAD
     */
    bool exportProjectToWav(const juce::String& outputFilePath,
                           double durationSeconds = 10.0,
                           double sampleRate = 0.0);

    //==========================================================================
    // AudioIODeviceCallback interface (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Called when audio device is about to start
     * @note Runs on AUDIO THREAD - must be real-time safe!
     */
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;

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
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    //==========================================================================
    // Phase 2A: MidiInputCallback interface
    //==========================================================================

    /**
     * @brief Handle incoming MIDI messages from input devices
     * @note Runs on MIDI input thread, routes to armed tracks
     */
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

private:
    //==========================================================================
    // Audio Processing (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Process audio when playing
     * @note AUDIO THREAD - real-time safe!
     */
    void processAudio(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples);

    /**
     * @brief Process audio recording (AUDIO THREAD)
     * @note RT-safe: only writes to ThreadedWriter (lock-free FIFO)
     */
    void processAudioRecording(
        const float* const* inputChannelData,
        int numInputChannels,
        int numSamples);

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
    void bakeAudioRecordingIntoTrack(
        zenith::Track& track,
        const juce::File& file,
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
    juce::MidiMessageSequence quantizeMidiSequence(
        const juce::MidiMessageSequence& input,
        double tempo,
        double quantizeGrid);

    // Track management (message thread only)
    void prepareTracks(int samplesPerBlockExpected, double sampleRate);

    // Phase 2A: MIDI input management (message thread only)
    void enableMidiInput();
    void disableMidiInput();

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
    std::atomic<juce::int64> loopEndSamples_{0};  // 0 = no loop end set

    // Test tone generator (Phase 0 testing)
    double phase{0.0};
    std::atomic<bool> enableTestTone_{false};

    // Track container (message thread for modification, audio thread for iteration)
    std::vector<std::unique_ptr<zenith::Track>> tracks_;

    // Phase 1.2: Audio file pool (message thread for load/unload, RT-safe for access)
    std::unique_ptr<zenith::AudioFilePool> audioFilePool_;

    // Phase 3: Plugin hosting
    std::unique_ptr<zenith::PluginHost> pluginHost_;
    std::unique_ptr<zenith::PluginEditorWindowManager> pluginEditorWindowManager_;

    // Project state reference (non-owning, for tempo/time sig/automation access)
    ProjectState* projectState_ = nullptr;

    // Automation synchronizer
    std::unique_ptr<TrackAutomationSynchronizer> automationSynchronizer;

    // Unified render path: Pre-allocated track buffers (avoid allocation in audio thread)
    std::vector<juce::AudioBuffer<float>> trackBuffers_;
    juce::AudioBuffer<float> masterBuffer_;

    // Phase 2A: MIDI input handling
    std::unique_ptr<juce::MidiInput> midiInput_;
    juce::MidiBuffer incomingMidiBuffer_;  // Buffered MIDI from input
    juce::CriticalSection midiInputLock_;  // Protects incomingMidiBuffer_

    // Phase 2A: MIDI recording state (per-track)
    struct MidiRecordingBuffer
    {
        std::vector<juce::MidiMessageSequence> trackRecordings;  // One per track
        juce::int64 recordingStartSamples = 0;  // Playhead when recording started
    };
    MidiRecordingBuffer midiRecording_;
    juce::CriticalSection midiRecordingLock_;

    //==========================================================================
    // Phase 2D: Audio Recording Infrastructure
    //==========================================================================

    // Background thread for audio file writing
    std::unique_ptr<juce::TimeSliceThread> audioWriterThread_;

    // Audio recording session (per-track)
    struct AudioRecordingSession
    {
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
        juce::File file;
        int numChannels = 0;
        double sampleRate = 44100.0;
        juce::int64 recordingStartSamples = 0;
        int trackIndex = -1;  // Which track this session belongs to
    };

    // Active recording sessions (message thread creates, audio thread writes)
    std::vector<AudioRecordingSession> audioRecordingSessions_;

    // Flag to prevent use-after-free in async callbacks (CODEX FIX P2)
    std::atomic<bool> isShuttingDown_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
