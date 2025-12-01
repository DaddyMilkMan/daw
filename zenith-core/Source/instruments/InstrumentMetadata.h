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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <string>
#include <vector>


namespace zenith {

//==============================================================================
/**
    Parameter metadata for an instrument parameter
*/
struct ParameterMetadata {
  juce::String id;       // Stable ID (e.g., "filter_cutoff")
  juce::String name;     // Human-readable name (e.g., "Filter Cutoff")
  juce::String category; // Category for grouping (e.g., "Filter", "Envelope")

  enum class Type {
    Float, // Continuous value
    Bool,  // On/Off
    Choice // Discrete choices
  };

  Type type = Type::Float;

  float defaultValue = 0.5f; // Default normalized value (0-1)
  float minValue = 0.0f;     // Minimum value
  float maxValue = 1.0f;     // Maximum value

  juce::String units; // Display units (e.g., "Hz", "dB", "%")

  // For Choice parameters
  juce::StringArray choices;

  // Extended metadata for AI preset design
  juce::String group; // Functional group (e.g., "Oscillator", "Filter",
                      // "Envelope", "LFO", "FX")
  juce::String role;  // Semantic role (e.g., "tone", "mod", "time", "level",
                      // "stereo", "distortion")
  float recommendedStep = 0.01f; // Recommended step size for UI/automation

  // Safe range for randomization (optional, defaults to full range)
  float minSafeRange = -1.0f; // -1 means use minValue
  float maxSafeRange = -1.0f; // -1 means use maxValue

  /**
   * @brief Convert to JSON var for CommandAPI
   */
  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("type", typeToString(type));
    obj->setProperty("defaultValue", defaultValue);
    obj->setProperty("minValue", minValue);
    obj->setProperty("maxValue", maxValue);
    obj->setProperty("units", units);

    if (type == Type::Choice && !choices.isEmpty()) {
      juce::Array<juce::var> choicesArray;
      for (const auto &choice : choices)
        choicesArray.add(choice);
      obj->setProperty("choices", choicesArray);
    }

    // Extended metadata for AI preset design
    if (group.isNotEmpty())
      obj->setProperty("group", group);
    if (role.isNotEmpty())
      obj->setProperty("role", role);
    obj->setProperty("recommendedStep", recommendedStep);

    // Safe range for randomization (only include if specified)
    if (minSafeRange >= 0.0f || maxSafeRange >= 0.0f) {
      auto *safeRange = new juce::DynamicObject();
      safeRange->setProperty("min",
                             minSafeRange >= 0.0f ? minSafeRange : minValue);
      safeRange->setProperty("max",
                             maxSafeRange >= 0.0f ? maxSafeRange : maxValue);
      obj->setProperty("safeRangeForRandomisation", juce::var(safeRange));
    }

    return juce::var(obj);
  }

private:
  static juce::String typeToString(Type t) {
    switch (t) {
    case Type::Float:
      return "float";
    case Type::Bool:
      return "bool";
    case Type::Choice:
      return "choice";
    default:
      return "float";
    }
  }
};

//==============================================================================
/**
    Macro target - defines which parameter a macro affects and by how much
*/
struct MacroTarget {
  juce::String parameterId; // Parameter this macro affects
  float amount = 1.0f;      // Amount of influence (0-1)

  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("parameterId", parameterId);
    obj->setProperty("amount", amount);
    return juce::var(obj);
  }
};

//==============================================================================
/**
    Macro metadata - high-level control that affects multiple parameters
*/
struct MacroMetadata {
  juce::String id;          // Stable ID (e.g., "macro_warmth")
  juce::String name;        // Human-readable name (e.g., "Warmth")
  juce::String description; // What this macro does

  std::vector<MacroTarget> targets; // Parameters affected by this macro

  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("description", description);

    juce::Array<juce::var> targetsArray;
    for (const auto &target : targets)
      targetsArray.add(target.toVar());
    obj->setProperty("targets", targetsArray);

    return juce::var(obj);
  }
};

//==============================================================================
/**
    Instrument Preset Structure
*/
struct InstrumentPreset {
    juce::String name;
    juce::String category;
    juce::String description;
    juce::String author;
    juce::String tags;
    juce::var parameters; // JSON object of parameter values

    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        obj->setProperty("category", category);
        obj->setProperty("description", description);
        obj->setProperty("author", author);
        obj->setProperty("tags", tags);
        obj->setProperty("parameters", parameters);
        return juce::var(obj);
    }
    
    static InstrumentPreset fromVar(const juce::var& v) {
        InstrumentPreset p;
        p.name = v.getProperty("name", "Unnamed").toString();
        p.category = v.getProperty("category", "User").toString();
        p.description = v.getProperty("description", "").toString();
        p.author = v.getProperty("author", "").toString();
        p.tags = v.getProperty("tags", "").toString();
        p.parameters = v.getProperty("parameters", juce::var(new juce::DynamicObject()));
        return p;
    }
};

