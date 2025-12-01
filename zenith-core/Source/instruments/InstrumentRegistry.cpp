/*
  ==============================================================================

    InstrumentRegistry.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Implementation of InstrumentRegistry.

  ==============================================================================
*/

#include "InstrumentRegistry.h"

namespace zenith {

//==============================================================================
InstrumentRegistry::InstrumentRegistry()
{
    DBG("InstrumentRegistry: Initialized");
}

InstrumentRegistry& InstrumentRegistry::getInstance()
{
    static InstrumentRegistry instance;
    return instance;
}

//==============================================================================
juce::StringArray InstrumentRegistry::getInstrumentIds() const
{
    juce::StringArray ids;
    for (const auto& pair : instruments_)
        ids.add(pair.first);
    return ids;
}

bool InstrumentRegistry::getMetadata(const juce::String& instrumentId, InstrumentMetadata& outMetadata) const
{
    auto it = instruments_.find(instrumentId);
    if (it != instruments_.end())
    {
        outMetadata = it->second.metadata;
        return true;
    }
    return false;
}

juce::Array<juce::var> InstrumentRegistry::getInstrumentList() const
{
    juce::Array<juce::var> result;

    for (const auto& pair : instruments_)
    {
        const auto& metadata = pair.second.metadata;

        // Wrap DynamicObject in var immediately for proper memory management
        juce::var obj(new juce::DynamicObject());
        obj.getDynamicObject()->setProperty("id", metadata.instrumentId);
        obj.getDynamicObject()->setProperty("name", metadata.name);
        obj.getDynamicObject()->setProperty("category", metadata.category);

        result.add(obj);
    }

    return result;
}

//==============================================================================
std::unique_ptr<Instrument> InstrumentRegistry::createInstrument(const juce::String& instrumentId) const
{
    auto it = instruments_.find(instrumentId);
    if (it != instruments_.end())
        return it->second.factory();

    DBG("InstrumentRegistry: Unknown instrument ID: " + instrumentId);
    return nullptr;
}

//==============================================================================
void InstrumentRegistry::registerInstrument(const juce::String& instrumentId,
                                           InstrumentMetadata metadata,
                                           InstrumentFactory factory)
{
    InstrumentInfo info;
    info.metadata = std::move(metadata);
    info.factory = std::move(factory);

    instruments_[instrumentId] = std::move(info);

    DBG("InstrumentRegistry: Registered instrument '" + instrumentId + "'");
}

//==============================================================================
std::vector<InstrumentPreset> InstrumentRegistry::getPresetsForInstrument(const juce::String& instrumentId) const
{
    std::vector<InstrumentPreset> result;
    auto it = presets_.find(instrumentId);
    if (it != presets_.end())
    {
        for (const auto& pair : it->second)
            result.push_back(pair.second);
    }
    return result;
}

bool InstrumentRegistry::getPreset(const juce::String& instrumentId, const juce::String& presetName, InstrumentPreset& outPreset) const
{
    auto it = presets_.find(instrumentId);
    if (it != presets_.end())
    {
        auto presetIt = it->second.find(presetName);
        if (presetIt != it->second.end())
        {
            outPreset = presetIt->second;
            return true;
        }
    }
    return false;
}

void InstrumentRegistry::addPreset(const juce::String& instrumentId, const InstrumentPreset& preset)
{
    presets_[instrumentId][preset.name] = preset;
}

bool InstrumentRegistry::deletePreset(const juce::String& instrumentId, const juce::String& presetName)
{
    auto it = presets_.find(instrumentId);
    if (it != presets_.end())
    {
        return it->second.erase(presetName) > 0;
    }
    return false;
}

juce::var InstrumentRegistry::getParameterSchema(const juce::String& instrumentId) const
{
    auto it = instruments_.find(instrumentId);
    if (it != instruments_.end())
    {
        return it->second.metadata.toVar();
    }
    return juce::var();
}

} // namespace zenith

