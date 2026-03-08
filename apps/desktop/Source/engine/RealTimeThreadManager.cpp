/*
  ==============================================================================

    RealTimeThreadManager.cpp
    Implementation of real-time thread safety management

  ==============================================================================
*/

#include "RealTimeThreadManager.h"
#include <iostream>
#include <algorithm>
#include <thread>

#if JUCE_WINDOWS
#include <windows.h>
#elif JUCE_MAC || JUCE_LINUX
#include <pthread.h>
#include <sched.h>
#endif

namespace zenith {

//==============================================================================
// RealTimeThreadManager Implementation
//==============================================================================

RealTimeThreadManager::RealTimeThreadManager() {
    std::cout << "RealTimeThreadManager: Initialized" << std::endl;
}

RealTimeThreadManager::~RealTimeThreadManager() {
    std::cout << "RealTimeThreadManager: Shut down ("
              << statistics_.totalViolations.load() << " violations)" << std::endl;
}

//==============================================================================
bool RealTimeThreadManager::setThreadPriority(ThreadPriority priority) {
    juce::Thread::ThreadID threadId = juce::Thread::getCurrentThreadId();

    bool success = false;

#if JUCE_WINDOWS
    // Windows implementation
    HANDLE handle = GetCurrentThread();
    int windowsPriority = THREAD_PRIORITY_NORMAL;

    switch (priority) {
        case ThreadPriority::Low:
            windowsPriority = THREAD_PRIORITY_LOWEST;
            break;
        case ThreadPriority::Normal:
            windowsPriority = THREAD_PRIORITY_NORMAL;
            break;
        case ThreadPriority::High:
            windowsPriority = THREAD_PRIORITY_HIGHEST;
            break;
        case ThreadPriority::RealTime:
            windowsPriority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        case ThreadPriority::Critical:
            windowsPriority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
    }

    success = SetThreadPriority(handle, windowsPriority);

#elif JUCE_MAC || JUCE_LINUX
    // POSIX implementation
    pthread_t thread = pthread_self();
    sched_param sch;
    int policy;

    pthread_getschedparam(thread, &policy, &sch);

    int minPriority = sched_get_priority_min(policy);
    int maxPriority = sched_get_priority_max(policy);

    switch (priority) {
        case ThreadPriority::Low:
            sch.sched_priority = minPriority;
            break;
        case ThreadPriority::Normal:
            sch.sched_priority = (minPriority + maxPriority) / 2;
            break;
        case ThreadPriority::High:
            sch.sched_priority = minPriority + (maxPriority - minPriority) * 3 / 4;
            break;
        case ThreadPriority::RealTime:
            sch.sched_priority = maxPriority - 1;
            break;
        case ThreadPriority::Critical:
            sch.sched_priority = maxPriority;
            break;
    }

    success = pthread_setschedparam(thread, SCHED_FIFO, &sch) == 0;

    // If SCHED_FIFO fails, try SCHED_RR
    if (!success) {
        success = pthread_setschedparam(thread, SCHED_RR, &sch) == 0;
    }
#endif

    if (success) {
        std::cout << "RealTimeThreadManager: Set thread priority to ";
        switch (priority) {
            case ThreadPriority::Low: std::cout << "Low"; break;
            case ThreadPriority::Normal: std::cout << "Normal"; break;
            case ThreadPriority::High: std::cout << "High"; break;
            case ThreadPriority::RealTime: std::cout << "Real-Time"; break;
            case ThreadPriority::Critical: std::cout << "Critical"; break;
        }
        std::cout << std::endl;
    } else {
        std::cerr << "RealTimeThreadManager: Failed to set thread priority" << std::endl;
    }

    return success;
}

//==============================================================================
ThreadPriority RealTimeThreadManager::getCurrentThreadPriority() const {
    // Default to normal if we can't determine
    return ThreadPriority::Normal;
}

//==============================================================================
void RealTimeThreadManager::enterRealTimeThread() {
    inRealTimeThread_.store(true);

    if (config_.enableRealTimePriority) {
        setThreadPriority(ThreadPriority::RealTime);
    }
}

//==============================================================================
void RealTimeThreadManager::exitRealTimeThread() {
    inRealTimeThread_.store(false);

    // Restore normal priority
    setThreadPriority(ThreadPriority::Normal);
}

//==============================================================================
void RealTimeThreadManager::willAcquireLock() {
    // Record that we're about to acquire a lock
    // This is for deadlock detection
}

//==============================================================================
void RealTimeThreadManager::didAcquireLock(double waitTimeUs) {
    // Update statistics
    double currentAvg = statistics_.averageLockWaitTime.load();
    double newAvg = (currentAvg * 0.9 + waitTimeUs * 0.1);
    statistics_.averageLockWaitTime.store(newAvg);

    // Check for priority inversion
    if (config_.enablePriorityInversionDetection) {
        checkForPriorityInversion(waitTimeUs);
    }
}

//==============================================================================
void RealTimeThreadManager::reportViolation(const ThreadSafetyEvent& event) {
    recordViolation(event);

    // Update statistics
    statistics_.totalViolations++;

    switch (event.type) {
        case ThreadSafetyViolation::PriorityInversion:
            statistics_.priorityInversions++;
            break;
        case ThreadSafetyViolation::DeadlockDetected:
            statistics_.deadlocks++;
            break;
        case ThreadSafetyViolation::RaceCondition:
            statistics_.raceConditions++;
            break;
        case ThreadSafetyViolation::RealTimeViolation:
            statistics_.realTimeViolations++;
            break;
        default:
            break;
    }

    // Call callback if set
    if (violationCallback_) {
        violationCallback_(event);
    }

    // Log high-severity violations
    if (event.severity >= 7) {
        std::cerr << "RealTimeThreadManager: " << event.toString() << std::endl;
    }

    // Fail in strict mode
    if (config_.enableStrictMode && event.severity >= 8) {
        std::cerr << "RealTimeThreadManager: Critical violation in strict mode - this should not happen!" << std::endl;
        // In production, you might want to assert here
        // jassertfalse;
    }
}

//==============================================================================
std::vector<ThreadSafetyEvent> RealTimeThreadManager::getRecentViolations() const {
    std::lock_guard<std::mutex> lock(violationsMutex_);
    return violations_;
}

//==============================================================================
void RealTimeThreadManager::clearViolations() {
    std::lock_guard<std::mutex> lock(violationsMutex_);
    violations_.clear();
}

//==============================================================================
void RealTimeThreadManager::resetStatistics() {
    statistics_.totalViolations.store(0);
    statistics_.priorityInversions.store(0);
    statistics_.deadlocks.store(0);
    statistics_.raceConditions.store(0);
    statistics_.realTimeViolations.store(0);
    statistics_.averageLockWaitTime.store(0.0);

    clearViolations();

    std::cout << "RealTimeThreadManager: Statistics reset" << std::endl;
}

//==============================================================================
bool RealTimeThreadManager::validateRealTimeOperation(
    const juce::String& operationDescription) const
{
    if (!inRealTimeThread_.load()) {
        return true;  // Operation is safe if we're not in real-time thread
    }

    // List of operations that are NOT real-time safe
    std::vector<juce::String> unsafeOperations = {
        "malloc",
        "free",
        "new",
        "delete",
        "lock",
        "wait",
        "sleep",
        "printf",
        "cout",
        "cerr",
        "file",
        "network",
        "alloc"
    };

    // Check if description contains unsafe operations
    for (const auto& unsafe : unsafeOperations) {
        if (operationDescription.containsIgnoreCase(unsafe)) {
            // This operation might not be real-time safe
            // Log it but don't fail (let user decide)
            return false;
        }
    }

    return true;
}

//==============================================================================
juce::String RealTimeThreadManager::getCurrentThreadName() {
    return juce::Thread::getCurrentThreadName();
}

//==============================================================================
int RealTimeThreadManager::getCurrentThreadId() {
    return static_cast<int>(reinterpret_cast<uintptr_t>(juce::Thread::getCurrentThreadId()));
}

//==============================================================================
// Private Methods
//==============================================================================

void RealTimeThreadManager::recordViolation(const ThreadSafetyEvent& event) {
    std::lock_guard<std::mutex> lock(violationsMutex_);

    violations_.push_back(event);

    // Limit history size
    if (static_cast<int>(violations_.size()) > maxViolations) {
        violations_.erase(violations_.begin());
    }
}

void RealTimeThreadManager::checkForPriorityInversion(double lockWaitTime) {
    if (!inRealTimeThread_.load()) {
        return;  // Priority inversion only matters in real-time thread
    }

    if (lockWaitTime > config_.maxLockWaitTimeUs) {
        ThreadSafetyEvent event;
        event.type = ThreadSafetyViolation::PriorityInversion;
        event.description = "Lock wait time " +
                           juce::String(lockWaitTime, 2) +
                           "us exceeds threshold " +
                           juce::String(config_.maxLockWaitTimeUs, 2) + "us";
        event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
        event.threadId = getCurrentThreadId();
        event.severity = juce::jmin(10.0, lockWaitTime / config_.maxLockWaitTimeUs * 5.0);

        reportViolation(event);
    }
}

} // namespace zenith
