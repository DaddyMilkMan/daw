/**
 * @file RealtimeAudioEngineAgent.h
 * @brief Coordinator agent for real-time audio engine operations
 * 
 * Thread Safety:
 * - coordinateAudioGraph() is MESSAGE THREAD ONLY
 * - All atomic reads are safe from any thread
 * - Never call from audio thread directly
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

namespace zenith {

/**
 * @class RealtimeAudioEngineAgent
 * @brief Coordinates real-time audio engine operations with thread safety
 * 
 * This agent acts as a coordinator between the message thread (UI/control)
 * and the audio thread, ensuring all communications follow the DAW's
 * strict threading model.
 */
class RealtimeAudioEngineAgent {
public:
    RealtimeAudioEngineAgent();
    ~RealtimeAudioEngineAgent();

    /**
     * Initialize the agent with audio system parameters
     * @param sampleRate Sample rate in Hz
     * @param bufferSize Audio buffer size in samples
     */
    void initialize(double sampleRate, int bufferSize);

    /**
     * Coordinate audio graph changes (MESSAGE THREAD ONLY)
     * @return true if coordination was successful
     */
    bool coordinateAudioGraph();

    /**
     * Get current RT performance status (thread-safe)
     * @return true if RT performance is healthy
     */
    bool isRealtimePerformanceHealthy() const;

    /**
     * Get xrun count since initialization (thread-safe)
     * @return Number of buffer underruns/overruns detected
     */
    int getXrunCount() const;

private:
    // Audio parameters
    std::atomic<double> sampleRate_{44100.0};
    std::atomic<int> bufferSize_{512};
    
    // RT performance monitoring (lock-free atomics)
    std::atomic<bool> rtPerformanceHealthy_{true};
    std::atomic<int> xrunCount_{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealtimeAudioEngineAgent)
};

} // namespace zenith
