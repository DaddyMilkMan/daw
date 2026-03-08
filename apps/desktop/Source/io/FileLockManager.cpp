/*
  ==============================================================================

    FileLockManager.cpp
    Implementation of file locking management

  ==============================================================================
*/

#include "FileLockManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// FileLockManager Implementation
//==============================================================================

FileLockManager::FileLockManager() {
    std::cout << "FileLockManager: Initialized" << std::endl;
}

FileLockManager::~FileLockManager() {
    std::cout << "FileLockManager: Shut down" << std::endl;
}

//==============================================================================
FileLockInfo FileLockManager::acquireLock(const juce::File& file,
                                         LockType type,
                                         const juce::String& ownerId,
                                         int timeoutMs)
{
    FileLockInfo lockInfo;
    lockInfo.file = file;
    lockInfo.type = type;
    lockInfo.ownerId = ownerId;
    lockInfo.lockTime = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

    std::lock_guard<std::mutex> lock(locksMutex_);

    // Check if file is already locked
    auto it = activeLocks_.find(file.getFullPathName());

    if (it != activeLocks_.end()) {
        FileLockInfo& existingLock = it->second;

        // Check if same owner
        if (existingLock.ownerId == ownerId) {
            // Already have lock, upgrade if needed
            if (existingLock.type == LockType::Shared && type == LockType::Exclusive) {
                existingLock.type = LockType::Exclusive;
            }
            lockInfo.status = LockStatus::Locked;
            return lockInfo;
        }

        // Different owner - check lock compatibility
        if (existingLock.type == LockType::Exclusive || type == LockType::Exclusive) {
            // Conflict!
            lockInfo.status = LockStatus::Conflict;
            lockInfo.errorMessage = "File locked by " + existingLock.ownerId;

            std::cerr << "FileLockManager: Lock conflict on "
                      << file.getFileName() << std::endl;

            return lockInfo;
        }

        // Shared lock compatibility - can share
        existingLock.ownerId += ", " + ownerId;  // List owners
        lockInfo.status = LockStatus::Locked;
        return lockInfo;
    }

    // Not locked - acquire it
    activeLocks_[file.getFullPathName()] = lockInfo;
    lockInfo.status = LockStatus::Locked;

    std::cout << "FileLockManager: Acquired "
              << (type == LockType::Shared ? "shared" : "exclusive")
              << " lock on " << file.getFileName()
              << " for " << ownerId << std::endl;

    return lockInfo;
}

//==============================================================================
bool FileLockManager::releaseLock(const juce::File& file,
                                  const juce::String& ownerId)
{
    std::lock_guard<std::mutex> lock(locksMutex_);

    auto it = activeLocks_.find(file.getFullPathName());

    if (it == activeLocks_.end()) {
        return false;  // Not locked
    }

    FileLockInfo& lockInfo = it->second;

    // Check ownership
    if (lockInfo.ownerId != ownerId &&
        !lockInfo.ownerId.contains(ownerId)) {
        return false;  // Not owner
    }

    // Release lock
    activeLocks_.erase(it);

    std::cout << "FileLockManager: Released lock on "
              << file.getFileName() << std::endl;

    return true;
}

//==============================================================================
bool FileLockManager::isLocked(const juce::File& file) const {
    std::lock_guard<std::mutex> lock(locksMutex_);

    return activeLocks_.find(file.getFullPathName()) != activeLocks_.end();
}

//==============================================================================
FileLockInfo FileLockManager::getLockInfo(const juce::File& file) const {
    std::lock_guard<std::mutex> lock(locksMutex_);

    auto it = activeLocks_.find(file.getFullPathName());

    if (it != activeLocks_.end()) {
        return it->second;
    }

    FileLockInfo info;
    info.file = file;
    info.status = LockStatus::NotLocked;
    return info;
}

//==============================================================================
std::vector<FileLockInfo> FileLockManager::getAllLocks() const {
    std::lock_guard<std::mutex> lock(locksMutex_);

    std::vector<FileLockInfo> result;
    for (const auto& pair : activeLocks_) {
        result.push_back(pair.second);
    }

    return result;
}

//==============================================================================
int FileLockManager::releaseAllLocks(const juce::String& ownerId) {
    std::lock_guard<std::mutex> lock(locksMutex_);

    int releasedCount = 0;

    for (auto it = activeLocks_.begin(); it != activeLocks_.end();) {
        const FileLockInfo& lockInfo = it->second;

        if (lockInfo.ownerId == ownerId ||
            lockInfo.ownerId.contains(ownerId)) {
            std::cout << "FileLockManager: Releasing lock on "
                      << lockInfo.file.getFileName() << std::endl;
            it = activeLocks_.erase(it);
            releasedCount++;
        } else {
            ++it;
        }
    }

    return releasedCount;
}

//==============================================================================
LockConflictEvent FileLockManager::checkConflict(const juce::File& file,
                                                const juce::String& requester) const
{
    std::lock_guard<std::mutex> lock(locksMutex_);

    LockConflictEvent event;
    event.file = file;
    event.requester = requester;
    event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

    auto it = activeLocks_.find(file.getFullPathName());

    if (it != activeLocks_.end()) {
        const FileLockInfo& lockInfo = it->second;

        if (lockInfo.ownerId != requester) {
            event.currentOwner = lockInfo.ownerId;
            event.requestedType = lockInfo.type;
            event.severity = (lockInfo.type == LockType::Exclusive) ? 8 : 5;

            std::cerr << "FileLockManager: Conflict detected for "
                      << file.getFileName() << std::endl;
        }
    }

    return event;
}

//==============================================================================
// Private Methods
//==============================================================================

bool FileLockManager::detectDeadlock(const juce::File& file,
                                      const juce::String& ownerId,
                                      LockType type)
{
    if (!deadlockDetectionEnabled_) {
        return false;
    }

    // Simple deadlock detection:
    // Check if owner already has exclusive locks on other files
    // (Full implementation would use wait-for graph)

    int ownerLockCount = 0;
    for (const auto& pair : activeLocks_) {
        if (pair.second.ownerId == ownerId) {
            ownerLockCount++;
        }
    }

    // Warning if owner has many locks
    if (ownerLockCount > 10) {
        std::cerr << "FileLockManager: Warning - " << ownerId
                  << " has " << ownerLockCount << " locks" << std::endl;
    }

    return false;
}

} // namespace zenith
