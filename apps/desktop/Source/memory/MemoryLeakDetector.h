/*
  ==============================================================================

    MemoryLeakDetector.h
    Created: 2026-02-19
    Month 9, Gap #1 - Memory Leak Detection

    Runtime memory leak tracking and reporting system.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>

namespace zenith {

//==============================================================================
/**
 * @brief Memory allocation record for tracking
 */
struct MemoryAllocation {
    void* address;
    size_t size;
    juce::String file;
    int line;
    juce::String category;
    juce::Time allocationTime;
    juce::uint32 allocationId;
};

//==============================================================================
/**
 * @brief Memory leak statistics
 */
struct MemoryLeakStats {
    juce::uint32 totalAllocations = 0;
    juce::uint32 currentAllocations = 0;
    juce::uint64 totalBytesAllocated = 0;
    juce::uint64 currentBytesAllocated = 0;
    juce::uint32 suspectedLeaks = 0;

    juce::String toString() const {
        return juce::String::formatted(
            "Allocations: %u current, %u total | Bytes: %llu current, %llu total | Suspected leaks: %u",
            currentAllocations, totalAllocations,
            currentBytesAllocated, totalBytesAllocated,
            suspectedLeaks
        );
    }
};

//==============================================================================
/**
 * @brief Memory leak detection configuration
 */
struct MemoryLeakDetectorConfig {
    bool enableTracking = true;
    bool enableStackTrace = false;
    size_t leakThresholdBytes = 1024 * 1024;  // 1MB
    juce::uint32 leakThresholdCount = 1000;
    bool reportOnShutdown = true;
    bool autoCleanup = false;
};

//==============================================================================
/**
 * @brief Runtime memory leak detector
 *
 * Features:
 * - Tracks all allocations/deallocations
 * - Detects leaks at shutdown
 * - Categorizes allocations by type
 * - Reports suspected leaks with file/line info
 * - Memory usage statistics
 */
class MemoryLeakDetector {
public:
    //==========================================================================
    MemoryLeakDetector(const MemoryLeakDetectorConfig& config = {});
    ~MemoryLeakDetector();

    //==========================================================================
    /**
     * @brief Record a memory allocation
     */
    void recordAllocation(
        void* ptr,
        size_t size,
        const char* file,
        int line,
        const juce::String& category = "General"
    );

    //==========================================================================
    /**
     * @brief Record a memory deallocation
     */
    void recordDeallocation(void* ptr);

    //==========================================================================
    /**
     * @brief Get current memory statistics
     */
    MemoryLeakStats getStatistics() const;

    //==========================================================================
    /**
     * @brief Detect and report leaks
     * @return List of suspected memory leaks
     */
    std::vector<MemoryAllocation> detectLeaks();

    //==========================================================================
    /**
     * @brief Generate leak report
     */
    juce::String generateLeakReport();

    //==========================================================================
    /**
     * @brief Set memory leak callback
     */
    using LeakCallback = std::function<void(const MemoryAllocation&)>;
    void setLeakCallback(LeakCallback callback);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static MemoryLeakDetector& getInstance();

private:
    //==========================================================================
    mutable std::mutex mutex_;
    std::map<void*, MemoryAllocation> allocations_;
    std::atomic<juce::uint32> nextAllocationId_{0};

    MemoryLeakDetectorConfig config_;
    MemoryLeakStats stats_;
    LeakCallback leakCallback_;

    //==========================================================================
    juce::String formatLeak(const MemoryAllocation& alloc) const;
    bool isSuspectedLeak(const MemoryAllocation& alloc) const;
    void updateStatistics();
};

//==============================================================================
/**
 * @brief RAII helper for tracked allocations
 */
template<typename T>
class TrackedAllocation {
public:
    TrackedAllocation(
        T* ptr,
        const char* file,
        int line,
        const juce::String& category = "Tracked"
    ) : pointer_(ptr) {
        MemoryLeakDetector::getInstance().recordAllocation(
            ptr, sizeof(T), file, line, category
        );
    }

    ~TrackedAllocation() {
        if (pointer_ != nullptr) {
            MemoryLeakDetector::getInstance().recordDeallocation(pointer_);
        }
    }

    T* get() const { return pointer_; }

private:
    T* pointer_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackedAllocation)
};

//==============================================================================
// Macros for automatic leak tracking
#define ZENITH_TRACK_ALLOCATION(ptr, size, category) \
    zenith::MemoryLeakDetector::getInstance().recordAllocation( \
        ptr, size, __FILE__, __LINE__, category)

#define ZENITH_TRACK_DEALLOCATION(ptr) \
    zenith::MemoryLeakDetector::getInstance().recordDeallocation(ptr)

} // namespace zenith
