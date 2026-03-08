/*
  ==============================================================================

    XRUNPreventer.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #1)

    Predictive XRUN prevention using CPU monitoring and adaptive strategies.

  ==============================================================================
*/

#pragma once

#include "XRUNDetector.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>
#include <atomic>

namespace zenith {

//==============================================================================
/**
 * @brief Prevention action type
 */
enum class PreventionAction {
    None,                       // No action needed
    IncreaseBufferSize,         // Increase buffer size
    SuspendPlugins,             // Suspend non-critical plugins
    ReduceProcessing,           // Reduce processing quality
    WarnUser,                   // Just warn the user
    EmergencyStop               // Stop playback to prevent crash
};

//==============================================================================
/**
 * @brief Prevention action result
 */
struct PreventionResult {
    PreventionAction action;
    juce::String description;
    bool success = false;
    double cpuUsageBefore = 0.0;
    double cpuUsageAfter = 0.0;
    int newBufferSize = 0;

    juce::String toString() const {
        juce::String actionStr;
        switch (action) {
            case PreventionAction::None: actionStr = "None"; break;
            case PreventionAction::IncreaseBufferSize: actionStr = "Increase Buffer"; break;
            case PreventionAction::SuspendPlugins: actionStr = "Suspend Plugins"; break;
            case PreventionAction::ReduceProcessing: actionStr = "Reduce Quality"; break;
            case PreventionAction::WarnUser: actionStr = "Warn User"; break;
            case PreventionAction::EmergencyStop: actionStr = "Emergency Stop"; break;
        }
        return "[" + actionStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Prevention settings
 */
struct PreventionSettings {
    double cpuWarningThreshold = 70.0;      // Warn at 70% CPU
    double cpuCriticalThreshold = 85.0;     // Critical at 85% CPU
    double xrunPredictionThreshold = 0.85;  // Predict XRUN at 85% buffer usage
    bool enableAutoBufferIncrease = true;
    bool enablePluginSuspension = true;
    bool enableQualityReduction = false;     // Optional: reduce quality
    std::vector<int> bufferSizes = {64, 128, 256, 512, 1024, 2048};
    int maxBufferSize = 2048;
};

//==============================================================================
/**
 * @brief CPU usage snapshot
 */
struct CPUSnapshot {
    double usage = 0.0;         // Percentage
    double timestamp = 0.0;     // Seconds
    int coreCount = 0;
};

//==============================================================================
/**
 * @brief XRUN preventer
 *
 * Features:
 * - Predictive XRUN prevention (before it happens!)
 * - CPU load monitoring
 * - Automatic buffer size adjustment
 * - Plugin suspension when overloaded
 * - Multiple prevention strategies
 * - User-configurable thresholds
 */
class XRUNPreventer {
public:
    //==========================================================================
    XRUNPreventer();
    ~XRUNPreventer();

    //==========================================================================
    /**
     * @brief Update and check if prevention action needed
     * Call this periodically (e.g., every 100ms)
     * @return Action to take (if any)
     */
    PreventionResult update();

    //==========================================================================
    /**
     * @brief Update CPU usage manually
     * @param usage CPU usage percentage (0-100)
     */
    void updateCPUUsage(double usage);

    //==========================================================================
    /**
     * @brief Get current CPU usage
     */
    double getCurrentCPUUsage() const {
        return currentCPUUsage_;
    }

    //==========================================================================
    /**
     * @brief Get recent CPU usage history
     */
    std::vector<CPUSnapshot> getCPUHistory() const {
        return cpuHistory_;
    }

    //==========================================================================
    /**
     * @brief Get average CPU usage over time window
     * @param timeWindowSeconds Time window to average
     */
    double getAverageCPUUsage(double timeWindowSeconds = 1.0) const;

    //==========================================================================
    /**
     * @brief Check if CPU usage is in warning territory
     */
    bool isCPUWarning() const {
        return currentCPUUsage_ >= settings_.cpuWarningThreshold &&
               currentCPUUsage_ < settings_.cpuCriticalThreshold;
    }

    //==========================================================================
    /**
     * @brief Check if CPU usage is critical
     */
    bool isCPUCritical() const {
        return currentCPUUsage_ >= settings_.cpuCriticalThreshold;
    }

    //==========================================================================
    /**
     * @brief Check if XRUN is predicted
     * Based on CPU trend and XRUN detector predictions
     */
    bool isXRUNPredicted() const;

    //==========================================================================
    /**
     * @brief Get recommended buffer size
     * Returns suggested buffer size based on current load
     */
    int getRecommendedBufferSize() const;

    //==========================================================================
    /**
     * @brief Get current settings
     */
    PreventionSettings getSettings() const {
        return settings_;
    }

    //==========================================================================
    /**
     * @brief Set prevention settings
     */
    void setSettings(const PreventionSettings& settings) {
        settings_ = settings;
    }

    //==========================================================================
    /**
     * @brief Set current buffer size
     */
    void setCurrentBufferSize(int size) {
        currentBufferSize_ = size;
    }

    //==========================================================================
    /**
     * @brief Get current buffer size
     */
    int getCurrentBufferSize() const {
        return currentBufferSize_;
    }

    //==========================================================================
    /**
     * @brief Register callback for prevention actions
     * @param callback Function to call when action is needed
     */
    void setActionCallback(std::function<void(const PreventionResult&)> callback) {
        actionCallback_ = callback;
    }

    //==========================================================================
    /**
     * @brief Enable/disable prevention
     */
    void setEnabled(bool enable) {
        enabled_ = enable;
    }

    //==========================================================================
    /**
     * @brief Check if prevention is enabled
     */
    bool isEnabled() const {
        return enabled_;
    }

    //==========================================================================
    /**
     * @brief Get statistics
     */
    juce::String getStatistics() const {
        return "XRUN Prevention: " +
               juce::String(actionsTaken_.load()) + " actions taken, " +
               "CPU: " + juce::String(currentCPUUsage_, 1) + "%";
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        actionsTaken_.store(0);
        cpuHistory_.clear();
    }

private:
    //==========================================================================
    PreventionResult decideAction();
    PreventionResult increaseBufferSize();
    PreventionResult suspendPlugins();
    PreventionResult reduceQuality();
    PreventionResult warnUser();
    void updateCPUHistory(double usage);
    double calculateCPUTrend() const;

    //==========================================================================
    // Settings
    PreventionSettings settings_;
    bool enabled_ = true;

    // Current state
    double currentCPUUsage_ = 0.0;
    int currentBufferSize_ = 512;

    // CPU history
    std::vector<CPUSnapshot> cpuHistory_;
    static constexpr int maxHistorySize = 100;
    static constexpr double historyWindowSeconds = 5.0;

    // Statistics
    std::atomic<int> actionsTaken_{0};

    // Callbacks
    std::function<void(const PreventionResult&)> actionCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XRUNPreventer)
};

//==============================================================================
/**
 * @brief Singleton accessor for XRUN preventer
 */
class XRUNPreventerHolder {
public:
    static XRUNPreventer& getInstance() {
        static XRUNPreventer instance;
        return instance;
    }

    XRUNPreventerHolder(const XRUNPreventerHolder&) = delete;
    XRUNPreventerHolder& operator=(const XRUNPreventerHolder&) = delete;

private:
    XRUNPreventerHolder() = default;
    ~XRUNPreventerHolder() = default;
};

} // namespace zenith
