/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    InstrumentRegistry.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Registry for all built-in instruments.
    Allows enumeration and creation of instruments by ID.


  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
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
    InstrumentRegistry();
    ~InstrumentRegistry() = default;

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
    // Preset Management
    //==========================================================================

    std::vector<InstrumentPreset> getPresetsForInstrument(const juce::String& instrumentId) const;
    bool getPreset(const juce::String& instrumentId, const juce::String& presetName, InstrumentPreset& outPreset) const;
    void addPreset(const juce::String& instrumentId, const InstrumentPreset& preset);
    bool deletePreset(const juce::String& instrumentId, const juce::String& presetName);
    juce::var getParameterSchema(const juce::String& instrumentId) const;

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
    struct InstrumentInfo
    {
        InstrumentMetadata metadata;
        InstrumentFactory factory;
    };

    std::map<juce::String, InstrumentInfo> instruments_;
    // Map<InstrumentID, Map<PresetName, Preset>>
    std::map<juce::String, std::map<juce::String, InstrumentPreset>> presets_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentRegistry)
};

} // namespace zenith

