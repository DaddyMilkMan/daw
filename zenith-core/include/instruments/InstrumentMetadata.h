/**
 * @file InstrumentMetadata.h
 * @brief Shared metadata schema for built-in instruments
 *
 * Provides a data-driven schema for:
 * - Parameter metadata (id, name, type, range, category)
 * - Macro definitions (targets, amounts, descriptions)
 * - Instrument metadata (aggregates parameters and macros)
 *
 * This schema is designed to be:
 * - Easy for AI and CommandAPI to reason about
 * - Audio-thread friendly (precomputed, no dynamic allocations)
 * - Extensible for future modulation features
 */

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <string>
#include <map>

namespace zenith {

//==============================================================================
/**
 * @brief Parameter type enumeration
 */
enum class ParameterType
{
    Float,      ///< Continuous float parameter
    Bool,       ///< Boolean on/off parameter
    Enum        ///< Discrete enumeration parameter
};

//==============================================================================
/**
 * @brief Parameter category for UI organization
 */
enum class ParameterCategory
{
    Oscillator,     ///< Oscillator controls
    Filter,         ///< Filter controls
    Envelope,       ///< Envelope controls
    Modulation,     ///< Modulation controls
    Effect,         ///< Effect controls
    Global,         ///< Global/master controls
    Sample,         ///< Sample-specific controls (sampler)
    Voice           ///< Voice/polyphony controls
};

//==============================================================================
/**
 * @brief Complete metadata for a single instrument parameter
 */
struct InstrumentParameterInfo
{
    std::string id;                 ///< Stable parameter ID (e.g., "osc1_wave")
    std::string name;               ///< Human-readable name (e.g., "Osc 1 Waveform")
    ParameterType type;             ///< Parameter type
    ParameterCategory category;     ///< UI category

    // Range information (for Float type)
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.5f;
    float stepSize = 0.01f;         ///< 0 = continuous

    // Enum information (for Enum type)
    std::vector<std::string> enumChoices;  ///< e.g., {"Sine", "Saw", "Square"}

    // Optional metadata
    std::string units;              ///< e.g., "Hz", "dB", "%"
    bool isAutomatable = true;      ///< Can be automated from DAW

    InstrumentParameterInfo() = default;

    InstrumentParameterInfo(
        const std::string& id_,
        const std::string& name_,
        ParameterType type_,
        ParameterCategory category_,
        float min_ = 0.0f,
        float max_ = 1.0f,
        float default_ = 0.5f,
        const std::string& units_ = ""
    ) : id(id_), name(name_), type(type_), category(category_),
        minValue(min_), maxValue(max_), defaultValue(default_), units(units_)
    {}
};

//==============================================================================
/**
 * @brief Macro target: maps macro to parameter with amount scalar
 */
struct InstrumentMacroTarget
{
    std::string parameterId;        ///< Target parameter ID
    float amount = 0.0f;            ///< Amount scalar (-1.0 to +1.0)
                                    ///< Positive = increase when macro increases
                                    ///< Negative = decrease when macro increases

    InstrumentMacroTarget() = default;

    InstrumentMacroTarget(const std::string& paramId, float amt)
        : parameterId(paramId), amount(amt)
    {}
};

//==============================================================================
/**
 * @brief Complete metadata for a macro control
 */
struct InstrumentMacroInfo
{
    std::string id;                 ///< Stable macro ID (e.g., "macro_warmth")
    std::string name;               ///< Human-readable name (e.g., "Warmth")
    std::string description;        ///< Detailed description
    std::vector<InstrumentMacroTarget> targets;  ///< Parameter targets

    InstrumentMacroInfo() = default;

    InstrumentMacroInfo(
        const std::string& id_,
        const std::string& name_,
        const std::string& description_
    ) : id(id_), name(name_), description(description_)
    {}

    void addTarget(const std::string& paramId, float amount)
    {
        targets.push_back(InstrumentMacroTarget(paramId, amount));
    }
};

//==============================================================================
/**
 * @brief Complete metadata for an instrument
 */
struct InstrumentMetadata
{
    std::string instrumentId;       ///< Unique instrument ID (e.g., "zenith_poly_synth")
    std::string name;               ///< Human-readable name (e.g., "Zenith PolySynth")
    std::string description;        ///< Detailed description
    std::string version;            ///< Version string (e.g., "1.0.0")

    std::vector<InstrumentParameterInfo> parameters;  ///< All parameters
    std::vector<InstrumentMacroInfo> macros;          ///< All macros

    InstrumentMetadata() = default;

    InstrumentMetadata(
        const std::string& id,
        const std::string& name_,
        const std::string& description_ = "",
        const std::string& version_ = "1.0.0"
    ) : instrumentId(id), name(name_), description(description_), version(version_)
    {}

