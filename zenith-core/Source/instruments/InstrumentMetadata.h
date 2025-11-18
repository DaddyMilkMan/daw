/*
  ==============================================================================

    InstrumentMetadata.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Metadata structures for instruments, parameters, and macros.
    Used by InstrumentRegistry and CommandAPI to expose instrument capabilities.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <string>

namespace zenith {

//==============================================================================
/**
    Parameter metadata for an instrument parameter
*/
struct ParameterMetadata
{
    juce::String id;              // Stable ID (e.g., "filter_cutoff")
    juce::String name;            // Human-readable name (e.g., "Filter Cutoff")
    juce::String category;        // Category for grouping (e.g., "Filter", "Envelope")

    enum class Type
    {
        Float,                    // Continuous value
        Bool,                     // On/Off
        Choice                    // Discrete choices
    };

    Type type = Type::Float;

    float defaultValue = 0.5f;    // Default normalized value (0-1)
    float minValue = 0.0f;        // Minimum value
    float maxValue = 1.0f;        // Maximum value

    juce::String units;           // Display units (e.g., "Hz", "dB", "%")

    // For Choice parameters
    juce::StringArray choices;

    /**
     * @brief Convert to JSON var for CommandAPI
     */
    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", id);
        obj->setProperty("name", name);
        obj->setProperty("category", category);
        obj->setProperty("type", typeToString(type));
        obj->setProperty("defaultValue", defaultValue);
        obj->setProperty("minValue", minValue);
        obj->setProperty("maxValue", maxValue);
        obj->setProperty("units", units);

        if (type == Type::Choice && !choices.isEmpty())
        {
            juce::Array<juce::var> choicesArray;
            for (const auto& choice : choices)
                choicesArray.add(choice);
            obj->setProperty("choices", choicesArray);
        }

        return juce::var(obj);
    }

private:
    static juce::String typeToString(Type t)
    {
        switch (t)
        {
            case Type::Float:  return "float";
            case Type::Bool:   return "bool";
            case Type::Choice: return "choice";
            default:           return "float";
        }
    }
};

//==============================================================================
/**
    Macro target - defines which parameter a macro affects and by how much
*/
struct MacroTarget
{
    juce::String parameterId;     // Parameter this macro affects
    float amount = 1.0f;          // Amount of influence (0-1)

    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("parameterId", parameterId);
        obj->setProperty("amount", amount);
        return juce::var(obj);
    }
};

//==============================================================================
/**
    Macro metadata - high-level control that affects multiple parameters
*/
struct MacroMetadata
{
    juce::String id;              // Stable ID (e.g., "macro_warmth")
    juce::String name;            // Human-readable name (e.g., "Warmth")
    juce::String description;     // What this macro does

    std::vector<MacroTarget> targets;  // Parameters affected by this macro

    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", id);
        obj->setProperty("name", name);
        obj->setProperty("description", description);

        juce::Array<juce::var> targetsArray;
        for (const auto& target : targets)
            targetsArray.add(target.toVar());
        obj->setProperty("targets", targetsArray);

        return juce::var(obj);
    }
};

//==============================================================================
/**
    Complete instrument metadata
*/
struct InstrumentMetadata
{
    juce::String instrumentId;    // Stable ID (e.g., "zenith_poly_synth")
    juce::String name;            // Human-readable name
    juce::String category;        // Category (e.g., "Synth", "Sampler")
    juce::String description;     // Brief description

    std::vector<ParameterMetadata> parameters;
    std::vector<MacroMetadata> macros;

    /**
     * @brief Find parameter by ID
     */
    const ParameterMetadata* findParameter(const juce::String& paramId) const
    {
        for (const auto& param : parameters)
            if (param.id == paramId)
                return &param;
        return nullptr;
    }

    /**
     * @brief Find macro by ID
     */
    const MacroMetadata* findMacro(const juce::String& macroId) const
    {
        for (const auto& macro : macros)
            if (macro.id == macroId)
                return &macro;
        return nullptr;
    }

    /**
     * @brief Convert to JSON var for CommandAPI
     */
    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("instrumentId", instrumentId);
        obj->setProperty("name", name);
        obj->setProperty("category", category);
        obj->setProperty("description", description);

        juce::Array<juce::var> paramsArray;
        for (const auto& param : parameters)
            paramsArray.add(param.toVar());
        obj->setProperty("parameters", paramsArray);

        juce::Array<juce::var> macrosArray;
        for (const auto& macro : macros)
            macrosArray.add(macro.toVar());
        obj->setProperty("macros", macrosArray);

        return juce::var(obj);
    }
};

} // namespace zenith
