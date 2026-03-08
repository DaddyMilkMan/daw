/*
  ==============================================================================

    CPUMonitor.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #5)

    Real-time CPU load monitoring with overload protection.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>
#include <mutex>

namespace zenith {

//==============================================================================
/**
 * @brief CPU load snapshot
 */
struct CPUSnapshot {
    double usage = 0.0;         // Percentage (0-100)
    double timestamp = 0.0;     // Seconds
    int coreCount = 0;          // Number of CPU cores

    juce::String toString() const {
        return "CPU: " + juce::String(usage, 1) + "% (" +
               juce::String(coreCount) + " cores)";
    }
};

//==============================================================================
/**
 * @brief CPU overload event
 */
struct CPUOverloadEvent {
    double usage = 0.0;         // Percentage
    double duration = 0.0;      // Seconds
    double timestamp = 0.0;
    int severity = 0;           // 0-10

    juce::String toString() const {
        return "CPU Overload: " + juce::String(usage, 1) + "% for " +
               juce::String(duration, 2) + "s";
    }
};

//==============================================================================
/**
 * @brief CPU monitor statistics
 */
struct CPUMonitorStatistics {
    std::atomic<double> averageUsage{0.0};
    std::atomic<double> peakUsage{0.0};
    std::atomic<int> overloadCount{0};
    std::atomic<double> totalOverloadTime{0.0};  // Seconds
    std::atomic<double> currentUsage{0.0};

    juce::String toString() const {
        return "CPU: " + juce::String(currentUsage.load(), 1) + "% avg, " +
               juce::String(peakUsage.load(), 1) + "% peak, " +
               juce::String(overloadCount.load()) + " overloads";
    }
};

//==============================================================================
/**
 * @brief CPU monitor configuration
 */
struct CPUMonitorConfig {
    double warningThreshold = 70.0;     // Warning at 70%
    double criticalThreshold = 85.0;    // Critical at 85%
    double overloadThreshold = 95.0;    // Overload at 95%
    double smoothingFactor = 0.1;       // EMA factor for averaging
    bool enablePerCoreMonitoring = false; // Per-core vs total
    int historySize = 100;              // Number of snapshots to keep
};

//==============================================================================
/**
 * @brief Real-time CPU monitor
 *
 * Features:
 * - Real-time CPU usage monitoring
 * - Per-core monitoring support
 * - Overload detection
 * - CPU trend analysis
 * - Statistics tracking
 */
class CPUMonitor {
public:
    //==========================================================================
    CPUMonitor();
    ~CPUMonitor();

    //==========================================================================
    /**
     * @brief Update CPU usage
     * Call this periodically (e.g., every 100ms)
     * @return Current CPU usage percentage
     */
    double update();

    //==========================================================================
    /**
     * @brief Set current CPU usage manually
     * For use when system monitoring isn't available
     */
    void setCPUUsage(double usage);

    //==========================================================================
    /**
     * @brief Get current CPU usage
     */
    double getCurrentUsage() const {
        return statistics_.currentUsage.load();
    }

    //==========================================================================
    /**
     * @brief Get average CPU usage over time window
     */
    double getAverageUsage(double timeWindowSeconds = 1.0) const;

    //==========================================================================
    /**
     * @brief Get peak CPU usage
     */
    double getPeakUsage() const {
        return statistics_.peakUsage.load();
    }

    //==========================================================================
    /**
     * @brief Check if CPU is in warning territory
     */
    bool isWarning() const {
        return getCurrentUsage() >= config_.warningThreshold &&
               getCurrentUsage() < config_.criticalThreshold;
    }

    //==========================================================================
    /**
     * @brief Check if CPU is critical
     */
    bool isCritical() const {
        return getCurrentUsage() >= config_.criticalThreshold &&
               getCurrentUsage() < config_.overloadThreshold;
    }

    //==========================================================================
    /**
     * @brief Check if CPU is overloaded
     */
    bool isOverloaded() const {
        return getCurrentUsage() >= config_.overloadThreshold;
    }

    //==========================================================================
    /**
     * @brief Get CPU usage history
     */
    std::vector<CPUSnapshot> getHistory() const;

    //==========================================================================
    /**
     * @brief Get recent history (within time window)
     */
    std::vector<CPUSnapshot> getRecentHistory(double timeWindowSeconds = 5.0) const;

    //==========================================================================
    /**
     * @brief Calculate CPU trend (percent per second)
     * Positive = increasing, negative = decreasing
     */
    double calculateTrend() const;

    //==========================================================================
    /**
     * @brief Get statistics
     */
    CPUMonitorStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

    //==========================================================================
    /**
     * @brief Get configuration
     */
    CPUMonitorConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const CPUMonitorConfig& config) {
        config_ = config;
    }

    //==========================================================================
    /**
     * @brief Get number of CPU cores
     */
    int getCoreCount() const {
        return coreCount_;
    }

private:
    //==========================================================================
    double getSystemCPUUsage();
    void updateStatistics(double usage);
    void checkOverload(double usage);

    //==========================================================================
    // Configuration
    CPUMonitorConfig config_;

    // History
    std::vector<CPUSnapshot> history_;
    mutable std::mutex historyMutex_;

    // System info
    int coreCount_ = 1;

    // Statistics
    CPUMonitorStatistics statistics_;

    // Overload tracking
    double overloadStartTime_ = 0.0;
    bool inOverload_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPUMonitor)
};

//==============================================================================
/**
 * @brief Singleton accessor for CPU monitor
 */
class CPMonitorHolder {
public:
    static CPUMonitor& getInstance() {
        static CPUMonitor instance;
        return instance;
    }

    CPMonitorHolder(const CPMonitorHolder&) = delete;
    CPMonitorHolder& operator=(const CPMonitorHolder&) = delete;

private:
    CPMonitorHolder() = default;
    ~CPUMonitorHolder() = default;
};

} // namespace zenith
