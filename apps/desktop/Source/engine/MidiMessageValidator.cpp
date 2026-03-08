/*
  ==============================================================================

    MidiMessageValidator.cpp
    Implementation of MIDI message validation

  ==============================================================================
*/

#include "MidiMessageValidator.h"
#include <iostream>
#include <algorithm>

namespace zenith {

using juce::uint8;

//==============================================================================
// MidiValidationResult Helper Methods
//==============================================================================
int MidiValidationResult::getErrorCount() const {
    int count = 0;
    for (const auto& issue : issues) {
        if (issue.severity >= 7.0) {
            count++;
        }
    }
    return count;
}

int MidiValidationResult::getWarningCount() const {
    int count = 0;
    for (const auto& issue : issues) {
        if (issue.severity >= 1.0 && issue.severity < 7.0) {
            count++;
        }
    }
    return count;
}

//==============================================================================
// MidiMessageValidator Implementation
//==============================================================================

MidiMessageValidator::MidiMessageValidator() {
    std::cout << "MidiMessageValidator: Initialized" << std::endl;
}

MidiMessageValidator::~MidiMessageValidator() {
    std::cout << "MidiMessageValidator: Shut down" << std::endl;
}

//==============================================================================
MidiValidationResult MidiMessageValidator::validateMessage(
    const juce::MidiMessage& message,
    int messageIndex) const {

    MidiValidationResult result;
    result.isValid = true;

    statistics_.totalMessagesValidated++;

    // Get raw data
    const juce::uint8* data = message.getRawData();
    int length = message.getRawDataSize();

    if (length == 0 || data == nullptr) {
        MidiValidationIssue issue;
        issue.type = MidiValidationIssue::IncompleteMessage;
        issue.description = "Empty MIDI message";
        issue.messageIndex = messageIndex;
        issue.severity = 9.0;
        result.addIssue(issue);
        result.invalidMessages++;
        statistics_.totalErrorsDetected++;
        return result;
    }

    // Validate status byte
    juce::uint8 status = data[0];
    if (!isValidStatusByte(status)) {
        MidiValidationIssue issue;
        issue.type = MidiValidationIssue::InvalidStatusByte;
        issue.description = "Status byte 0x" +
                           juce::String::toHexString(status) +
                           " is invalid (must be 0x80-0xFF)";
        issue.messageIndex = messageIndex;
        issue.severity = 9.0;
        result.addIssue(issue);
        result.invalidMessages++;
        statistics_.totalErrorsDetected++;
        statistics_.errorCounts[issue.type]++;
        return result;
    }

    // Route to appropriate validator
    if (status >= 0x80 && status <= 0xEF) {
        // Channel voice messages
        if (!validateChannelMessage(message, result, messageIndex)) {
            return result;
        }
    } else if (status >= 0xF0 && status <= 0xFF) {
        // System messages
        if (status == 0xF0) {
            // SysEx
            if (!validateSysExMessage(message, result, messageIndex)) {
                return result;
            }
        } else {
            // Other system messages
            if (!validateSystemMessage(message, result, messageIndex)) {
                return result;
            }
        }
    }

    // Count issues
    for (const auto& issue : result.issues) {
        if (issue.severity >= 7.0) {
            statistics_.totalErrorsDetected++;
        } else {
            statistics_.totalWarningsDetected++;
        }
        statistics_.errorCounts[issue.type]++;
    }

    if (result.issues.empty()) {
        result.validMessages = 1;
    } else {
        result.invalidMessages = 1;
    }

    return result;
}

//==============================================================================
MidiValidationResult MidiMessageValidator::validateBuffer(
    const juce::MidiBuffer& buffer,
    double sampleRate) const {

    MidiValidationResult result;
    result.isValid = true;

    int index = 0;

    for (const auto& metadata : buffer) {
        const auto& message = metadata.getMessage();
        auto msgResult = validateMessage(message, index);

        // Merge results
        result.issues.insert(result.issues.end(),
                           msgResult.issues.begin(),
                           msgResult.issues.end());
        result.validMessages += msgResult.validMessages;
        result.invalidMessages += msgResult.invalidMessages;

        if (!msgResult.isValid && strictValidation_) {
            result.isValid = false;
        }

        index++;
    }

    return result;
}

//==============================================================================
MidiValidationResult MidiMessageValidator::validateRawData(
    const juce::uint8* data,
    int length) const {

    MidiValidationResult result;
    result.isValid = true;

    if (data == nullptr || length == 0) {
        MidiValidationIssue issue;
        issue.type = MidiValidationIssue::IncompleteMessage;
        issue.description = "Empty MIDI data";
        issue.severity = 9.0;
        result.addIssue(issue);
        return result;
    }

    int index = 0;
    while (index < length) {
        juce::uint8 status = data[index];

        // Check for running status
        if (status < 0x80) {
            MidiValidationIssue issue;
            issue.type = MidiValidationIssue::RunningStatusViolation;
            issue.description = "Running status without previous status byte";
            issue.messageIndex = index;
            issue.severity = 7.0;
            result.addIssue(issue);
            break;
        }

        // Get expected length
        int expectedLength = getExpectedMessageLength(status);

        if (expectedLength == 0) {
            // Variable length (SysEx)
            // Find end (0xF7)
            int end = index + 1;
            while (end < length && data[end] != 0xF7) {
                end++;
            }

            if (end >= length) {
                MidiValidationIssue issue;
                issue.type = MidiValidationIssue::MalformedSysEx;
                issue.description = "SysEx message without end byte";
                issue.messageIndex = index;
                issue.severity = 8.0;
                result.addIssue(issue);
                break;
            }

            index = end + 1;
        } else {
            // Fixed length
            if (index + expectedLength > length) {
                MidiValidationIssue issue;
                issue.type = MidiValidationIssue::IncompleteMessage;
                issue.description = "Message truncated (expected " +
                                   juce::String(expectedLength) + " bytes)";
                issue.messageIndex = index;
                issue.severity = 8.0;
                result.addIssue(issue);
                break;
            }

            // Validate data bytes
            for (int i = 1; i < expectedLength; ++i) {
                if (!isValidDataByte(data[index + i])) {
                    MidiValidationIssue issue;
                    issue.type = MidiValidationIssue::InvalidDataByte;
                    issue.description = "Data byte 0x" +
                                       juce::String::toHexString(data[index + i]) +
                                       " is invalid (must be 0x00-0x7F)";
                    issue.messageIndex = index + i;
                    issue.severity = 7.0;
                    result.addIssue(issue);
                }
            }

            index += expectedLength;
        }
    }

    return result;
}

//==============================================================================
int MidiMessageValidator::filterInvalidMessages(juce::MidiBuffer& buffer) const {
    if (!autoFilter_) {
        return 0;
    }

    juce::MidiBuffer filteredBuffer;
    int filtered = 0;
    int index = 0;

    for (const auto& metadata : buffer) {
        const auto& message = metadata.getMessage();
        auto result = validateMessage(message, index);

        if (result.isValid) {
            // Keep valid messages
            filteredBuffer.addEvent(message, metadata.samplePosition);
        } else {
            // Filter invalid messages
            filtered++;
            statistics_.totalMessagesFiltered++;
        }

        index++;
    }

    buffer = filteredBuffer;

    if (filtered > 0) {
        std::cout << "MidiMessageValidator: Filtered " << filtered
                  << " invalid messages" << std::endl;
    }

    return filtered;
}

//==============================================================================
bool MidiMessageValidator::isValidStatusByte(juce::uint8 status) {
    return status >= 0x80 && status <= 0xFF;
}

bool MidiMessageValidator::isValidDataByte(juce::uint8 data) {
    return data <= 0x7F;
}

//==============================================================================
bool MidiMessageValidator::isValidSysEx(const juce::uint8* data, int length) {
    if (length < 2) {
        return false;  // At minimum need 0xF0 and 0xF7
    }

    if (data[0] != 0xF0) {
        return false;  // Must start with 0xF0
    }

    if (data[length - 1] != 0xF7) {
        return false;  // Must end with 0xF7
    }

    return true;
}

//==============================================================================
int MidiMessageValidator::getExpectedMessageLength(juce::uint8 status) {
    // Channel voice messages (0x80-0xEF)
    if (status >= 0x80 && status <= 0xEF) {
        juce::uint8 highNibble = status & 0xF0;
        switch (highNibble) {
            case 0x80:  // Note off
            case 0x90:  // Note on
            case 0xA0:  // Polyphonic aftertouch
            case 0xB0:  // Control change
            case 0xE0:  // Pitch bend
                return 3;  // Status, data1, data2
            case 0xC0:  // Program change
            case 0xD0:  // Channel aftertouch
                return 2;  // Status, data1
        }
    }

    // System messages (0xF0-0xFF)
    switch (status) {
        case 0xF0:  // SysEx (variable length)
            return 0;
        case 0xF1:  // Time code quarter frame
        case 0xF3:  // Song select
            return 2;
        case 0xF2:  // Song position pointer
            return 3;
        case 0xF4:  // Undefined (reserved)
        case 0xF5:  // Undefined (reserved)
            return 1;
        case 0xF6:  // Tune request
        case 0xF7:  // End of SysEx
        case 0xF8:  // Timing clock
        case 0xFA:  // Start
        case 0xFB:  // Continue
        case 0xFC:  // Stop
        case 0xFE:  // Active sensing
        case 0xFF:  // Reset
            return 1;
    }

    return 0;  // Unknown
}

//==============================================================================
bool MidiMessageValidator::verifySysExChecksum(const juce::uint8* data, int length) {
    if (!isValidSysEx(data, length)) {
        return false;
    }

    // Skip SysEx start (0xF0) and end (0xF7)
    // Checksums vary by manufacturer, so this is a simple check

    // For Roland: sum of bytes (excluding 0xF0, manufacturer ID, and 0xF7)
    // should result in 0 when modulo 128

    // This is a placeholder - full implementation would be manufacturer-specific
    return true;
}

//==============================================================================
// Private Methods
//==============================================================================
bool MidiMessageValidator::validateChannelMessage(
    const juce::MidiMessage& message,
    MidiValidationResult& result,
    int messageIndex) const {

    int channel = message.getChannel();
    if (channel < 1 || channel > 16) {
        MidiValidationIssue issue;
        issue.type = MidiValidationIssue::InvalidChannel;
        issue.description = "Channel " + juce::String(channel) + " is invalid (must be 1-16)";
        issue.messageIndex = messageIndex;
        issue.severity = 8.0;
        result.addIssue(issue);
        return false;
    }

    // Validate based on message type
    if (message.isNoteOnOrOff()) {
        int noteNumber = message.getNoteNumber();
        if (noteNumber < 0 || noteNumber > 127) {
            MidiValidationIssue issue;
            issue.type = MidiValidationIssue::InvalidNoteNumber;
            issue.description = "Note number " + juce::String(noteNumber) +
                               " is invalid (must be 0-127)";
            issue.messageIndex = messageIndex;
            issue.severity = 7.0;
            result.addIssue(issue);
        }

        int velocity = message.getVelocity();
        if (velocity < 0 || velocity > 127) {
            MidiValidationIssue issue;
            issue.type = MidiValidationIssue::InvalidVelocity;
            issue.description = "Velocity " + juce::String(velocity) +
                               " is invalid (must be 0-127)";
            issue.messageIndex = messageIndex;
            issue.severity = 6.0;
            result.addIssue(issue);
        }
    } else if (message.isController()) {
        int controllerNumber = message.getControllerNumber();
        if (controllerNumber < 0 || controllerNumber > 127) {
            MidiValidationIssue issue;
            issue.type = MidiValidationIssue::InvalidControllerNumber;
            issue.description = "CC number " + juce::String(controllerNumber) +
                               " is invalid (must be 0-127)";
            issue.messageIndex = messageIndex;
            issue.severity = 7.0;
            result.addIssue(issue);
        }
    }

    return true;
}

bool MidiMessageValidator::validateSystemMessage(
    const juce::MidiMessage& message,
    MidiValidationResult& result,
    int messageIndex) const {

    // Most system messages are just status bytes (no data)
    // They should already be validated by status byte check

    return true;
}

bool MidiMessageValidator::validateSysExMessage(
    const juce::MidiMessage& message,
    MidiValidationResult& result,
    int messageIndex) const {

    const juce::uint8* data = message.getRawData();
    int length = message.getRawDataSize();

    if (!isValidSysEx(data, length)) {
        MidiValidationIssue issue;
        issue.type = MidiValidationIssue::MalformedSysEx;
        issue.description = "Invalid SysEx message format";
        issue.messageIndex = messageIndex;
        issue.severity = 8.0;
        result.addIssue(issue);
        return false;
    }

    // Verify checksum if present
    if (!verifySysExChecksum(data, length)) {
        MidiValidationIssue issue;
        issue.type = MidiValidationIssue::InvalidChecksum;
        issue.description = "SysEx checksum verification failed";
        issue.messageIndex = messageIndex;
        issue.severity = 7.0;
        result.addIssue(issue);
    }

    return true;
}

} // namespace zenith
