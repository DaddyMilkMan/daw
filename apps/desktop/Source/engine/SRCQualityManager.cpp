/*
  ==============================================================================

    SRCQualityManager.cpp
    Implementation of SRC quality management

  ==============================================================================
*/

#include "SRCQualityManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// SRCQualityManager Implementation
//==============================================================================

SRCQualityManager::SRCQualityManager() {
    initializeDefaultProfiles();
    std::cout << "SRCQualityManager: Initialized" << std::endl;
}

SRCQualityManager::~SRCQualityManager() {
    std::cout << "SRCQualityManager: Shut down" << std::endl;
}

//==============================================================================
QualityProfile SRCQualityManager::getProfile(const juce::String& name) const {
    auto it = profiles_.find(name);
    if (it != profiles_.end()) {
        return it->second;
    }

    // Return default if not found
    return profiles_.at(defaultProfile_);
}

//==============================================================================
std::vector<QualityProfile> SRCQualityManager::getAllProfiles() const {
    std::vector<QualityProfile> result;
    for (const auto& pair : profiles_) {
        result.push_back(pair.second);
    }
    return result;
}

//==============================================================================
void SRCQualityManager::addProfile(const juce::String& name, const QualityProfile& profile) {
    profiles_[name] = profile;
}

//==============================================================================
void SRCQualityManager::removeProfile(const juce::String& name) {
    profiles_.erase(name);
}

//==============================================================================
QualityProfile SRCQualityManager::selectProfileForCPULoad(double cpuUsagePercent) const {
    // Select based on CPU load
    if (cpuUsagePercent >= 85.0) {
        // Very high CPU - use fastest
        return getProfile("Fastest");
    } else if (cpuUsagePercent >= 70.0) {
        // High CPU - use low quality
        return getProfile("Low");
    } else if (cpuUsagePercent >= 50.0) {
        // Medium CPU - use medium quality
        return getProfile("Medium");
    } else if (cpuUsagePercent >= 30.0) {
        // Low CPU - use high quality
        return getProfile("High");
    } else {
        // Very low CPU - use best quality
        return getProfile("Best");
    }
}

//==============================================================================
QualityProfile SRCQualityManager::selectProfileForRatio(double ratio) const {
    // Select based on conversion ratio
    if (ratio > 4.0 || ratio < 0.25) {
        // Extreme ratio - need best quality
        return getProfile("Best");
    } else if (ratio > 2.0 || ratio < 0.5) {
        // Large ratio - use high quality
        return getProfile("High");
    } else if (ratio > 1.5 || ratio < 0.67) {
        // Medium ratio - use medium quality
        return getProfile("Medium");
    } else {
        // Small ratio - can use lower quality
        return getProfile("Low");
    }
}

//==============================================================================
QualityProfile SRCQualityManager::getRecommendedQuality(
    double cpuUsagePercent,
    double ratio) const
{
    if (!autoAdjustEnabled_) {
        return getDefaultProfile();
    }

    // Get recommendations from both methods
    QualityProfile cpuProfile = selectProfileForCPULoad(cpuUsagePercent);
    QualityProfile ratioProfile = selectProfileForRatio(ratio);

    // Use the lower quality (more conservative)
    if (cpuProfile.cpuCostMultiplier < ratioProfile.cpuCostMultiplier) {
        return cpuProfile;
    } else {
        return ratioProfile;
    }
}

//==============================================================================
// Private Methods
//==============================================================================

void SRCQualityManager::initializeDefaultProfiles() {
    // Fastest profile
    QualityProfile fastest;
    fastest.name = "Fastest";
    fastest.quality = SRCQuality::Fastest;
    fastest.cpuCostMultiplier = 0.2;  // 20% of baseline
    fastest.latencyMs = 0.1;
    fastest.thdPercent = 3.0;
    profiles_["Fastest"] = fastest;

    // Low quality profile
    QualityProfile low;
    low.name = "Low";
    low.quality = SRCQuality::Low;
    low.cpuCostMultiplier = 0.5;
    low.latencyMs = 0.2;
    low.thdPercent = 1.0;
    profiles_["Low"] = low;

    // Medium quality profile
    QualityProfile medium;
    medium.name = "Medium";
    medium.quality = SRCQuality::Medium;
    medium.cpuCostMultiplier = 1.0;  // Baseline
    medium.latencyMs = 0.5;
    medium.thdPercent = 0.1;
    profiles_["Medium"] = medium;

    // High quality profile
    QualityProfile high;
    high.name = "High";
    high.quality = SRCQuality::High;
    high.cpuCostMultiplier = 2.0;
    high.latencyMs = 1.0;
    high.thdPercent = 0.01;
    profiles_["High"] = high;

    // Best quality profile
    QualityProfile best;
    best.name = "Best";
    best.quality = SRCQuality::Best;
    best.cpuCostMultiplier = 5.0;
    best.latencyMs = 2.0;
    best.thdPercent = 0.001;
    profiles_["Best"] = best;

    // Balanced profile (default)
    QualityProfile balanced;
    balanced.name = "Balanced";
    balanced.quality = SRCQuality::High;
    balanced.cpuCostMultiplier = 1.5;
    balanced.latencyMs = 0.7;
    balanced.thdPercent = 0.05;
    profiles_["Balanced"] = balanced;

    std::cout << "SRCQualityManager: Initialized " << profiles_.size()
              << " quality profiles" << std::endl;
}

} // namespace zenith
