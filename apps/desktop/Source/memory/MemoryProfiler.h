/*
  ==============================================================================

    MemoryProfiler.h
    Created: 2026-02-19
    Month 9, Gap #8 - Memory Profiling

    Real-time memory allocation profiling and hotspot analysis.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

namespace zenith {

//==============================================================================
/**
 * @brief Memory allocation record
 */
struct AllocationRecord {
    void* address;
    size_t size;
    const char* file;
    int line;
    const char* category;
    std::chrono::high_resolution_clock::time_point timestamp;
    juce::Thread::ThreadID threadId;

    juce::String toString() const {
        return juce::String::formatted(
            "%p [%zu bytes] %s:%d (%s) [thread: %llu]",
            address, size, file != nullptr ? file : "unknown",
            line, category != nullptr ? category : "none",
            (juce::uint64)threadId
        );
    }
};

//==============================================================================
/**
 * @brief Allocation statistics for a category
 */
struct CategoryStats {
    juce::String category;
    juce::uint64 totalAllocations = 0;
    juce::uint64 totalDeallocations = 0;
    juce::uint64 currentBytes = 0;
    juce::uint64 peakBytes = 0;
    juce::uint64 totalBytesAllocated = 0;
    double avgAllocationSize = 0.0;
    juce::uint32 allocationCount = 0;

    juce::String toString() const {
        return juce::String::formatted(
            "%s: %llu allocs, %llu deallocs, %llu KB current, %llu KB peak, %.1f avg bytes",
            category,
            totalAllocations,
            totalDeallocations,
            currentBytes / 1024,
            peakBytes / 1024,
            avgAllocationSize
        );
    }
};

//==============================================================================
/**
 * @brief Memory profiling snapshot
 */
struct ProfilingSnapshot {
    juce::Time timestamp;
    juce::uint64 totalAllocations = 0;
    juce::uint64 totalDeallocations = 0;
    juce::uint64 currentBytes = 0;
    juce::uint64 peakBytes = 0;
    std::vector<juce::String> topCategories;
    std::vector<juce::String> hotspots;

    juce::String toString() const {
        return juce::String::formatted(
            "Snapshot: %llu KB current, %llu KB peak, %llu allocations",
            currentBytes / 1024,
            peakBytes / 1024,
            totalAllocations
        );
    }
};

//==============================================================================
/**
 * @brief Memory profiler configuration
 */
struct MemoryProfilerConfig {
    bool enableTracking = true;
    bool enableStackTraces = false;        // Expensive
    bool enableCategoryTracking = true;
    bool enableThreadTracking = true;
    juce::uint32 maxAllocationRecords = 10000;  // Ring buffer size
    juce::uint32 snapshotIntervalMs = 1000;     // Auto-snapshot interval
};

//==============================================================================
/**
 * @brief Real-time memory allocation profiler
 *
 * Features:
 * - Track allocations by category/file/line
 * - Hotspot identification
 * - Memory leak detection
 * - Peak usage tracking
 * - Thread-wise allocation tracking
 * - Historical snapshots
 *
 * Usage:
 * ```cpp
 * MemoryProfiler& profiler = MemoryProfiler::getInstance();
 * profiler.start();
 *
 * // Track allocation
 * profiler.recordAllocation(ptr, size, __FILE__, __LINE__, "Audio");
 * profiler.recordDeallocation(ptr);
 *
 * // Get statistics
 * auto stats = profiler.getCategoryStats("Audio");
 * auto snapshot = profiler.getSnapshot();
 * ```
 */
class MemoryProfiler {
public:
    //==========================================================================
    explicit MemoryProfiler(const MemoryProfilerConfig& config = {});
    ~MemoryProfiler();

    //==========================================================================
    /**
     * @brief Start profiling
     */
    void start();

    //==========================================================================
    /**
     * @brief Stop profiling
     */
    void stop();

    //==========================================================================
    /**
     * @brief Record memory allocation
     */
    void recordAllocation(void* ptr, size_t size, const char* file,
                         int line, const char* category = nullptr);

    //==========================================================================
    /**
     * @brief Record memory deallocation
     */
    void recordDeallocation(void* ptr);

    //==========================================================================
    /**
     * @brief Get statistics for a specific category
     */
    CategoryStats getCategoryStats(const juce::String& category) const;

    //==========================================================================
    /**
     * @brief Get all category statistics
     */
    std::vector<CategoryStats> getAllCategoryStats() const;

    //==========================================================================
    /**
     * @brief Get current profiling snapshot
     */
    ProfilingSnapshot getSnapshot() const;

