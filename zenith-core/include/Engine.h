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
class Track;

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
 * 5. Track management
 */
class Engine : public juce::AudioIODeviceCallback
{
public:
    using SamplePos = juce::int64;
    //==========================================================================
    Engine();
    ~Engine() override;

    //==========================================================================
    // Initialization / Shutdown
    //==========================================================================

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
     * @note MESSAGE THREAD ONLY
     */
    void play();

    /**
     * @brief Pause playback (keep position)
     * @note MESSAGE THREAD ONLY
     */
    void pause();

    /**
     * @brief Stop playback and reset state
     * @note MESSAGE THREAD ONLY
     */
    void stop();

    /**
     * @brief Check if playing
     */
    bool isPlaying() const { return isPlaying_.load(); }

    /**
     * @brief Seek to sample position
     * @param targetSample Sample position to seek to
     * @note MESSAGE THREAD ONLY - MUST NOT be called while playing
     */
    void seekSamples(SamplePos targetSample);

    /**
     * @brief Get current transport position in samples
     * @return Current playback position
     */
    SamplePos getTransportSamples() const { return playbackPosition.load(); }

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
     * @brief Set number of tracks
     * @param numTracks Number of tracks to create
     * @note MESSAGE THREAD ONLY
     */
    void setNumTracks(int numTracks);

    /**
     * @brief Get number of tracks
     * @return Current track count
     */
    int getNumTracks() const { return static_cast<int>(tracks_.size()); }

    /**
     * @brief Get track by index
     * @param index Track index (0-based)
     * @return Pointer to track or nullptr if invalid
     * @note MESSAGE THREAD ONLY
     */
    Track* getTrack(int index);

    /**
     * @brief Get track by index (const version)
     * @param index Track index (0-based)
     * @return Pointer to track or nullptr if invalid
     */
    const Track* getTrack(int index) const;

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

    // Track management (Phase 1)
    std::vector<std::unique_ptr<Track>> tracks_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
