/*
  ==============================================================================

    MemoryMonitor.cpp
    Implementation of memory monitoring

  ==============================================================================
*/

#include "MemoryMonitor.h"
#include <iostream>
#include <algorithm>
#include <numeric>

#ifdef JUCE_LINUX
#include <sys/sysinfo.h>
#include <sys/resource.h>
#elif defined(JUCE_WINDOWS)
#include <windows.h>
#include <psapi.h>
#endif

namespace zenith {

//==============================================================================
MemoryMonitor::MemoryMonitor(const MemoryMonitorConfig& config)
    : config_(config) {

    history_.reserve(config_.historySize);

    std::cout << "MemoryMonitor: Initialized" << std::endl;
    std::cout << "  - Sampling interval: " << config_.samplingIntervalMs << " ms" << std::endl;
    std::cout << "  - History size: " << config_.historySize << " samples" << std::endl;
    std::cout << "  - Report threshold: " << config_.reportThresholdPercent << "%" << std::endl;
}

//==============================================================================
MemoryMonitor::~MemoryMonitor() {
    stop();
    std::cout << "MemoryMonitor: Shut down" << std::endl;
}

//==============================================================================
void MemoryMonitor::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }

    startTimer(config_.samplingIntervalMs);

    std::cout << "MemoryMonitor: Started monitoring" << std::endl;
}

//==============================================================================
void MemoryMonitor::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }

    stopTimer();

    std::cout << "MemoryMonitor: Stopped monitoring" << std::endl;
}

//==============================================================================
MemorySample MemoryMonitor::getCurrentSample() const {
    return takeSample();
}

//==============================================================================
MemoryWindowStats MemoryMonitor::getWindowStats(double durationSeconds) const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    MemoryWindowStats stats;

    // Calculate how many samples to include
    juce::uint32 samplesNeeded = static_cast<juce::uint32>(
        (durationSeconds * 1000.0) / config_.samplingIntervalMs
    );

    juce::uint32 startIndex = 0;
    if (history_.size() > samplesNeeded) {
        startIndex = history_.size() - samplesNeeded;
    }

    // Calculate statistics
    double sum = 0.0;
    for (size_t i = startIndex; i < history_.size(); ++i) {
        double usage = history_[i].usagePercent;
        sum += usage;
        stats.peakUsage = juce::jmax(stats.peakUsage, usage);
        stats.minUsage = juce::jmin(stats.minUsage, usage);
    }

    if (history_.size() > startIndex) {
        stats.sampleCount = static_cast<juce::uint32>(history_.size() - startIndex);
        stats.averageUsage = sum / stats.sampleCount;
    }

    return stats;
}

//==============================================================================
MemoryMonitor::Trend MemoryMonitor::getTrend(int sampleCount) const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    if (history_.size() < 2) {
        return Trend::Stable;
    }

    int actualCount = juce::jmin(sampleCount, static_cast<int>(history_.size()) - 1);
    int startIndex = static_cast<int>(history_.size()) - actualCount - 1;

    // Calculate linear regression
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    for (int i = 0; i <= actualCount; ++i) {
        double x = static_cast<double>(i);
        double y = history_[startIndex + i].usagePercent;
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    double slope = (actualCount * sumXY - sumX * sumY) /
                  (actualCount * sumX2 - sumX * sumX);

    if (slope > 1.0) return Trend::Increasing;
    if (slope < -1.0) return Trend::Decreasing;
    return Trend::Stable;
}

//==============================================================================
void MemoryMonitor::setThresholdCallback(
    juce::uint32 thresholdPercent,
    ThresholdCallback callback) {

    ThresholdAlert alert;
    alert.thresholdPercent = thresholdPercent;
    alert.callback = std::move(callback);

    std::lock_guard<std::mutex> lock(historyMutex_);
    thresholds_.push_back(alert);
}

//==============================================================================
MemoryMonitor& MemoryMonitor::getInstance() {
    static MemoryMonitor instance;
    return instance;
}

//==============================================================================
void MemoryMonitor::timerCallback() {
    sampleMemory();
}

//==============================================================================
void MemoryMonitor::sampleMemory() {
    if (!running_.load()) {
        return;
    }

    auto sample = takeSample();
    addToHistory(sample);
    checkThresholds(sample);
}

//==============================================================================
void MemoryMonitor::checkThresholds(const MemorySample& sample) {
    std::lock_guard<std::mutex> lock(historyMutex_);

    for (const auto& alert : thresholds_) {
        if (sample.usagePercent >= alert.thresholdPercent) {
            if (alert.callback) {
                alert.callback(sample.usagePercent, sample);
            }
        }
    }
}

//==============================================================================
void MemoryMonitor::addToHistory(const MemorySample& sample) {
    std::lock_guard<std::mutex> lock(historyMutex_);

    history_.push_back(sample);

    // Trim to max size
    if (history_.size() > config_.historySize) {
        history_.erase(history_.begin());
    }
}

//==============================================================================
MemorySample MemoryMonitor::takeSample() const {
    MemorySample sample;
    sample.timestamp = juce::Time::getCurrentTime();

#ifdef JUCE_LINUX
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    sample.processMemoryUsed = usage.ru_maxrss * 1024;

    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        sample.totalPhysicalMemory = info.totalram * info.mem_unit;
        sample.availablePhysicalMemory = info.freeram * info.mem_unit;
    }

#elif defined(JUCE_WINDOWS)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        sample.processMemoryUsed = pmc.WorkingSetSize;
    }

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        sample.totalPhysicalMemory = statex.ullTotalPhys;
        sample.availablePhysicalMemory = statex.ullAvailPhys;
    }

#endif

    if (sample.totalPhysicalMemory > 0) {
        sample.usagePercent = (sample.processMemoryUsed * 100.0) /
                             sample.totalPhysicalMemory;
    }

    return sample;
}

} // namespace zenith
