/**
 * @file ClockSyncAgent.h
 * @brief Coordinator agent for distributed clock synchronization
 * 
 * Thread Safety:
 * - processSyncPacket() can be called from BACKGROUND THREADS
 * - Clock adjustments are atomic
 * - Uses lock-free data structures for metrics
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <chrono>
#include <memory>

namespace zenith {

/**
 * @class ClockSyncAgent
 * @brief Manages clock synchronization across distributed systems
 */
class ClockSyncAgent {
public:
    ClockSyncAgent();
    ~ClockSyncAgent();

    /**
     * Initialize the agent
     * @param localSampleRate Local system sample rate
     */
    void initialize(double localSampleRate);

    /**
     * Process incoming sync packet (BACKGROUND THREAD SAFE)
     * @param remoteTimestamp Remote system timestamp
     * @param localReceiveTime Local time when packet was received
     */
    void processSyncPacket(
        std::chrono::microseconds remoteTimestamp,
        std::chrono::microseconds localReceiveTime);

    /**
     * Get current clock offset in microseconds (thread-safe)
     * @return Estimated offset from remote clock
     */
    std::chrono::microseconds getClockOffset() const;

    /**
     * Get sync quality metric (thread-safe)
     * @return Quality metric (0.0 = poor, 1.0 = excellent)
     */
    double getSyncQuality() const;

    /**
     * Get estimated round-trip latency (thread-safe)
     * @return Latency in microseconds
     */
    std::chrono::microseconds getEstimatedLatency() const;

private:
    std::atomic<double> sampleRate_{44100.0};
    std::atomic<int64_t> clockOffsetMicros_{0};
    std::atomic<double> syncQuality_{0.0};
    std::atomic<int64_t> estimatedLatencyMicros_{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClockSyncAgent)
};

} // namespace zenith
