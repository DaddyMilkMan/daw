/*
  ==============================================================================

    LatencyCompensator.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #10)

    Automatic plugin delay compensation (PDL) for sample-accurate timing.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>

namespace zenith {

//==============================================================================
/**
 * @brief Latency compensation error type
 */
enum class LatencyCompensationError {
    InvalidLatencyValue,      // Negative or too large
    CompensationFailed,       // Compensation failed
    PluginNotReported,        // Plugin didn't report latency
    Overflow,                 // Delay compensation overflow
    Unknown
};

//==============================================================================
/**
 * @brief Latency compensation event
 */
struct LatencyCompensationEvent {
    LatencyCompensationError error;
    juce::String pluginId;
    juce::String description;
    int reportedLatency = 0;       // Samples
    int compensatedLatency = 0;    // Samples
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String errorStr;
        switch (error) {
            case LatencyCompensationError::InvalidLatencyValue: errorStr = "Invalid Latency"; break;
            case LatencyCompensationError::CompensationFailed: errorStr = "Comp Failed"; break;
            case LatencyCompensationError::PluginNotReported: errorStr = "Not Reported"; break;
            case LatencyCompensationError::Overflow: errorStr = "Overflow"; break;
            case LatencyCompensationError::Unknown: errorStr = "Unknown"; break;
        }
        return "[" + errorStr + "] " + pluginId + ": " + description;
    }
};

//==============================================================================
/**
 * @brief Plugin latency info
 */
struct PluginLatencyInfo {
    juce::String pluginId;
    int latencySamples = 0;         // Reported latency
    int compensatedSamples = 0;     // Actual compensation applied
    bool isValid = true;
    double timestamp = 0.0;

    juce::String toString() const {
        return pluginId + ": " +
               juce::String(latencySamples) + " samples latency, " +
               juce::String(compensatedSamples) + " compensated";
    }
};

//==============================================================================
/**
 * @brief Latency compensation statistics
 */
struct LatencyCompensationStatistics {
    int totalPluginsCompensated = 0;
    int totalLatencySamples = 0;
    int compensationErrors = 0;
    double averageLatency = 0.0;

    juce::String toString() const {
        return "PDL: " + juce::String(totalPluginsCompensated) + " plugins, " +
               juce::String(totalLatencySamples) + " samples total";
    }
};

//==============================================================================
/**
 * @brief Automatic latency compensator
 *
 * Features:
 * - Automatic plugin delay compensation
 * - Latency measurement and validation
 * - Per-track compensation
 * - Side-chain latency compensation
 * - Recording offset compensation
 */
class LatencyCompensator {
public:
    //==========================================================================
    LatencyCompensator();
    ~LatencyCompensator();

    //==========================================================================
    /**
     * @brief Register plugin latency
     * @param pluginId Plugin identifier
     * @param latencySamples Reported latency in samples
     */
    void registerPluginLatency(const juce::String& pluginId, int latencySamples);

    //==========================================================================
    /**
     * @brief Update plugin latency
     * @param pluginId Plugin identifier
     * @param latencySamples New latency in samples
     */
    void updatePluginLatency(const juce::String& pluginId, int latencySamples);

    //==========================================================================
    /**
     * @brief Remove plugin latency
     * @param pluginId Plugin identifier
     */
    void removePlugin(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Get total latency for all plugins
     */
    int getTotalLatency() const;

    //==========================================================================
    /**
     * @brief Get compensation for specific plugin
     */
    int getPluginCompensation(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Get all plugin latencies
     */
    std::vector<PluginLatencyInfo> getAllPluginLatencies() const;

    //==========================================================================
    /**
     * @brief Validate latency value
     * @return true if latency is valid (>= 0 and reasonable)
     */
    bool isValidLatency(int latencySamples) const;

    //==========================================================================
    /**
     * @brief Enable/disable compensation
     */
    void setEnabled(bool enable) {
        enabled_ = enable;
    }

    //==========================================================================
    /**
     * @brief Check if compensation is enabled
     */
    bool isEnabled() const {
        return enabled_;
    }

    //==========================================================================
    /**
     * @brief Get statistics
     */
    LatencyCompensationStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

    //==========================================================================
    /**
     * @brief Clear all plugin latencies
     */
    void clear();

    //==========================================================================
    /**
     * @brief Register callback for compensation events
     * @param callback Function to call when event occurs
     */
    void setEventCallback(std::function<void(const LatencyCompensationEvent&)> callback) {
        eventCallback_ = callback;
    }

private:
    //==========================================================================
    void reportError(const LatencyCompensationEvent& event);
    void updateStatistics();

    //==========================================================================
    // Plugin latency tracking: pluginId -> latency info
    std::map<juce::String, PluginLatencyInfo> pluginLatencies_;

    // Settings
    bool enabled_ = true;

    // Statistics
    LatencyCompensationStatistics statistics_;

    // Callbacks
    std::function<void(const LatencyCompensationEvent&)> eventCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LatencyCompensator)
};

//==============================================================================
/**
 * @brief Singleton accessor for latency compensator
 */
class LatencyCompensatorHolder {
public:
    static LatencyCompensator& getInstance() {
        static LatencyCompensator instance;
        return instance;
    }

    LatencyCompensatorHolder(const LatencyCompensatorHolder&) = delete;
    LatencyCompensatorHolder& operator=(const LatencyCompensatorHolder&) = delete;

private:
    LatencyCompensatorHolder() = default;
    ~LatencyCompensatorHolder() = default;
};

} // namespace zenith
