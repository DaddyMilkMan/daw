/*
  ==============================================================================

    RealTimeThreadManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #8)

    Manages real-time audio thread safety, priorities, and communication.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

namespace zenith {

//==============================================================================
/**
 * @brief Thread safety violation type
 */
enum class ThreadSafetyViolation {
    PriorityInversion,      // Low-priority thread blocking high-priority
    DeadlockDetected,       // Deadlock detected
    RaceCondition,          // Potential race condition
    UnsafeMemoryAccess,     // Unsafe memory sharing
    RealTimeViolation,      // Non-real-time-safe operation in audio thread
    Unknown
};

//==============================================================================
/**
 * @brief Thread safety event
 */
struct ThreadSafetyEvent {
    ThreadSafetyViolation type;
    juce::String description;
    double timestamp = 0.0;
    int threadId = 0;
    int severity = 0;  // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case ThreadSafetyViolation::PriorityInversion: typeStr = "Priority Inversion"; break;
            case ThreadSafetyViolation::DeadlockDetected: typeStr = "Deadlock"; break;
            case ThreadSafetyViolation::RaceCondition: typeStr = "Race Condition"; break;
            case ThreadSafetyViolation::UnsafeMemoryAccess: typeStr = "Unsafe Memory"; break;
            case ThreadSafetyViolation::RealTimeViolation: typeStr = "RT Violation"; break;
            case ThreadSafetyViolation::Unknown: typeStr = "Unknown"; break;
        }
        return "[" + typeStr + "] " + description + " (thread " + juce::String(threadId) + ")";
    }
};

//==============================================================================
/**
 * @brief Thread priority level
 */
enum class ThreadPriority {
    Low,
    Normal,
    High,
    RealTime,         // Audio thread priority
    Critical
};

//==============================================================================
/**
 * @brief Thread statistics
 */
struct ThreadStatistics {
    std::atomic<int> totalViolations{0};
    std::atomic<int> priorityInversions{0};
    std::atomic<int> deadlocks{0};
    std::atomic<int> raceConditions{0};
    std::atomic<int> realTimeViolations{0};
    std::atomic<double> averageLockWaitTime{0.0};  // microseconds

    juce::String toString() const {
        return "Thread Safety: " +
               juce::String(totalViolations.load()) + " violations (" +
               juce::String(priorityInversions.load()) + " priority inversions, " +
               juce::String(deadlocks.load()) + " deadlocks)";
    }
};

//==============================================================================
/**
 * @brief Real-time thread configuration
 */
struct RealTimeThreadConfig {
    bool enableRealTimePriority = true;
    int realTimePriorityLevel = 10;  // Platform-specific
    bool enablePriorityInversionDetection = true;
    bool enableDeadlockDetection = true;
    double maxLockWaitTimeUs = 100.0;  // Max acceptable lock wait (100us)
    bool enableStrictMode = false;     // Fail immediately on violation
};

//==============================================================================
/**
 * @brief Real-time thread manager
 *
 * Features:
 * - Audio thread priority management
 * - Priority inversion detection
 * - Thread safety validation
 * - Real-time safety enforcement
 * - Lock time monitoring
 * - Cross-platform support
 */
class RealTimeThreadManager {
public:
    //==========================================================================
    RealTimeThreadManager();
    ~RealTimeThreadManager();

    //==========================================================================
    /**
     * @brief Set current thread priority
     * @return true if successful
     */
    bool setThreadPriority(ThreadPriority priority);

    //==========================================================================
    /**
     * @brief Get current thread priority
     */
    ThreadPriority getCurrentThreadPriority() const;

    //==========================================================================
    /**
     * @brief Enter real-time audio thread
     * Call this at the start of your audio callback
     */
    void enterRealTimeThread();

    //==========================================================================
    /**
     * @brief Exit real-time audio thread
     * Call this at the end of your audio callback
     */
    void exitRealTimeThread();

