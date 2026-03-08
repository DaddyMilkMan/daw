/*
  ==============================================================================

    FormatConverter.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #7)

    Safe audio format conversion with clamping and quality management.

  ==============================================================================
*/

#pragma once

#include "AudioFormatValidator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Conversion statistics
 */
struct FormatConversionStatistics {
    int totalConversions = 0;
    int samplesConverted = 0;
    int clampedValues = 0;
    int overflows = 0;
    int underflows = 0;

    juce::String toString() const {
        return "Format Conv: " +
               juce::String(totalConversions) + " conversions, " +
               juce::String(clampedValues) + " clamped, " +
               juce::String(overflows) + " overflows";
    }
};

//==============================================================================
/**
 * @brief Format converter configuration
 */
struct FormatConverterConfig {
    bool enableClamping = true;        // Clamp values to valid range
    bool enableDithering = false;      // Add dither when reducing precision
    bool enableStatistics = true;
    double clampThreshold = 1.0;       // Clamping threshold
};

//==============================================================================
/**
 * @brief Safe audio format converter
 *
 * Features:
 * - Bit depth conversion
 * - Sample rate validation
 * - Safe clamping
 * - Overflow/underflow detection
 * - Statistics tracking
 */
class FormatConverter {
public:
    //==========================================================================
    FormatConverter();
    ~FormatConverter();

    //==========================================================================
    /**
     * @brief Convert audio buffer to new format
     * @param inputBuffer Input buffer
     * @param inputFormat Input format
     * @param outputFormat Output format
     * @param config Conversion configuration
     * @return Converted buffer (caller takes ownership)
     */
    juce::AudioBuffer<float>* convertBuffer(
        const juce::AudioBuffer<float>& inputBuffer,
        const AudioFormatSpec& inputFormat,
        const AudioFormatSpec& outputFormat,
        const FormatConverterConfig& config = FormatConverterConfig());

    //==========================================================================
    /**
     * @brief Convert bit depth in-place
     * @param buffer Audio buffer (modified in-place)
     * @param fromBitDepth Source bit depth
     * @param toBitDepth Target bit depth
     * @param config Conversion configuration
     */
    void convertBitDepth(
        juce::AudioBuffer<float>& buffer,
        AudioBitDepth fromBitDepth,
        AudioBitDepth toBitDepth,
        const FormatConverterConfig& config = FormatConverterConfig());

    //==========================================================================
    /**
     * @brief Clamp buffer values to valid range
     * @param buffer Audio buffer to clamp
     * @param threshold Clamping threshold (default 1.0)
     * @return Number of values clamped
     */
    int clampBuffer(
        juce::AudioBuffer<float>& buffer,
        double threshold = 1.0);

    //==========================================================================
    /**
     * @brief Check for overflow/underflow
     * @param buffer Audio buffer to check
     * @param threshold Threshold for detection
     * @return Number of overflows/underflows
     */
    std::pair<int, int> detectOverflowUnderflow(
        const juce::AudioBuffer<float>& buffer,
        double threshold = 1.0) const;

    //==========================================================================
    /**
     * @brief Get statistics
     */
    FormatConversionStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

private:
    //==========================================================================
    float convertSample(float value,
                       AudioBitDepth fromDepth,
                       AudioBitDepth toDepth,
                       const FormatConverterConfig& config);

    //==========================================================================
    // Statistics
    FormatConversionStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FormatConverter)
};

//==============================================================================
/**
 * @brief Singleton accessor for format converter
 */
class FormatConverterHolder {
public:
    static FormatConverter& getInstance() {
        static FormatConverter instance;
        return instance;
    }

    FormatConverterHolder(const FormatConverterHolder&) = delete;
    FormatConverterHolder& operator=(const FormatConverterHolder&) = delete;

private:
    FormatConverterHolder() = default;
    ~FormatConverterHolder() = default;
};

} // namespace zenith
