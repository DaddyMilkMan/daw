/*
  ==============================================================================

    ChannelMapper.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #3)

    Safe audio channel mapping with validation and conversion.

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
 * @brief Channel mapping error type
 */
enum class ChannelMappingError {
    InvalidInputLayout,     // Input channel layout is invalid
    InvalidOutputLayout,    // Output channel layout is invalid
    ChannelCountMismatch,   // Channel counts don't match
    InvalidMapping,         // Mapping itself is invalid
    Overflow,               // Overflow during mapping
    Unknown
};

//==============================================================================
/**
 * @brief Channel mapping issue
 */
struct ChannelMappingIssue {
    ChannelMappingError error;
    juce::String description;
    int inputChannels = 0;
    int outputChannels = 0;
    int severity = 0;  // 0-10

    juce::String toString() const {
        juce::String errorStr;
        switch (error) {
            case ChannelMappingError::InvalidInputLayout: errorStr = "Invalid Input"; break;
            case ChannelMappingError::InvalidOutputLayout: errorStr = "Invalid Output"; break;
            case ChannelMappingError::ChannelCountMismatch: errorStr = "Count Mismatch"; break;
            case ChannelMappingError::InvalidMapping: errorStr = "Invalid Mapping"; break;
            case ChannelMappingError::Overflow: errorStr = "Overflow"; break;
            case ChannelMappingError::Unknown: errorStr = "Unknown"; break;
        }
        return "[" + errorStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Channel mapping configuration
 */
struct ChannelMappingConfig {
    juce::AudioChannelSet inputLayout;
    juce::AudioChannelSet outputLayout;
    std::vector<std::pair<int, int>> mappings;  // (inputChannel, outputChannel)
    bool validateBeforeMapping = true;
    bool allowUpmix = true;
    bool allowDownmix = true;
    bool strictMode = false;  // Fail if exact mapping not possible
};

//==============================================================================
/**
 * @brief Channel mapper statistics
 */
struct ChannelMapperStatistics {
    int totalMappings = 0;
    int successfulMappings = 0;
    int failedMappings = 0;
    int upmixes = 0;
    int downmixes = 0;
    int directMappings = 0;

    juce::String toString() const {
        return "Channel Mapper: " +
               juce::String(successfulMappings) + "/" +
               juce::String(totalMappings) + " successful";
    }
};

//==============================================================================
/**
 * @brief Safe audio channel mapper
 *
 * Features:
 * - Validates input/output channel layouts
 * - Safe channel mapping (mono->stereo, stereo->mono, etc.)
 * - Upmixing and downmixing support
 * - Comprehensive error checking
 * - Statistics tracking
 * - Strict mode for safety-critical applications
 */
class ChannelMapper {
public:
    //==========================================================================
    ChannelMapper();
    ~ChannelMapper();

    //==========================================================================
    /**
     * @brief Map audio buffer from input layout to output layout
     * @param inputBuffer Input audio buffer
     * @param inputLayout Input channel layout
     * @param outputBuffer Output audio buffer (will be filled)
     * @param outputLayout Output channel layout
     * @return List of issues (empty if successful)
     */
    std::vector<ChannelMappingIssue> mapBuffer(
        const juce::AudioBuffer<float>& inputBuffer,
        const juce::AudioChannelSet& inputLayout,
        juce::AudioBuffer<float>& outputBuffer,
        const juce::AudioChannelSet& outputLayout);

    //==========================================================================
    /**
     * @brief Map using configuration
     */
    std::vector<ChannelMappingIssue> mapBuffer(
        const juce::AudioBuffer<float>& inputBuffer,
        juce::AudioBuffer<float>& outputBuffer,
        const ChannelMappingConfig& config);

    //==========================================================================
    /**
     * @brief Validate channel layout
     * @return true if layout is valid
     */
    static bool isValidLayout(const juce::AudioChannelSet& layout);

    //==========================================================================
    /**
     * @brief Validate mapping configuration
     * @return List of issues (empty if valid)
     */
    std::vector<ChannelMappingIssue> validateConfig(
        const ChannelMappingConfig& config) const;

    //==========================================================================
    /**
     * @brief Create standard upmix mapping (mono->stereo, etc.)
     */
    static ChannelMappingConfig createStandardMapping(
        const juce::AudioChannelSet& inputLayout,
        const juce::AudioChannelSet& outputLayout);

    //==========================================================================
    /**
     * @brief Get description of layout
     */
    static juce::String getLayoutDescription(const juce::AudioChannelSet& layout);

    //==========================================================================
    /**
     * @brief Get required output buffer size
     * Returns minimum number of channels needed for output
     */
    static int getRequiredOutputChannels(
        const juce::AudioChannelSet& inputLayout,
        const juce::AudioChannelSet& outputLayout);

    //==========================================================================
    /**
     * @brief Check if upmixing is needed
     */
    static bool needsUpmix(
        const juce::AudioChannelSet& inputLayout,
        const juce::AudioChannelSet& outputLayout);

    //==========================================================================
    /**
     * @brief Check if downmixing is needed
     */
    static bool needsDownmix(
        const juce::AudioChannelSet& inputLayout,
        const juce::AudioChannelSet& outputLayout);

    //==========================================================================
    /**
     * @brief Get statistics
     */
    ChannelMapperStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        statistics_ = ChannelMapperStatistics{};
    }

private:
    //==========================================================================
    std::vector<ChannelMappingIssue> performMapping(
        const juce::AudioBuffer<float>& inputBuffer,
        juce::AudioBuffer<float>& outputBuffer,
        const ChannelMappingConfig& config);

    void mapMonoToStereo(
        const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& output);

    void mapStereoToMono(
        const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& output);

    void mapDirect(
        const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& output,
        const std::vector<std::pair<int, int>>& mappings);

    void mapAmbient(
        const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& output,
        const juce::AudioChannelSet& inputLayout,
        const juce::AudioChannelSet& outputLayout);

    //==========================================================================
    // Statistics
    ChannelMapperStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelMapper)
};

//==============================================================================
/**
 * @brief Singleton accessor for channel mapper
 */
class ChannelMapperHolder {
public:
    static ChannelMapper& getInstance() {
        static ChannelMapper instance;
        return instance;
    }

    ChannelMapperHolder(const ChannelMapperHolder&) = delete;
    ChannelMapperHolder& operator=(const ChannelMapperHolder&) = delete;

private:
    ChannelMapperHolder() = default;
    ~ChannelMapperHolder() = default;
};

} // namespace zenith
