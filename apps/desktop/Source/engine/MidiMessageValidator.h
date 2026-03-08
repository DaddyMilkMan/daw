/*
  ==============================================================================

    MidiMessageValidator.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #1)

    Validates MIDI messages to prevent corruption and crashes.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>

namespace zenith {

using juce::uint8;

//==============================================================================
/**
 * @brief MIDI validation issue
 */
struct MidiValidationIssue {
    enum Type {
        InvalidStatusByte,         // Status byte not in 0x80-0xFF range
        InvalidDataByte,           // Data byte not in 0x00-0x7F range
        MalformedSysEx,            // SysEx message invalid
        IncompleteMessage,         // Message too short for type
        UnknownMessageType,        // Unrecognized message type
        InvalidChannel,            // Channel number out of range
        InvalidNoteNumber,         // Note number out of range
        InvalidVelocity,           // Velocity out of range
        InvalidControllerNumber,   // CC number out of range
        InvalidChecksum,           // SysEx checksum invalid
        RunningStatusViolation     // Running status used incorrectly
    };

    Type type;
    juce::String description;
    int messageIndex = -1;         // Index in buffer
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case InvalidStatusByte: typeStr = "Invalid Status"; break;
            case InvalidDataByte: typeStr = "Invalid Data"; break;
            case MalformedSysEx: typeStr = "Malformed SysEx"; break;
            case IncompleteMessage: typeStr = "Incomplete"; break;
            case UnknownMessageType: typeStr = "Unknown Type"; break;
            case InvalidChannel: typeStr = "Invalid Channel"; break;
            case InvalidNoteNumber: typeStr = "Invalid Note"; break;
            case InvalidVelocity: typeStr = "Invalid Velocity"; break;
            case InvalidControllerNumber: typeStr = "Invalid CC"; break;
            case InvalidChecksum: typeStr = "Bad Checksum"; break;
            case RunningStatusViolation: typeStr = "Running Status"; break;
        }
        return "[" + typeStr + "] " + description +
               (messageIndex >= 0 ? (" (msg #" + juce::String(messageIndex) + ")") : "");
    }
};

//==============================================================================
/**
 * @brief MIDI message validation result
 */
struct MidiValidationResult {
    bool isValid = true;
    std::vector<MidiValidationIssue> issues;
    int validMessages = 0;
    int invalidMessages = 0;
    int filteredMessages = 0;

    int getIssueCount() const { return static_cast<int>(issues.size()); }
    int getErrorCount() const;        // Severity >= 7
    int getWarningCount() const;      // Severity 1-6

    void addIssue(const MidiValidationIssue& issue) {
        issues.push_back(issue);
        if (issue.severity >= 7.0) {
            isValid = false;
        }
    }

    juce::String toString() const {
        return "MIDI Validation: " + juce::String(isValid ? "PASS" : "FAIL") +
               " (" + juce::String(validMessages) + " valid, " +
               juce::String(invalidMessages) + " invalid, " +
               juce::String(filteredMessages) + " filtered)";
    }
};

//==============================================================================
/**
 * @brief MIDI message validation statistics
 */
struct MidiValidationStatistics {
    int totalMessagesValidated = 0;
    int totalErrorsDetected = 0;
    int totalWarningsDetected = 0;
    int totalMessagesFiltered = 0;
    std::map<MidiValidationIssue::Type, int> errorCounts;

    juce::String toString() const {
        return "MIDI Validation Stats: " +
               juce::String(totalMessagesValidated) + " messages, " +
               juce::String(totalErrorsDetected) + " errors, " +
               juce::String(totalWarningsDetected) + " warnings, " +
               juce::String(totalMessagesFiltered) + " filtered";
    }
};

//==============================================================================
/**
 * @brief Validates MIDI messages for safety
 *
 * Features:
 * - Status byte validation
 * - Data byte range checking
 * - SysEx message validation
 * - Running status handling
 * - Message completeness checking
 * - Checksum verification
 */
class MidiMessageValidator {
public:
    //==========================================================================
    MidiMessageValidator();
    ~MidiMessageValidator();

    //==========================================================================
    /**
     * @brief Validate a single MIDI message
     * @param message Message to validate
     * @param messageIndex Index in buffer (for error reporting)
     * @return Validation result
     */
    MidiValidationResult validateMessage(const juce::MidiMessage& message,
                                        int messageIndex = -1) const;