//==============================================================================
/**
    Complete instrument metadata
*/
struct InstrumentMetadata {
  juce::String instrumentId; // Stable ID (e.g., "zenith_poly_synth")
  juce::String name;         // Human-readable name
  juce::String category;     // Category (e.g., "Synth", "Sampler")
  juce::String description;  // Brief description
  juce::StringArray tags;    // Search tags (e.g., "drums", "808", "vintage")

  std::vector<ParameterMetadata> parameters;
  std::vector<MacroMetadata> macros;

  // Default preset categories for this instrument
  // Used by ZenithPresetManager to organize presets
  // Examples: "Bass", "Lead", "Pad", "Keys", "FX", "808"
  juce::StringArray defaultPresetCategories;

  /**
   * @brief Find parameter by ID
   */
  const ParameterMetadata *findParameter(const juce::String &paramId) const {
    for (const auto &param : parameters)
      if (param.id == paramId)
        return &param;
    return nullptr;
  }

  /**
   * @brief Find macro by ID
   */
  const MacroMetadata *findMacro(const juce::String &macroId) const {
    for (const auto &macro : macros)
      if (macro.id == macroId)
        return &macro;
    return nullptr;
  }

  /**
   * @brief Convert to JSON var for CommandAPI
   */
  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("instrumentId", instrumentId);
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("description", description);

    // Add tags array
    if (!tags.isEmpty()) {
      juce::Array<juce::var> tagsArray;
      for (const auto &tag : tags)
        tagsArray.add(tag);
      obj->setProperty("tags", tagsArray);
    }

    juce::Array<juce::var> paramsArray;
    for (const auto &param : parameters)
      paramsArray.add(param.toVar());
    obj->setProperty("parameters", paramsArray);

    juce::Array<juce::var> macrosArray;
    for (const auto &macro : macros)
      macrosArray.add(macro.toVar());
    obj->setProperty("macros", macrosArray);

    // Default preset categories
    if (!defaultPresetCategories.isEmpty()) {
      juce::Array<juce::var> categoriesArray;
      for (const auto &cat : defaultPresetCategories)
        categoriesArray.add(cat);
      obj->setProperty("defaultPresetCategories", categoriesArray);
    }

    return juce::var(obj);
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
class MacroEngine {
public:
  MacroEngine() = default;

  /**
   * @brief Initialize with instrument metadata
   * @param metadata Instrument metadata containing macro definitions
   * @note Must be called before use, from message thread
   */
  void initialize(const InstrumentMetadata &metadata) {
    metadata_ = &metadata;

    // Allocate storage for macro values
    macroValues_.resize(metadata.macros.size(), 0.5f);

    // Build lookup map for fast macro value access
    macroIdToIndex_.clear();
    for (size_t i = 0; i < metadata.macros.size(); ++i) {
      macroIdToIndex_[metadata.macros[i].id] = (int)i;
    }
  }

  /**
   * @brief Set macro value
   * @param macroId Macro ID
   * @param value Normalized value [0..1]
   * @note Thread-safe
   */
  void setMacroValue(const juce::String &macroId, float value) {
    auto it = macroIdToIndex_.find(macroId);
    if (it != macroIdToIndex_.end()) {
      macroValues_[it->second] = juce::jlimit(0.0f, 1.0f, value);
    }
  }

  /**
   * @brief Get macro value
   * @param macroId Macro ID
   * @return Normalized value [0..1], or 0.5 if not found
   * @note Thread-safe
   */
  float getMacroValue(const juce::String &macroId) const {
    auto it = macroIdToIndex_.find(macroId);
    if (it != macroIdToIndex_.end()) {
      return macroValues_[it->second];
    }
    return 0.5f; // Default centered value
  }

  /**
   * @brief Compute macro contribution to a parameter
   * @param parameterId Parameter ID
   * @param baseValue Base parameter value (before macro application)
   * @param paramRange Parameter range (max - min)
   * @return Contribution to add to base value
   * @note Audio-thread safe (no allocations)
   */
  float computeMacroContribution(const juce::String &parameterId,
                                 float baseValue, float paramRange) const {
    if (!metadata_)
      return 0.0f;

    float contribution = 0.0f;

    // Iterate through all macros and their targets
    for (size_t macroIdx = 0; macroIdx < metadata_->macros.size(); ++macroIdx) {
      const auto &macro = metadata_->macros[macroIdx];
      float macroValue = macroValues_[macroIdx];

      // Check if this macro targets our parameter
      for (const auto &target : macro.targets) {
        if (target.parameterId == parameterId) {
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
  size_t getNumMacros() const { return macroValues_.size(); }

  /**
   * @brief Get macro value by index
   */
  float getMacroValue(size_t index) const {
    if (index < macroValues_.size())
      return macroValues_[index];
    return 0.5f;
  }

  /**
   * @brief Set macro value by index
   */
  void setMacroValue(size_t index, float value) {
    if (index < macroValues_.size())
      macroValues_[index] = juce::jlimit(0.0f, 1.0f, value);
  }

private:
  const InstrumentMetadata *metadata_ = nullptr;
  std::vector<float> macroValues_;             ///< Macro values [0..1]
  std::map<juce::String, int> macroIdToIndex_; ///< Fast macro ID lookup

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroEngine)
};

} // namespace zenith
