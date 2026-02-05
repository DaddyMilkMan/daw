/*
  ==============================================================================

    PresetSchemaValidator.cpp
    Created: 2025-12-31
    Author:  Deep-Synth Agent

    Implementation of preset schema validation based on 
    ZENITH_POLYSYNTH_PARAMETER_SCHEMA.md

  ==============================================================================
*/

#include "PresetSchemaValidator.h"
#include <cmath>

namespace zenith {
namespace ai {

//==============================================================================
// Constructor
//==============================================================================

PresetSchemaValidator::PresetSchemaValidator()
{
    loadSchema();
}

//==============================================================================
// Schema Loading - Implements ZENITH_POLYSYNTH_PARAMETER_SCHEMA.md
//==============================================================================

void PresetSchemaValidator::loadSchema()
{
    parameters_.clear();
    parameterIndex_.clear();
    
    // Reserve space for efficiency
    parameters_.reserve(80);
    
    //--------------------------------------------------------------------------
    // 1. Oscillators (3x)
    //--------------------------------------------------------------------------
    addOscillatorParameters(1);
    addOscillatorParameters(2);
    addOscillatorParameters(3);
    
    //--------------------------------------------------------------------------
    // 2. Unison
    //--------------------------------------------------------------------------
    {
        ParameterDefinition def;
        def.id = "unison_voices";
        def.name = "Unison Voices";
        def.type = ParameterType::Int;
        def.minValue = 1.0f;
        def.maxValue = 7.0f;
        def.defaultValue = 1.0f;
        def.description = "Number of unison voices per note";
        def.group = "Unison";
        addParameter(def);
    }
    {
        ParameterDefinition def;
        def.id = "unison_detune";
        def.name = "Unison Detune";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 100.0f;
        def.defaultValue = 0.0f;
        def.unit = "cents";
        def.description = "Unison voice detune spread";
        def.group = "Unison";
        addParameter(def);
    }
    
    //--------------------------------------------------------------------------
    // 3. Filters (2x)
    //--------------------------------------------------------------------------
    addFilterParameters(1);
    addFilterParameters(2);
    
    // Filter routing
    {
        ParameterDefinition def;
        def.id = "filter_routing";
        def.name = "Filter Routing";
        def.type = ParameterType::Choice;
        def.choices = {"serial", "parallel"};
        def.defaultChoiceIndex = 0;
        def.description = "Filter 1 → Filter 2 (serial) or both in parallel";
        def.group = "Filter";
        addParameter(def);
    }
    
    //--------------------------------------------------------------------------
    // 4. Envelopes (2x ADSR)
    //--------------------------------------------------------------------------
    addEnvelopeParameters("amp", "Amplitude");
    addEnvelopeParameters("mod", "Modulation");
    
    //--------------------------------------------------------------------------
    // 5. LFOs (2x)
    //--------------------------------------------------------------------------
    addLFOParameters(1);
    addLFOParameters(2);
    
    //--------------------------------------------------------------------------
    // 6. Modulation Matrix (8 slots)
    //--------------------------------------------------------------------------
    for (int i = 0; i < 8; ++i) {
        addModulationSlot(i);
    }
    
    //--------------------------------------------------------------------------
    // 7. Effects
    //--------------------------------------------------------------------------
    addEffectParameters();
    
    //--------------------------------------------------------------------------
    // 8. Global Settings
    //--------------------------------------------------------------------------
    addGlobalParameters();
    
    DBG("PresetSchemaValidator: Loaded " + juce::String((int)parameters_.size()) + " parameters");
}

void PresetSchemaValidator::addOscillatorParameters(int oscNumber)
{
    juce::String prefix = "osc" + juce::String(oscNumber) + "_";
    juce::String group = "Oscillator " + juce::String(oscNumber);
    
    // Waveform
    {
        ParameterDefinition def;
        def.id = prefix + "waveform";
        def.name = "Waveform";
        def.type = ParameterType::Choice;
        def.choices = {"sine", "saw", "square", "triangle", "noise", "supersaw"};
        def.defaultChoiceIndex = 1;  // saw
        def.description = "Oscillator waveform type";
        def.group = group;
        addParameter(def);
    }
    
    // Detune
    {
        ParameterDefinition def;
        def.id = prefix + "detune";
        def.name = "Detune";
        def.type = ParameterType::Float;
        def.minValue = -100.0f;
        def.maxValue = 100.0f;
        def.defaultValue = 0.0f;
        def.unit = "cents";
        def.description = "Pitch detune in cents";
        def.group = group;
        addParameter(def);
    }
    
    // Mix
    {
        ParameterDefinition def;
        def.id = prefix + "mix";
        def.name = "Mix";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 1.0f;
        def.defaultValue = (oscNumber == 1) ? 1.0f : 0.0f;  // Only osc1 on by default
        def.description = "Oscillator mix level";
        def.group = group;
        def.safeMinValue = 0.0f;
        def.safeMaxValue = 1.0f;
        def.hasSafeRange = true;
        addParameter(def);
    }
}

void PresetSchemaValidator::addFilterParameters(int filterNumber)
{
    juce::String prefix = (filterNumber == 1) ? "filter_" : "filter2_";
    juce::String group = "Filter " + juce::String(filterNumber);
    
    // Type
    {
        ParameterDefinition def;
        def.id = prefix + "type";
        def.name = "Filter Type";
        def.type = ParameterType::Choice;
        def.choices = {"lowpass", "bandpass", "highpass"};
        def.defaultChoiceIndex = 0;  // lowpass
        def.description = "Filter type";
        def.group = group;
        addParameter(def);
    }
    
    // Cutoff
    {
        ParameterDefinition def;
        def.id = prefix + "cutoff";
        def.name = "Cutoff";
        def.type = ParameterType::Float;
        def.minValue = 20.0f;
        def.maxValue = 20000.0f;
        def.defaultValue = 1000.0f;
        def.unit = "Hz";
        def.description = "Filter cutoff frequency";
        def.group = group;
        def.isLogarithmic = true;
        def.safeMinValue = 50.0f;
        def.safeMaxValue = 18000.0f;
        def.hasSafeRange = true;
        addParameter(def);
    }
    
    // Resonance
    {
        ParameterDefinition def;
        def.id = prefix + "resonance";
        def.name = "Resonance";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 1.0f;
        def.defaultValue = 0.0f;
        def.description = "Filter resonance/Q";
        def.group = group;
        def.safeMinValue = 0.0f;
        def.safeMaxValue = 0.9f;  // Avoid self-oscillation
        def.hasSafeRange = true;
        addParameter(def);
    }
    
    // Drive
    {
        ParameterDefinition def;
        def.id = prefix + "drive";
        def.name = "Drive";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 10.0f;
        def.defaultValue = 1.0f;
        def.description = "Filter drive/saturation";
        def.group = group;
        addParameter(def);
    }
}

void PresetSchemaValidator::addEnvelopeParameters(const juce::String& prefix, const juce::String& name)
{
    juce::String group = name + " Envelope";
    
    // Attack
    {
        ParameterDefinition def;
        def.id = prefix + "_attack";
        def.name = "Attack";
        def.type = ParameterType::Float;
        def.minValue = 0.001f;
        def.maxValue = 5.0f;
        def.defaultValue = 0.01f;
        def.unit = "seconds";
        def.description = name + " envelope attack time";
        def.group = group;
        def.isLogarithmic = true;
        addParameter(def);
    }
    
    // Decay
    {
        ParameterDefinition def;
        def.id = prefix + "_decay";
        def.name = "Decay";
        def.type = ParameterType::Float;
        def.minValue = 0.001f;
        def.maxValue = 5.0f;
        def.defaultValue = 0.1f;
        def.unit = "seconds";
        def.description = name + " envelope decay time";
        def.group = group;
        def.isLogarithmic = true;
        addParameter(def);
    }
    
    // Sustain
    {
        ParameterDefinition def;
        def.id = prefix + "_sustain";
        def.name = "Sustain";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 1.0f;
        def.defaultValue = 0.8f;
        def.description = name + " envelope sustain level";
        def.group = group;
        addParameter(def);
    }
    
    // Release
    {
        ParameterDefinition def;
        def.id = prefix + "_release";
        def.name = "Release";
        def.type = ParameterType::Float;
        def.minValue = 0.001f;
        def.maxValue = 10.0f;
        def.defaultValue = 0.5f;
        def.unit = "seconds";
        def.description = name + " envelope release time";
        def.group = group;
        def.isLogarithmic = true;
        addParameter(def);
    }
}

void PresetSchemaValidator::addLFOParameters(int lfoNumber)
{
    juce::String prefix = "lfo" + juce::String(lfoNumber) + "_";
    juce::String group = "LFO " + juce::String(lfoNumber);
    
    // Rate
    {
        ParameterDefinition def;
        def.id = prefix + "rate";
        def.name = "Rate";
        def.type = ParameterType::Float;
        def.minValue = 0.1f;
        def.maxValue = 20.0f;
        def.defaultValue = 1.0f;
        def.unit = "Hz";
        def.description = "LFO frequency";
        def.group = group;
        addParameter(def);
    }
    
    // Amount
    {
        ParameterDefinition def;
        def.id = prefix + "amount";
        def.name = "Amount";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 1.0f;
        def.defaultValue = 0.0f;
        def.description = "LFO modulation depth";
        def.group = group;
        addParameter(def);
    }
    
    // Target
    {
        ParameterDefinition def;
        def.id = prefix + "target";
        def.name = "Target";
        def.type = ParameterType::Choice;
        def.choices = {"filter_cutoff", "osc1_pitch", "osc2_pitch", "osc1_mix", "osc2_mix"};
        def.defaultChoiceIndex = 0;  // filter_cutoff
        def.description = "LFO modulation target";
        def.group = group;
        addParameter(def);
    }
}

void PresetSchemaValidator::addModulationSlot(int slotNumber)
{
    juce::String prefix = "mod_slot_" + juce::String(slotNumber) + "_";
    juce::String group = "Modulation";
    
    // Source
    {
        ParameterDefinition def;
        def.id = prefix + "source";
        def.name = "Source " + juce::String(slotNumber);
        def.type = ParameterType::Choice;
        def.choices = {"none", "lfo1", "lfo2", "env1", "env2", "velocity", "modwheel", "aftertouch"};
        def.defaultChoiceIndex = 0;  // none
        def.group = group;
        addParameter(def);
    }
    
    // Destination
    {
        ParameterDefinition def;
        def.id = prefix + "destination";
        def.name = "Destination " + juce::String(slotNumber);
        def.type = ParameterType::Choice;
        def.choices = {"none", "filter_cutoff", "filter_resonance", "osc1_pitch", "osc2_pitch", 
                        "osc3_pitch", "wavetable_pos", "pan", "volume", "osc1_mix", "osc2_mix",
                        "osc3_mix", "osc_shape"};
        def.defaultChoiceIndex = 0;  // none
        def.group = group;
        addParameter(def);
    }
    
    // Amount
    {
        ParameterDefinition def;
        def.id = prefix + "amount";
        def.name = "Amount " + juce::String(slotNumber);
        def.type = ParameterType::Float;
        def.minValue = -1.0f;
        def.maxValue = 1.0f;
        def.defaultValue = 0.0f;
        def.description = "Modulation depth";
        def.group = group;
        addParameter(def);
    }
}

void PresetSchemaValidator::addEffectParameters()
{
    juce::String group = "Effects";
    
    // Distortion
    {
        ParameterDefinition def;
        def.id = "distortion";
        def.name = "Distortion";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 1.0f;
        def.defaultValue = 0.0f;
        def.description = "Distortion amount";
        def.group = group;
        addParameter(def);
    }
    
    // Chorus
    {
        ParameterDefinition def;
        def.id = "chorus";
        def.name = "Chorus";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 1.0f;
        def.defaultValue = 0.0f;
        def.description = "Chorus effect amount";
        def.group = group;
        addParameter(def);
    }
}

void PresetSchemaValidator::addGlobalParameters()
{
    juce::String group = "Global";
    
    // Glide Time
    {
        ParameterDefinition def;
        def.id = "glide_time";
        def.name = "Glide Time";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 2.0f;
        def.defaultValue = 0.0f;
        def.unit = "seconds";
        def.description = "Portamento/glide time";
        def.group = group;
        addParameter(def);
    }
    
    // Mono Mode
    {
        ParameterDefinition def;
        def.id = "mono_mode";
        def.name = "Mono Mode";
        def.type = ParameterType::Bool;
        def.minValue = 0.0f;
        def.maxValue = 1.0f;
        def.defaultValue = 0.0f;
        def.description = "Monophonic mode (legato)";
        def.group = group;
        addParameter(def);
    }
    
    // Master Gain
    {
        ParameterDefinition def;
        def.id = "master_gain";
        def.name = "Master Gain";
        def.type = ParameterType::Float;
        def.minValue = 0.0f;
        def.maxValue = 2.0f;
        def.defaultValue = 0.7f;
        def.description = "Master output gain";
        def.group = group;
        def.safeMinValue = 0.1f;
        def.safeMaxValue = 1.5f;
        def.hasSafeRange = true;
        addParameter(def);
    }
    
    // Quality Setting
    {
        ParameterDefinition def;
        def.id = "quality_setting";
        def.name = "Quality";
        def.type = ParameterType::Choice;
        def.choices = {"low", "medium", "high"};
        def.defaultChoiceIndex = 1;  // medium
        def.description = "CPU quality preset";
        def.group = group;
        addParameter(def);
    }
}

void PresetSchemaValidator::addParameter(const ParameterDefinition& def)
{
    parameterIndex_[def.id] = parameters_.size();
    parameters_.push_back(def);
}

//==============================================================================
// Validation
//==============================================================================

PresetValidationResult PresetSchemaValidator::validate(const Preset& preset) const
{
    PresetValidationResult result;
    
    // Get preset parameters (Preset is defined in InstrumentPreset.h)
    const auto& params = preset.parameters;
    
    // Track which parameters we've seen
    std::set<juce::String> seenParameters;
    
    // Validate each parameter in the preset
    for (const auto& [paramId, value] : params) {
        seenParameters.insert(paramId);
        
        auto paramResult = validateParameter(paramId, value);
        result.parameterResults.push_back(paramResult);
        
        if (paramResult.isValid) {
            result.validParameters++;
        } else {
            result.invalidParameters++;
            result.isValid = false;
            result.errors.push_back("Invalid parameter '" + paramId + "': " + paramResult.error);
        }
        
        if (paramResult.wasClamped) {
            result.clampedParameters++;
            result.clampedParameterIds.push_back(paramId);
            result.warnings.push_back("Parameter '" + paramId + "' clamped from " +
                                       juce::String(paramResult.originalValue) + " to " +
                                       juce::String(paramResult.clampedValue));
        }
    }
    
    // Check for missing required parameters
    for (const auto& def : parameters_) {
        if (seenParameters.find(def.id) == seenParameters.end()) {
            // Parameter missing - this is a warning, not an error
            result.missingParameters++;
            result.missingParameterIds.push_back(def.id);
            result.warnings.push_back("Missing parameter '" + def.id + "' (using default: " +
                                       juce::String(def.defaultValue) + ")");
        }
    }
    
    return result;
}

ParameterValidationResult PresetSchemaValidator::validateParameter(const juce::String& parameterId, 
                                                                     float value) const
{
    ParameterValidationResult result;
    result.parameterId = parameterId;
    result.originalValue = value;
    result.clampedValue = value;
    
    // Find parameter definition
    auto it = parameterIndex_.find(parameterId);
    if (it == parameterIndex_.end()) {
        result.isValid = false;
        result.error = "Unknown parameter ID '" + parameterId + "'";
        return result;
    }
    
    const auto& def = parameters_[it->second];
    
    // Validate based on type
    switch (def.type) {
        case ParameterType::Float:
        case ParameterType::Int: {
            if (value < def.minValue || value > def.maxValue) {
                result.wasClamped = true;
                result.clampedValue = juce::jlimit(def.minValue, def.maxValue, value);
                result.warning = "Value " + juce::String(value) + " outside range [" +
                                  juce::String(def.minValue) + ", " + juce::String(def.maxValue) + "]";
            }
            if (def.type == ParameterType::Int) {
                float rounded = std::round(value);
                if (std::abs(value - rounded) > 0.001f) {
                    result.wasClamped = true;
                    result.clampedValue = rounded;
                    result.warning = "Integer parameter rounded from " + juce::String(value) +
                                      " to " + juce::String(rounded);
                }
            }
            break;
        }
        
        case ParameterType::Bool: {
            if (value != 0.0f && value != 1.0f) {
                result.wasClamped = true;
                result.clampedValue = (value >= 0.5f) ? 1.0f : 0.0f;
                result.warning = "Boolean parameter coerced to " + juce::String((int)result.clampedValue);
            }
            break;
        }
        
        case ParameterType::Choice: {
            int index = static_cast<int>(std::round(value));
            if (index < 0 || index >= static_cast<int>(def.choices.size())) {
                result.isValid = false;
                result.error = "Choice index " + juce::String(index) + " out of range [0, " +
                                juce::String((int)def.choices.size() - 1) + "]";
            }
            break;
        }
    }
    
    result.isValid = result.error.isEmpty();
    return result;
}

bool PresetSchemaValidator::isValid(const Preset& preset) const
{
    return validate(preset).isValid;
}

//==============================================================================
// Clamping
//==============================================================================

Preset PresetSchemaValidator::clampToSchema(const Preset& preset) const
{
    Preset result = preset;
    
    for (auto& [paramId, value] : result.parameters) {
        value = clampParameter(paramId, value);
    }
    
    return result;
}

float PresetSchemaValidator::clampParameter(const juce::String& parameterId, float value) const
{
    auto it = parameterIndex_.find(parameterId);
    if (it == parameterIndex_.end()) {
        return value;  // Unknown parameter, return as-is
    }
    
    const auto& def = parameters_[it->second];
    
    switch (def.type) {
        case ParameterType::Float:
            return juce::jlimit(def.minValue, def.maxValue, value);
            
        case ParameterType::Int:
            return juce::jlimit(def.minValue, def.maxValue, std::round(value));
            
        case ParameterType::Bool:
            return (value >= 0.5f) ? 1.0f : 0.0f;
            
        case ParameterType::Choice: {
            int index = juce::jlimit(0, static_cast<int>(def.choices.size()) - 1, 
                                      static_cast<int>(std::round(value)));
            return static_cast<float>(index);
        }
    }
    
    return value;
}

float PresetSchemaValidator::clampToSafeRange(const juce::String& parameterId, float value) const
{
    auto it = parameterIndex_.find(parameterId);
    if (it == parameterIndex_.end()) {
        return value;
    }
    
    const auto& def = parameters_[it->second];
    
    if (def.hasSafeRange) {
        return juce::jlimit(def.safeMinValue, def.safeMaxValue, value);
    }
    
    return clampParameter(parameterId, value);
}

//==============================================================================
// Schema Introspection
//==============================================================================

std::vector<juce::String> PresetSchemaValidator::getAllParameterIds() const
{
    std::vector<juce::String> ids;
    ids.reserve(parameters_.size());
    
    for (const auto& def : parameters_) {
        ids.push_back(def.id);
    }
    
    return ids;
}

const ParameterDefinition* PresetSchemaValidator::getParameterDefinition(const juce::String& parameterId) const
{
    auto it = parameterIndex_.find(parameterId);
    if (it == parameterIndex_.end()) {
        return nullptr;
    }
    return &parameters_[it->second];
}

std::pair<float, float> PresetSchemaValidator::getParameterRange(const juce::String& parameterId) const
{
    if (auto* def = getParameterDefinition(parameterId)) {
        return {def->minValue, def->maxValue};
    }
    return {0.0f, 1.0f};
}

ParameterType PresetSchemaValidator::getParameterType(const juce::String& parameterId) const
{
    if (auto* def = getParameterDefinition(parameterId)) {
        return def->type;
    }
    return ParameterType::Float;
}

float PresetSchemaValidator::getDefaultValue(const juce::String& parameterId) const
{
    if (auto* def = getParameterDefinition(parameterId)) {
        return def->defaultValue;
    }
    return 0.0f;
}

std::vector<juce::String> PresetSchemaValidator::getChoiceValues(const juce::String& parameterId) const
{
    if (auto* def = getParameterDefinition(parameterId)) {
        return def->choices;
    }
    return {};
}

bool PresetSchemaValidator::hasParameter(const juce::String& parameterId) const
{
    return parameterIndex_.find(parameterId) != parameterIndex_.end();
}

std::map<juce::String, std::vector<juce::String>> PresetSchemaValidator::getParametersByGroup() const
{
    std::map<juce::String, std::vector<juce::String>> groups;
    
    for (const auto& def : parameters_) {
        groups[def.group].push_back(def.id);
    }
    
    return groups;
}

//==============================================================================
// Preset Generation Helpers
//==============================================================================

Preset PresetSchemaValidator::createDefaultPreset() const
{
    Preset preset;
    preset.name = "Init";
    preset.category = "init";
    
    for (const auto& def : parameters_) {
        preset.parameters[def.id] = def.defaultValue;
    }
    
    return preset;
}

Preset PresetSchemaValidator::createPresetFromNormalized(const std::vector<float>& normalizedValues,
                                                          const juce::String& name) const
{
    Preset preset;
    preset.name = name;
    
    size_t numParams = std::min(normalizedValues.size(), parameters_.size());
    
    for (size_t i = 0; i < numParams; ++i) {
        const auto& def = parameters_[i];
        float normalized = juce::jlimit(0.0f, 1.0f, normalizedValues[i]);
        float value = denormalizeValue(def, normalized);
        preset.parameters[def.id] = value;
    }
    
    // Fill remaining parameters with defaults
    for (size_t i = numParams; i < parameters_.size(); ++i) {
        const auto& def = parameters_[i];
        preset.parameters[def.id] = def.defaultValue;
    }
    
    return preset;
}

std::vector<float> PresetSchemaValidator::presetToNormalized(const Preset& preset) const
{
    std::vector<float> normalized;
    normalized.reserve(parameters_.size());
    
    for (const auto& def : parameters_) {
        auto it = preset.parameters.find(def.id);
        float value = (it != preset.parameters.end()) ? it->second : def.defaultValue;
        normalized.push_back(normalizeValue(def, value));
    }
    
    return normalized;
}

float PresetSchemaValidator::normalizeValue(const ParameterDefinition& def, float value) const
{
    if (def.type == ParameterType::Bool || def.type == ParameterType::Choice) {
        // For discrete types, normalize to index ratio
        float range = def.maxValue - def.minValue;
        return (range > 0) ? (value - def.minValue) / range : 0.0f;
    }
    
    if (def.isLogarithmic && def.minValue > 0 && def.maxValue > def.minValue) {
        // Log scale normalization
        float logMin = std::log10(def.minValue);
        float logMax = std::log10(def.maxValue);
        float logValue = std::log10(std::max(def.minValue, value));
        return (logValue - logMin) / (logMax - logMin);
    }
    
    // Linear normalization
    float range = def.maxValue - def.minValue;
    return (range > 0) ? (value - def.minValue) / range : 0.0f;
}

float PresetSchemaValidator::denormalizeValue(const ParameterDefinition& def, float normalized) const
{
    if (def.type == ParameterType::Bool || def.type == ParameterType::Choice) {
        float range = def.maxValue - def.minValue;
        return def.minValue + normalized * range;
    }
    
    if (def.isLogarithmic && def.minValue > 0 && def.maxValue > def.minValue) {
        float logMin = std::log10(def.minValue);
        float logMax = std::log10(def.maxValue);
        float logValue = logMin + normalized * (logMax - logMin);
        return std::pow(10.0f, logValue);
    }
    
    // Linear denormalization
    return def.minValue + normalized * (def.maxValue - def.minValue);
}

//==============================================================================
// Sound Type Templates
//==============================================================================

std::map<juce::String, float> PresetSchemaValidator::getTemplateForSoundType(const juce::String& soundType) const
{
    std::map<juce::String, float> template_;
    
    // Start with defaults
    for (const auto& def : parameters_) {
        template_[def.id] = def.defaultValue;
    }
    
    juce::String type = soundType.toLowerCase();
    
    if (type == "pad") {
        // Slow attack, long release, multiple oscillators, low-pass filter
        template_["amp_attack"] = 1.5f;
        template_["amp_decay"] = 0.5f;
        template_["amp_sustain"] = 0.8f;
        template_["amp_release"] = 2.0f;
        template_["osc1_waveform"] = 1.0f;  // saw
        template_["osc1_mix"] = 0.7f;
        template_["osc2_waveform"] = 0.0f;  // sine
        template_["osc2_mix"] = 0.3f;
        template_["osc2_detune"] = 7.0f;
        template_["filter_cutoff"] = 800.0f;
        template_["filter_resonance"] = 0.3f;
        template_["lfo1_rate"] = 0.3f;
        template_["lfo1_amount"] = 0.2f;
        template_["chorus"] = 0.3f;
    }
    else if (type == "bass") {
        // Fast attack, short release, low cutoff
        template_["amp_attack"] = 0.005f;
        template_["amp_decay"] = 0.2f;
        template_["amp_sustain"] = 0.6f;
        template_["amp_release"] = 0.3f;
        template_["osc1_waveform"] = 1.0f;  // saw
        template_["osc1_mix"] = 1.0f;
        template_["osc2_waveform"] = 2.0f;  // square
        template_["osc2_mix"] = 0.5f;
        template_["osc2_detune"] = 0.0f;
        template_["filter_cutoff"] = 400.0f;
        template_["filter_resonance"] = 0.5f;
        template_["filter_drive"] = 2.0f;
    }
    else if (type == "lead") {
        // Medium attack, bright filter
        template_["amp_attack"] = 0.02f;
        template_["amp_decay"] = 0.3f;
        template_["amp_sustain"] = 0.7f;
        template_["amp_release"] = 0.5f;
        template_["osc1_waveform"] = 1.0f;  // saw
        template_["osc1_mix"] = 1.0f;
        template_["filter_cutoff"] = 3000.0f;
        template_["filter_resonance"] = 0.6f;
        template_["lfo1_rate"] = 5.0f;
        template_["lfo1_amount"] = 0.1f;
    }
    else if (type == "pluck") {
        // Very fast attack, short decay, low sustain
        template_["amp_attack"] = 0.002f;
        template_["amp_decay"] = 0.15f;
        template_["amp_sustain"] = 0.1f;
        template_["amp_release"] = 0.1f;
        template_["osc1_waveform"] = 1.0f;  // saw
        template_["osc1_mix"] = 1.0f;
        template_["filter_cutoff"] = 5000.0f;
        template_["filter_resonance"] = 0.4f;
        template_["mod_attack"] = 0.001f;
        template_["mod_decay"] = 0.2f;
        template_["mod_sustain"] = 0.0f;
        template_["mod_release"] = 0.1f;
    }
    else if (type == "arp") {
        // Fast, clean
        template_["amp_attack"] = 0.005f;
        template_["amp_decay"] = 0.1f;
        template_["amp_sustain"] = 0.5f;
        template_["amp_release"] = 0.15f;
        template_["osc1_waveform"] = 2.0f;  // square
        template_["osc1_mix"] = 1.0f;
        template_["filter_cutoff"] = 2000.0f;
        template_["filter_resonance"] = 0.3f;
    }
    
    return template_;
}

//==============================================================================
// Global Instance
//==============================================================================

PresetSchemaValidator& getPresetSchemaValidator()
{
    static PresetSchemaValidator instance;
    return instance;
}

} // namespace ai
} // namespace zenith
