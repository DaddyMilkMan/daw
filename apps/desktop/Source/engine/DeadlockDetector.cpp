/*
  ==============================================================================

    DeadlockDetector.cpp
    Implementation of deadlock detection

  ==============================================================================
*/

#include "DeadlockDetector.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// DeadlockDetector Implementation
//==============================================================================

DeadlockDetector::DeadlockDetector() {
    std::cout << "DeadlockDetector: Initialized" << std::endl;
}

DeadlockDetector::~DeadlockDetector() {
    std::cout << "DeadlockDetector: Shut down ("
              << statistics_.deadlocksDetected.load() << " deadlocks detected)" << std::endl;
}

//==============================================================================
void DeadlockDetector::willAcquireLock(const juce::String& lockName, void* lockAddress) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    int threadId = getCurrentThreadId();

    // Record that this thread is waiting for this lock
    // Find which thread (if any) currently holds this lock
    auto it = locks_.find(lockName);
    if (it != locks_.end() && it->second.threadId != threadId) {
        // Another thread holds this lock - add to wait-for graph
        waitForGraph_[threadId] = it->second.threadId;
    }
}

//==============================================================================
void DeadlockDetector::didAcquireLock(const juce::String& lockName, void* lockAddress) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    int threadId = getCurrentThreadId();

    // Remove from wait-for graph (no longer waiting)
    waitForGraph_.erase(threadId);

    // Update lock info
    LockNode& node = locks_[lockName];
    node.lockName = lockName;
    node.lockAddress = lockAddress;
    node.threadId = threadId;
    node.lockCount++;

    // Add to thread's held locks
    threadLocks_[threadId].insert(lockName);

    // Update statistics
    statistics_.locksTracked.store(static_cast<int>(locks_.size()));

    // Track lock order
    if (config_.trackLockOrder) {
        for (const auto& heldLock : threadLocks_[threadId]) {
            if (heldLock != lockName) {
                lockOrder_[heldLock].insert(lockName);
            }
        }
    }
}

//==============================================================================
void DeadlockDetector::didReleaseLock(const juce::String& lockName) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    int threadId = getCurrentThreadId();

    // Decrement lock count
    auto it = locks_.find(lockName);
    if (it != locks_.end()) {
        it->second.lockCount--;
        if (it->second.lockCount <= 0) {
            // Lock is no longer held
            it->second.threadId = 0;
            locks_.erase(it);
        }
    }

    // Remove from thread's held locks
    threadLocks_[threadId].erase(lockName);
}

//==============================================================================
DeadlockEvent DeadlockDetector::checkForDeadlock() {
    if (!enabled_) {
        return DeadlockEvent{};
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Check for cycles in wait-for graph
    std::set<int> visited;
    std::set<int> recStack;
    std::vector<int> path;

    for (const auto& pair : waitForGraph_) {
        int threadId = pair.first;
        if (visited.find(threadId) == visited.end()) {
            // Check for cycle from this thread
            std::vector<int> currentPath;
            std::set<int> currentVisited;
            std::set<int> currentRecStack;

            if (detectCycleDFS(threadId, currentVisited, currentRecStack, currentPath)) {
                // Found a deadlock!
                DeadlockEvent event;
                event.description = "Circular wait detected in wait-for graph";
                event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
                event.severity = 10;

                // Add involved threads
                for (int t : currentPath) {
                    event.involvedThreads.push_back(t);
                }

                // Add involved locks
                for (int thread : event.involvedThreads) {
                    auto it = threadLocks_.find(thread);
                    if (it != threadLocks_.end()) {
                        for (const auto& lockName : it->second) {
                            event.involvedLocks.push_back(lockName);
                        }
                    }
                }

                statistics_.deadlocksDetected++;

                if (config_.autoReportDeadlocks) {
                    std::cerr << "DeadlockDetector: " << event.toString() << std::endl;
                }

                return event;
            }
        }
    }

    return DeadlockEvent{};  // No deadlock
}

//==============================================================================
std::vector<std::vector<juce::String>> DeadlockDetector::checkForCircularWaits() {
    std::vector<std::vector<juce::String>> circularWaits;

    if (!enabled_ || !config_.enableCircularWaitDetection) {
        return circularWaits;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::set<juce::String> visited;
    std::set<juce::String> recStack;

    for (const auto& pair : lockOrder_) {
        const juce::String& lockName = pair.first;
        if (visited.find(lockName) == visited.end()) {
            std::vector<juce::String> path;
            if (detectCycleDFS(lockName, visited, recStack, path)) {
                circularWaits.push_back(path);
                statistics_.circularWaits++;
                statistics_.potentialDeadlocks++;
            }
        }
    }

    return circularWaits;
}

//==============================================================================
std::vector<LockNode> DeadlockDetector::getLocksHeldByThread(int threadId) {
    std::vector<LockNode> result;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = threadLocks_.find(threadId);
    if (it != threadLocks_.end()) {
        for (const auto& lockName : it->second) {
            auto lockIt = locks_.find(lockName);
            if (lockIt != locks_.end()) {
                result.push_back(lockIt->second);
            }
        }
    }

    return result;
}

//==============================================================================
std::vector<LockNode> DeadlockDetector::getAllLocks() const {
    std::vector<LockNode> result;

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& pair : locks_) {
        result.push_back(pair.second);
    }

    return result;
}

//==============================================================================
void DeadlockDetector::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    locks_.clear();
    threadLocks_.clear();
    waitForGraph_.clear();
    lockOrder_.clear();

    std::cout << "DeadlockDetector: Cleared all tracking data" << std::endl;
}

//==============================================================================
// Private Methods
//==============================================================================

bool DeadlockDetector::detectCycleDFS(const juce::String& lockName,
                                     std::set<juce::String>& visited,
                                     std::set<juce::String>& recStack,
                                     std::vector<juce::String>& path)
{
    if (recStack.find(lockName) != recStack.end()) {
        // Found a cycle
        path.push_back(lockName);
        return true;
    }

    if (visited.find(lockName) != visited.end()) {
        return false;  // Already visited, no cycle from here
    }

    visited.insert(lockName);
    recStack.insert(lockName);
    path.push_back(lockName);

    // Visit all locks acquired after this one
    auto it = lockOrder_.find(lockName);
    if (it != lockOrder_.end()) {
        for (const auto& nextLock : it->second) {
            if (detectCycleDFS(nextLock, visited, recStack, path)) {
                return true;
            }
        }
    }

    // Backtrack
    recStack.erase(lockName);
    path.pop_back();

    return false;
}

int DeadlockDetector::getCurrentThreadId() const {
    return static_cast<int>(reinterpret_cast<uintptr_t>(juce::Thread::getCurrentThreadId()));
}

} // namespace zenith
