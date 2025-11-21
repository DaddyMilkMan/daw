/*
  ==============================================================================

    InstrumentRegistry.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Registry for all built-in instruments.
    Allows enumeration and creation of instruments by ID.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Instrument.h"
#include "InstrumentMetadata.h"
#include <memory>
#include <map>
#include <functional>

namespace zenith {

//==============================================================================
/**
    Registry for built-in instruments

    Singleton that manages all available instruments:
    - Registers instrument factories
    - Enumerates available instruments
    - Creates instrument instances by ID
*/
class InstrumentRegistry
{
public:
    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static InstrumentRegistry& getInstance();

    //==========================================================================
    // Instrument Enumeration
    //==========================================================================

    /**
     * @brief Get list of all available instrument IDs
     */
    juce::StringArray getInstrumentIds() const;

    /**
     * @brief Get metadata for a specific instrument
     * @param instrumentId Instrument identifier
     * @param outMetadata Output parameter to receive metadata
     * @return true if found, false otherwise
     */
    bool getMetadata(const juce::String& instrumentId, InstrumentMetadata& outMetadata) const;

    /**
     * @brief Get basic info for all instruments (for list_instruments command)
     * @return Array of {id, name, category} objects
     */
    juce::Array<juce::var> getInstrumentList() const;

    //==========================================================================
    // Instrument Creation
    //==========================================================================

    /**
     * @brief Create an instrument instance by ID
     * @param instrumentId Instrument identifier
     * @return New instrument instance, or nullptr if not found
     */
    std::unique_ptr<Instrument> createInstrument(const juce::String& instrumentId) const;

    //==========================================================================
    // Registration (called during initialization)
    //==========================================================================

    using InstrumentFactory = std::function<std::unique_ptr<Instrument>()>;

    /**
     * @brief Register an instrument type
     * @param instrumentId Unique instrument ID
     * @param metadata Instrument metadata
     * @param factory Factory function to create instances
     */
    void registerInstrument(const juce::String& instrumentId,
                           InstrumentMetadata metadata,
                           InstrumentFactory factory);

private:
    InstrumentRegistry();
    ~InstrumentRegistry() = default;

    struct InstrumentInfo
    {
        InstrumentMetadata metadata;
        InstrumentFactory factory;
    };

    std::map<juce::String, InstrumentInfo> instruments_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentRegistry)
};

} // namespace zenith

