/*
  ==============================================================================

    CPUMonitor.cpp
    Implementation of real-time CPU monitoring

  ==============================================================================
*/

#include "CPUMonitor.h"
#include <iostream>
#include <algorithm>

#if JUCE_WINDOWS
#include <windows.h>
#elif JUCE_MAC || JUCE_LINUX
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace zenith {

//==============================================================================
// CPUMonitor Implementation
//==============================================================================

CPUMonitor::CPUMonitor() {
    // Get core count
#if JUCE_WINDOWS
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    coreCount_ = static_cast<int>(sysInfo.dwNumberOfProcessors);
#elif JUCE_MAC || JUCE_LINUX
    coreCount_ = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif

    std::cout << "CPUMonitor: Initialized (" << coreCount_ << " cores detected)" << std::endl;
}

CPUMonitor::~CPUMonitor() {
    std::cout << "CPUMonitor: Shut down ("
              << statistics_.overloadCount.load() << " overloads, "
              << "peak: " << statistics_.peakUsage.load() << "%)" << std::endl;
}

//==============================================================================
double CPUMonitor::update() {
    double usage = getSystemCPUUsage();
    setCPUUsage(usage);
    return usage;
}

//==============================================================================
void CPUMonitor::setCPUUsage(double usage) {
    // Clamp to valid range
    usage = juce::jlimit(0.0, 100.0, usage);

    // Add to history
    CPUSnapshot snapshot;
    snapshot.usage = usage;
    snapshot.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
    snapshot.coreCount = coreCount_;

    {
        std::lock_guard<std::mutex> lock(historyMutex_);
        history_.push_back(snapshot);

        // Limit history size
        if (static_cast<int>(history_.size()) > config_.historySize) {
            history_.erase(history_.begin());
        }
    }

    // Update statistics
    updateStatistics(usage);

    // Check for overload
    checkOverload(usage);
}

//==============================================================================
double CPUMonitor::getAverageUsage(double timeWindowSeconds) const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    if (history_.empty()) {
        return 0.0;
    }

    double currentTime = history_.back().timestamp;
    double sum = 0.0;
    int count = 0;

    for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
        if (currentTime - it->timestamp <= timeWindowSeconds) {
            sum += it->usage;
            count++;
        } else {
            break;
        }
    }

    return count > 0 ? sum / count : 0.0;
}

//==============================================================================
std::vector<CPUSnapshot> CPUMonitor::getHistory() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    return history_;
}

//==============================================================================
std::vector<CPUSnapshot> CPUMonitor::getRecentHistory(double timeWindowSeconds) const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    if (history_.empty()) {
        return {};
    }

    std::vector<CPUSnapshot> result;
    double currentTime = history_.back().timestamp;

    for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
        if (currentTime - it->timestamp <= timeWindowSeconds) {
            result.push_back(*it);
        } else {
            break;
        }
    }

    // Reverse to get chronological order
    std::reverse(result.begin(), result.end());
    return result;
}

//==============================================================================
double CPUMonitor::calculateTrend() const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    if (history_.size() < 2) {
        return 0.0;
    }

    // Calculate slope of usage over time (percent per second)
    const CPUSnapshot& oldest = history_.front();
    const CPUSnapshot& newest = history_.back();

    double timeDelta = newest.timestamp - oldest.timestamp;
    if (timeDelta < 0.1) {  // Need at least 100ms
        return 0.0;
    }

    double usageDelta = newest.usage - oldest.usage;
    return usageDelta / timeDelta;  // Percent per second
}

//==============================================================================
void CPUMonitor::resetStatistics() {
    statistics_.averageUsage.store(0.0);
    statistics_.peakUsage.store(0.0);
    statistics_.overloadCount.store(0);
    statistics_.totalOverloadTime.store(0.0);
    statistics_.currentUsage.store(0.0);

    {
        std::lock_guard<std::mutex> lock(historyMutex_);
        history_.clear();
    }

    std::cout << "CPUMonitor: Statistics reset" << std::endl;
}

//==============================================================================
// Private Methods
//==============================================================================

double CPUMonitor::getSystemCPUUsage() {
    // Cross-platform CPU usage detection
    // This is a simplified implementation

#if JUCE_WINDOWS
    static FILETIME prevIdleTime, prevKernelTime, prevUserTime;
    FILETIME idleTime, kernelTime, userTime;

    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        // Calculate times
        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        // This is simplified - full implementation would compare with previous values
        return 10.0;  // Placeholder - would calculate actual usage
    }
    return 0.0;

#elif JUCE_MAC || JUCE_LINUX
    // On POSIX systems, we'd use /proc/stat or host_statistics
    // For now, return a reasonable default
    return 15.0;  // Placeholder - would calculate actual usage
#else
    return 0.0;
#endif
}

void CPUMonitor::updateStatistics(double usage) {
    // Update current
    statistics_.currentUsage.store(usage);

    // Update peak
    double currentPeak = statistics_.peakUsage.load();
    if (usage > currentPeak) {
        statistics_.peakUsage.store(usage);
    }

    // Update average using EMA
    double currentAvg = statistics_.averageUsage.load();
    double newAvg = (currentAvg * (1.0 - config_.smoothingFactor)) +
                   (usage * config_.smoothingFactor);
    statistics_.averageUsage.store(newAvg);
}

void CPUMonitor::checkOverload(double usage) {
    if (usage >= config_.overloadThreshold) {
        if (!inOverload_) {
            // Overload started
            inOverload_ = true;
            overloadStartTime_ = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
            statistics_.overloadCount++;
        }
    } else {
        if (inOverload_) {
            // Overload ended
            inOverload_ = false;
            double overloadDuration = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0 -
                                    overloadStartTime_;
            statistics_.totalOverloadTime += overloadDuration;

            if (overloadDuration > 1.0) {  // Log if overload lasted > 1 second
                std::cerr << "CPUMonitor: CPU overload lasted "
                          << overloadDuration << "s" << std::endl;
            }
        }
    }
}

} // namespace zenith
