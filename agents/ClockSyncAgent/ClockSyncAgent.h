/**
 * @file ClockSyncAgent.h
 * @brief Agent for clock synchronization and drift correction
 * 
 * Thread Safety:
 * - Configuration methods are MESSAGE THREAD ONLY
 * - updateSync() can be called from AUDIO THREAD (RT-safe)
 * - Uses atomics for sync state communication
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace zenith {

/**
 * @enum SyncSource
 * @brief Clock synchronization sources
 */
enum class SyncSource {
    Internal,       // Use internal sample clock
    MidiClock,      // Sync to MIDI clock
    MTC,            // Sync to MIDI Time Code
    WordClock,      // Sync to word clock input
    NetworkPTP,     // Sync to Precision Time Protocol
    AbletonLink     // Sync to Ableton Link
};

/**
 * @enum SyncStatus
 * @brief Clock synchronization status
 */
enum class SyncStatus {
    Unlocked,       // Not synchronized
    Locking,        // Attempting to achieve lock
    Locked,         // Synchronized and stable
    Drifting,       // Synchronized but drifting
    Lost            // Sync lost
};

/**
 * @class ClockSyncAgent
 * @brief Manages clock synchronization with external sources
 */
class ClockSyncAgent {
public:
    ClockSyncAgent();
    ~ClockSyncAgent();

    /**
     * Initialize the agent
     * MESSAGE THREAD ONLY
     */
    void initialize(double sampleRate);

    /**
     * Set the active sync source
     * MESSAGE THREAD ONLY
     * 
     * @param source Sync source to use
     */
    void setSyncSource(SyncSource source);

    /**
     * Update synchronization state from audio thread
     * AUDIO THREAD SAFE - RT-safe, lock-free
     * 
     * @param externalTimestamp Timestamp from external clock source
     * @param localSamplePosition Current local sample position
     */
    void updateSync(int64_t externalTimestamp, int64_t localSamplePosition);

    /**
     * Get current clock drift in samples
     * Thread-safe via atomic
     * 
     * @return Drift amount (positive = running ahead, negative = behind)
     */
    int64_t getDriftSamples() const;

    /**
     * Get current sync status
     * Thread-safe via atomic
     */
    SyncStatus getSyncStatus() const;

    /**
     * Get drift in milliseconds
     * Thread-safe via atomic
     */
    double getDriftMs() const;

    /**
     * Generate sync status report
     * MESSAGE THREAD ONLY
     */
    std::string generateStatusReport();

private:
    double sampleRate_{44100.0};
    SyncSource syncSource_{SyncSource::Internal};
    
    // Atomic sync state
    std::atomic<int64_t> driftSamples_{0};
    std::atomic<SyncStatus> syncStatus_{SyncStatus::Unlocked};
    std::atomic<int64_t> lastExternalTimestamp_{0};
    std::atomic<int64_t> lastLocalSample_{0};
    
    // Drift tolerance in samples
    static constexpr int64_t kDriftToleranceSamples = 64;
    
    void updateSyncStatus();
};

} // namespace zenith