    //==========================================================================
    /**
     * @brief Check if we're in real-time thread
     */
    bool isInRealTimeThread() const {
        return inRealTimeThread_.load();
    }

    //==========================================================================
    /**
     * @brief Register a lock acquisition
     * Call this before acquiring a mutex in audio thread
     */
    void willAcquireLock();

    //==========================================================================
    /**
     * @brief Register a lock acquisition
     * Call this after acquiring a mutex
     * @param waitTimeUs Time spent waiting for lock (microseconds)
     */
    void didAcquireLock(double waitTimeUs);

    //==========================================================================
    /**
     * @brief Report thread safety violation
     */
    void reportViolation(const ThreadSafetyEvent& event);

    //==========================================================================
    /**
     * @brief Get recent violations
     */
    std::vector<ThreadSafetyEvent> getRecentViolations() const;

    //==========================================================================
    /**
     * @brief Clear violation history
     */
    void clearViolations();

    //==========================================================================
    /**
     * @brief Get statistics
     */
    ThreadStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

    //==========================================================================
    /**
     * @brief Get configuration
     */
    RealTimeThreadConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const RealTimeThreadConfig& config) {
        config_ = config;
    }

    //==========================================================================
    /**
     * @brief Register callback for violations
     * @param callback Function to call when violation detected
     */
    void setViolationCallback(std::function<void(const ThreadSafetyEvent&)> callback) {
        violationCallback_ = callback;
    }

    //==========================================================================
    /**
     * @brief Validate operation is real-time safe
     * Use this to check if an operation should be performed in audio thread
     * @param operationDescription Description of operation to validate
     * @return true if operation is safe
     */
    bool validateRealTimeOperation(const juce::String& operationDescription) const;

    //==========================================================================
    /**
     * @brief Get thread name for current thread
     */
    static juce::String getCurrentThreadName();

    //==========================================================================
    /**
     * @brief Get platform-specific thread ID
     */
    static int getCurrentThreadId();

private:
    //==========================================================================
    void recordViolation(const ThreadSafetyEvent& event);
    void checkForPriorityInversion(double lockWaitTime);

    //==========================================================================
    // State
    std::atomic<bool> inRealTimeThread_{false};

    // Configuration
    RealTimeThreadConfig config_;

    // Violation tracking
    std::vector<ThreadSafetyEvent> violations_;
    static constexpr int maxViolations = 100;
    mutable std::mutex violationsMutex_;

    // Statistics
    ThreadStatistics statistics_;

    // Callbacks
    std::function<void(const ThreadSafetyEvent&)> violationCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeThreadManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for real-time thread manager
 */
class RealTimeThreadManagerHolder {
public:
    static RealTimeThreadManager& getInstance() {
        static RealTimeThreadManager instance;
        return instance;
    }

    RealTimeThreadManagerHolder(const RealTimeThreadManagerHolder&) = delete;
    RealTimeThreadManagerHolder& operator=(const RealTimeThreadManagerHolder&) = delete;

private:
    RealTimeThreadManagerHolder() = default;
    ~RealTimeThreadManagerHolder() = default;
};

//==============================================================================
/**
 * @brief Real-time safe lock guard
 * Monitors lock acquisition time and reports violations
 */
class RealTimeLockGuard {
public:
    explicit RealTimeLockGuard(std::mutex& mutex)
        : mutex_(mutex)
        , locked_(false)
    {
        lock();
    }

    ~RealTimeLockGuard() {
        unlock();
    }

    void lock() {
        if (locked_) return;

        auto& manager = RealTimeThreadManagerHolder::getInstance();
        manager.willAcquireLock();

        auto startTime = std::chrono::high_resolution_clock::now();
        mutex_.lock();
        auto endTime = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> waitTime = endTime - startTime;

        manager.didAcquireLock(waitTime.count());
        locked_ = true;
    }

    void unlock() {
        if (!locked_) return;
        mutex_.unlock();
        locked_ = false;
    }

private:
    std::mutex& mutex_;
    bool locked_;
};

} // namespace zenith
