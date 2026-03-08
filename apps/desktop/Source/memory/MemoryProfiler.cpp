/*
  ==============================================================================

    MemoryProfiler.cpp
    Implementation of memory profiler

  ==============================================================================
*/

#include "MemoryProfiler.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace zenith {

//==============================================================================
MemoryProfiler::MemoryProfiler(const MemoryProfilerConfig& config)
    : config_(config) {

    std::cout << "MemoryProfiler: Initialized" << std::endl;
    std::cout << "  - Category tracking: "
              << (config_.enableCategoryTracking ? "enabled" : "disabled") << std::endl;
    std::cout << "  - Thread tracking: "
              << (config_.enableThreadTracking ? "enabled" : "disabled") << std::endl;
    std::cout << "  - Max records: " << config_.maxAllocationRecords << std::endl;
}

//==============================================================================
MemoryProfiler::~MemoryProfiler() {
    stop();

    if (config_.enableTracking && totalAllocations_.load() > 0) {
        std::cout << "MemoryProfiler: Final statistics" << std::endl;
        std::cout << generateReport() << std::endl;
    }
}

//==============================================================================
void MemoryProfiler::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }

    std::cout << "MemoryProfiler: Started profiling" << std::endl;
}

//==============================================================================
void MemoryProfiler::stop() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }

    std::cout << "MemoryProfiler: Stopped profiling" << std::endl;
    takeSnapshot();
}

//==============================================================================
void MemoryProfiler::recordAllocation(void* ptr, size_t size, const char* file,
                                     int line, const char* category) {
    if (!running_.load() || ptr == nullptr) {
        return;
    }

    auto now = std::chrono::high_resolution_clock::now();
    juce::String cat = getCategoryOrDefault(category);

    // Update allocation records
    {
        std::lock_guard<std::mutex> lock(allocationsMutex_);

        AllocationInfo info;
        info.size = size;
        info.file = juce::String(file != nullptr ? file : "unknown");
        info.line = line;
        info.category = cat;
        info.timestamp = now;
        info.threadId = juce::Thread::getCurrentThreadId();

        activeAllocations_[ptr] = std::move(info);

        // Limit records if needed
        if (activeAllocations_.size() > config_.maxAllocationRecords) {
            // Remove oldest entry (first in map)
            auto it = activeAllocations_.begin();
            if (it != activeAllocations_.end()) {
                activeAllocations_.erase(it);
            }
        }
    }

    // Update category statistics
    if (config_.enableCategoryTracking) {
        std::lock_guard<std::mutex> lock(statsMutex_);

        auto& stats = categoryStats_[cat];
        stats.category = cat;
        stats.totalAllocations++;
        stats.currentBytes += size;
        stats.totalBytesAllocated += size;
        stats.allocationCount++;
        stats.avgAllocationSize = (double)stats.totalBytesAllocated / stats.allocationCount;

        if (stats.currentBytes > stats.peakBytes) {
            stats.peakBytes = stats.currentBytes;
        }
    }

    // Update hotspot tracking
    if (file != nullptr) {
        std::lock_guard<std::mutex> lock(hotspotMutex_);
        juce::String fileKey = juce::String(file) + ":" + juce::String(line);
        fileAllocations_[fileKey]++;
    }

    // Update global statistics
    totalAllocations_.fetch_add(1, std::memory_order_relaxed);
    currentBytes_.fetch_add(size, std::memory_order_relaxed);
    updatePeakBytes();
}

//==============================================================================
void MemoryProfiler::recordDeallocation(void* ptr) {
    if (!running_.load() || ptr == nullptr) {
        return;
    }

    size_t size = 0;
    juce::String category;

    {
        std::lock_guard<std::mutex> lock(allocationsMutex_);

        auto it = activeAllocations_.find(ptr);
        if (it != activeAllocations_.end()) {
            size = it->second.size;
            category = it->second.category;
            activeAllocations_.erase(it);
        }
    }

    if (size > 0) {
        // Update category statistics
        if (config_.enableCategoryTracking && !category.isEmpty()) {
            std::lock_guard<std::mutex> lock(statsMutex_);

            auto& stats = categoryStats_[category];
            stats.totalDeallocations++;
            stats.currentBytes -= size;

            if (stats.currentBytes > stats.peakBytes) {
                stats.peakBytes = stats.currentBytes;
            }
        }

        // Update global statistics
        totalDeallocations_.fetch_add(1, std::memory_order_relaxed);
        currentBytes_.fetch_sub(size, std::memory_order_relaxed);
    }
}

//==============================================================================
CategoryStats MemoryProfiler::getCategoryStats(const juce::String& category) const {
    std::lock_guard<std::mutex> lock(statsMutex_);

    auto it = categoryStats_.find(category);
    if (it != categoryStats_.end()) {
        return it->second;
    }

    return CategoryStats{};  // Return empty stats
}

//==============================================================================
std::vector<CategoryStats> MemoryProfiler::getAllCategoryStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);

    std::vector<CategoryStats> stats;
    stats.reserve(categoryStats_.size());

    for (const auto& pair : categoryStats_) {
        stats.push_back(pair.second);
    }

    // Sort by current bytes (descending)
    std::sort(stats.begin(), stats.end(),
              [](const CategoryStats& a, const CategoryStats& b) {
                  return a.currentBytes > b.currentBytes;
              });

    return stats;
}

