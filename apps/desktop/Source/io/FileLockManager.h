/*
  ==============================================================================

    FileLockManager.h
    Created: 2026-02-19
    Author:  Zenith DAW - Month 8: File I/O Safety (Gap #3)

    File locking management to prevent concurrent access conflicts.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <map>
#include <functional>
#include <mutex>

namespace zenith {

//==============================================================================
/**
 * @brief Lock type
 */
enum class LockType {
    Shared,       // Multiple readers allowed
    Exclusive     // Single writer, no readers
};

//==============================================================================
/**
 * @brief Lock status
 */
enum class LockStatus {
    NotLocked,
    Locked,
    Conflict,
    Timeout,
    Error
};

//==============================================================================
/**
 * @brief File lock information
 */
struct FileLockInfo {
    juce::File file;
    LockType type = LockType::Exclusive;
    juce::String ownerId;        // Who owns the lock
    double lockTime = 0.0;       // When lock was acquired
    LockStatus status = LockStatus::NotLocked;
    juce::String errorMessage;

    juce::String toString() const {
        juce::String typeStr = (type == LockType::Shared) ? "Shared" : "Exclusive";
        juce::String statusStr;
        switch (status) {
            case LockStatus::NotLocked: statusStr = "Not Locked"; break;
            case LockStatus::Locked: statusStr = "Locked"; break;
            case LockStatus::Conflict: statusStr = "Conflict"; break;
            case LockStatus::Timeout: statusStr = "Timeout"; break;
            case LockStatus::Error: statusStr = "Error"; break;
        }
        return file.getFileName() + " [" + typeStr + "] " + statusStr;
    }
};

//==============================================================================
/**
 * @brief Lock conflict event
 */
struct LockConflictEvent {
    juce::File file;
    juce::String requester;
    juce::String currentOwner;
    LockType requestedType = LockType::Exclusive;
    double timestamp = 0.0;
    int severity = 0;  // 0-10

    juce::String toString() const {
        return "Lock conflict: " + file.getFileName() +
               " (requested by " + requester +
               ", held by " + currentOwner + ")";
    }
};

//==============================================================================
/**
 * @brief File lock manager
 *
 * Features:
 * - Detect file-in-use conflicts
 * - Shared vs exclusive locking
 * - Lock timeout handling
 * - Deadlock prevention
 * - Cross-platform locking
 */
class FileLockManager {
public:
    //==========================================================================
    FileLockManager();
    ~FileLockManager();

    //==========================================================================
    /**
     * @brief Try to acquire lock on file
     * @param file File to lock
     * @param type Lock type
     * @param ownerId Requester identifier
     * @param timeoutMs Timeout in milliseconds (0 = no wait)
     * @return Lock info
     */
    FileLockInfo acquireLock(const juce::File& file,
                           LockType type,
                           const juce::String& ownerId,
                           int timeoutMs = 0);

    //==========================================================================
    /**
     * @brief Release lock on file
     * @param file File to unlock
     * @param ownerId Owner identifier
     * @return true if released successfully
     */
    bool releaseLock(const juce::File& file,
                    const juce::String& ownerId);

    //==========================================================================
    /**
     * @brief Check if file is locked
     * @param file File to check
     * @return true if locked
     */
    bool isLocked(const juce::File& file) const;

    //==========================================================================
    /**
     * @brief Get lock info for file
     * @param file File to query
     * @return Lock info
     */
    FileLockInfo getLockInfo(const juce::File& file) const;

    //==========================================================================
    /**
     * @brief Get all active locks
     * @return List of lock infos
     */
    std::vector<FileLockInfo> getAllLocks() const;

    //==========================================================================
    /**
     * @brief Release all locks owned by owner
     * @param ownerId Owner identifier
     * @return Number of locks released
     */
    int releaseAllLocks(const juce::String& ownerId);

    //==========================================================================
    /**
     * @brief Check for lock conflicts
     * @param file File to check
     * @param requester Requester identifier
     * @return Conflict event (empty if no conflict)
     */
    LockConflictEvent checkConflict(const juce::File& file,
                                   const juce::String& requester) const;

    //==========================================================================
    /**
     * @brief Enable/disable deadlock detection
     */
    void setDeadlockDetectionEnabled(bool enable) {
        deadlockDetectionEnabled_ = enable;
    }

    //==========================================================================
    /**
     * @brief Get lock timeout
     */
    int getDefaultTimeout() const {
        return defaultTimeoutMs_;
    }

    //==========================================================================
    /**
     * @brief Set default lock timeout
     */
    void setDefaultTimeout(int timeoutMs) {
        defaultTimeoutMs_ = timeoutMs;
    }

private:
    //==========================================================================
    bool detectDeadlock(const juce::File& file,
                      const juce::String& ownerId,
                      LockType type);

    //==========================================================================
    // Active locks: filePath -> lock info
    std::map<juce::String, FileLockInfo> activeLocks_;
    mutable std::mutex locksMutex_;

    // Settings
    int defaultTimeoutMs_ = 5000;  // 5 second default timeout
    bool deadlockDetectionEnabled_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileLockManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for file lock manager
 */
class FileLockManagerHolder {
public:
    static FileLockManager& getInstance() {
        static FileLockManager instance;
        return instance;
    }

    FileLockManagerHolder(const FileLockManagerHolder&) = delete;
    FileLockManagerHolder& operator=(const FileLockManagerHolder&) = delete;

private:
    FileLockManagerHolder() = default;
    ~FileLockManagerHolder() = default;
};

} // namespace zenith
