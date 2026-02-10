/*
  ==============================================================================

    PresetSchemaValidator.h
    Created: 2025-12-31
    Author:  Deep-Synth Agent

    Validates synthesizer presets against ZENITH_POLYSYNTH_PARAMETER_SCHEMA.md
    
    Anti-Corner-Cutting Protocol:
    - ALL neural-generated presets MUST pass validation before use
    - Parameters outside valid ranges are clamped with warnings logged
    - Invalid enum values are rejected with detailed error messages

  ==============================================================================
*/

#pragma once

#include <instruments/InstrumentPreset.h>
#include <juce_core/juce_core.h>
#include <map>
#include <set>
#include <vector>

namespace zenith {
namespace ai {

using Preset = ZenithInstrumentPreset;

//==============================================================================
/**
    Parameter type enumeration (matches schema)
*/
enum class ParameterType {
    Float,      // Continuous range [min, max]
    Int,        // Integer range [min, max]
    Bool,       // true/false
    Choice      // Discrete set of string values
};

//==============================================================================
/**
    Parameter definition from schema
*/
struct ParameterDefinition {
    juce::String id;
    juce::String name;
    ParameterType type = ParameterType::Float;
    
    // For float/int types
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    
    // For choice types
    std::vector<juce::String> choices;
    int defaultChoiceIndex = 0;
    
    // Optional metadata
    juce::String unit;          // "Hz", "dB", "cents", etc.
    juce::String description;
    juce::String group;         // "Oscillator", "Filter", "Envelope", etc.
    bool isLogarithmic = false;
    
    // Safe randomization range
    float safeMinValue = 0.0f;
    float safeMaxValue = 1.0f;
    bool hasSafeRange = false;
};

//==============================================================================
/**
    Validation result for a single parameter
*/
struct ParameterValidationResult {
    juce::String parameterId;
    bool isValid = true;
    bool wasClamped = false;
    float originalValue = 0.0f;
    float clampedValue = 0.0f;
    juce::String error;
    juce::String warning;
};

//==============================================================================
/**
    Overall validation result for a preset
*/
struct PresetValidationResult {
    bool isValid = true;
    int validParameters = 0;
    int invalidParameters = 0;
    int clampedParameters = 0;
    int missingParameters = 0;
    
    std::vector<ParameterValidationResult> parameterResults;
    std::vector<juce::String> errors;
    std::vector<juce::String> warnings;
    std::vector<juce::String> clampedParameterIds;
    std::vector<juce::String> missingParameterIds;
    
    juce::String getSummary() const {
        juce::String summary;
        summary << "Preset Validation: " << (isValid ? "PASSED" : "FAILED") << "\n";
        summary << "  Valid: " << validParameters << "\n";
        summary << "  Invalid: " << invalidParameters << "\n";
        summary << "  Clamped: " << clampedParameters << "\n";
        summary << "  Missing: " << missingParameters << "\n";
        return summary;
    }
};

//==============================================================================
/**
    PresetSchemaValidator
    
    Enforces strict adherence to ZENITH_POLYSYNTH_PARAMETER_SCHEMA.md
    
    Usage:
    @code
    PresetSchemaValidator validator;
    
    // Validate a preset
    auto result = validator.validate(preset);
    if (!result.isValid) {
        for (const auto& error : result.errors) {
            DBG("Validation error: " + error);
        }
    }
    
    // Or validate and clamp to safe values
    Preset safePreset = validator.clampToSchema(unsafePreset);
    @endcode
*/
class PresetSchemaValidator {
public:
    //==========================================================================
    PresetSchemaValidator();
    ~PresetSchemaValidator() = default;
    
    //==========================================================================
    // Validation
    //==========================================================================
    
    /**
     * @brief Validate a preset against the schema
     * @param preset The preset to validate
     * @return Detailed validation result
     */
    PresetValidationResult validate(const Preset& preset) const;
    
    /**
     * @brief Validate a single parameter value
     * @param parameterId The parameter ID
     * @param value The value to validate
     * @return Validation result for this parameter
     */
    ParameterValidationResult validateParameter(const juce::String& parameterId, 
                                                  float value) const;
    
    /**
     * @brief Check if a preset is valid (quick check, no details)
     */
    bool isValid(const Preset& preset) const;
    
