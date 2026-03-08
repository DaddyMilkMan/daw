/*
  ==============================================================================

    PredictiveMonitor.h
    Created: 2026-02-20
    Predictive memory monitoring - catches issues BEFORE critical

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <deque>
#include "OOMHandler.h"

namespace zenith {

//==============================================================================
/**
 * @brief Memory trend prediction
 */
enum class MemoryTrend {
    Improving,      // Memory usage decreasing
    Stable,         // Memory usage flat
    SlowIncrease,   // Gradual increase (concerning)
    FastIncrease,   // Rapid increase (critical)
    Unknown
};

//==============================================================================
/**
 * @brief Memory prediction result
 */
struct MemoryPrediction {
    MemoryTrend trend = MemoryTrend::Unknown;
    double slope = 0.0;              // MB per second
    double predictedSecondsToOOM = 0.0;  // Seconds until OOM at current rate
    juce::String recommendation;
    int confidence = 0;              // 0-100%

    juce::String toString() const {
        juce::String trendStr;
        switch (trend) {
            case MemoryTrend::Improving: trendStr = "Improving"; break;
            case MemoryTrend::Stable: trendStr = "Stable"; break;
            case MemoryTrend::SlowIncrease: trendStr = "Slowly Increasing"; break;
            case MemoryTrend::FastIncrease: trendStr = "RAPIDLY INCREASING"; break;
            default: trendStr = "Unknown"; break;
        }

        return juce::String::formatted(
            "Trend: %s | Rate: %.1f MB/s | OOM in: %.0f sec | Confidence: %d%%",
            trendStr.toRawUTF8(), slope, predictedSecondsToOOM, confidence
        );
    }
};

//==============================================================================
/**
 * @brief Predictive memory monitor
 *
 * Goes beyond reactive recovery by PREDICTING issues before they happen.
 * Uses linear regression on memory history to forecast trends.
 */
class PredictiveMonitor {
public:
    //==========================================================================
    PredictiveMonitor();
    ~PredictiveMonitor();

    //==========================================================================
    /**
     * @brief Add memory sample to history
     */
    void addSample(const MemorySnapshot& snapshot);

    //==========================================================================
    /**
     * @brief Get current prediction
     */
    MemoryPrediction predict();

    //==========================================================================
    /**
     * @brief Check if action recommended
     */
    bool isActionRecommended() const;

    //==========================================================================
    /**
     * @brief Get recommended action based on prediction
     */
    OOMRecoveryAction getRecommendedAction() const;

    //==========================================================================
    /**
     * @brief Configure monitoring parameters
     */
    struct Config {
        size_t historySize = 60;           // Keep 60 samples (1 minute at 1Hz)
        double warningThresholdMBPerSec = 10.0;  // Warn if growing >10 MB/s
        double criticalThresholdMBPerSec = 50.0; // Critical if growing >50 MB/s
        int minConfidence = 70;            // Only act if confidence >= 70%
    };

    void setConfig(const Config& config) { config_ = config; }

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static PredictiveMonitor& getInstance();

private:
    //==========================================================================
    Config config_;

    // History of memory snapshots
    std::deque<MemorySnapshot> history_;

    // Cached prediction
    MemoryPrediction lastPrediction_;

    // Perform linear regression to calculate trend
    MemoryPrediction calculateTrend();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PredictiveMonitor)
};

} // namespace zenith
