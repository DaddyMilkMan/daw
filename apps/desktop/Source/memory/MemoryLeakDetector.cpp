/*
  ==============================================================================

    MemoryLeakDetector.cpp
    Implementation of memory leak detection

  ==============================================================================
*/

#include "MemoryLeakDetector.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace zenith {

//==============================================================================
MemoryLeakDetector::MemoryLeakDetector(const MemoryLeakDetectorConfig& config)
    : config_(config) {

    std::cout << "MemoryLeakDetector: Initialized" << std::endl;
    std::cout << "  - Tracking: " << (config_.enableTracking ? "enabled" : "disabled") << std::endl;
    std::cout << "  - Leak threshold: " << (config_.leakThresholdBytes / 1024) << " KB" << std::endl;
    std::cout << "  - Auto cleanup: " << (config_.autoCleanup ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
MemoryLeakDetector::~MemoryLeakDetector() {
    if (config_.reportOnShutdown && config_.enableTracking) {
        auto leaks = detectLeaks();

        if (!leaks.empty()) {
            std::cerr << "\n========================================" << std::endl;
            std::cerr << "MEMORY LEAK DETECTION REPORT" << std::endl;
            std::cerr << "========================================\n" << std::endl;

            for (const auto& leak : leaks) {
                std::cerr << formatLeak(leak) << std::endl;
            }

            std::cerr << "\nTotal: " << leaks.size() << " suspected leaks" << std::endl;
            std::cerr << "========================================\n" << std::endl;
        }

        std::cout << "MemoryLeakDetector: Shutdown complete" << std::endl;
    }
}

//==============================================================================
void MemoryLeakDetector::recordAllocation(
    void* ptr,
    size_t size,
    const char* file,
    int line,
    const juce::String& category) {

    if (!config_.enableTracking || ptr == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    MemoryAllocation alloc;
    alloc.address = ptr;
    alloc.size = size;
    alloc.file = juce::String(file);
    alloc.line = line;
    alloc.category = category;
    alloc.allocationTime = juce::Time::getCurrentTime();
    alloc.allocationId = ++nextAllocationId_;

    allocations_[ptr] = alloc;

    updateStatistics();
}

//==============================================================================
void MemoryLeakDetector::recordDeallocation(void* ptr) {
    if (!config_.enableTracking || ptr == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = allocations_.find(ptr);
    if (it != allocations_.end()) {
        stats_.currentAllocations--;
        stats_.currentBytesAllocated -= it->second.size;
        allocations_.erase(it);
    }
}

//==============================================================================
MemoryLeakStats MemoryLeakDetector::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

//==============================================================================
std::vector<MemoryAllocation> MemoryLeakDetector::detectLeaks() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<MemoryAllocation> leaks;

    for (const auto& pair : allocations_) {
        if (isSuspectedLeak(pair.second)) {
            leaks.push_back(pair.second);
        }
    }

    // Sort by allocation time (oldest first)
    std::sort(leaks.begin(), leaks.end(),
        [](const MemoryAllocation& a, const MemoryAllocation& b) {
            return a.allocationTime < b.allocationTime;
        });

    return leaks;
}

//==============================================================================
juce::String MemoryLeakDetector::generateLeakReport() {
    auto leaks = detectLeaks();

    juce::String report;
    report << "Memory Leak Report\n";
    report << "==================\n\n";
    report << "Detected " + juce::String(leaks.size()) + " suspected leaks:\n\n";

    for (const auto& leak : leaks) {
        report << formatLeak(leak) << "\n";
    }

    report << "\n" + stats_.toString();

    return report;
}

//==============================================================================
void MemoryLeakDetector::setLeakCallback(LeakCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    leakCallback_ = std::move(callback);
}

//==============================================================================
MemoryLeakDetector& MemoryLeakDetector::getInstance() {
    static MemoryLeakDetector instance;
    return instance;
}

//==============================================================================
juce::String MemoryLeakDetector::formatLeak(const MemoryAllocation& alloc) const {
    juce::String report;

    report << "Leak #" << static_cast<int>(alloc.allocationId) << ": ";
    report << alloc.size << " bytes from " << alloc.category;
    report << " at " << alloc.file << ":" << alloc.line;

    // Add time info
    auto timeSinceAlloc = juce::Time::getCurrentTime() - alloc.allocationTime;
    report << " (" << juce::String(timeSinceAlloc.inSeconds(), 1) << "s ago)";

    return report;
}

//==============================================================================
bool MemoryLeakDetector::isSuspectedLeak(const MemoryAllocation& alloc) const {
    // Large allocations are suspicious
    if (alloc.size > config_.leakThresholdBytes) {
        return true;
    }

    // Allocations that have been around a long time are suspicious
    auto age = (juce::Time::getCurrentTime() - alloc.allocationTime).inSeconds();
    if (age > 60.0) {  // 1 minute
        return true;
    }

    // High allocation counts in category are suspicious
    size_t categoryCount = 0;
    for (const auto& pair : allocations_) {
        if (pair.second.category == alloc.category) {
            categoryCount++;
        }
    }
    if (categoryCount > config_.leakThresholdCount) {
        return true;
    }

    return false;
}

//==============================================================================
void MemoryLeakDetector::updateStatistics() {
    stats_.totalAllocations++;
    stats_.currentAllocations++;

    juce::uint64 totalSize = 0;
    for (const auto& pair : allocations_) {
        totalSize += pair.second.size;
    }

    stats_.currentBytesAllocated = totalSize;
    stats_.totalBytesAllocated += totalSize;  // Approximation

    if (stats_.totalAllocations % 1000 == 0) {
        auto leaks = detectLeaks();
        stats_.suspectedLeaks = static_cast<juce::uint32>(leaks.size());

        if (leakCallback_ && !leaks.empty()) {
            for (const auto& leak : leaks) {
                leakCallback_(leak);
            }
        }
    }
}

} // namespace zenith
