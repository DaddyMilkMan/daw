/*
  ==============================================================================

    AudioFileValidator.h
    Created: 2026-02-19
    Author:  Zenith DAW - Month 8: File I/O Safety (Gap #4)

    Validates audio files before loading to prevent corruption and crashes.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Validation issue type
 */
enum class AudioFileValidationIssue {
    CorruptHeader,          // File header is corrupted
    InvalidFormat,          // Not a recognized audio format
    UnsupportedCodec,       // Codec not supported
    InvalidSampleRate,      // Sample rate out of range
    InvalidBitDepth,        // Bit depth invalid
    InvalidChannels,        // Channel count invalid
    FileTooLarge,           // File exceeds size limits
    FileTruncated,          // File appears incomplete
    SecurityIssue,          // Security concerns (path traversal, etc.)
    Unknown
};

//==============================================================================
/**
 * @brief Audio file validation result
 */
struct AudioFileValidationResult {
    bool isValid = false;
    std::vector<AudioFileValidationIssue> issues;
    juce::String format;            // Detected format
    double sampleRate = 0.0;
    int channels = 0;
    int bitDepth = 0;
    double lengthSeconds = 0.0;
    juce::int64 fileSize = 0;

    juce::String toString() const {
        if (isValid) {
            return "Valid audio file: " + format + " " +
                   juce::String(channels) + "ch " +
                   juce::String(sampleRate, 0) + "Hz " +
                   juce::String(lengthSeconds, 1) + "s";
        } else {
            juce::String result = "Invalid audio file:";
            for (const auto& issue : issues) {
                result += "\n  - " + getIssueString(issue);
            }
            return result;
        }
    }

    static juce::String getIssueString(AudioFileValidationIssue issue) {
        switch (issue) {
            case AudioFileValidationIssue::CorruptHeader: return "Corrupt Header";
            case AudioFileValidationIssue::InvalidFormat: return "Invalid Format";
            case AudioFileValidationIssue::UnsupportedCodec: return "Unsupported Codec";
            case AudioFileValidationIssue::InvalidSampleRate: return "Invalid Sample Rate";
            case AudioFileValidationIssue::InvalidBitDepth: return "Invalid Bit Depth";
            case AudioFileValidationIssue::InvalidChannels: return "Invalid Channels";
            case AudioFileValidationIssue::FileTooLarge: return "File Too Large";
            case AudioFileValidationIssue::FileTruncated: return "File Truncated";
            case AudioFileValidationIssue::SecurityIssue: return "Security Issue";
            case AudioFileValidationIssue::Unknown: return "Unknown";
        }
        return "Unknown";
    }
};

//==============================================================================
/**
 * @brief Audio file validator
 *
 * Features:
 * - Header validation before full load
 * - Format verification
 * - Corrupt file detection
 * - Malformed file handling
 * - Security checks
 * - File size validation
 */
class AudioFileValidator {
public:
    //==========================================================================
    AudioFileValidator();
    ~AudioFileValidator();

    //==========================================================================
    /**
     * @brief Validate audio file
     * @param file File to validate
     * @return Validation result
     */
    AudioFileValidationResult validateFile(const juce::File& file);

    //==========================================================================
    /**
     * @brief Quick validation (header only)
     * Faster than full validation
     */
    AudioFileValidationResult validateHeader(const juce::File& file);

    //==========================================================================
    /**
     * @brief Check if format is supported
     * @param file File to check
     * @return true if format is supported
     */
    static bool isFormatSupported(const juce::File& file);

    //==========================================================================
    /**
     * @brief Get supported formats
     */
    static std::vector<juce::String> getSupportedFormats();

    //==========================================================================
    /**
     * @brief Validate sample rate
     * @param sampleRate Sample rate to validate
     * @return true if valid
     */
    static bool isValidSampleRate(double sampleRate) {
        return sampleRate >= 8000 && sampleRate <= 384000;
    }

    //==========================================================================
    /**
     * @brief Validate file size
     * @param fileSize File size in bytes
     * @return true if within acceptable limits
     */
    static bool isValidFileSize(juce::int64 fileSize) {
        // Max 4GB for safety
        return fileSize > 0 && fileSize < 4LL * 1024LL * 1024LL * 1024LL;
    }

private:
    //==========================================================================
    bool checkForPathTraversal(const juce::File& file) const;
    bool checkFileSignature(const juce::File& file) const;

    //==========================================================================
    // Audio format manager
    juce::AudioFormatManager formatManager_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileValidator)
};

//==============================================================================
/**
 * @brief Singleton accessor for audio file validator
 */
class AudioFileValidatorHolder {
public:
    static AudioFileValidator& getInstance() {
        static AudioFileValidator instance;
        return instance;
    }

    AudioFileValidatorHolder(const AudioFileValidatorHolder&) = delete;
    AudioFileValidatorHolder& operator=(const AudioFileValidatorHolder&) = delete;

private:
    AudioFileValidatorHolder() = default;
    ~AudioFileValidatorHolder() = default;
};

} // namespace zenith
