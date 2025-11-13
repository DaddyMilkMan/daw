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
#include <cstdint>
#include <vector>

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
#include "Mixer.h"
#include "rt/SpscRing.h"
#endif

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
    // Transport Controls (Phase 0 - gated when Phase 1 enabled)
    //==========================================================================

#if !defined(ZENITH_ENABLE_PHASE1_AUDIO) || !ZENITH_ENABLE_PHASE1_AUDIO
    /**
     * @brief Start playback (Phase 0)
     */
    void play();

    /**
     * @brief Stop playback (Phase 0)
     */
    void stop();

    /**
     * @brief Check if playing (Phase 0)
     */
    bool isPlaying() const { return isPlaying_.load(); }
#endif

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

    /**
     * @brief Get audio device manager (for UI configuration)
     * @return Reference to AudioDeviceManager
     */
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
    /**
     * @brief Get mixer (W10: Phase 1 audio engine)
     * @return Reference to Mixer
     */
    Mixer& getMixer() { return mixer_; }

    //==========================================================================
    // W10.2: Sample-accurate transport & event scheduler (gated)
    //==========================================================================

    struct TransportEvent {
        enum class Type : int32_t { StartClip = 0, StopClip = 1 };
        Type      type{};
        int32_t   trackIndex{-1};
        int32_t   clipId{-1};
        int64_t   whenSamples{0};  // absolute transport sample time
    };

    /** Transport controls (message thread only) */
    void play() noexcept;                        // start advancing transport
    void pause() noexcept;                       // stop advancing transport
    void stop() noexcept;                        // stop and reset to sample 0
    void seekSamples(int64_t absolute) noexcept; // set absolute transport time
    int64_t transportSamples() const noexcept { return transportSamples_.load(std::memory_order_relaxed); }
    int64_t getTransportSamples() const noexcept { return transportSamples(); }
    bool    isPlaying() const noexcept { return isPlaying_.load(std::memory_order_relaxed); }

    /** Schedule events (message thread only, lock-free enqueue). */
    bool scheduleClipStart(int trackIndex, int clipId, int64_t atSample) noexcept;
    bool scheduleClipStop (int trackIndex, int clipId, int64_t atSample) noexcept;

    /** Debug/introspection (message thread only). */
    std::size_t pendingEventCount() const noexcept { return eventQ_.size(); }

    //==========================================================================
    // Track Management (MESSAGE THREAD ONLY)
    //==========================================================================

    /**
     * @brief Set number of tracks (creates/removes as needed)
     * @param numTracks Desired number of tracks
     * @note MESSAGE THREAD ONLY - delegates to mixer
     */
    void setNumTracks(int numTracks);

    /**
     * @brief Get number of tracks
     * @return Number of tracks in mixer
     */
    int getNumTracks() const;

    /**
     * @brief Get track by index
     * @param index Track index (0-based)
     * @return Pointer to track or nullptr if invalid index
     * @note MESSAGE THREAD ONLY - non-owning pointer
     */
    AudioTrack* getTrack(int index);

    /**
     * @brief Get track by index (const version)
     * @param index Track index (0-based)
     * @return Const pointer to track or nullptr if invalid index
     */
    const AudioTrack* getTrack(int index) const;

    //==========================================================================
    // W10.3: Sample-accurate event draining for segment loop
    //==========================================================================

    /** Due event with offset within current block. */
    struct DueEvent {
        TransportEvent ev;
        int offsetInBlock;
    };

    static constexpr int kMaxEventsPerBlock = 64;

    /**
     * @brief Drain all events due in [blockStart, blockEnd) and return sorted by offset.
     * @param blockStart Start of current block (absolute transport samples)
     * @param blockEnd End of current block (absolute transport samples)
     * @param outCount Number of events drained (will be written here)
     * @return Pointer to static array of DueEvent (valid until next call)
     * @note AUDIO THREAD ONLY - uses peek/pop pattern to avoid dropping future events
     */
    DueEvent* drainScheduledEvents(int64_t blockStart, int64_t blockEnd, int& outCount) noexcept;

#if JUCE_DEBUG
    //==========================================================================
    // Debug Metrics (MESSAGE THREAD - read-only, RT-safe via atomics)
    //==========================================================================

    /** Debug-only metrics for Phase 1 HUD. */
    struct Phase1DebugMetrics {
        int64_t transportSamples = 0;
        bool    isPlaying = false;
        int     numTracks = 0;
        int     activeVoices = 0;
        int     queuedEvents = 0;
        uint64_t droppedEvents = 0;
        float   peakL = 0.0f;
        float   peakR = 0.0f;
        double  sampleRate = 44100.0;
    };

    /**
     * @brief Get current Phase 1 debug metrics (MESSAGE THREAD only).
     * @note Uses atomics for RT-safe reading, no locks.
     */
    Phase1DebugMetrics getPhase1DebugMetrics() const noexcept;
#endif // JUCE_DEBUG
#endif // ZENITH_ENABLE_PHASE1_AUDIO

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

    /**
     * @brief Prepare for playback (W10: allocate mix buffers when flag ON)
     * @note AUDIO THREAD - Called before audio starts
     */
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

    /**
     * @brief Release resources (W10: deallocate mix buffers)
     * @note AUDIO THREAD - Called when audio stops
     */
    void releaseResources() override;

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

    // Audio settings (std::atomic for thread-safe access)
    std::atomic<double> currentSampleRate{44100.0};
    std::atomic<int> currentBufferSize{512};

    // CPU usage tracking
    mutable std::atomic<double> cpuUsage_{0.0};
    juce::int64 lastCpuCheckTime{0};

#if !defined(ZENITH_ENABLE_PHASE1_AUDIO) || !ZENITH_ENABLE_PHASE1_AUDIO
    // Phase 0 transport state (std::atomic for thread-safe access)
    std::atomic<bool> isPlaying_{false};
    std::atomic<bool> isRecording_{false};

    // Playback position (in samples)
    std::atomic<juce::int64> playbackPosition{0};

    // Test tone generator (Phase 0 testing)
    double phase{0.0};
    std::atomic<bool> enableTestTone_{false};
#endif

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
    // W10: Phase 1 audio engine (mixer + tracks)
    Mixer mixer_;
    juce::AudioBuffer<float> mixBuffer_;  // Temp buffer for mixer output

    // W10.2: Transport + scheduler (exists only when engine flag is ON)
    std::atomic<int64_t> transportSamples_{0};
    std::atomic<bool>    isPlaying_{false};
    std::atomic<bool>    isRecording_{false};  // For future use
    rt::SpscRing<TransportEvent, 4096> eventQ_;
    std::vector<TransportEvent> dueEventsScratch_;

    // W10.3: Pre-allocated array for due events (RT-safe)
    DueEvent dueEvents_[kMaxEventsPerBlock];

    // Test tone generator (available in Phase 1 for testing)
    double phase{0.0};
    std::atomic<bool> enableTestTone_{false};

#if JUCE_DEBUG
    // Debug: Peak tracking for HUD (RT writes, message thread reads)
    std::atomic<float> debugPeakL_{0.0f};
    std::atomic<float> debugPeakR_{0.0f};
#endif
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