    //==========================================================================
    // Clamping / Correction
    //==========================================================================
    
    /**
     * @brief Clamp all parameters in a preset to valid schema ranges
     * @param preset The input preset (may have out-of-range values)
     * @return A new preset with all values clamped to valid ranges
     */
    Preset clampToSchema(const Preset& preset) const;
    
    /**
     * @brief Clamp a single parameter value to its valid range
     * @param parameterId The parameter ID
     * @param value The value to clamp
     * @return Clamped value
     */
    float clampParameter(const juce::String& parameterId, float value) const;
    
    /**
     * @brief Clamp value to safe randomization range (more conservative)
     */
    float clampToSafeRange(const juce::String& parameterId, float value) const;
    
    //==========================================================================
    // Schema Introspection
    //==========================================================================
    
    /**
     * @brief Get all parameter IDs defined in the schema
     */
    std::vector<juce::String> getAllParameterIds() const;
    
    /**
     * @brief Get parameter definition
     */
    const ParameterDefinition* getParameterDefinition(const juce::String& parameterId) const;
    
    /**
     * @brief Get parameter range [min, max]
     */
    std::pair<float, float> getParameterRange(const juce::String& parameterId) const;
    
    /**
     * @brief Get parameter type
     */
    ParameterType getParameterType(const juce::String& parameterId) const;
    
    /**
     * @brief Get default value for a parameter
     */
    float getDefaultValue(const juce::String& parameterId) const;
    
    /**
     * @brief Get choice values for a choice parameter
     */
    std::vector<juce::String> getChoiceValues(const juce::String& parameterId) const;
    
    /**
     * @brief Check if a parameter ID is defined in schema
     */
    bool hasParameter(const juce::String& parameterId) const;
    
    /**
     * @brief Get parameters grouped by category
     */
    std::map<juce::String, std::vector<juce::String>> getParametersByGroup() const;
    
    /**
     * @brief Get total parameter count
     */
    int getParameterCount() const { return static_cast<int>(parameters_.size()); }
    
    //==========================================================================
    // Preset Generation Helpers
    //==========================================================================
    
    /**
     * @brief Create a preset with all default values
     */
    Preset createDefaultPreset() const;
    
    /**
     * @brief Create a preset from a flat vector of normalized values
     * @param normalizedValues Values in [0,1] range, one per parameter
     * @param name Preset name
     * @return New preset with denormalized values
     * 
     * This is useful for converting neural network output to preset format.
     */
    Preset createPresetFromNormalized(const std::vector<float>& normalizedValues,
                                       const juce::String& name) const;
    
    /**
     * @brief Convert a preset to a flat vector of normalized values
     * @param preset The preset to convert
     * @return Vector of normalized [0,1] values
     */
    std::vector<float> presetToNormalized(const Preset& preset) const;
    
    //==========================================================================
    // Schema Templates
    //==========================================================================
    
    /**
     * @brief Get template values for a sound type (pad, bass, lead, etc.)
     * @param soundType One of: "pad", "bass", "lead", "pluck", "arp"
     * @return Map of parameter ID to suggested value
     */
    std::map<juce::String, float> getTemplateForSoundType(const juce::String& soundType) const;
    
private:
    //==========================================================================
    // Schema initialization
    void loadSchema();
    void addOscillatorParameters(int oscNumber);
    void addFilterParameters(int filterNumber);
    void addEnvelopeParameters(const juce::String& prefix, const juce::String& name);
    void addLFOParameters(int lfoNumber);
    void addModulationSlot(int slotNumber);
    void addEffectParameters();
    void addGlobalParameters();
    
    //==========================================================================
    // Internal helpers
    void addParameter(const ParameterDefinition& def);
    float normalizeValue(const ParameterDefinition& def, float value) const;
    float denormalizeValue(const ParameterDefinition& def, float normalized) const;
    
    //==========================================================================
    // Parameter storage
    std::vector<ParameterDefinition> parameters_;
    std::map<juce::String, size_t> parameterIndex_;  // ID -> index in parameters_
    
    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetSchemaValidator)
};

//==============================================================================
// Global instance for convenience
PresetSchemaValidator& getPresetSchemaValidator();

} // namespace ai
} // namespace zenith
