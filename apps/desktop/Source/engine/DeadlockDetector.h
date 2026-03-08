/*
  ==============================================================================

    DeadlockDetector.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #8)

    Detects and prevents deadlocks in multi-threaded audio applications.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <atomic>

namespace zenith {

//==============================================================================
/**
 * @brief Lock node in the dependency graph
 */
struct LockNode {
    juce::String lockName;
    void* lockAddress = nullptr;  // Address of the mutex
    int threadId = 0;             // Thread currently holding lock
    int lockCount = 0;            // For recursive locks

    juce::String toString() const {
        return lockName + " (thread " + juce::String(threadId) +
               ", count " + juce::String(lockCount) + ")";
    }
};

//==============================================================================
/**
 * @brief Deadlock event
 */
struct DeadlockEvent {
    juce::String description;
    std::vector<juce::String> involvedLocks;
    std::vector<int> involvedThreads;
    double timestamp = 0.0;
    int severity = 0;  // 0-10

    juce::String toString() const {
        juce::String result = "Deadlock detected: " + description;
        result += "\n  Locks: ";
        for (const auto& lock : involvedLocks) {
            result += lock + " ";
        }
        result += "\n  Threads: ";
        for (int thread : involvedThreads) {
            result += juce::String(thread) + " ";
        }
        return result;
    }
};

//==============================================================================
/**
 * @brief Deadlock detector statistics
 */
struct DeadlockDetectorStatistics {
    std::atomic<int> deadlocksDetected{0};
    std::atomic<int> potentialDeadlocks{0};
    std::atomic<int> circularWaits{0};
    std::atomic<int> locksTracked{0};

    juce::String toString() const {
        return "Deadlock Detector: " +
               juce::String(deadlocksDetected.load()) + " deadlocks, " +
               juce::String(potentialDeadlocks.load()) + " potential";
    }
};

//==============================================================================
/**
 * @brief Deadlock detector configuration
 */
struct DeadlockDetectorConfig {
    bool enableDetection = true;
    bool enableCircularWaitDetection = true;
    bool autoReportDeadlocks = true;
    int maxLockWaitTimeMs = 5000;  // Consider deadlock after 5 seconds
    bool trackLockOrder = true;    // Track lock acquisition order
};

//==============================================================================
/**
 * @brief Deadlock detector
 *
 * Features:
 * - Real-time deadlock detection using wait-for graph
 * - Circular wait detection
 * - Lock order tracking
 * - Potential deadlock warnings
 * - Thread-holds lock tracking
 * - Cross-platform
 */
class DeadlockDetector {
public:
    //==========================================================================
    DeadlockDetector();
    ~DeadlockDetector();

    //==========================================================================
    /**
     * @brief Register a lock acquisition attempt
     * Call this BEFORE trying to acquire a lock
     * @param lockName Name/identifier for the lock
     * @param lockAddress Address of the mutex
     */
    void willAcquireLock(const juce::String& lockName, void* lockAddress);

    //==========================================================================
    /**
     * @brief Register successful lock acquisition
     * Call this AFTER successfully acquiring a lock
     * @param lockName Name/identifier for the lock
     * @param lockAddress Address of the mutex
     */
    void didAcquireLock(const juce::String& lockName, void* lockAddress);

    //==========================================================================
    /**
     * @brief Register lock release
     * Call this when releasing a lock
     * @param lockName Name/identifier for the lock
     */
    void didReleaseLock(const juce::String& lockName);

    //==========================================================================
    /**
     * @brief Check for deadlock
     * @return Deadlock event (empty if no deadlock)
     */
    DeadlockEvent checkForDeadlock();

    //==========================================================================
    /**
     * @brief Check for potential deadlock (circular wait)
     * @return List of circular wait paths (empty if safe)
     */
    std::vector<std::vector<juce::String>> checkForCircularWaits();

    //==========================================================================
    /**
     * @brief Get all locks currently held by a thread
     */
    std::vector<LockNode> getLocksHeldByThread(int threadId);

    //==========================================================================
    /**
     * @brief Get all tracked locks
     */
    std::vector<LockNode> getAllLocks() const;

    //==========================================================================
    /**
     * @brief Get deadlock statistics
     */
    DeadlockDetectorStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        statistics_.deadlocksDetected.store(0);
        statistics_.potentialDeadlocks.store(0);
        statistics_.circularWaits.store(0);
        statistics_.locksTracked.store(0);
    }

    //==========================================================================
    /**
     * @brief Get configuration
     */
    DeadlockDetectorConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const DeadlockDetectorConfig& config) {
        config_ = config;
    }

    //==========================================================================
    /**
     * @brief Enable/disable detection
     */
    void setEnabled(bool enable) {
        enabled_ = enable;
    }

    //==========================================================================
    /**
     * @brief Check if detection is enabled
     */
    bool isEnabled() const {
        return enabled_;
    }

    //==========================================================================
    /**
     * @brief Clear all tracking data
     */
    void clear();

private:
    //==========================================================================
    bool detectCycleDFS(const juce::String& lockName,
                       std::set<juce::String>& visited,
                       std::set<juce::String>& recStack,
                       std::vector<juce::String>& path);

    int getCurrentThreadId() const;

    //==========================================================================
    // Lock tracking
    // lockName -> LockNode
    std::map<juce::String, LockNode> locks_;

    // threadId -> {lockName} (locks held by each thread)
    std::map<int, std::set<juce::String>> threadLocks_;

    // Wait-for graph
    // waitingThread -> threadHoldingLock
    std::map<int, int> waitForGraph_;

    // Lock order tracking (for detecting inconsistent ordering)
    // lockA -> {lockB} (lockA acquired before lockB)
    std::map<juce::String, std::set<juce::String>> lockOrder_;

    // Configuration
    DeadlockDetectorConfig config_;
    bool enabled_ = true;

    // Statistics
    DeadlockDetectorStatistics statistics_;

    // Thread safety
    mutable std::mutex mutex_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeadlockDetector)
};

//==============================================================================
/**
 * @brief Singleton accessor for deadlock detector
 */
class DeadlockDetectorHolder {
public:
    static DeadlockDetector& getInstance() {
        static DeadlockDetector instance;
        return instance;
    }

    DeadlockDetectorHolder(const DeadlockDetectorHolder&) = delete;
    DeadlockDetectorHolder& operator=(const DeadlockDetectorHolder&) = delete;

private:
    DeadlockDetectorHolder() = default;
    ~DeadlockDetectorHolder() = default;
};

//==============================================================================
/**
 * @brief RAII helper for deadlock-aware locking
 * Automatically registers lock acquisition/release
 */
class DeadlockAwareLock {
public:
    DeadlockAwareLock(const juce::String& lockName, std::mutex& mutex)
        : lockName_(lockName)
        , mutex_(mutex)
        , locked_(false)
    {
        lock();
    }

    ~DeadlockAwareLock() {
        unlock();
    }

    void lock() {
        if (locked_) return;

        auto& detector = DeadlockDetectorHolder::getInstance();
        detector.willAcquireLock(lockName_, &mutex_);

        mutex_.lock();

        detector.didAcquireLock(lockName_, &mutex_);
        locked_ = true;
    }

    void unlock() {
        if (!locked_) return;

        auto& detector = DeadlockDetectorHolder::getInstance();
        detector.didReleaseLock(lockName_);

        mutex_.unlock();
        locked_ = false;
    }

private:
    juce::String lockName_;
    std::mutex& mutex_;
    bool locked_;
};

} // namespace zenith
