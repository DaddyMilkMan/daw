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
namespace zenith {
    class Track;
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
    // W10: Transport Position
    //==========================================================================

    /**
     * @brief Set transport position in samples (MESSAGE THREAD)
     */
    void setTransportSamples(juce::int64 pos) { playbackPosition.store(pos); }

    /**
     * @brief Get transport position in samples
     */
    juce::int64 getTransportSamples() const { return playbackPosition.load(); }

    //==========================================================================
    // W10: Master Controls
    //==========================================================================

    /**
     * @brief Set master gain (MESSAGE THREAD)
     * @param g Gain [0..2], default 1.0
     */
    void setMasterGain(float g) { masterGain_.store(juce::jlimit(0.0f, 2.0f, g)); }

    /**
     * @brief Get master gain
     */
    float getMasterGain() const { return masterGain_.load(); }

    /**
     * @brief Set master pan (MESSAGE THREAD)
     * @param p Pan [-1..1], L..R, default 0.0 (center)
     */
    void setMasterPan(float p) { masterPan_.store(juce::jlimit(-1.0f, 1.0f, p)); }

    /**
     * @brief Get master pan
     */
    float getMasterPan() const { return masterPan_.load(); }

    //==========================================================================
    // W10: Track Management (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Get number of tracks
     */
    int getNumTracks() const;

    /**
     * @brief Get track by index
     */
    zenith::Track* getTrack(int index);

    /**
     * @brief Add test tracks for Phase 1 development
     * @param count Number of tracks to add
     */
    void addTestTracks(int count);

    //==========================================================================
    // W11.0: FX Chain Management (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Get number of FX slots per track (fixed at 5)
     */
    static constexpr int getNumFxSlots() { return zenith::Track::getNumFxSlots(); }

    /**
     * @brief Set GainPanNode in FX slot for debugging/testing
     * @param trackIndex Track index
     * @param slotIndex FX slot index [0..4]
     * @param gain Gain [0..2], default 1.0
     * @param pan Pan [-1..1], default 0.0 (center)
     *
     * @note Convenience method for testing W11.0 plugin chain
     * @note For VST3, use setFxNode() with VST3 instance
     */
    void setTrackGainFx(int trackIndex, int slotIndex, float gain, float pan);

    /**
     * @brief Set FX bypass state
     * @param trackIndex Track index
     * @param slotIndex FX slot index [0..4]
     * @param shouldBypass true = bypass, false = active
     */
    void setTrackFxBypassed(int trackIndex, int slotIndex, bool shouldBypass);

    /**
     * @brief Check if FX is bypassed
     * @param trackIndex Track index
     * @param slotIndex FX slot index [0..4]
     * @return true if bypassed or slot is empty
     */
    bool isTrackFxBypassed(int trackIndex, int slotIndex) const;

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

#if JUCE_DEBUG && ZENITH_ENABLE_PHASE1_AUDIO
    //==========================================================================
    // Debug HUD: Peak Meters (MESSAGE THREAD reads, AUDIO THREAD writes)
    //==========================================================================

    /**
     * @brief Get last peak level for left channel [0..1]
     */
    float getLastPeakL() const { return lastPeakL_.load(std::memory_order_relaxed); }

    /**
     * @brief Get last peak level for right channel [0..1]
     */
    float getLastPeakR() const { return lastPeakR_.load(std::memory_order_relaxed); }
#endif

    /**
     * @brief Get audio device manager (for UI configuration)
     * @return Reference to AudioDeviceManager
     */
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

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

    // --------- W10 state (no RT allocations; lock discipline documented) ----
    std::atomic<float> masterGain_{1.0f};   // [0..2], default 1.0
    std::atomic<float> masterPan_{0.0f};    // [-1..1], L..R, default 0.0 (center)

#if ZENITH_ENABLE_PHASE1_AUDIO
    // Preallocated mix buffer sized in prepareToPlay()
    juce::AudioBuffer<float> mixBuffer_;

    // Track container (MESSAGE THREAD access only)
    std::vector<std::unique_ptr<zenith::Track>> tracks_;

    #if JUCE_DEBUG
        // Debug HUD: Peak meters (AUDIO THREAD writes, MESSAGE THREAD reads)
        std::atomic<float> lastPeakL_{0.0f};
        std::atomic<float> lastPeakR_{0.0f};
    #endif
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