//==============================================================================
ProfilingSnapshot MemoryProfiler::getSnapshot() const {
    ProfilingSnapshot snapshot;
    snapshot.timestamp = juce::Time::getCurrentTime();
    snapshot.totalAllocations = totalAllocations_.load(std::memory_order_relaxed);
    snapshot.totalDeallocations = totalDeallocations_.load(std::memory_order_relaxed);
    snapshot.currentBytes = currentBytes_.load(std::memory_order_relaxed);
    snapshot.peakBytes = peakBytes_.load(std::memory_order_relaxed);

    // Get top categories
    auto allStats = getAllCategoryStats();
    for (size_t i = 0; i < std::min(size_t(5), allStats.size()); ++i) {
        snapshot.topCategories.push_back(allStats[i].toString());
    }

    // Get hotspots
    auto hotspots = getHotspots(5);
    snapshot.hotspots = hotspots;

    return snapshot;
}

//==============================================================================
std::vector<juce::String> MemoryProfiler::getTopConsumers(juce::uint32 count) const {
    auto allStats = getAllCategoryStats();

    std::vector<juce::String> consumers;
    consumers.reserve(std::min((size_t)count, allStats.size()));

    for (size_t i = 0; i < std::min((size_t)count, allStats.size()); ++i) {
        consumers.push_back(allStats[i].toString());
    }

    return consumers;
}

//==============================================================================
std::vector<juce::String> MemoryProfiler::getHotspots(juce::uint32 count) const {
    std::lock_guard<std::mutex> lock(hotspotMutex_);

    // Convert to vector for sorting
    std::vector<std::pair<juce::String, juce::uint64>> hotspots(
        fileAllocations_.begin(),
        fileAllocations_.end()
    );

    // Sort by allocation count (descending)
    std::sort(hotspots.begin(), hotspots.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    // Format results
    std::vector<juce::String> results;
    results.reserve(std::min((size_t)count, hotspots.size()));

    for (size_t i = 0; i < std::min((size_t)count, hotspots.size()); ++i) {
        results.push_back(juce::String::formatted(
            "%s: %llu allocations",
            hotspots[i].first,
            hotspots[i].second
        ));
    }

    return results;
}

//==============================================================================
juce::String MemoryProfiler::generateReport() const {
    juce::String report;
    report << "=== Memory Profiler Report ===\n\n";

    auto snapshot = getSnapshot();

    report << "Overall Statistics:\n";
    report << "  Total allocations: " << snapshot.totalAllocations << "\n";
    report << "  Total deallocations: " << snapshot.totalDeallocations << "\n";
    report << "  Current memory: " << formatSize(snapshot.currentBytes) << "\n";
    report << "  Peak memory: " << formatSize(snapshot.peakBytes) << "\n";
    report << "  Active allocations: "
           << (snapshot.totalAllocations - snapshot.totalDeallocations) << "\n\n";

    auto allStats = getAllCategoryStats();
    if (!allStats.empty()) {
        report << "Category Breakdown:\n";
        for (const auto& stats : allStats) {
            report << "  " << stats.toString() << "\n";
        }
        report << "\n";
    }

    auto hotspots = getHotspots(10);
    if (!hotspots.empty()) {
        report << "Allocation Hotspots:\n";
        for (const auto& hotspot : hotspots) {
            report << "  " << hotspot << "\n";
        }
        report << "\n";
    }

    return report;
}

//==============================================================================
void MemoryProfiler::reset() {
    std::lock_guard<std::mutex> lockAlloc(allocationsMutex_);
    std::lock_guard<std::mutex> lockStats(statsMutex_);
    std::lock_guard<std::mutex> lockHot(hotspotMutex_);
    std::lock_guard<std::mutex> lockSnap(snapshotsMutex_);

    activeAllocations_.clear();
    categoryStats_.clear();
    fileAllocations_.clear();
    snapshots_.clear();

    totalAllocations_.store(0, std::memory_order_relaxed);
    totalDeallocations_.store(0, std::memory_order_relaxed);
    currentBytes_.store(0, std::memory_order_relaxed);
    peakBytes_.store(0, std::memory_order_relaxed);

    std::cout << "MemoryProfiler: Reset all data" << std::endl;
}

//==============================================================================
void MemoryProfiler::setSnapshotCallback(SnapshotCallback callback) {
    snapshotCallback_ = std::move(callback);
}

//==============================================================================
MemoryProfiler& MemoryProfiler::getInstance() {
    static MemoryProfiler instance;
    return instance;
}

//==============================================================================
void MemoryProfiler::updatePeakBytes() {
    juce::uint64 current = currentBytes_.load(std::memory_order_relaxed);
    juce::uint64 peak = peakBytes_.load(std::memory_order_relaxed);

    while (current > peak) {
        if (peakBytes_.compare_exchange_weak(peak, current,
                                            std::memory_order_relaxed)) {
            break;
        }
    }
}

//==============================================================================
void MemoryProfiler::takeSnapshot() {
    auto snapshot = getSnapshot();

    std::lock_guard<std::mutex> lock(snapshotsMutex_);

    snapshots_.push_back(snapshot);

    // Keep only last 100 snapshots
    if (snapshots_.size() > 100) {
        snapshots_.erase(snapshots_.begin());
    }

    // Trigger callback if set
    if (snapshotCallback_) {
        snapshotCallback_(snapshot);
    }
}

//==============================================================================
juce::String MemoryProfiler::formatSize(juce::uint64 bytes) const {
    if (bytes < 1024) {
        return juce::String(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return juce::String((double)bytes / 1024.0, 1) + " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        return juce::String((double)bytes / (1024.0 * 1024.0), 2) + " MB";
    } else {
        return juce::String((double)bytes / (1024.0 * 1024.0 * 1024.0), 3) + " GB";
    }
}

//==============================================================================
juce::String MemoryProfiler::getCategoryOrDefault(const char* category) const {
    if (category != nullptr && strlen(category) > 0) {
        return category;
    }
    return "Default";
}

} // namespace zenith
