/**
 * @file RealtimeAudioEngineAgent.h
 * @brief Coordinator agent for real-time audio engine operations
 * 
 * Thread Safety:
 * - processAudioCallback() is AUDIO THREAD ONLY (RT-safe)
 * - All other methods are MESSAGE THREAD ONLY unless otherwise noted
 * - getMetrics() uses atomics (safe from any thread)
 */

#pragma once

#include <atomic>
#include <memory>
#include <vector>

namespace zenith {

/**
 * @class RealtimeAudioEngineAgent
 * @brief Manages real-time audio processing coordination and monitoring
 * 
 * This agent coordinates audio thread operations, ensuring lock-free
 * communication and maintaining performance metrics.
 */
class RealtimeAudioEngineAgent {
public:
    RealtimeAudioEngineAgent();
    ~RealtimeAudioEngineAgent();

    /**
     * @brief Initialize the agent with audio configuration
     * @param sampleRate Audio device sample rate
     * @param bufferSize Audio device buffer size
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void initialize(double sampleRate, int bufferSize);

    /**
     * @brief Process audio callback (RT-safe)
     * @param inputBuffers Input audio buffers
     * @param outputBuffers Output audio buffers
     * @param numChannels Number of channels
     * @param numSamples Number of samples to process
     * 
     * Thread: AUDIO THREAD ONLY
     * RT-Safe: Yes (no allocation, no locking)
     */
    void processAudioCallback(const float** inputBuffers,
                              float** outputBuffers,
                              int numChannels,
                              int numSamples);

    /**
     * @brief Get current CPU usage estimate
     * @return CPU usage as percentage (0.0 - 100.0)
     * 
     * Thread: Any (uses atomics)
     */
    float getCpuUsage() const;

    /**
     * @brief Get buffer underrun count
     * @return Number of underruns since initialization
     * 
     * Thread: Any (uses atomics)
     */
    int getUnderrunCount() const;

private:
    std::atomic<float> cpuUsage_{0.0f};
    std::atomic<int> underrunCount_{0};
    std::atomic<bool> initialized_{false};
    
    double sampleRate_{44100.0};
    int bufferSize_{512};
    
    // TODO: Add lock-free FIFO for command passing
    // TODO: Add buffer pool management
    // TODO: Add audio graph snapshot
};

} // namespace zenith
