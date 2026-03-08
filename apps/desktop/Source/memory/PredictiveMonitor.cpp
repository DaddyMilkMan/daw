/*
  ==============================================================================

    PredictiveMonitor.cpp
    Implementation of predictive memory monitoring

  ==============================================================================
*/

#include "PredictiveMonitor.h"
#include <iostream>
#include <numeric>
#include <algorithm>

namespace zenith {

//==============================================================================
PredictiveMonitor::PredictiveMonitor() {
    std::cout << "PredictiveMonitor: Initialized" << std::endl;
}

//==============================================================================
PredictiveMonitor::~PredictiveMonitor() {
    std::cout << "PredictiveMonitor: Shut down" << std::endl;
}

//==============================================================================
void PredictiveMonitor::addSample(const MemorySnapshot& snapshot) {
    history_.push_back(snapshot);

    // Maintain history size limit
    while (history_.size() > config_.historySize) {
        history_.pop_front();
    }

    // Update prediction when we have enough samples
    if (history_.size() >= 5) {
        lastPrediction_ = calculateTrend();
    }
}

//==============================================================================
MemoryPrediction PredictiveMonitor::predict() {
    if (history_.size() < 5) {
        MemoryPrediction pred;
        pred.trend = MemoryTrend::Unknown;
        pred.confidence = 0;
        pred.recommendation = "Insufficient data";
        return pred;
    }

    return lastPrediction_;
}

//==============================================================================
bool PredictiveMonitor::isActionRecommended() const {
    if (lastPrediction_.confidence < config_.minConfidence) {
        return false;
    }

    return lastPrediction_.trend == MemoryTrend::FastIncrease ||
           lastPrediction_.predictedSecondsToOOM < 30.0;  // < 30 seconds to OOM
}

//==============================================================================
OOMRecoveryAction PredictiveMonitor::getRecommendedAction() const {
    OOMRecoveryAction action;

    if (lastPrediction_.predictedSecondsToOOM < 10.0) {
        // Critical - immediate action
        action.type = OOMRecoveryAction::SaveProjectAndExit;
        action.description = "PREDICTED OOM IMMINENT - Emergency save";
        action.priority = 10;
    } else if (lastPrediction_.predictedSecondsToOOM < 60.0) {
        // Urgent - aggressive recovery
        action.type = OOMRecoveryAction::ClosePlugins;
        action.description = "PREDICTED OOM soon - Closing plugins";
        action.memoryToFree = 1024 * 1024 * 1024;  // 1GB
        action.priority = 8;
    } else if (lastPrediction_.slope > config_.criticalThresholdMBPerSec) {
        // Fast increase - suspend and free
        action.type = OOMRecoveryAction::FreeUnusedMemory;
        action.description = "PREDICTED Fast memory increase - Free unused";
        action.memoryToFree = 500 * 1024 * 1024;  // 500MB
        action.priority = 6;
    } else if (lastPrediction_.slope > config_.warningThresholdMBPerSec) {
        // Slow increase - drop caches
        action.type = OOMRecoveryAction::DropCache;
        action.description = "PREDICTED Memory increasing - Dropping caches";
        action.memoryToFree = 100 * 1024 * 1024;  // 100MB
        action.priority = 3;
    } else {
        action.type = OOMRecoveryAction::None;
        action.description = "Memory usage stable";
        action.priority = 0;
    }

    return action;
}

//==============================================================================
MemoryPrediction PredictiveMonitor::calculateTrend() {
    MemoryPrediction pred;

    // Need at least 5 samples for meaningful trend
    if (history_.size() < 5) {
        pred.trend = MemoryTrend::Unknown;
        pred.confidence = 0;
        return pred;
    }

    // Perform linear regression on memory usage
    // y = mx + b, where m is slope (MB/s), b is intercept

    size_t n = history_.size();
    std::vector<double> x(n);
    std::vector<double> y(n);

    double now = history_.back().timestamp.toMilliseconds();

    for (size_t i = 0; i < n; ++i) {
        // Time in seconds (relative to now)
        x[i] = (history_[i].timestamp.toMilliseconds() - now) / 1000.0;
        // Memory in MB
        y[i] = static_cast<double>(history_[i].processMemoryUsed) / (1024.0 * 1024.0);
    }

    // Calculate sums for linear regression
    double sumX = std::accumulate(x.begin(), x.end(), 0.0);
    double sumY = std::accumulate(y.begin(), y.end(), 0.0);
    double sumXY = 0.0;
    double sumXX = 0.0;

    for (size_t i = 0; i < n; ++i) {
        sumXY += x[i] * y[i];
        sumXX += x[i] * x[i];
    }

    // Calculate slope (m) and intercept (b)
    double denominator = (n * sumXX - sumX * sumX);

    if (std::abs(denominator) < 0.0001) {
        // Not enough variation in time
        pred.trend = MemoryTrend::Stable;
        pred.slope = 0.0;
        pred.confidence = 50;
        return pred;
    }

    double slope = (n * sumXY - sumX * sumY) / denominator;
    double intercept = (sumY - slope * sumX) / n;

    pred.slope = slope;  // MB per second

    // Determine trend based on slope
    if (slope < -1.0) {
        pred.trend = MemoryTrend::Improving;
    } else if (slope < 1.0) {
        pred.trend = MemoryTrend::Stable;
    } else if (slope < config_.warningThresholdMBPerSec) {
        pred.trend = MemoryTrend::SlowIncrease;
    } else {
        pred.trend = MemoryTrend::FastIncrease;
    }

    // Calculate confidence based on R-squared
    double meanY = sumY / n;
    double ssTot = 0.0;
    double ssRes = 0.0;

    for (size_t i = 0; i < n; ++i) {
        double yPred = slope * x[i] + intercept;
        ssTot += (y[i] - meanY) * (y[i] - meanY);
        ssRes += (y[i] - yPred) * (y[i] - yPred);
    }

    double rSquared = (ssTot > 0.0001) ? (1.0 - ssRes / ssTot) : 0.0;
    pred.confidence = static_cast<int>(rSquared * 100.0);

    // Calculate predicted seconds to OOM
    // Current memory + predicted growth = total physical memory
    double currentMB = y.back();  // Most recent sample
    double totalMB = static_cast<double>(history_.back().totalPhysicalMemory) / (1024.0 * 1024.0);

    if (slope > 0.1) {  // Only predict if increasing
        double availableMB = totalMB - currentMB;
        pred.predictedSecondsToOOM = availableMB / slope;
    } else {
        pred.predictedSecondsToOOM = 999999.0;  // Not increasing
    }

    // Generate recommendation
    if (pred.trend == MemoryTrend::FastIncrease) {
        pred.recommendation = "CRITICAL: Memory increasing rapidly!";
    } else if (pred.trend == MemoryTrend::SlowIncrease) {
        pred.recommendation = "Warning: Memory gradually increasing";
    } else if (pred.trend == MemoryTrend::Improving) {
        pred.recommendation = "Good: Memory usage decreasing";
    } else {
        pred.recommendation = "Stable: Memory usage normal";
    }

    return pred;
}

//==============================================================================
PredictiveMonitor& PredictiveMonitor::getInstance() {
    static PredictiveMonitor instance;
    return instance;
}

} // namespace zenith
