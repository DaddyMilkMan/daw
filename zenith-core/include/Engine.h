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

#if ZENITH_ENABLE_PHASE1_AUDIO
#include "lockfree/SpscRing.h"
#include "../Source/engine/Mixer.h"
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

#if ZENITH_ENABLE_PHASE1_AUDIO
    //==========================================================================
    // Sample-Accurate Transport (Phase 1)
    //==========================================================================

    /**
     * @brief Transport event types for sample-accurate scheduling
     */
    enum class TransportEventType : uint8_t
    {
        ClipStart,  ///< Start a clip at specified sample
        ClipStop    ///< Stop a clip at specified sample
    };

    /**
     * @brief Sample-accurate transport event
     *
     * Scheduled from message thread, consumed in audio callback
     * All fields are trivially copyable for lock-free queue
     */
    struct TransportEvent
    {
        TransportEventType type;
        int64_t whenSamples;  ///< Absolute sample position
        int trackIndex;       ///< Which track
        int64_t clipId;       ///< Unique clip identifier

        TransportEvent() = default;
        TransportEvent(TransportEventType t, int64_t when, int track, int64_t clip)
            : type(t), whenSamples(when), trackIndex(track), clipId(clip) {}
    };

    /**
     * @brief Schedule a clip to start at specific sample position
     * @param trackIndex Track index (0-based)
     * @param clipId Unique clip identifier
     * @param startSample Absolute sample position to start
     * @return true if scheduled, false if event queue is full
     *
     * Thread-safe: Call from message thread
     */
    bool scheduleClipStart(int trackIndex, int64_t clipId, int64_t startSample);

    /**
     * @brief Schedule a clip to stop at specific sample position
     * @param trackIndex Track index (0-based)
     * @param clipId Unique clip identifier
     * @param stopSample Absolute sample position to stop
     * @return true if scheduled, false if event queue is full
     *
     * Thread-safe: Call from message thread
     */
    bool scheduleClipStop(int trackIndex, int64_t clipId, int64_t stopSample);

    /**
     * @brief Seek to specific sample position
     * @param targetSample Target sample position
     *
     * ⚠️ CRITICAL: You MUST call stop() before seekSamples()
     * Reason: seekSamples() rewrites the event queue (both consumes and produces)
     * This violates SPSC contract if audio thread is also consuming
     * Enforced by jassert(!isPlaying_) in implementation
     *
     * Purges all queued events < targetSample to prevent stale events
     * Preserves chronological order of future events >= targetSample
     *
     * Thread-safe: Call from message thread ONLY when playback is stopped
     *
     * Usage:
     *   engine.stop();
     *   engine.seekSamples(newPosition);
     *   // reschedule clips as needed
     *   engine.play();
     */
    void seekSamples(int64_t targetSample);

    /**
     * @brief Get current transport position in samples
     * @return Current sample position (monotonic)
     */
    int64_t getTransportSamples() const { return transportSamples_.load(); }

    /**
     * @brief Get number of dropped events (queue full)
     * @return Count of events that couldn't be scheduled
     */
    uint64_t getDroppedEvents() const { return droppedEvents_.load(); }

    /**
     * @brief Get mixer for track/clip management
     * @return Reference to mixer
     *
     * Thread-safe: Returns reference, but track/clip modifications must be
     * done when playback is stopped
     */
    zenith::Mixer& getMixer() { return mixer_; }
    const zenith::Mixer& getMixer() const { return mixer_; }

    /**
     * @brief Process audio block offline (no device I/O)
     * @param buffer Destination buffer (caller-provided)
     * @param numSamples Number of samples to process
     *
     * MESSAGE THREAD ONLY - offline rendering
     * Drives same internal processing path as audio callback but writes to
     * caller-supplied buffer instead of device output.
     * Used for offline export/bounce.
     *
     * Notes:
     * - Transport advances by numSamples
     * - Events are drained and processed
     * - Buffer must be pre-allocated (numChannels x numSamples)
     */
    void processBlockOffline(juce::AudioBuffer<float>& buffer, int numSamples);

#endif // ZENITH_ENABLE_PHASE1_AUDIO

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

#if ZENITH_ENABLE_PHASE1_AUDIO
    //==========================================================================
    // Phase 1: Sample-Accurate Scheduling
    //==========================================================================

    /**
     * @brief Event scheduled for specific offset within current block
     */
    struct DueEvent
    {
        TransportEvent ev;
        int offsetInBlock; ///< 0..numSamples-1
    };

    // Lock-free event queue (message thread → audio thread)
    static constexpr size_t kEventRingCap = 4096;
    zenith::SpscRing<TransportEvent> eventQ_{kEventRingCap};

    // Transport sample position (monotonic, updated on audio thread)
    std::atomic<int64_t> transportSamples_{0};

    // Dropped events counter (queue full)
    std::atomic<uint64_t> droppedEvents_{0};

    // Mixer (track/clip rendering)
    zenith::Mixer mixer_;

    // Mix buffer (pre-allocated, reused each block)
    juce::AudioBuffer<float> mixBuffer_;

    // Due events for current block (pre-allocated)
    static constexpr int kMaxEventsPerBlock = 64;
    DueEvent dueEvents_[kMaxEventsPerBlock];
    int numDueEvents_ = 0;

    /**
     * @brief Drain scheduled events for current block into dueEvents_ array
     * @param blockStart Start of block (absolute samples)
     * @param blockEnd End of block (absolute samples)
     * @param numSamples Block length
     * @note AUDIO THREAD - uses peek-then-pop to avoid losing future events
     */
    void drainScheduledEvents(int64_t blockStart, int64_t blockEnd, int numSamples);

#endif // ZENITH_ENABLE_PHASE1_AUDIO

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};
