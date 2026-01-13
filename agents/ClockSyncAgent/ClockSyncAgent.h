/**
 * @file ClockSyncAgent.h
 * @brief Coordinator agent for distributed clock synchronization
 * 
 * Thread Safety:
 * - getSyncedTimestamp() is AUDIO THREAD SAFE (RT-safe, uses atomics)
 * - updateClockOffset() is BACKGROUND THREAD (network callback)
 * - All configuration is MESSAGE THREAD ONLY
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <chrono>

namespace zenith {

/**
 * @struct ClockSyncMetrics
 * @brief Clock synchronization quality metrics
 */
struct ClockSyncMetrics {
    float offsetMs{0.0f};          // Current clock offset
    float driftPpm{0.0f};          // Clock drift in parts per million
    float jitterMs{0.0f};          // Clock jitter
    float confidencePercent{0.0f}; // Synchronization confidence
};

/**
 * @class ClockSyncAgent
 * @brief Manages distributed clock synchronization
 * 
 * This agent handles sample-accurate clock synchronization across
 * networked audio systems, with drift compensation and quality monitoring.
 */
class ClockSyncAgent {
public:
    ClockSyncAgent();
    ~ClockSyncAgent();

    /**
     * @brief Initialize clock synchronization
     * @param sampleRate Local audio sample rate
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void initialize(double sampleRate);

    /**
     * @brief Get synchronized timestamp for current sample (RT-safe)
     * @param localSampleTime Local sample counter
     * @return Synchronized sample timestamp
     * 
     * Thread: AUDIO THREAD (RT-safe)
     */
    int64_t getSyncedTimestamp(int64_t localSampleTime) const;

    /**
     * @brief Update clock offset from network peer
     * @param peerTimestamp Peer's timestamp
     * @param localTimestamp Our timestamp at reception
     * @param roundTripMs Round-trip network latency
     * 
     * Thread: BACKGROUND THREAD (network callback)
     */
    void updateClockOffset(int64_t peerTimestamp,
                          int64_t localTimestamp,
                          float roundTripMs);

    /**
     * @brief Get current clock synchronization metrics
     * @return Sync quality metrics
     * 
     * Thread: Any (uses atomics)
     */
    ClockSyncMetrics getMetrics() const;

    /**
     * @brief Enable/disable drift compensation
     * @param enable True to enable drift compensation
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void setDriftCompensation(bool enable);

    /**
     * @brief Get estimated clock drift
     * @return Drift in parts per million (ppm)
     * 
     * Thread: Any (uses atomics)
     */
    float getClockDrift() const;

private:
    std::atomic<int64_t> clockOffsetSamples_{0};
    std::atomic<float> driftPpm_{0.0f};
    std::atomic<float> confidencePercent_{0.0f};
    std::atomic<bool> driftCompensationEnabled_{false};
    
    double sampleRate_{44100.0};
    
    // TODO: Add clock sample history for drift estimation
    // TODO: Add Kalman filter for offset smoothing
    // TODO: Add NTP-style synchronization protocol
};

} // namespace zenith
