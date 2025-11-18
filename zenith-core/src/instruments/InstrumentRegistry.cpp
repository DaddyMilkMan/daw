#include "instruments/InstrumentRegistry.h"

namespace zenith {

InstrumentRegistry& InstrumentRegistry::getInstance()
{
    static InstrumentRegistry instance;
    return instance;
}

void InstrumentRegistry::registerInstrument(const InstrumentDefinition& definition)
{
    if (definition.id.isEmpty())
    {
        jassertfalse; // Invalid instrument ID
        return;
    }

    if (!definition.factory)
    {
        jassertfalse; // No factory function provided
        return;
    }

    instruments_[definition.id] = definition;
}

std::unique_ptr<juce::AudioProcessor> InstrumentRegistry::createInstrument(const juce::String& instrumentId) const
{
    auto it = instruments_.find(instrumentId);
    if (it == instruments_.end())
        return nullptr;

    return it->second.factory();
}

const InstrumentDefinition* InstrumentRegistry::getDefinition(const juce::String& instrumentId) const
{
    auto it = instruments_.find(instrumentId);
    if (it == instruments_.end())
        return nullptr;

    return &it->second;
}

std::vector<InstrumentDefinition> InstrumentRegistry::getAllDefinitions() const
{
    std::vector<InstrumentDefinition> result;
    result.reserve(instruments_.size());

    for (const auto& pair : instruments_)
        result.push_back(pair.second);

    return result;
}

std::vector<juce::String> InstrumentRegistry::getInstrumentsByCategory(const juce::String& category) const
{
    std::vector<juce::String> result;

    for (const auto& pair : instruments_)
    {
        if (pair.second.category == category)
            result.push_back(pair.first);
    }

    return result;
}

bool InstrumentRegistry::hasInstrument(const juce::String& instrumentId) const
{
    return instruments_.find(instrumentId) != instruments_.end();
}

void InstrumentRegistry::clear()
{
    instruments_.clear();
}

} // namespace zenith
