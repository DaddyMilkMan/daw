/*
  ==============================================================================

    PluginValidator.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 3: Plugin Safety

    Validates plugin state and presets before loading to prevent crashes.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Result of plugin state validation
 */
struct PluginValidationResult {
    bool isValid = true;
    juce::String errorMessage;
    int stateSize = 0;
    int parameterCount = 0;
    int invalidParameters = 0;
    juce::StringArray warnings;

    void addWarning(const juce::String& warning) {
        warnings.add(warning);
    }
};

//==============================================================================
/**
 * @class PluginValidator
 * @brief Validates plugin state and presets for safety
 *
 * Prevents crashes from:
 * - Corrupted plugin state
 * - Invalid parameter values
 * - Out-of-range parameters
 * - Missing parameters
 * - Malformed preset data
 */
class PluginValidator {
public:
    friend struct SafePluginState;
    PluginValidator() = default;
    ~PluginValidator() = default;

    //==========================================================================
    // State Validation
    //==========================================================================

    /**
     * @brief Validate plugin state before loading
     * @param plugin Plugin instance to validate against
     * @param stateData State data to validate
     * @return Validation result
     *
     * Checks:
     * - State size is reasonable (< 10MB)
     * - State format is valid
     * - Can be loaded without crashes
     */
    PluginValidationResult validateState(juce::AudioPluginInstance& plugin,
                                         const juce::MemoryBlock& stateData);

    /**
     * @brief Validate plugin state (string version)
     */
    PluginValidationResult validateState(juce::AudioPluginInstance& plugin,
                                         const juce::String& stateData);

    //==========================================================================
    // Parameter Validation
    //==========================================================================

    /**
     * @brief Validate parameter value is in range
     * @param parameter Parameter to validate
     * @param value Value to check
     * @return true if value is valid
     */
    bool validateParameterValue(juce::AudioProcessorParameter& parameter,
                               float value) const;

    /**
     * @brief Sanitize parameter value to valid range
     * @param parameter Parameter
     * @param value Value to sanitize
     * @return Clamped value in [min, max]
     */
    float sanitizeParameterValue(juce::AudioProcessorParameter& parameter,
                                float value) const;

    /**
     * @brief Validate all plugin parameters
     * @param plugin Plugin instance
     * @return Validation result with warnings
     */
    PluginValidationResult validateAllParameters(juce::AudioPluginInstance& plugin);

    //==========================================================================
    // Preset Validation
    //==========================================================================

    /**
     * @brief Validate preset file before loading
     * @param presetFile Preset file to validate
     * @param plugin Plugin instance (for compatibility check)
     * @return Validation result
     */
    PluginValidationResult validatePreset(const juce::File& presetFile,
                                          juce::AudioPluginInstance& plugin);

    //==========================================================================
    // Round-trip Testing
    //==========================================================================

    /**
     * @brief Test state save/load round-trip
     * @param plugin Plugin instance
     * @return true if round-trip successful
     *
     * Saves state, restores it, and compares parameters.
     * Catches serialization issues.
     */
    bool testStateRoundTrip(juce::AudioPluginInstance& plugin);

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set maximum allowed state size
     * @param maxSizeBytes Maximum size in bytes (default: 10MB)
     */
    void setMaxStateSize(juce::int64 maxSizeBytes) {
        maxStateSize_ = maxSizeBytes;
    }

    /**
     * @brief Enable/disable strict validation
     * @param strict If true, reject all invalid states. If false, warn only.
     */
    void setStrictMode(bool strict) {
        strictMode_ = strict;
    }

private:
    juce::int64 maxStateSize_ = 10 * 1024 * 1024;  // 10MB default
    bool strictMode_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginValidator)
};

//==============================================================================
/**
 * @brief Safe plugin state wrapper with validation
 */
struct SafePluginState {
    juce::MemoryBlock stateData;
    PluginValidationResult validationResult;
    bool isValid = false;

    /**
     * @brief Create safe state from plugin
     * @return true if state created and validated
     */
    bool captureFrom(juce::AudioPluginInstance& plugin, PluginValidator& validator);

    /**
     * @brief Apply state to plugin with validation
     * @return true if state applied successfully
     */
    bool applyTo(juce::AudioPluginInstance& plugin, PluginValidator& validator);
};

} // namespace zenith
