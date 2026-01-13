/**
 * @file RealtimeAudioEngineAgent.h
 * @brief Agent for monitoring and optimizing realtime audio engine performance
 * 
 * Thread Safety:
 * - All public methods are MESSAGE THREAD ONLY unless marked otherwise
 * - collectMetrics() can be called from AUDIO THREAD (RT-safe)
 * - Uses atomics for cross-thread communication
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace zenith {

/**
 * @class RealtimeAudioEngineAgent
 * @brief Monitors and optimizes the realtime audio processing pipeline
 * 
 * This agent tracks audio thread performance metrics and provides
 * recommendations for optimization. It operates in a lock-free manner
 * to avoid interfering with audio processing.
 */
class RealtimeAudioEngineAgent {
public:
    RealtimeAudioEngineAgent();
    ~RealtimeAudioEngineAgent();

    /**
     * Initialize the agent with engine configuration
     * MESSAGE THREAD ONLY
     */
    void initialize(double sampleRate, int bufferSize);

    /**
     * Collect performance metrics from audio thread
     * AUDIO THREAD SAFE - RT-safe, lock-free
     * 
     * @param callbackDurationUs Duration of audio callback in microseconds
     * @param bufferUnderrun Whether an underrun occurred
     */
    void collectMetrics(double callbackDurationUs, bool bufferUnderrun);

    /**
     * Analyze collected metrics and generate recommendations
     * MESSAGE THREAD ONLY
     * 
     * @return Performance report string
     */
    std::string analyzePerformance();

    /**
     * Get current audio load percentage
     * Thread-safe via atomic
     */
    float getAudioLoad() const;

private:
    // Configuration
    double sampleRate_{44100.0};
    int bufferSize_{512};
    
    // Atomic metrics (safe from any thread)
    std::atomic<double> maxCallbackDuration_{0.0};
    std::atomic<uint64_t> underrunCount_{0};
    std::atomic<float> currentLoad_{0.0f};
    
    // Message thread state
    uint64_t totalCallbacks_{0};
};

} // namespace zenith
