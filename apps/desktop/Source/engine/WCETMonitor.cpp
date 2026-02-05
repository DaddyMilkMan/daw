/**
 * @file WCETMonitor.cpp
 * @brief WCET monitoring implementation
 */

#include "WCETMonitor.h"

namespace zenith {
namespace profiling {

WCETMonitor::WCETMonitor() = default;

void WCETMonitor::setBudgetMs(double budgetMs) noexcept {
    budgetUs_.store(static_cast<int64_t>(budgetMs * 1000.0));
}

void WCETMonitor::startMeasurement() noexcept {
    if (!enabled_.load()) return;
    startTimeUs_.store(getCurrentTimeUs(), std::memory_order_relaxed);
}

bool WCETMonitor::endMeasurement(int numSamples, double sampleRate) noexcept {
    if (!enabled_.load()) return true;
    
    int64_t endTimeUs = getCurrentTimeUs();
    int64_t startTimeUs = startTimeUs_.load(std::memory_order_relaxed);
    int64_t executionTimeUs = endTimeUs - startTimeUs;
    
    if (executionTimeUs < 0) {
        executionTimeUs = 0;
    }
    
    currentExecutionTimeUs_.store(executionTimeUs, std::memory_order_relaxed);
    
    updateMax(executionTimeUs);
    updateMin(executionTimeUs);
    updateEMA(executionTimeUs);
    
    totalCallbacks_.fetch_add(1, std::memory_order_relaxed);
    
    int64_t budgetUs = budgetUs_.load(std::memory_order_relaxed);
    if (budgetUs <= 0) {
        budgetUs = static_cast<int64_t>((numSamples / sampleRate) * 1000000.0);
    }
    
    bool withinBudget = executionTimeUs <= budgetUs;
    
    if (!withinBudget) {
        totalOverruns_.fetch_add(1, std::memory_order_relaxed);
        consecutiveOverruns_.fetch_add(1, std::memory_order_relaxed);
    } else {
        int64_t currentStreak = consecutiveOverruns_.exchange(0, std::memory_order_relaxed);
        updateMaxConsecutive(currentStreak);
    }
    
    return withinBudget;
}

WCETStatistics WCETMonitor::getStatistics() const {
    WCETStatistics stats;
    
    stats.totalCallbacks = totalCallbacks_.load(std::memory_order_acquire);
    stats.overruns = totalOverruns_.load(std::memory_order_acquire);
    stats.consecutiveOverruns = consecutiveOverruns_.load(std::memory_order_acquire);
    stats.maxConsecutiveOverruns = maxConsecutiveOverruns_.load(std::memory_order_acquire);
    
    int64_t avgNs = avgExecutionTimeNs_.load(std::memory_order_acquire);
    stats.avgExecutionTimeUs = avgNs / 1000.0;
    
    stats.maxExecutionTimeUs = maxExecutionTimeUs_.load(std::memory_order_acquire);
    stats.minExecutionTimeUs = minExecutionTimeUs_.load(std::memory_order_acquire);
    if (stats.minExecutionTimeUs == INT64_MAX) {
        stats.minExecutionTimeUs = 0;
    }
    
    stats.currentExecutionTimeUs = currentExecutionTimeUs_.load(std::memory_order_acquire);
    
    int64_t budgetUs = budgetUs_.load(std::memory_order_acquire);
    if (budgetUs > 0 && stats.avgExecutionTimeUs > 0) {
        stats.budgetUtilizationPercent = (stats.avgExecutionTimeUs / budgetUs) * 100.0;
    }
    
    return stats;
}

void WCETMonitor::resetStatistics() noexcept {
    totalCallbacks_.store(0, std::memory_order_release);
    totalOverruns_.store(0, std::memory_order_release);
    consecutiveOverruns_.store(0, std::memory_order_release);
    maxConsecutiveOverruns_.store(0, std::memory_order_release);
    avgExecutionTimeNs_.store(0, std::memory_order_release);
    maxExecutionTimeUs_.store(0, std::memory_order_release);
    minExecutionTimeUs_.store(INT64_MAX, std::memory_order_release);
    currentExecutionTimeUs_.store(0, std::memory_order_release);
}

juce::String WCETMonitor::getReport() const {
    auto stats = getStatistics();
    
    juce::String report;
    report << "=== WCET Report ===\n";
    report << "Total Callbacks: " << stats.totalCallbacks << "\n";
    report << "Overruns: " << stats.overruns << "\n";
    report << "Consecutive Overruns: " << stats.consecutiveOverruns << "\n";
    report << "Max Consecutive Overruns: " << stats.maxConsecutiveOverruns << "\n";
    report << "\n";
    report << "Execution Times:\n";
    report << "  Current: " << juce::String(stats.currentExecutionTimeUs, 2) << " us\n";
    report << "  Average: " << juce::String(stats.avgExecutionTimeUs, 2) << " us\n";
    report << "  Min: " << juce::String(stats.minExecutionTimeUs, 2) << " us\n";
    report << "  Max: " << juce::String(stats.maxExecutionTimeUs, 2) << " us\n";
    report << "\n";
    report << "Budget: " << juce::String(getBudgetMs(), 2) << " ms\n";
    report << "Utilization: " << juce::String(stats.budgetUtilizationPercent, 1) << "%\n";
    report << "Status: ";
    
    if (stats.isCritical()) {
        report << "CRITICAL - Xruns likely!";
    } else if (stats.hasRisk()) {
        report << "WARNING - Monitor closely";
    } else {
        report << "HEALTHY";
    }
    
    return report;
}

int64_t WCETMonitor::getCurrentTimeUs() noexcept {
    using namespace std::chrono;
    auto now = steady_clock::now();
    return duration_cast<microseconds>(now.time_since_epoch()).count();
}

void WCETMonitor::updateEMA(int64_t newValueUs) noexcept {
    int64_t newValueNs = newValueUs * 1000;
    int64_t oldAvgNs = avgExecutionTimeNs_.load(std::memory_order_relaxed);
    
    if (oldAvgNs == 0) {
        avgExecutionTimeNs_.store(newValueNs, std::memory_order_relaxed);
    } else {
        double alpha = EMA_ALPHA;
        double newAvgNs = (alpha * newValueNs) + ((1.0 - alpha) * oldAvgNs);
        avgExecutionTimeNs_.store(static_cast<int64_t>(newAvgNs), std::memory_order_relaxed);
    }
}

void WCETMonitor::updateMax(int64_t value) noexcept {
    int64_t current = maxExecutionTimeUs_.load(std::memory_order_relaxed);
    while (value > current && !maxExecutionTimeUs_.compare_exchange_weak(
        current, value, std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
}

void WCETMonitor::updateMin(int64_t value) noexcept {
    int64_t current = minExecutionTimeUs_.load(std::memory_order_relaxed);
    while (value < current && !minExecutionTimeUs_.compare_exchange_weak(
        current, value, std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
}

void WCETMonitor::updateMaxConsecutive(int64_t value) noexcept {
    int64_t current = maxConsecutiveOverruns_.load(std::memory_order_relaxed);
    while (value > current && !maxConsecutiveOverruns_.compare_exchange_weak(
        current, value, std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
}

} // namespace profiling
} // namespace zenith
