/*
  ==============================================================================

    XRUNDetector.cpp
    Implementation of XRUN detection

  ==============================================================================
*/

#include "XRUNDetector.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// XRUNDetector Implementation
//==============================================================================

XRUNDetector::XRUNDetector() {
    std::cout << "XRUNDetector: Initialized" << std::endl;
}

XRUNDetector::~XRUNDetector() {
    std::cout << "XRUNDetector: Shut down (" <<
              statistics_.totalXRUNs.load() << " XRUNs detected)" << std::endl;
}

//==============================================================================
void XRUNDetector::startCallback(int sampleRate, int bufferSize) {
    currentSampleRate_ = sampleRate;
    currentBufferSize_ = bufferSize;
    callbackStartTime_ = std::chrono::high_resolution_clock::now();
}

//==============================================================================
bool XRUNDetector::endCallback() {
    // Calculate callback duration
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> callbackTime =
        endTime - callbackStartTime_;
    double callbackDurationMs = callbackTime.count();

    // Calculate expected buffer duration
    double bufferDurationMs = (currentBufferSize_ * 1000.0) / currentSampleRate_;

    // Calculate percentage
    currentCallbackPercentage_ = callbackDurationMs / bufferDurationMs;

    // Update statistics
    updateStatistics(callbackDurationMs, bufferDurationMs);

    // Check for XRUN
    bool xrunOccurred = false;

    if (currentCallbackPercentage_ >= 1.0) {
        // XRUN occurred!
        XRUNEvent event;
        event.type = classifyXRUN(callbackDurationMs, bufferDurationMs);
        event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
        event.callbackDuration = callbackDurationMs;
        event.bufferDuration = bufferDurationMs;
        event.overrunAmount = callbackDurationMs - bufferDurationMs;
        event.sampleRate = currentSampleRate_;
        event.bufferSize = currentBufferSize_;

        if (event.type == XRUNType::Underrun) {
            event.description = "Callback took " +
                               juce::String(callbackDurationMs, 2) +
                               "ms but only had " +
                               juce::String(bufferDurationMs, 2) + "ms";
            statistics_.underruns++;
        } else if (event.type == XRUNType::Overrun) {
            event.description = "Input overrun - data arrived faster than processing";
            statistics_.overruns++;
        } else {
            event.description = "Unknown XRUN type";
        }

        recordXRUN(event);
        xrunOccurred = true;
    }

    return xrunOccurred;
}

//==============================================================================
void XRUNDetector::resetStatistics() {
    statistics_.totalXRUNs.store(0);
    statistics_.underruns.store(0);
    statistics_.overruns.store(0);
    statistics_.totalOverrunAmount.store(0.0);
    statistics_.worstCallbackDuration.store(0.0);
    statistics_.averageCallbackDuration.store(0.0);
    statistics_.consecutiveXRUNs.store(0);
    statistics_.timeSinceLastXRUN.store(0.0);

    std::cout << "XRUNDetector: Statistics reset" << std::endl;
}

//==============================================================================
bool XRUNDetector::predictXRUN() const {
    if (!settings_.enablePredictiveDetection) {
        return false;
    }

    // Predictive: if average callback duration is approaching warning threshold
    double avgPercentage = (statistics_.averageCallbackDuration.load() / 1000.0) /
                          ((currentBufferSize_ * 1000.0) / currentSampleRate_);

    return avgPercentage >= (settings_.warningThreshold * 0.9);
}

//==============================================================================
// Private Methods
//==============================================================================

void XRUNDetector::recordXRUN(const XRUNEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);

    lastXRUN_ = event;

    // Add to history
    xrunHistory_.push_back(event);

    // Limit history size
    if (static_cast<int>(xrunHistory_.size()) > maxHistorySize) {
        xrunHistory_.erase(xrunHistory_.begin());
    }

    // Update consecutive counter
    statistics_.consecutiveXRUNs++;
    statistics_.totalXRUNs++;
    statistics_.totalOverrunAmount += event.overrunAmount;

    // Reset time since last XRUN
    statistics_.timeSinceLastXRUN.store(0.0);

    // Log high-severity XRUNs
    if (event.overrunAmount > 10.0) {  // More than 10ms overrun
        std::cerr << "XRUNDetector: " << event.toString() << std::endl;
    }
}

void XRUNDetector::updateStatistics(double callbackDuration, double bufferDuration) {
    // Update worst case
    double currentWorst = statistics_.worstCallbackDuration.load();
    if (callbackDuration > currentWorst) {
        statistics_.worstCallbackDuration.store(callbackDuration);
    }

    // Update average using exponential moving average
    double currentAvg = statistics_.averageCallbackDuration.load();
    double newAvg = (currentAvg * (1.0 - settings_.statisticsSmoothing)) +
                   (callbackDuration * settings_.statisticsSmoothing);
    statistics_.averageCallbackDuration.store(newAvg);

    // Update time since last XRUN
    double currentTimeSince = statistics_.timeSinceLastXRUN.load();
    if (currentTimeSince >= 0.0) {
        statistics_.timeSinceLastXRUN.store(currentTimeSince + bufferDuration / 1000.0);
    }
}

XRUNType XRUNDetector::classifyXRUN(double callbackDuration,
                                    double bufferDuration) const {
    // If callback took longer than buffer time, it's an underrun
    if (callbackDuration > bufferDuration) {
        return XRUNType::Underrun;
    }

    // Otherwise, assume overrun (input came too fast)
    // In practice, true overruns are hard to detect without hardware support
    return XRUNType::Overrun;
}

} // namespace zenith
