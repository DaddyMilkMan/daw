/*
  ==============================================================================

    Instrument.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of InstrumentBase helper class.

  ==============================================================================
*/

#include "Instrument.h"

namespace zenith {

//==============================================================================
InstrumentBase::InstrumentBase(std::unique_ptr<juce::AudioProcessor> processor,
                               InstrumentMetadata metadata)
    : processor_(std::move(processor))
    , metadata_(std::move(metadata))
{
    jassert(processor_ != nullptr);
}

//==============================================================================
bool InstrumentBase::setParameter(const juce::String& parameterId, float normalizedValue)
{
    auto it = parameterIndexMap_.find(parameterId);
    if (it == parameterIndexMap_.end())
        return false;

    int paramIndex = it->second;
    auto* param = processor_->getParameters()[paramIndex];

    if (param != nullptr)
    {
        // Clamp to valid range
        normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);
        param->setValue(normalizedValue);
        return true;
    }

    return false;
}

float InstrumentBase::getParameter(const juce::String& parameterId) const
{
    auto it = parameterIndexMap_.find(parameterId);
    if (it == parameterIndexMap_.end())
        return 0.0f;

    int paramIndex = it->second;
    auto* param = processor_->getParameters()[paramIndex];

    if (param != nullptr)
        return param->getValue();

    return 0.0f;
}

//==============================================================================
bool InstrumentBase::setMacro(const juce::String& macroId, float normalizedValue)
{
    // Find macro in metadata
    const auto* macro = metadata_.findMacro(macroId);
    if (macro == nullptr)
        return false;

    // Clamp value
    normalizedValue = juce::jlimit(0.0f, 1.0f, normalizedValue);

    // Store macro value
    macroValues_[macroId] = normalizedValue;

    // Apply to all target parameters
    for (const auto& target : macro->targets)
    {
        // Get base parameter value (could be from a preset or default)
        float baseValue = getParameter(target.parameterId);

        // Apply macro influence
        // Simple linear mapping: macroValue * amount affects the parameter
        float newValue = baseValue + (normalizedValue - 0.5f) * 2.0f * target.amount;
        newValue = juce::jlimit(0.0f, 1.0f, newValue);

        setParameter(target.parameterId, newValue);
    }

    return true;
}

float InstrumentBase::getMacro(const juce::String& macroId) const
{
    auto it = macroValues_.find(macroId);
    if (it != macroValues_.end())
        return it->second;

    return 0.5f;  // Default macro value is centered
}

//==============================================================================
juce::StringArray InstrumentBase::getPresetIds() const
{
    juce::StringArray ids;
    for (const auto& pair : presets_)
        ids.add(pair.first);
    return ids;
}

bool InstrumentBase::loadPreset(const juce::String& presetId)
{
    auto it = presets_.find(presetId);
    if (it == presets_.end())
        return false;

    const auto& preset = it->second;

    // Apply all parameter values from preset
    for (const auto& pair : preset.parameterValues)
    {
        setParameter(pair.first, pair.second);
    }

    currentPresetId_ = presetId;
    return true;
}

bool InstrumentBase::applyPreset(const ZenithInstrumentPreset& preset)
{
    // Verify preset is for this instrument
    juce::String presetInstrumentId = preset.instrumentId;
    juce::String thisInstrumentId = metadata_.id;

    if (presetInstrumentId != thisInstrumentId)
        return false;

    // Apply all parameter values from preset
    for (const auto& [paramId, value] : preset.parameters)
    {
        juce::String paramIdStr = paramId;
        setParameter(paramIdStr, value);
    }

    // Apply macro values if present
    for (const auto& [macroId, value] : preset.macros)
    {
        juce::String macroIdStr = macroId;
        setMacro(macroIdStr, value);
    }

    // Update current preset ID
    currentPresetId_ = preset.id;

    return true;
}

void InstrumentBase::capturePreset(ZenithInstrumentPreset& preset) const
{
    // Set basic info
    preset.instrumentId = metadata_.id.toStdString();

    // Capture all parameter values
    preset.parameters.clear();
    for (const auto& paramMetadata : metadata_.parameters)
    {
        float value = getParameter(paramMetadata.id);
        preset.parameters[paramMetadata.id.toStdString()] = value;
    }

    // Capture all macro values
    preset.macros.clear();
    for (const auto& macroMetadata : metadata_.macros)
    {
        float value = getMacro(macroMetadata.id);
        preset.macros[macroMetadata.id.toStdString()] = value;
    }
}

//==============================================================================
void InstrumentBase::registerPreset(const juce::String& presetId,
                                   const juce::String& presetName,
                                   const std::map<juce::String, float>& parameterValues)
{
    Preset preset;
    preset.id = presetId;
    preset.name = presetName;
    preset.parameterValues = parameterValues;

    presets_[presetId] = preset;
}

void InstrumentBase::mapParameter(const juce::String& parameterId, int juceParameterIndex)
{
    parameterIndexMap_[parameterId] = juceParameterIndex;
}

} // namespace zenith
