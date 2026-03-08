/*
  ==============================================================================

    SRCQualityManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #2)

    Manages sample rate conversion quality vs performance trade-offs.

  ==============================================================================
*/

#pragma once

#include "SampleRateConverter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <map>

namespace zenith {

//==============================================================================
/**
 * @brief Quality profile
 */
struct QualityProfile {
    juce::String name;
    SRCQuality quality;
    double cpuCostMultiplier = 1.0;  // Relative to baseline
    double latencyMs = 0.0;
    double thdPercent = 0.0;          // Total harmonic distortion

    juce::String toString() const {
        return name + " (cost: " + juce::String(cpuCostMultiplier, 2) + "x, " +
               "latency: " + juce::String(latencyMs, 2) + "ms, " +
               "THD: " + juce::String(thdPercent, 4) + "%)";
    }
};

//==============================================================================
/**
 * @brief SRC quality manager
 *
 * Features:
 * - Quality profiles for different use cases
 * - Automatic quality selection based on CPU load
 * - Quality vs performance tracking
 * - Custom quality profiles
 */
class SRCQualityManager {
public:
    //==========================================================================
    SRCQualityManager();
    ~SRCQualityManager();

    //==========================================================================
    /**
     * @brief Get quality profile by name
     */
    QualityProfile getProfile(const juce::String& name) const;

    //==========================================================================
    /**
     * @brief Get all available profiles
     */
    std::vector<QualityProfile> getAllProfiles() const;

    //==========================================================================
    /**
     * @brief Add custom quality profile
     */
    void addProfile(const juce::String& name, const QualityProfile& profile);

    //==========================================================================
    /**
     * @brief Remove quality profile
     */
    void removeProfile(const juce::String& name);

    //==========================================================================
    /**
     * @brief Select profile based on CPU load
     * @param cpuUsagePercent Current CPU usage (0-100)
     * @return Recommended quality profile
     */
    QualityProfile selectProfileForCPULoad(double cpuUsagePercent) const;

    //==========================================================================
    /**
     * @brief Select profile based on conversion ratio
     * @param ratio Conversion ratio
     * @return Recommended quality profile
     */
    QualityProfile selectProfileForRatio(double ratio) const;

    //==========================================================================
    /**
     * @brief Get recommended quality combining CPU and ratio
     */
    QualityProfile getRecommendedQuality(
        double cpuUsagePercent,
        double ratio) const;

    //==========================================================================
    /**
     * @brief Set default profile
     */
    void setDefaultProfile(const juce::String& name) {
        defaultProfile_ = name;
    }

    //==========================================================================
    /**
     * @brief Get default profile
     */
    QualityProfile getDefaultProfile() const {
        return getProfile(defaultProfile_);
    }

    //==========================================================================
    /**
     * @brief Enable automatic quality adjustment
     */
    void setAutoAdjustEnabled(bool enable) {
        autoAdjustEnabled_ = enable;
    }

    //==========================================================================
    /**
     * @brief Check if auto-adjust is enabled
     */
    bool isAutoAdjustEnabled() const {
        return autoAdjustEnabled_;
    }

private:
    //==========================================================================
    void initializeDefaultProfiles();

    //==========================================================================
    // Quality profiles: name -> profile
    std::map<juce::String, QualityProfile> profiles_;

    // Settings
    juce::String defaultProfile_ = "Balanced";
    bool autoAdjustEnabled_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SRCQualityManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for SRC quality manager
 */
class SRCQualityManagerHolder {
public:
    static SRCQualityManager& getInstance() {
        static SRCQualityManager instance;
        return instance;
    }

    SRCQualityManagerHolder(const SRCQualityManagerHolder&) = delete;
    SRCQualityManagerHolder& operator=(const SRCQualityManagerHolder&) = delete;

private:
    SRCQualityManagerHolder() = default;
    ~SRCQualityManagerHolder() = default;
};

} // namespace zenith
