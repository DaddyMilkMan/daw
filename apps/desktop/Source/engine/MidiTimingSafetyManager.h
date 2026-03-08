/*
  ==============================================================================

    MidiTimingSafetyManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #4)

    Ensures MIDI timing accuracy and detects drift.

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
 * @brief MIDI timing issue
 */
struct MidiTimingIssue {
    enum Type {
        ClockDrift,                 // MIDI clock drifted too much
        LateMessage,                // Message arrived late
        TimestampInvalid,           // Invalid timestamp
        TempoChanged,               // Tempo changed unexpectedly
        ClockMissing,               // Clock signal missing
        OutOfOrder,                // Messages out of order
        DuplicateTimestamp          // Same timestamp used twice
    };

    Type type;
    juce::String description;
    double timestamp = 0.0;         // When issue occurred
    double deviation = 0.0;         // Amount of deviation (ms)
    double severity = 0.0;          // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case ClockDrift: typeStr = "Clock Drift"; break;
            case LateMessage: typeStr = "Late Message"; break;
            case TimestampInvalid: typeStr = "Invalid Timestamp"; break;
            case TempoChanged: typeStr = "Tempo Changed"; break;
            case ClockMissing: typeStr = "Clock Missing"; break;
            case OutOfOrder: typeStr = "Out of Order"; break;
            case DuplicateTimestamp: typeStr = "Duplicate Timestamp"; break;
        }
        return "[" + typeStr + "] " + description +
               " (drift: " + juce::String(deviation, 2) + "ms)";
    }
};

//==============================================================================
/**
 * @brief MIDI clock statistics
 */
struct MidiClockStatistics {
    double averageTempo = 120.0;    // BPM
    double tempoVariance = 0.0;     // Tempo variance
    int clockMessagesReceived = 0;
    int lateMessagesDetected = 0;
    double averageDrift = 0.0;      // ms
    double maxDrift = 0.0;          // ms
    bool isClockMaster = false;

    juce::String toString() const {
        return "MIDI Clock: " + juce::String(averageTempo, 1) + " BPM" +
               (isClockMaster ? " (Master)" : " (Slave)") +
               ", drift: " + juce::String(averageDrift, 2) + "ms";
    }
};

//==============================================================================
/**
 * @brief MIDI timing manager
 *
 * Features:
 * - MIDI clock drift detection
 * - Timestamp validation
 * - Clock master/slave negotiation
 * - Tempo smoothing
 * - Quantization safety
 * - Late message detection
 */
class MidiTimingSafetyManager {
public:
    //==========================================================================
    MidiTimingSafetyManager();
    ~MidiTimingSafetyManager();

    //==========================================================================
    /**
     * @brief Process MIDI message for timing validation
     * @param message MIDI message
     * @param samplePosition Sample position within buffer
     * @param sampleRate Sample rate
     * @return List of timing issues detected
     */
    std::vector<MidiTimingIssue> processMessage(
        const juce::MidiMessage& message,
        int samplePosition,
        double sampleRate = 48000.0);

    //==========================================================================
    /**
     * @brief Validate MIDI buffer timestamps
     * @param buffer Buffer to validate
     * @param sampleRate Sample rate
     * @return List of timing issues
     */
    std::vector<MidiTimingIssue> validateBufferTiming(
        const juce::MidiBuffer& buffer,
        double sampleRate = 48000.0) const;

    //==========================================================================
    /**
     * @brief Detect MIDI clock drift
     * @param expectedTempo Expected tempo (BPM)
     * @param toleranceMs Tolerance in milliseconds
     * @return true if drift detected
     */
    bool detectClockDrift(double expectedTempo,
                         double toleranceMs = 5.0) const;

    //==========================================================================
    /**
     * @brief Get current clock statistics
     * @return Clock statistics
     */
    MidiClockStatistics getClockStatistics() const {
        return clockStats_;
    }

    //==========================================================================
    /**
     * @brief Set clock mode
     * @param isMaster true = clock master, false = clock slave
     */
    void setClockMode(bool isMaster) {
        clockStats_.isClockMaster = isMaster;
    }

    //==========================================================================
    /**
     * @brief Enable/disable tempo smoothing
     * @param enable true to smooth tempo changes
     * @param smoothingFactor Smoothing factor (0-1)
     */
    void setTempoSmoothingEnabled(bool enable, double smoothingFactor = 0.1) {
        tempoSmoothingEnabled_ = enable;
        tempoSmoothingFactor_ = juce::jlimit(0.0, 1.0, smoothingFactor);
    }

    //==========================================================================
    /**
     * @brief Set maximum allowed drift
     * @param maxDriftMs Maximum drift in milliseconds
     */
    void setMaxAllowedDrift(double maxDriftMs) {
        maxDriftMs_ = juce::jmax(0.0, maxDriftMs);
    }

    /**
     * @brief Get maximum allowed drift
     * @return Maximum drift in ms
     */
    double getMaxAllowedDrift() const { return maxDriftMs_; }

    //==========================================================================
    /**
     * @brief Register callback for timing issues
     * @param callback Function to call when issue detected
     */
    void setTimingIssueCallback(std::function<void(const MidiTimingIssue&)> callback) {
        timingIssueCallback_ = callback;
    }

private:
    //==========================================================================
    void processClockMessage(const juce::MidiMessage& message);
    void updateTempoEstimate(double newTempo);
    double calculateExpectedTimestamp(int messageIndex,
                                     double tempo,
                                     double sampleRate) const;

    //==========================================================================
    // Clock tracking
    double currentTempo_ = 120.0;     // BPM
    juce::Time lastClockTime_;
    int clockTicks_ = 0;
    int clockTicksPerQuarter_ = 24;   // Standard MIDI clock

    // Statistics
    MidiClockStatistics clockStats_;

    // Settings
    double maxDriftMs_ = 5.0;
    bool tempoSmoothingEnabled_ = true;
    double tempoSmoothingFactor_ = 0.1;

    // Callbacks
    std::function<void(const MidiTimingIssue&)> timingIssueCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiTimingSafetyManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for MIDI timing safety manager
 */
class MidiTimingSafetyManagerHolder {
public:
    static MidiTimingSafetyManager& getInstance() {
        static MidiTimingSafetyManager instance;
        return instance;
    }

    MidiTimingSafetyManagerHolder(const MidiTimingSafetyManagerHolder&) = delete;
    MidiTimingSafetyManagerHolder& operator=(const MidiTimingSafetyManagerHolder&) = delete;

private:
    MidiTimingSafetyManagerHolder() = default;
    ~MidiTimingSafetyManagerHolder() = default;
};

} // namespace zenith
