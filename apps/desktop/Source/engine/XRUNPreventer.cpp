/*
  ==============================================================================

    XRUNPreventer.cpp
    Implementation of predictive XRUN prevention

  ==============================================================================
*/

#include "XRUNPreventer.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// XRUNPreventer Implementation
//==============================================================================

XRUNPreventer::XRUNPreventer() {
    std::cout << "XRUNPreventer: Initialized" << std::endl;
}

XRUNPreventer::~XRUNPreventer() {
    std::cout << "XRUNPreventer: Shut down (" <<
              actionsTaken_.load() << " actions taken)" << std::endl;
}

//==============================================================================
PreventionResult XRUNPreventer::update() {
    if (!enabled_) {
        return {PreventionAction::None, "Prevention disabled"};
    }

    // Check if action is needed
    PreventionResult result = decideAction();

    // Execute action if needed
    if (result.action != PreventionAction::None) {
        actionsTaken_++;

        if (actionCallback_) {
            actionCallback_(result);
        }

        // Log important actions
        if (result.action == PreventionAction::IncreaseBufferSize ||
            result.action == PreventionAction::EmergencyStop) {
            std::cout << "XRUNPreventer: " << result.toString() << std::endl;
        }
    }

    return result;
}

//==============================================================================
void XRUNPreventer::updateCPUUsage(double usage) {
    currentCPUUsage_ = juce::jlimit(0.0, 100.0, usage);
    updateCPUHistory(currentCPUUsage_);
}

//==============================================================================
double XRUNPreventer::getAverageCPUUsage(double timeWindowSeconds) const {
    if (cpuHistory_.empty()) {
        return 0.0;
    }

    double currentTime = cpuHistory_.back().timestamp;
    double sum = 0.0;
    int count = 0;

    for (auto it = cpuHistory_.rbegin(); it != cpuHistory_.rend(); ++it) {
        if (currentTime - it->timestamp <= timeWindowSeconds) {
            sum += it->usage;
            count++;
        } else {
            break;
        }
    }

    return count > 0 ? sum / count : 0.0;
}

//==============================================================================
bool XRUNPreventer::isXRUNPredicted() const {
    // Check XRUN detector prediction
    auto& xrunDetector = XRUNDetectorHolder::getInstance();
    if (xrunDetector.predictXRUN()) {
        return true;
    }

    // Check CPU trend
    double trend = calculateCPUTrend();
    if (trend > 5.0) {  // CPU rising faster than 5% per second
        return true;
    }

    // Check if CPU is in critical territory
    if (isCPUCritical()) {
        return true;
    }

    return false;
}

//==============================================================================
int XRUNPreventer::getRecommendedBufferSize() const {
    // Find next larger buffer size based on CPU load
    int recommended = currentBufferSize_;

    if (isCPUCritical()) {
        // Critical: jump to next size
        for (int size : settings_.bufferSizes) {
            if (size > currentBufferSize_ && size <= settings_.maxBufferSize) {
                recommended = size;
                break;
            }
        }
    } else if (isCPUWarning()) {
        // Warning: consider next size if we're trending up
        double trend = calculateCPUTrend();
        if (trend > 2.0) {  // Rising faster than 2% per second
            for (int size : settings_.bufferSizes) {
                if (size > currentBufferSize_ && size <= settings_.maxBufferSize) {
                    recommended = size;
                    break;
                }
            }
        }
    }

    return recommended;
}

//==============================================================================
// Private Methods
//==============================================================================