    //==========================================================================
    /**
     * @brief Get top memory consumers
     */
    std::vector<juce::String> getTopConsumers(juce::uint32 count = 10) const;

    //==========================================================================
    /**
     * @brief Get memory hotspots (files with most allocations)
     */
    std::vector<juce::String> getHotspots(juce::uint32 count = 10) const;

    //==========================================================================
    /**
     * @brief Generate profiling report
     */
    juce::String generateReport() const;

    //==========================================================================
    /**
     * @brief Reset profiling data
     */
    void reset();

    //==========================================================================
    /**
     * @brief Set snapshot callback
     */
    using SnapshotCallback = std::function<void(const ProfilingSnapshot&)>;
    void setSnapshotCallback(SnapshotCallback callback);

    //==========================================================================
    /**
     * @brief Check if profiler is running
     */
    bool isRunning() const { return running_.load(); }

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static MemoryProfiler& getInstance();

private:
    //==========================================================================
    MemoryProfilerConfig config_;
    std::atomic<bool> running_{false};

    // Allocation tracking
    struct AllocationInfo {
        size_t size;
        juce::String file;
        int line;
        juce::String category;
        std::chrono::high_resolution_clock::time_point timestamp;
        juce::Thread::ThreadID threadId;
    };

    std::map<void*, AllocationInfo> activeAllocations_;
    mutable std::mutex allocationsMutex_;

    // Category statistics
    std::map<juce::String, CategoryStats> categoryStats_;
    mutable std::mutex statsMutex_;

    // Hotspot tracking (file -> allocations)
    std::map<juce::String, juce::uint64> fileAllocations_;
    mutable std::mutex hotspotMutex_;

    // Historical snapshots
    std::vector<ProfilingSnapshot> snapshots_;
    mutable std::mutex snapshotsMutex_;

    // Snapshot callback
    SnapshotCallback snapshotCallback_;

    // Statistics
    std::atomic<juce::uint64> totalAllocations_{0};
    std::atomic<juce::uint64> totalDeallocations_{0};
    std::atomic<juce::uint64> currentBytes_{0};
    std::atomic<juce::uint64> peakBytes_{0};

    //==========================================================================
    void updatePeakBytes();
    void takeSnapshot();
    juce::String formatSize(juce::uint64 bytes) const;
    juce::String getCategoryOrDefault(const char* category) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryProfiler)
};

//==============================================================================
/**
 * @brief RAII profiling scope
 */
class MemoryProfilingScope {
public:
    explicit MemoryProfilingScope(const juce::String& scopeName)
        : scopeName_(scopeName)
        , profiler_(MemoryProfiler::getInstance()) {

        start_ = std::chrono::high_resolution_clock::now();
        profiler_.start();
    }

    ~MemoryProfilingScope() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);

        auto snapshot = profiler_.getSnapshot();

        std::cout << "=== Memory Profile: " << scopeName_ << " ===" << std::endl;
        std::cout << "  Duration: " << (juce::uint64)duration.count() << " ms" << std::endl;
        std::cout << "  Peak memory: " << (snapshot.peakBytes / 1024) << " KB" << std::endl;
        std::cout << "  Total allocations: " << snapshot.totalAllocations << std::endl;
    }

private:
    juce::String scopeName_;
    MemoryProfiler& profiler_;
    std::chrono::high_resolution_clock::time_point start_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryProfilingScope)
};

//==============================================================================
/**
 * @brief Memory allocation tracker macro
 */
#define ZENITH_MEMORY_PROFILE_CATEGORY(category) \
    ZenithMemoryTracker __zenith_tracker__(__FILE__, __LINE__, category)

//==============================================================================
/**
 * @brief Internal memory tracker class
 */
class ZenithMemoryTracker {
public:
    ZenithMemoryTracker(const char* file, int line, const char* category)
        : file_(file), line_(line), category_(category) {

        if (zenith::MemoryProfiler::getInstance().isRunning()) {
            // Track allocation at construction point
            profiler_.recordAllocation(this, sizeof(ZenithMemoryTracker),
                                      file, line, category);
        }
    }

    ~ZenithMemoryTracker() {
        if (zenith::MemoryProfiler::getInstance().isRunning()) {
            profiler_.recordDeallocation(this);
        }
    }

private:
    const char* file_;
    int line_;
    const char* category_;
    zenith::MemoryProfiler& profiler_ = zenith::MemoryProfiler::getInstance();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithMemoryTracker)
};

} // namespace zenith
