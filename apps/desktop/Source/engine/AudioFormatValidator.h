/*
  ==============================================================================

    AudioFormatValidator.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #7)

    Validates audio formats and ensures safe format conversion.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Audio bit depth
 */
enum class AudioBitDepth {
    Int16,    // 16-bit integer
    Int24,    // 24-bit integer
    Int32,    // 32-bit integer
    Float32,  // 32-bit float
    Float64,  // 64-bit float (double precision)
    Unknown
};

//==============================================================================
/**
 * @brief Format validation issue
 */
struct FormatValidationIssue {
    enum Type {
        InvalidBitDepth,
        InvalidSampleRate,
        InvalidChannelCount,
        IncompatibleFormat,
        Overflow,
        Underflow,
        PrecisionLoss,
        Unknown
    };

    Type type;
    juce::String description;
    int severity = 0;  // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case InvalidBitDepth: typeStr = "Invalid Bit Depth"; break;
            case InvalidSampleRate: typeStr = "Invalid Sample Rate"; break;
            case InvalidChannelCount: typeStr = "Invalid Channels"; break;
            case IncompatibleFormat: typeStr = "Incompatible"; break;
            case Overflow: typeStr = "Overflow"; break;
            case Underflow: typeStr = "Underflow"; break;
            case PrecisionLoss: typeStr = "Precision Loss"; break;
            case Unknown: typeStr = "Unknown"; break;
        }
        return "[" + typeStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Audio format specification
 */
struct AudioFormatSpec {
    int sampleRate = 48000;
    int channels = 2;
    AudioBitDepth bitDepth = AudioBitDepth::Float32;
    bool isFloatingPoint = false;
    int bytesPerSample = 4;

    juce::String toString() const {
        juce::String depthStr;
        switch (bitDepth) {
            case AudioBitDepth::Int16: depthStr = "16-bit"; break;
            case AudioBitDepth::Int24: depthStr = "24-bit"; break;
            case AudioBitDepth::Int32: depthStr = "32-bit int"; break;
            case AudioBitDepth::Float32: depthStr = "32-bit float"; break;
            case AudioBitDepth::Float64: depthStr = "64-bit float"; break;
            case AudioBitDepth::Unknown: depthStr = "Unknown"; break;
        }
        return juce::String(channels) + "ch, " +
               juce::String(sampleRate) + "Hz, " +
               depthStr;
    }

    int getBitsPerSample() const {
        switch (bitDepth) {
            case AudioBitDepth::Int16: return 16;
            case AudioBitDepth::Int24: return 24;
            case AudioBitDepth::Int32: return 32;
            case AudioBitDepth::Float32: return 32;
            case AudioBitDepth::Float64: return 64;
            case AudioBitDepth::Unknown: return 0;
        }
        return 0;
    }
};

//==============================================================================
/**
 * @brief Audio format validator
 *
 * Features:
 * - Validates audio formats
 * - Checks for valid sample rates
 * - Validates bit depths
 * - Checks channel counts
 */
class AudioFormatValidator {
public:
    //==========================================================================
    AudioFormatValidator();
    ~AudioFormatValidator();

    //==========================================================================
    /**
     * @brief Validate format specification
     * @return List of issues (empty if valid)
     */
    std::vector<FormatValidationIssue> validateFormat(
        const AudioFormatSpec& format) const;

    //==========================================================================
    /**
     * @brief Check if format is valid
     */
    bool isValidFormat(const AudioFormatSpec& format) const;

    //==========================================================================
    /**
     * @brief Check if sample rate is valid
     */
    static bool isValidSampleRate(int sampleRate) {
        // Common sample rates (can be extended)
        static const std::vector<int> validRates = {
            8000, 11025, 16000, 22050, 32000, 44100, 48000,
            64000, 88200, 96000, 176400, 192000, 352800, 384000
        };

        // Also accept any rate between 8kHz and 384kHz
        return sampleRate >= 8000 && sampleRate <= 384000;
    }

    //==========================================================================
    /**
     * @brief Check if bit depth is valid
     */
    static bool isValidBitDepth(AudioBitDepth depth) {
        return depth != AudioBitDepth::Unknown;
    }

    //==========================================================================
    /**
     * @brief Check if channel count is valid
     */
    static bool isValidChannelCount(int channels) {
        return channels > 0 && channels <= 64;  // Reasonable limits
    }

    //==========================================================================
    /**
     * @brief Get standard sample rates
     */
    static std::vector<int> getStandardSampleRates() {
        return {44100, 48000, 88200, 96000, 192000};
    }

    //==========================================================================
    /**
     * @brief Get recommended format for quality
     */
    static AudioFormatSpec getRecommendedFormat(bool highQuality = true) {
        AudioFormatSpec format;
        format.sampleRate = highQuality ? 96000 : 48000;
        format.channels = 2;
        format.bitDepth = highQuality ? AudioBitDepth::Float64 : AudioBitDepth::Float32;
        format.isFloatingPoint = true;
        format.bytesPerSample = highQuality ? 8 : 4;
        return format;
    }

private:
    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFormatValidator)
};

//==============================================================================
/**
 * @brief Singleton accessor for audio format validator
 */
class AudioFormatValidatorHolder {
public:
    static AudioFormatValidator& getInstance() {
        static AudioFormatValidator instance;
        return instance;
    }

    AudioFormatValidatorHolder(const AudioFormatValidatorHolder&) = delete;
    AudioFormatValidatorHolder& operator=(const AudioFormatValidatorHolder&) = delete;

private:
    AudioFormatValidatorHolder() = default;
    ~AudioFormatValidatorHolder() = default;
};

} // namespace zenith
