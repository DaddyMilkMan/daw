/**
 * @file Engine.h
 * @brief Core audio engine for Zenith DAW
 *
 * Manages:
 * - Audio device I/O
 * - Audio processing callback
 * - Transport state (play/stop/record)
 * - CPU usage monitoring
 * - Sample rate and buffer size
 *
 * Phase 0: Foundation
 * - Basic audio playback
 * - Transport controls
 * - Device management
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
class TempoMapSynchronizer;

// C3: Forward declarations for donor engine primitives
namespace zenith {
    class Track;
    class Clip;
    class MixerChannel;
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
 * 4. CPU usage monitoring
 */
class Engine : public juce::AudioIODeviceCallback
{
public:
    //==========================================================================
    Engine();
    ~Engine() override;

    //==========================================================================
    // Initialization / Shutdown
    //==========================================================================

    /**
     * @brief Set project state for automation synchronization
     * @param state Pointer to project state (can be nullptr to disable automation)
     * @note Must be called before initialize() or after automation is stopped
     */
    void setProjectState(ProjectState* state);

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
    // Phase 15: Tempo Map Runtime (RT-safe)
    //==========================================================================

    /**
     * @brief Tempo segment for RT-safe tempo queries
     */
    struct TempoSegment
    {
        double startBeat{0.0};           // Start beat of this segment
        double bpm{120.0};                // BPM for this segment
        double secondsAtStartBeat{0.0};   // Accumulated seconds at start beat
        int timeSigNumerator{4};
        int timeSigDenominator{4};
    };

    /**
     * @brief Set tempo map from ProjectState (MESSAGE THREAD)
     * @param tempoChanges Array of tempo changes from ProjectState
     * @note Precomputes segments for RT-safe access
     */
    void setTempoMap(const juce::Array<ProjectState::TempoChangeSpec>& tempoChanges);

    /**
     * @brief Get tempo at a specific sample position (RT-SAFE)
     * @param samplePos Sample position
     * @return BPM at that position
     * @note Can be called from audio thread
     */
    double getTempoAtSample(juce::int64 samplePos) const noexcept;

    /**
     * @brief Convert sample position to beat (RT-SAFE)
     * @param samplePos Sample position
     * @return Beat position
     * @note Can be called from audio thread
     */
    double sampleToBeat(juce::int64 samplePos) const noexcept;

    /**
     * @brief Convert beat to sample position (RT-SAFE)
     * @param beat Beat position
     * @return Sample position
     * @note Can be called from audio thread
     */
    juce::int64 beatToSample(double beat) const noexcept;

    /**
     * @brief Set playhead position (MESSAGE THREAD)
     * @param samplePos Sample position to set playhead to
     */
    void setPlayheadPosition(juce::int64 samplePos);

    /**
     * @brief Get playhead position (RT-SAFE)
     * @return Current playhead position in samples
     */
    juce::int64 getPlayheadPosition() const noexcept { return playbackPosition.load(); }

    //==========================================================================
    // C3: Minimal Engine Surface (compile-only, no audio wiring)
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

    // Playback position (in samples)
    std::atomic<juce::int64> playbackPosition{0};

    // Test tone generator (Phase 0 testing)
    double phase{0.0};
    std::atomic<bool> enableTestTone_{false};

    // C3: Donor track container (no audio thread access yet)
    std::vector<std::unique_ptr<zenith::Track>> tracks_;

    // Phase 13: Automation synchronizer
    ProjectState* projectState_ = nullptr;
    std::unique_ptr<TrackAutomationSynchronizer> automationSynchronizer;

    // Phase 15: Tempo map runtime (RT-safe)
    std::vector<TempoSegment> tempoSegments_;  // Precomputed tempo segments
    mutable std::mutex tempoMapMutex_;         // Protect tempo map updates (message thread only)
    std::unique_ptr<TempoMapSynchronizer> tempoMapSynchronizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
