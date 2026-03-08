/*
  ==============================================================================

    XRUNDetector.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #1)

    Detects and reports XRUNs (buffer underruns/overruns) in real-time.

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
 * @brief XRUN event type
 */
enum class XRUNType {
    Underrun,     // Audio callback took too long (buffer underrun)
    Overrun,      // Audio input came too fast (buffer overrun)
    Unknown       // Unable to determine type
};

//==============================================================================
/**
 * @brief XRUN event record
 */
struct XRUNEvent {
    XRUNType type;
    double timestamp = 0.0;           // Seconds since epoch
    double callbackDuration = 0.0;    // How long the callback took (ms)
    double bufferDuration = 0.0;      // Expected buffer duration (ms)
    double overrunAmount = 0.0;       // How much we over/under-ran (ms)
    int sampleRate = 0;
    int bufferSize = 0;
    juce::String description;

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case XRUNType::Underrun: typeStr = "UNDERRUN"; break;
            case XRUNType::Overrun: typeStr = "OVERRUN"; break;
            case XRUNType::Unknown: typeStr = "UNKNOWN"; break;
        }
        return "[" + typeStr + "] " +
               juce::String(callbackDuration, 2) + "ms callback " +
               "(expected " + juce::String(bufferDuration, 2) + "ms) - " +
               description;
    }
};

//==============================================================================
/**
 * @brief XRUN statistics
 */
struct XRUNStatistics {
    std::atomic<int> totalXRUNs{0};
    std::atomic<int> underruns{0};
    std::atomic<int> overruns{0};
    std::atomic<double> totalOverrunAmount{0.0};  // Cumulative ms
    std::atomic<double> worstCallbackDuration{0.0};  // Worst case (ms)
    std::atomic<double> averageCallbackDuration{0.0}; // Average (ms)
    std::atomic<int> consecutiveXRUNs{0};  // Streak detection
    std::atomic<double> timeSinceLastXRUN{0.0};  // Seconds

    juce::String toString() const {
        return "XRUN Stats: " +
               juce::String(totalXRUNs.load()) + " total (" +
               juce::String(underruns.load()) + " underruns, " +
               juce::String(overruns.load()) + " overruns), " +
               "worst: " + juce::String(worstCallbackDuration.load(), 2) + "ms";
    }
};

//==============================================================================
/**
 * @brief XRUN detection settings
 */
struct XRUNDetectionSettings {
    double warningThreshold = 0.8;    // Warn at 80% of buffer time
    double criticalThreshold = 0.95;  // Critical at 95% of buffer time
    bool enablePredictiveDetection = true;
    bool enableConsecutiveTracking = true;
    int maxConsecutiveBeforeAction = 5;  // Trigger action after 5 in a row
    double statisticsSmoothing = 0.1;    // EMA factor for stats
};

//==============================================================================
/**
 * @brief XRUN detector
 *
 * Features:
 * - Real-time callback duration monitoring
 * - Underrun/overrun detection
 * - Predictive XRUN warning (before it happens!)
 * - Consecutive XRUN tracking
 * - Comprehensive statistics
 * - User-configurable thresholds
 */
class XRUNDetector {
public:
    //==========================================================================
    XRUNDetector();
    ~XRUNDetector();

    //==========================================================================
    /**
     * @brief Start measuring callback duration
     * Call this at the START of your audio callback
     */
    void startCallback(int sampleRate, int bufferSize);

    //==========================================================================
    /**
     * @brief End measuring and check for XRUN
     * Call this at the END of your audio callback
     * @return true if XRUN occurred
     */
    bool endCallback();

    //==========================================================================
    /**
     * @brief Get the most recent XRUN event
     */
    XRUNEvent getLastXRUN() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastXRUN_;
    }

    //==========================================================================
    /**
     * @brief Get all XRUN events since last cleared
     */
    std::vector<XRUNEvent> getXRUNHistory() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return xrunHistory_;
    }

    //==========================================================================
    /**
     * @brief Clear XRUN history
     */
    void clearHistory() {
        std::lock_guard<std::mutex> lock(mutex_);
        xrunHistory_.clear();
    }

    //==========================================================================
    /**
     * @brief Get current statistics
     */
    XRUNStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

    //==========================================================================
    /**
     * @brief Get detection settings
     */
    XRUNDetectionSettings getSettings() const {
        return settings_;
    }

    //==========================================================================
    /**
     * @brief Set detection settings
     */
    void setSettings(const XRUNDetectionSettings& settings) {
        settings_ = settings;
    }

    //==========================================================================
    /**
     * @brief Check if we're in a consecutive XRUN streak
     */
    bool isInXRUNStreak() const {
        return statistics_.consecutiveXRUNs.load() >
               settings_.maxConsecutiveBeforeAction;
    }

    //==========================================================================
    /**
     * @brief Get time since last XRUN (in seconds)
     */
    double getTimeSinceLastXRUN() const {
        return statistics_.timeSinceLastXRUN.load();
    }

    //==========================================================================
    /**
     * @brief Predictive: Is XRUN likely to occur?
     * Based on recent callback duration trends
     * @return true if XRUN is predicted (and we should take action)
     */
    bool predictXRUN() const;

    //==========================================================================
    /**
     * @brief Get current callback duration percentage
     * @return 0.0 to 1.0+ (1.0 = 100% of buffer time)
     */
    double getCallbackPercentage() const {
        return currentCallbackPercentage_;
    }

    //==========================================================================
    /**
     * @brief Get warning threshold
     */
    double getWarningThreshold() const {
        return settings_.warningThreshold;
    }

    //==========================================================================
    /**
     * @brief Check if we're in warning territory
     */
    bool isNearWarning() const {
        return currentCallbackPercentage_ >= settings_.warningThreshold &&
               currentCallbackPercentage_ < settings_.criticalThreshold;
    }

    //==========================================================================
    /**
     * @brief Check if we're in critical territory
     */
    bool isCritical() const {
        return currentCallbackPercentage_ >= settings_.criticalThreshold;
    }

private:
    //==========================================================================
    void recordXRUN(const XRUNEvent& event);
    void updateStatistics(double callbackDuration, double bufferDuration);
    XRUNType classifyXRUN(double callbackDuration, double bufferDuration) const;

    //==========================================================================
    // Timing
    std::chrono::high_resolution_clock::time_point callbackStartTime_;
    double currentCallbackPercentage_ = 0.0;

    // Current callback info
    int currentSampleRate_ = 0;
    int currentBufferSize_ = 0;

    // XRUN tracking
    XRUNEvent lastXRUN_;
    std::vector<XRUNEvent> xrunHistory_;
    static constexpr int maxHistorySize = 100;

    // Statistics
    XRUNStatistics statistics_;

    // Settings
    XRUNDetectionSettings settings_;

    // Thread safety
    mutable std::mutex mutex_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XRUNDetector)
};

//==============================================================================
/**
 * @brief Singleton accessor for XRUN detector
 */
class XRUNDetectorHolder {
public:
    static XRUNDetector& getInstance() {
        static XRUNDetector instance;
        return instance;
    }

    XRUNDetectorHolder(const XRUNDetectorHolder&) = delete;
    XRUNDetectorHolder& operator=(const XRUNDetectorHolder&) = delete;

private:
    XRUNDetectorHolder() = default;
    ~XRUNDetectorHolder() = default;
};

} // namespace zenith
