/**
 * @file WCETMonitor.h
 * @brief Worst-Case Execution Time monitoring for real-time audio
 * 
 * Tracks audio callback execution times to detect potential xruns before they happen.
 * Part of the Zenith DAW observability infrastructure.
 * 
 * Thread Safety:
 * - recordExecution() is AUDIO THREAD ONLY (lock-free)
 * - getStatistics() is MESSAGE THREAD ONLY
 * - All data uses std::atomic for lock-free access
 */

#pragma once

#include <atomic>
#include <chrono>
#include <array>
#include <functional>
#include <juce_core/juce_core.h>

namespace zenith {
namespace profiling {

struct WCETStatistics {
    double avgExecutionTimeUs{0.0};
    double maxExecutionTimeUs{0.0};
    double minExecutionTimeUs{0.0};
    double currentExecutionTimeUs{0.0};
    double budgetUtilizationPercent{0.0};
    int64_t totalCallbacks{0};
    int64_t overruns{0};
    int64_t consecutiveOverruns{0};
    double maxConsecutiveOverruns{0};
    
    bool isHealthy() const {
        return consecutiveOverruns == 0 && budgetUtilizationPercent < 80.0;
    }
    
    bool hasRisk() const {
        return budgetUtilizationPercent >= 80.0 || consecutiveOverruns > 0;
    }
    
    bool isCritical() const {
        return budgetUtilizationPercent >= 95.0 || consecutiveOverruns >= 3;
    }
};

class WCETMonitor {
public:
    WCETMonitor();
    ~WCETMonitor() = default;
    
    void setBudgetMs(double budgetMs) noexcept;
    double getBudgetMs() const noexcept { return budgetUs_.load() / 1000.0; }
    
    void setEnabled(bool enabled) noexcept { enabled_.store(enabled); }
    bool isEnabled() const noexcept { return enabled_.load(); }
    
    void startMeasurement() noexcept;
    bool endMeasurement(int numSamples, double sampleRate) noexcept;
    
    WCETStatistics getStatistics() const;
    void resetStatistics() noexcept;
    juce::String getReport() const;
    
    bool hasOverruns() const noexcept { return totalOverruns_.load() > 0; }
    int64_t getOverrunCount() const noexcept { return totalOverruns_.load(); }
    bool isInOverrunStreak() const noexcept { return consecutiveOverruns_.load() > 0; }
    
private:
    std::atomic<bool> enabled_{true};
    std::atomic<int64_t> budgetUs_{5000};
    std::atomic<int64_t> startTimeUs_{0};
    std::atomic<int64_t> totalCallbacks_{0};
    std::atomic<int64_t> totalOverruns_{0};
    std::atomic<int64_t> consecutiveOverruns_{0};
    std::atomic<int64_t> maxConsecutiveOverruns_{0};
    std::atomic<int64_t> avgExecutionTimeNs_{0};
    std::atomic<int64_t> maxExecutionTimeUs_{0};
    std::atomic<int64_t> minExecutionTimeUs_{INT64_MAX};
    std::atomic<int64_t> currentExecutionTimeUs_{0};
    
    static constexpr double EMA_ALPHA = 0.1;
    
    static int64_t getCurrentTimeUs() noexcept;
    void updateEMA(int64_t newValueUs) noexcept;
    void updateMax(int64_t value) noexcept;
    void updateMin(int64_t value) noexcept;
    void updateMaxConsecutive(int64_t value) noexcept;
};

class WCETScopeTimer {
public:
    WCETScopeTimer(WCETMonitor& monitor, int numSamples, double sampleRate) noexcept
        : monitor_(monitor), numSamples_(numSamples), sampleRate_(sampleRate) {
        monitor_.startMeasurement();
    }
    
    ~WCETScopeTimer() noexcept {
        monitor_.endMeasurement(numSamples_, sampleRate_);
    }
    
    WCETScopeTimer(const WCETScopeTimer&) = delete;
    WCETScopeTimer& operator=(const WCETScopeTimer&) = delete;
    WCETScopeTimer(WCETScopeTimer&&) = delete;
    WCETScopeTimer& operator=(WCETScopeTimer&&) = delete;
    
private:
    WCETMonitor& monitor_;
    int numSamples_;
    double sampleRate_;
};

} // namespace profiling
} // namespace zenith