PreventionResult XRUNPreventer::decideAction() {
    // Check if XRUN is predicted
    if (isXRUNPredicted()) {
        // Strategy: Try least disruptive first

        // 1. Auto-increase buffer size (if enabled)
        if (settings_.enableAutoBufferIncrease) {
            return increaseBufferSize();
        }

        // 2. Suspend non-critical plugins (if enabled)
        if (settings_.enablePluginSuspension) {
            return suspendPlugins();
        }

        // 3. Reduce processing quality (if enabled)
        if (settings_.enableQualityReduction) {
            return reduceQuality();
        }

        // 4. Warn user
        return warnUser();
    }

    // Check if CPU is critical (might need immediate action)
    if (isCPUCritical()) {
        // Critical CPU usage - may need emergency action
        if (currentCPUUsage_ >= 95.0) {
            PreventionResult result;
            result.action = PreventionAction::EmergencyStop;
            result.description = "CPU usage critical (" +
                                juce::String(currentCPUUsage_, 1) + "%)";
            result.cpuUsageBefore = currentCPUUsage_;
            result.success = true;
            return result;
        }
    }

    // No action needed
    return {PreventionAction::None, ""};
}

PreventionResult XRUNPreventer::increaseBufferSize() {
    PreventionResult result;
    result.action = PreventionAction::IncreaseBufferSize;
    result.cpuUsageBefore = currentCPUUsage_;

    int recommended = getRecommendedBufferSize();

    if (recommended > currentBufferSize_) {
        result.description = "Increase buffer from " +
                            juce::String(currentBufferSize_) +
                            " to " + juce::String(recommended) +
                            " samples (CPU: " +
                            juce::String(currentCPUUsage_, 1) + "%)";
        result.newBufferSize = recommended;
        result.success = true;
    } else {
        result.action = PreventionAction::None;
        result.description = "Already at max buffer size";
        result.success = false;
    }

    return result;
}

PreventionResult XRUNPreventer::suspendPlugins() {
    PreventionResult result;
    result.action = PreventionAction::SuspendPlugins;
    result.cpuUsageBefore = currentCPUUsage_;

    result.description = "Suspend non-critical plugins (CPU: " +
                        juce::String(currentCPUUsage_, 1) + "%)";
    result.success = true;

    return result;
}

PreventionResult XRUNPreventer::reduceQuality() {
    PreventionResult result;
    result.action = PreventionAction::ReduceProcessing;
    result.cpuUsageBefore = currentCPUUsage_;

    result.description = "Reduce processing quality (CPU: " +
                        juce::String(currentCPUUsage_, 1) + "%)";
    result.success = true;

    return result;
}

PreventionResult XRUNPreventer::warnUser() {
    PreventionResult result;
    result.action = PreventionAction::WarnUser;

    result.description = "Warning: High CPU usage (" +
                        juce::String(currentCPUUsage_, 1) + "%) - " +
                        "XRUN may occur";
    result.success = true;

    return result;
}

void XRUNPreventer::updateCPUHistory(double usage) {
    CPUSnapshot snapshot;
    snapshot.usage = usage;
    snapshot.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

    cpuHistory_.push_back(snapshot);

    // Limit history size
    if (static_cast<int>(cpuHistory_.size()) > maxHistorySize) {
        cpuHistory_.erase(cpuHistory_.begin());
    }

    // Remove old samples outside time window
    double currentTime = snapshot.timestamp;
    cpuHistory_.erase(
        std::remove_if(cpuHistory_.begin(), cpuHistory_.end(),
            [currentTime, this](const CPUSnapshot& s) {
                return currentTime - s.timestamp > historyWindowSeconds;
            }),
        cpuHistory_.end()
    );
}

double XRUNPreventer::calculateCPUTrend() const {
    if (cpuHistory_.size() < 2) {
        return 0.0;
    }

    // Calculate slope of CPU usage over time (percent per second)
    const CPUSnapshot& oldest = cpuHistory_.front();
    const CPUSnapshot& newest = cpuHistory_.back();

    double timeDelta = newest.timestamp - oldest.timestamp;
    if (timeDelta < 0.1) {  // Need at least 100ms
        return 0.0;
    }

    double usageDelta = newest.usage - oldest.usage;
    return usageDelta / timeDelta;  // Percent per second
}

} // namespace zenith