    /**
     * @brief Add a parameter to the metadata
     */
    void addParameter(const InstrumentParameterInfo& param)
    {
        parameters.push_back(param);
    }

    /**
     * @brief Add a macro to the metadata
     */
    void addMacro(const InstrumentMacroInfo& macro)
    {
        macros.push_back(macro);
    }

    /**
     * @brief Find parameter by ID
     * @return Pointer to parameter info, or nullptr if not found
     */
    const InstrumentParameterInfo* findParameter(const std::string& paramId) const
    {
        for (const auto& param : parameters)
        {
            if (param.id == paramId)
                return &param;
        }
        return nullptr;
    }

    /**
     * @brief Find macro by ID
     * @return Pointer to macro info, or nullptr if not found
     */
    const InstrumentMacroInfo* findMacro(const std::string& macroId) const
    {
        for (const auto& macro : macros)
        {
            if (macro.id == macroId)
                return &macro;
        }
        return nullptr;
    }
};

//==============================================================================
/**
 * @brief Macro engine for audio-thread-friendly macro application
 *
 * This engine:
 * - Stores macro values [0..1]
 * - Precomputes effective parameter contributions
 * - Is audio-thread safe (no allocations)
 */
class MacroEngine
{
public:
    MacroEngine() = default;

    /**
     * @brief Initialize with instrument metadata
     * @param metadata Instrument metadata containing macro definitions
     * @note Must be called before use, from message thread
     */
    void initialize(const InstrumentMetadata& metadata)
    {
        metadata_ = &metadata;

        // Allocate storage for macro values
        macroValues_.resize(metadata.macros.size(), 0.5f);

        // Build lookup map for fast macro value access
        macroIdToIndex_.clear();
        for (size_t i = 0; i < metadata.macros.size(); ++i)
        {
            macroIdToIndex_[metadata.macros[i].id] = i;
        }
    }

    /**
     * @brief Set macro value
     * @param macroId Macro ID
     * @param value Normalized value [0..1]
     * @note Thread-safe
     */
    void setMacroValue(const std::string& macroId, float value)
    {
        auto it = macroIdToIndex_.find(macroId);
        if (it != macroIdToIndex_.end())
        {
            macroValues_[it->second] = juce::jlimit(0.0f, 1.0f, value);
        }
    }

    /**
     * @brief Get macro value
     * @param macroId Macro ID
     * @return Normalized value [0..1], or 0.5 if not found
     * @note Thread-safe
     */
    float getMacroValue(const std::string& macroId) const
    {
        auto it = macroIdToIndex_.find(macroId);
        if (it != macroIdToIndex_.end())
        {
            return macroValues_[it->second];
        }
        return 0.5f;  // Default centered value
    }

    /**
     * @brief Compute macro contribution to a parameter
     * @param parameterId Parameter ID
     * @param baseValue Base parameter value (before macro application)
     * @param paramRange Parameter range (max - min)
     * @return Contribution to add to base value
     * @note Audio-thread safe (no allocations)
     */
    float computeMacroContribution(const std::string& parameterId,
                                   float baseValue,
                                   float paramRange) const
    {
        if (!metadata_)
            return 0.0f;

        float contribution = 0.0f;

        // Iterate through all macros and their targets
        for (size_t macroIdx = 0; macroIdx < metadata_->macros.size(); ++macroIdx)
        {
            const auto& macro = metadata_->macros[macroIdx];
            float macroValue = macroValues_[macroIdx];

            // Check if this macro targets our parameter
            for (const auto& target : macro.targets)
            {
                if (target.parameterId == parameterId)
                {
                    // Macro value is [0..1], centered at 0.5
                    // Convert to [-1..+1] range
                    float normalizedMacro = (macroValue - 0.5f) * 2.0f;

                    // Contribution = macro value * amount * parameter range
                    contribution += normalizedMacro * target.amount * paramRange;
                }
            }
        }

        return contribution;
    }

    /**
     * @brief Get number of macros
     */
    size_t getNumMacros() const
    {
        return macroValues_.size();
    }

    /**
     * @brief Get macro value by index
     */
    float getMacroValue(size_t index) const
    {
        if (index < macroValues_.size())
            return macroValues_[index];
        return 0.5f;
    }

    /**
     * @brief Set macro value by index
     */
    void setMacroValue(size_t index, float value)
    {
        if (index < macroValues_.size())
            macroValues_[index] = juce::jlimit(0.0f, 1.0f, value);
    }

private:
    const InstrumentMetadata* metadata_ = nullptr;
    std::vector<float> macroValues_;                    ///< Macro values [0..1]
    std::map<std::string, size_t> macroIdToIndex_;      ///< Fast macro ID lookup

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroEngine)
};

} // namespace zenith
