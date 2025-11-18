/*
  ==============================================================================

    Instrument.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Base class for all built-in instruments.
    Wraps a JUCE AudioProcessor and provides metadata, presets, and parameter access.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "InstrumentMetadata.h"
#include <memory>
#include <map>

namespace zenith {

//==============================================================================
/**
    Base class for all built-in instruments

    Instruments are AudioProcessors with additional metadata and preset support.
    They expose parameters and macros that can be controlled via CommandAPI.
*/
class Instrument
{
public:
    virtual ~Instrument() = default;

    //==========================================================================
    // Metadata Access
    //==========================================================================

    /**
     * @brief Get instrument metadata (id, name, parameters, macros)
     */
    virtual const InstrumentMetadata& getMetadata() const = 0;

    /**
     * @brief Get the wrapped AudioProcessor for audio processing
     */
    virtual juce::AudioProcessor* getAudioProcessor() = 0;

    //==========================================================================
    // Parameter Control
    //==========================================================================

    /**
     * @brief Set a parameter by ID
     * @param parameterId Stable parameter ID from metadata
     * @param normalizedValue Value in range [0, 1]
     * @return true if parameter was found and set
     */
    virtual bool setParameter(const juce::String& parameterId, float normalizedValue) = 0;

    /**
     * @brief Get a parameter value by ID
     * @param parameterId Stable parameter ID from metadata
     * @return Normalized value [0, 1], or 0 if not found
     */
    virtual float getParameter(const juce::String& parameterId) const = 0;

    //==========================================================================
    // Macro Control
    //==========================================================================

    /**
     * @brief Set a macro by ID (affects multiple parameters)
     * @param macroId Stable macro ID from metadata
     * @param normalizedValue Value in range [0, 1]
     * @return true if macro was found and applied
     */
    virtual bool setMacro(const juce::String& macroId, float normalizedValue) = 0;

    /**
     * @brief Get a macro value by ID
     * @param macroId Stable macro ID from metadata
     * @return Normalized value [0, 1], or 0 if not found
     */
    virtual float getMacro(const juce::String& macroId) const = 0;

    //==========================================================================
    // Preset Management
    //==========================================================================

    /**
     * @brief Get list of available preset IDs
     */
    virtual juce::StringArray getPresetIds() const = 0;

    /**
     * @brief Load a preset by ID
     * @param presetId Preset identifier
     * @return true if preset was found and loaded
     */
    virtual bool loadPreset(const juce::String& presetId) = 0;

    /**
     * @brief Get current preset ID (or empty if no preset loaded)
     */
    virtual juce::String getCurrentPresetId() const = 0;

protected:
    Instrument() = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Instrument)
};

//==============================================================================
/**
    Helper base class for implementing instruments

    Provides default implementations for parameter/macro mapping and preset management.
*/
class InstrumentBase : public Instrument
{
public:
    InstrumentBase(std::unique_ptr<juce::AudioProcessor> processor,
                   InstrumentMetadata metadata);

    ~InstrumentBase() override = default;

    //==========================================================================
    // Implement Instrument interface
    //==========================================================================

    const InstrumentMetadata& getMetadata() const override { return metadata_; }
    juce::AudioProcessor* getAudioProcessor() override { return processor_.get(); }

    bool setParameter(const juce::String& parameterId, float normalizedValue) override;
    float getParameter(const juce::String& parameterId) const override;

    bool setMacro(const juce::String& macroId, float normalizedValue) override;
    float getMacro(const juce::String& macroId) const override;

    juce::StringArray getPresetIds() const override;
    bool loadPreset(const juce::String& presetId) override;
    juce::String getCurrentPresetId() const override { return currentPresetId_; }

protected:
    //==========================================================================
    // Preset Registration (called by derived classes)
    //==========================================================================

    /**
     * @brief Register a preset
     * @param presetId Unique preset ID
     * @param presetName Human-readable name
     * @param parameterValues Map of parameterId -> normalized value
     */
    void registerPreset(const juce::String& presetId,
                       const juce::String& presetName,
                       const std::map<juce::String, float>& parameterValues);

    //==========================================================================
    // Parameter Index Mapping (called by derived classes during construction)
    //==========================================================================

    /**
     * @brief Map a parameter ID to a JUCE parameter index
     * @param parameterId Stable parameter ID from metadata
     * @param juceParameterIndex Index in AudioProcessor::getParameters()
     */
    void mapParameter(const juce::String& parameterId, int juceParameterIndex);

private:
    std::unique_ptr<juce::AudioProcessor> processor_;
    InstrumentMetadata metadata_;

    // Parameter ID -> JUCE parameter index mapping
    std::map<juce::String, int> parameterIndexMap_;

    // Macro ID -> current value
    std::map<juce::String, float> macroValues_;

    // Preset storage
    struct Preset
    {
        juce::String id;
        juce::String name;
        std::map<juce::String, float> parameterValues;
    };
    std::map<juce::String, Preset> presets_;
    juce::String currentPresetId_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentBase)
};

} // namespace zenith
