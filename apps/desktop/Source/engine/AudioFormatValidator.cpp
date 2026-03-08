/*
  ==============================================================================

    AudioFormatValidator.cpp
    Implementation of audio format validation

  ==============================================================================
*/

#include "AudioFormatValidator.h"
#include <iostream>

namespace zenith {

//==============================================================================
// AudioFormatValidator Implementation
//==============================================================================

AudioFormatValidator::AudioFormatValidator() {
    std::cout << "AudioFormatValidator: Initialized" << std::endl;
}

AudioFormatValidator::~AudioFormatValidator() {
    std::cout << "AudioFormatValidator: Shut down" << std::endl;
}

//==============================================================================
std::vector<FormatValidationIssue> AudioFormatValidator::validateFormat(
    const AudioFormatSpec& format) const
{
    std::vector<FormatValidationIssue> issues;

    // Check sample rate
    if (!isValidSampleRate(format.sampleRate)) {
        FormatValidationIssue issue;
        issue.type = FormatValidationIssue::InvalidSampleRate;
        issue.description = "Sample rate " + juce::String(format.sampleRate) +
                           " is not valid (8kHz - 384kHz)";
        issue.severity = 8;
        issues.push_back(issue);
    }

    // Check channel count
    if (!isValidChannelCount(format.channels)) {
        FormatValidationIssue issue;
        issue.type = FormatValidationIssue::InvalidChannelCount;
        issue.description = "Channel count " + juce::String(format.channels) +
                           " is not valid (1-64)";
        issue.severity = 9;
        issues.push_back(issue);
    }

    // Check bit depth
    if (!isValidBitDepth(format.bitDepth)) {
        FormatValidationIssue issue;
        issue.type = FormatValidationIssue::InvalidBitDepth;
        issue.description = "Unknown bit depth specified";
        issue.severity = 10;
        issues.push_back(issue);
    }

    // Check bytes per sample matches bit depth
    int expectedBytes = format.getBitsPerSample() / 8;
    if (format.bytesPerSample != expectedBytes && expectedBytes > 0) {
        FormatValidationIssue issue;
        issue.type = FormatValidationIssue::InvalidBitDepth;
        issue.description = "Bytes per sample (" + juce::String(format.bytesPerSample) +
                           ") doesn't match bit depth (" + juce::String(expectedBytes) + ")";
        issue.severity = 7;
        issues.push_back(issue);
    }

    return issues;
}

//==============================================================================
bool AudioFormatValidator::isValidFormat(const AudioFormatSpec& format) const {
    auto issues = validateFormat(format);
    return issues.empty();
}

} // namespace zenith
