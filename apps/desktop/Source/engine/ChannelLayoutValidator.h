/*
  ==============================================================================

    ChannelLayoutValidator.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #3)

    Validates audio channel configurations and layouts.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Layout validation issue
 */
struct LayoutValidationIssue {
    enum Type {
        InvalidChannelCount,       // Channel count out of range
        UnsupportedLayout,         // Layout not supported
        InvalidChannelType,        // Channel type not recognized
        DuplicateChannel,          // Same channel appears twice
        MissingRequiredChannel,    // Required channel missing
        IncompatibleLayouts        // Two layouts incompatible
    };

    Type type;
    juce::String description;
    int channelCount = 0;
    int severity = 0;  // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case InvalidChannelCount: typeStr = "Invalid Count"; break;
            case UnsupportedLayout: typeStr = "Unsupported"; break;
            case InvalidChannelType: typeStr = "Invalid Type"; break;
            case DuplicateChannel: typeStr = "Duplicate"; break;
            case MissingRequiredChannel: typeStr = "Missing Channel"; break;
            case IncompatibleLayouts: typeStr = "Incompatible"; break;
        }
        return "[" + typeStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Layout validation result
 */
struct LayoutValidationResult {
    bool isValid = false;
    std::vector<LayoutValidationIssue> issues;
    juce::String summary;

    juce::String toString() const {
        if (isValid) {
            return "Layout is valid";
        }
        juce::String result = "Layout validation failed:";
        for (const auto& issue : issues) {
            result += "\n  - " + issue.toString();
        }
        return result;
    }
};

//==============================================================================
/**
 * @brief Channel layout validator
 *
 * Features:
 * - Validates channel layouts
 * - Checks for duplicate channels
 * - Ensures required channels are present
 * - Checks layout compatibility
 * - Supports all standard layouts (mono, stereo, 5.1, 7.1, ambisonic, etc.)
 */
class ChannelLayoutValidator {
public:
    //==========================================================================
    ChannelLayoutValidator();
    ~ChannelLayoutValidator();

    //==========================================================================
    /**
     * @brief Validate a single layout
     * @return Validation result
     */
    LayoutValidationResult validateLayout(
        const juce::AudioChannelSet& layout) const;

    //==========================================================================
    /**
     * @brief Validate two layouts are compatible
     * Use this before attempting to route between layouts
     */
    LayoutValidationResult validateCompatibility(
        const juce::AudioChannelSet& layout1,
        const juce::AudioChannelSet& layout2) const;

    //==========================================================================
    /**
     * @brief Check if layout is a standard layout
     * @return true if layout is a recognized standard
     */
    static bool isStandardLayout(const juce::AudioChannelSet& layout);

    //==========================================================================
    /**
     * @brief Check if layout is discrete (custom channel mapping)
     */
    static bool isDiscreteLayout(const juce::AudioChannelSet& layout);

    //==========================================================================
    /**
     * @brief Check if layout supports a specific channel type
     */
    static bool hasChannelType(const juce::AudioChannelSet& layout,
                               juce::AudioChannelSet::ChannelType type);

    //==========================================================================
    /**
     * @brief Get layout description with details
     */
    static juce::String getDetailedLayoutDescription(
        const juce::AudioChannelSet& layout);

    //==========================================================================
    /**
     * @brief Get minimum required channel count for a layout type
     */
    static int getMinimumChannelCount(
        const juce::AudioChannelSet& layout);

    //==========================================================================
    /**
     * @brief Get maximum supported channel count
     */
    static int getMaximumChannelCount() {
        return 64;  // Reasonable maximum for most use cases
    }

    //==========================================================================
    /**
     * @brief Check if channel count is valid
     */
    static bool isValidChannelCount(int count) {
        return count > 0 && count <= getMaximumChannelCount();
    }

    //==========================================================================
    /**
     * @brief Get all standard layouts
     */
    static std::vector<juce::AudioChannelSet> getStandardLayouts();

    //==========================================================================
    /**
     * @brief Check if layout is ambisonic
     */
    static bool isAmbisonic(const juce::AudioChannelSet& layout);

    //==========================================================================
    /**
     * @brief Get ambisonic order (0, 1, 2, etc.)
     * Returns -1 if not ambisonic
     */
    static int getAmbisonicOrder(const juce::AudioChannelSet& layout);

private:
    //==========================================================================
    bool checkForDuplicateChannels(
        const juce::AudioChannelSet& layout,
        LayoutValidationResult& result) const;

    bool checkRequiredChannels(
        const juce::AudioChannelSet& layout,
        LayoutValidationResult& result) const;

    bool checkChannelTypes(
        const juce::AudioChannelSet& layout,
        LayoutValidationResult& result) const;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelLayoutValidator)
};

//==============================================================================
/**
 * @brief Singleton accessor for layout validator
 */
class ChannelLayoutValidatorHolder {
public:
    static ChannelLayoutValidator& getInstance() {
        static ChannelLayoutValidator instance;
        return instance;
    }

    ChannelLayoutValidatorHolder(const ChannelLayoutValidatorHolder&) = delete;
    ChannelLayoutValidatorHolder& operator=(const ChannelLayoutValidatorHolder&) = delete;

private:
    ChannelLayoutValidatorHolder() = default;
    ~ChannelLayoutValidatorHolder() = default;
};

} // namespace zenith