    //==========================================================================
    /**
     * @brief Validate MIDI buffer
     * @param buffer Buffer to validate
     * @param sampleRate Sample rate (for timing validation)
     * @return Validation result
     */
    MidiValidationResult validateBuffer(const juce::MidiBuffer& buffer,
                                       double sampleRate = 48000.0) const;

    //==========================================================================
    /**
     * @brief Validate raw MIDI data
     * @param data Raw MIDI bytes
     * @param length Data length
     * @return Validation result
     */
    MidiValidationResult validateRawData(const juce::uint8* data, int length) const;

    //==========================================================================
    /**
     * @brief Filter invalid messages from buffer
     * @param buffer Buffer to filter (will be modified)
     * @return Number of messages filtered
     */
    int filterInvalidMessages(juce::MidiBuffer& buffer) const;

    //==========================================================================
    /**
     * @brief Validate status byte
     * @param status Status byte to validate
     * @return true if valid (0x80-0xFF)
     */
    static bool isValidStatusByte(juce::uint8 status);

    //==========================================================================
    /**
     * @brief Validate data byte
     * @param data Data byte to validate
     * @return true if valid (0x00-0x7F)
     */
    static bool isValidDataByte(juce::uint8 data);

    //==========================================================================
    /**
     * @brief Validate SysEx message
     * @param data SysEx data
     * @param length Data length
     * @return true if valid
     */
    static bool isValidSysEx(const juce::uint8* data, int length);

    //==========================================================================
    /**
     * @brief Calculate expected message length
     * @param status Status byte
     * @return Expected length in bytes (0 for variable-length)
     */
    static int getExpectedMessageLength(juce::uint8 status);

    //==========================================================================
    /**
     * @brief Verify SysEx checksum
     * @param data SysEx data
     * @param length Data length
     * @return true if checksum valid
     */
    static bool verifySysExChecksum(const juce::uint8* data, int length);

    //==========================================================================
    /**
     * @brief Enable/disable strict validation
     * @param enable true for strict mode (fail on any issue)
     */
    void setStrictValidationEnabled(bool enable) {
        strictValidation_ = enable;
    }

    /**
     * @brief Check if strict validation is enabled
     * @return true if strict mode
     */
    bool isStrictValidationEnabled() const { return strictValidation_; }

    //==========================================================================
    /**
     * @brief Enable/disable automatic filtering
     * @param enable true to automatically filter invalid messages
     */
    void setAutoFilteringEnabled(bool enable) {
        autoFilter_ = enable;
    }

    /**
     * @brief Check if auto-filtering is enabled
     * @return true if enabled
     */
    bool isAutoFilteringEnabled() const { return autoFilter_; }

    //==========================================================================
    /**
     * @brief Get validation statistics
     * @return Current statistics
     */
    MidiValidationStatistics getStatistics() const {
        return statistics_;
    }

    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        statistics_ = MidiValidationStatistics{};
    }

private:
    //==========================================================================
    bool validateChannelMessage(const juce::MidiMessage& message,
                                MidiValidationResult& result,
                                int messageIndex) const;

    bool validateSystemMessage(const juce::MidiMessage& message,
                               MidiValidationResult& result,
                               int messageIndex) const;

    bool validateSysExMessage(const juce::MidiMessage& message,
                              MidiValidationResult& result,
                              int messageIndex) const;

    //==========================================================================
    // Settings
    bool strictValidation_ = false;
    bool autoFilter_ = true;

    // Statistics
    mutable MidiValidationStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMessageValidator)
};

//==============================================================================
/**
 * @brief Singleton accessor for MIDI message validator
 */
class MidiMessageValidatorHolder {
public:
    static MidiMessageValidator& getInstance() {
        static MidiMessageValidator instance;
        return instance;
    }

    MidiMessageValidatorHolder(const MidiMessageValidatorHolder&) = delete;
    MidiMessageValidatorHolder& operator=(const MidiMessageValidatorHolder&) = delete;

private:
    MidiMessageValidatorHolder() = default;
    ~MidiMessageValidatorHolder() = default;
};

} // namespace zenith
