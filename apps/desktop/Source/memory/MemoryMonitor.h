/*
  ==============================================================================

    MemoryMonitor.h
    Created: 2026-02-19
    Month 9, Gap #5 - Memory Usage Monitoring

    Real-time memory usage tracking and monitoring system.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Memory usage sample
 */
struct MemorySample {
    juce::Time timestamp;
    juce::uint64 processMemoryUsed = 0;
    juce::uint64 totalPhysicalMemory = 0;
    juce::uint64 availablePhysicalMemory = 0;
    double usagePercent = 0.0;
    juce::uint64 threadCount = 0;
    juce::uint32 handleCount = 0;

    juce::String toString() const {
        return juce::String::formatted(
            "%.1f%% used (%llu MB)",
            usagePercent,
            processMemoryUsed / (1024 * 1024)
        );
    }
};

//==============================================================================
/**
 * @brief Memory statistics over a time window
 */
struct MemoryWindowStats {
    double averageUsage = 0.0;
    double peakUsage = 0.0;
    double minUsage = 100.0;
    juce::uint64 totalAllocated = 0;
    juce::uint64 totalFreed = 0;
    juce::uint32 sampleCount = 0;

    juce::String toString() const {
        return juce::String::formatted(
            "Avg: %.1f%% | Peak: %.1f%% | Min: %.1f%% | Samples: %u",
            averageUsage, peakUsage, minUsage, sampleCount
        );
    }
};

//==============================================================================
/**
 * @brief Memory monitor configuration
 */
struct MemoryMonitorConfig {
    juce::uint32 samplingIntervalMs = 1000;         // Sample every second
    juce::uint32 historySize = 3600;                 // Keep 1 hour of history
    bool enableThreadTracking = false;        // Expensive
    bool enableHandleTracking = false;        // Expensive
    bool enableAutoReporting = true;
    juce::uint32 reportThresholdPercent = 80;      // Report when usage exceeds 80%
};

//==============================================================================
/**
 * @brief Real-time memory usage monitor
 *
 * Features:
 * - Continuous memory sampling
 * - Historical tracking with configurable window size
 * - Peak/average/min statistics
 * - Threshold-based alerting
 * - Memory trend analysis
 */
class MemoryMonitor : private juce::Timer {
public:
    //==========================================================================
    MemoryMonitor(const MemoryMonitorConfig& config = {});
    ~MemoryMonitor();

    //==========================================================================
    /**
     * @brief Start monitoring
     */
    void start();

    //==========================================================================
    /**
     * @brief Stop monitoring
     */
    void stop();

    //==========================================================================
    /**
     * @brief Get current memory sample
     */
    MemorySample getCurrentSample() const;

    //==========================================================================
    /**
     * @brief Get statistics for time window
     */
    MemoryWindowStats getWindowStats(double durationSeconds) const;

    //==========================================================================
    /**
     * @brief Get memory trend (increasing/decreasing/stable)
     */
    enum class Trend { Increasing, Decreasing, Stable };
    Trend getTrend(int sampleCount = 10) const;

    //==========================================================================
    /**
     * @brief Set threshold alert callback
     */
    using ThresholdCallback = std::function<void(double usagePercent, const MemorySample&)>;
    void setThresholdCallback(juce::uint32 thresholdPercent, ThresholdCallback callback);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static MemoryMonitor& getInstance();

private:
    //==========================================================================
    MemoryMonitorConfig config_;
    std::vector<MemorySample> history_;
    mutable std::mutex historyMutex_;

    std::atomic<bool> running_{false};

    //==========================================================================
    void timerCallback() override;

    //==========================================================================
    void sampleMemory();
    void checkThresholds(const MemorySample& sample);
    void addToHistory(const MemorySample& sample);
    MemorySample takeSample() const;

    //==========================================================================
    struct ThresholdAlert {
        juce::uint32 thresholdPercent;
        ThresholdCallback callback;
    };
    std::vector<ThresholdAlert> thresholds_;
};

//==============================================================================
/**
 * @brief RAII helper for memory monitoring scope
 */
class MemoryMonitoringScope {
public:
    MemoryMonitoringScope() {
        zenith::MemoryMonitor::getInstance().start();
    }

    ~MemoryMonitoringScope() {
        zenith::MemoryMonitor::getInstance().stop();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryMonitoringScope)
};

} // namespace zenith
