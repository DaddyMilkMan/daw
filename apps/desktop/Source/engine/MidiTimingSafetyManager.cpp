/*
  ==============================================================================

    MidiTimingSafetyManager.cpp
    Implementation of MIDI timing safety

  ==============================================================================
*/

#include "MidiTimingSafetyManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// MidiTimingSafetyManager Implementation
//==============================================================================

MidiTimingSafetyManager::MidiTimingSafetyManager()
    : lastClockTime_(juce::Time::getCurrentTime()) {

    std::cout << "MidiTimingSafetyManager: Initialized" << std::endl;
}

MidiTimingSafetyManager::~MidiTimingSafetyManager() {
    std::cout << "MidiTimingSafetyManager: Shut down" << std::endl;
}

//==============================================================================
std::vector<MidiTimingIssue> MidiTimingSafetyManager::processMessage(
    const juce::MidiMessage& message,
    int samplePosition,
    double sampleRate) {

    std::vector<MidiTimingIssue> issues;

    // Process clock messages
    if (message.isMidiClock()) {
        processClockMessage(message);
        return issues;  // Clock messages are always valid
    }

    // Check for timing issues in other messages
    if (message.isNoteOn() || message.isNoteOff()) {
        // Validate timestamp
        double timestampMs = static_cast<double>(samplePosition) / sampleRate * 1000.0;

        juce::Time currentTime = juce::Time::getCurrentTime();
        juce::RelativeTime timeSinceLastClock = currentTime - lastClockTime_;
        double expectedTime = timeSinceLastClock.inMilliseconds();

        // Check if message is late
        if (std::abs(timestampMs - expectedTime) > maxDriftMs_) {
            MidiTimingIssue issue;
            issue.type = MidiTimingIssue::LateMessage;
            issue.description = "Message timestamp off by " +
                               juce::String(std::abs(timestampMs - expectedTime), 2) +
                               "ms";
            issue.timestamp = timestampMs;
            issue.deviation = timestampMs - expectedTime;
            issue.severity = juce::jmin(10.0, std::abs(timestampMs - expectedTime) / 2.0);
            issues.push_back(issue);

            clockStats_.lateMessagesDetected++;

            if (timingIssueCallback_) {
                timingIssueCallback_(issue);
            }
        }
    }

    return issues;
}

//==============================================================================
std::vector<MidiTimingIssue> MidiTimingSafetyManager::validateBufferTiming(
    const juce::MidiBuffer& buffer,
    double sampleRate) const {

    std::vector<MidiTimingIssue> issues;

    if (buffer.isEmpty()) {
        return issues;
    }

    auto iterator = buffer.begin();
    int lastIndex = -1;

    for (const auto& metadata : buffer) {
        int currentIndex = metadata.samplePosition;

        // Check for out-of-order messages
        if (currentIndex < lastIndex && lastIndex >= 0) {
            MidiTimingIssue issue;
            issue.type = MidiTimingIssue::OutOfOrder;
            issue.description = "Message out of order (index: " +
                               juce::String(currentIndex) +
                               " < previous: " + juce::String(lastIndex) + ")";
            issue.severity = 6.0;
            issues.push_back(issue);
        }

        // Check for duplicate timestamps
        if (currentIndex == lastIndex && lastIndex >= 0) {
            MidiTimingIssue issue;
            issue.type = MidiTimingIssue::DuplicateTimestamp;
            issue.description = "Duplicate timestamp at " +
                               juce::String(currentIndex);
            issue.severity = 5.0;
            issues.push_back(issue);
        }

        lastIndex = currentIndex;
    }

    return issues;
}

//==============================================================================
bool MidiTimingSafetyManager::detectClockDrift(double expectedTempo,
                                                double toleranceMs) const {
    // Calculate difference from expected tempo
    double tempoDiff = std::abs(currentTempo_ - expectedTempo);

    // Convert tempo difference to timing difference
    // At 120 BPM, one quarter note = 0.5s
    // Tempo difference of 1 BPM ≈ 4.17ms per beat
    double timingDriftMs = tempoDiff * (60.0 / (expectedTempo * expectedTempo)) * 1000.0;

    return timingDriftMs > toleranceMs;
}

//==============================================================================
// Private Methods
//==============================================================================
void MidiTimingSafetyManager::processClockMessage(const juce::MidiMessage& message) {
    juce::Time currentTime = juce::Time::getCurrentTime();

    if (clockTicks_ == 0) {
        // First clock message
        lastClockTime_ = currentTime;
        clockTicks_ = 1;
        return;
    }

    // Calculate time since last clock
    juce::RelativeTime elapsed = currentTime - lastClockTime_;
    double elapsedMs = elapsed.inMilliseconds();

    // Calculate tempo from clock ticks (24 ticks per quarter note)
    // 60000 ms / minute
    double newTempo = 60000.0 / (elapsedMs * clockTicksPerQuarter_);

    // Update tempo with smoothing
    if (tempoSmoothingEnabled_) {
        updateTempoEstimate(newTempo);
    } else {
        currentTempo_ = newTempo;
    }

    // Update statistics
    clockStats_.clockMessagesReceived++;
    clockStats_.averageTempo = currentTempo_;

    // Check for drift
    double tempoDrift = std::abs(newTempo - currentTempo_);
    double timingDriftMs = tempoDrift * (60.0 / (currentTempo_ * currentTempo_)) * 1000.0;

    clockStats_.averageDrift = (clockStats_.averageDrift * 0.9) + (timingDriftMs * 0.1);
    clockStats_.maxDrift = juce::jmax(clockStats_.maxDrift, timingDriftMs);

    // Reset clock tick counter
    if (clockTicks_ >= clockTicksPerQuarter_) {
        clockTicks_ = 0;
        lastClockTime_ = currentTime;
    } else {
        clockTicks_++;
    }

    // Check for excessive drift
    if (timingDriftMs > maxDriftMs_) {
        MidiTimingIssue issue;
        issue.type = MidiTimingIssue::ClockDrift;
        issue.description = "Clock drift detected: " +
                           juce::String(timingDriftMs, 2) + "ms";
        issue.deviation = timingDriftMs;
        issue.severity = juce::jmin(10.0, timingDriftMs / maxDriftMs_ * 5.0);

        if (timingIssueCallback_) {
            timingIssueCallback_(issue);
        }
    }
}

void MidiTimingSafetyManager::updateTempoEstimate(double newTempo) {
    // Exponential moving average
    currentTempo_ = currentTempo_ * (1.0 - tempoSmoothingFactor_) +
                    newTempo * tempoSmoothingFactor_;

    // Update variance
    double diff = newTempo - currentTempo_;
    clockStats_.tempoVariance = clockStats_.tempoVariance * 0.9 + diff * diff * 0.1;
}

double MidiTimingSafetyManager::calculateExpectedTimestamp(
    int messageIndex,
    double tempo,
    double sampleRate) const {

    // Calculate expected timestamp based on tempo
    // This is simplified - full implementation would consider message type
    double beatsPerSecond = tempo / 60.0;
    double secondsPerSample = 1.0 / sampleRate;

    return static_cast<double>(messageIndex) * secondsPerSample * beatsPerSecond * 1000.0;
}

} // namespace zenith
