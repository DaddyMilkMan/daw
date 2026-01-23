/**
 * @file InstrumentPreset.h
 * @brief Preset system for built-in instruments
 *
 * Provides:
 * - Preset data structure (parameters + macros)
 * - Preset manager for load/save
 * - JSON/ValueTree serialization
 * - File-based preset storage
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
#include <map>
#include <string>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @brief Preset for a built-in instrument
 *
 * Contains:
 * - Parameter values (map from paramId -> value)
 * - Macro values (map from macroId -> value)
 * - Metadata (name, author, description, tags)
 */
struct ZenithInstrumentPreset {
  // Identity
  std::string id;           ///< Unique preset ID (generated)
  std::string name;         ///< Preset name (e.g., "Warm Pad")
  std::string instrumentId; ///< Target instrument (e.g., "zenith_poly_synth")

  // New taxonomy fields
  std::string soundType;            /// < Sound type (e.g., "Bass", "Pad")
  std::vector<std::string> engines; /// < Engine tags (e.g., "VA", "FM")
  std::vector<std::string>
      characters; /// < Character tags (e.g., "Bright", "Warm")

  // Metadata
  std::string category;    ///< Category (e.g., "Bass", "Lead", "Pad", "808")
  std::string author;      ///< Preset author
  std::string description; ///< Detailed description
  std::vector<std::string>
      tags;            ///< Tags for searching (e.g., "pad", "warm", "lush")
  std::string version; ///< Preset version

  // Parameter values
  std::map<std::string, float> parameters; ///< Parameter ID -> value
  std::map<std::string, float> macros;     ///< Macro ID -> value [0..1]

  ZenithInstrumentPreset() = default;

  /**
   * @brief Create preset with basic info
   */
  ZenithInstrumentPreset(const std::string &name_,
                         const std::string &instrumentId_,
                         const std::string &author_ = "Factory")
      : name(name_), instrumentId(instrumentId_), author(author_) {
    // Generate unique ID from name and timestamp
    id = generateId(name_);
  }

  /**
   * @brief Create preset with existing ID (Optimized)
   * @note The id_ parameter must not be empty. Use the 3-parameter constructor
   *       to generate an ID automatically.
   */
  ZenithInstrumentPreset(const std::string &id_, const std::string &name_,
                         const std::string &instrumentId_,
                         const std::string &author_) noexcept
      : id(id_), name(name_), instrumentId(instrumentId_), author(author_) {
    jassert(!id_.empty() && "Preset ID must not be empty");
  }

  /**
   * @brief Set parameter value
   */
  void setParameter(const std::string &paramId, float value) {
    parameters[paramId] = value;
  }

  /**
   * @brief Set macro value
   */
  void setMacro(const std::string &macroId, float value) {
    macros[macroId] = value;
  }

  /**
   * @brief Get parameter value
   * @return Value, or defaultValue if not found
   */
  float getParameter(const std::string &paramId,
                     float defaultValue = 0.5f) const {
    auto it = parameters.find(paramId);
    return (it != parameters.end()) ? it->second : defaultValue;
  }

  /**
   * @brief Get macro value
   * @return Value [0..1], or 0.5 if not found
   */
  float getMacro(const std::string &macroId, float defaultValue = 0.5f) const {
    auto it = macros.find(macroId);
    return (it != macros.end()) ? it->second : defaultValue;
  }

  /**
   * @brief Convert to ValueTree for serialization
   */
  juce::ValueTree toValueTree() const {
    juce::ValueTree tree("InstrumentPreset");

    // Basic info
    tree.setProperty("id", juce::String(id), nullptr);
    tree.setProperty("name", juce::String(name), nullptr);
    tree.setProperty("instrumentId", juce::String(instrumentId), nullptr);
    tree.setProperty("category", juce::String(category), nullptr);
    tree.setProperty("author", juce::String(author), nullptr);
    tree.setProperty("description", juce::String(description), nullptr);
    // New taxonomy fields
    tree.setProperty("soundType", juce::String(soundType), nullptr);
    if (!engines.empty()) {
      juce::StringArray engArray;
      for (const auto &eng : engines)
        engArray.add(eng);
      tree.setProperty("engines", engArray.joinIntoString(","), nullptr);
    }
    if (!characters.empty()) {
      juce::StringArray charArray;
      for (const auto &ch : characters)
        charArray.add(ch);
      tree.setProperty("characters", charArray.joinIntoString(","), nullptr);
    }

    // Tags
    if (!tags.empty()) {
      juce::StringArray tagArray;
      for (const auto &tag : tags)
        tagArray.add(tag);
      tree.setProperty("tags", tagArray.joinIntoString(","), nullptr);
    }

    // Parameters
    juce::ValueTree paramsTree("Parameters");
    for (const auto &[paramId, value] : parameters) {
      juce::ValueTree paramTree("Param");
      paramTree.setProperty("id", juce::String(paramId), nullptr);
      paramTree.setProperty("value", value, nullptr);
      paramsTree.appendChild(paramTree, nullptr);
    }
    tree.appendChild(paramsTree, nullptr);

    // Macros
    juce::ValueTree macrosTree("Macros");
    for (const auto &[macroId, value] : macros) {
      juce::ValueTree macroTree("Macro");
      macroTree.setProperty("id", juce::String(macroId), nullptr);
      macroTree.setProperty("value", value, nullptr);
      macrosTree.appendChild(macroTree, nullptr);
    }
    tree.appendChild(macrosTree, nullptr);

    return tree;
  }

  /**
   * @brief Load from ValueTree
   */
  static ZenithInstrumentPreset fromValueTree(const juce::ValueTree &tree) {
    std::string id = tree.getProperty("id", "").toString().toStdString();
    std::string name =
        tree.getProperty("name", "Untitled").toString().toStdString();
    std::string instrumentId =
        tree.getProperty("instrumentId", "").toString().toStdString();
    std::string author =
        tree.getProperty("author", "Unknown").toString().toStdString();

    // Generate ID if missing to ensure all presets have valid IDs
    if (id.empty()) {
      id = generateId(name);
    }

    ZenithInstrumentPreset preset(id, name, instrumentId, author);

    preset.category = tree.getProperty("category", "").toString().toStdString();
    preset.description =
        tree.getProperty("description", "").toString().toStdString();
    // New taxonomy fields
    preset.soundType =
        tree.getProperty("soundType", "").toString().toStdString();
    {
      juce::String engStr = tree.getProperty("engines", "").toString();
      if (engStr.isNotEmpty()) {
        juce::StringArray engArray =
            juce::StringArray::fromTokens(engStr, ",", "");
        for (const auto &e : engArray)
          preset.engines.push_back(e.trim().toStdString());
      }
    }
    {
      juce::String charStr = tree.getProperty("characters", "").toString();
      if (charStr.isNotEmpty()) {
        juce::StringArray charArray =
            juce::StringArray::fromTokens(charStr, ",", "");
        for (const auto &c : charArray)
          preset.characters.push_back(c.trim().toStdString());
      }
    }

    // Tags
    juce::String tagsStr = tree.getProperty("tags", "").toString();
    if (tagsStr.isNotEmpty()) {
      juce::StringArray tagArray =
          juce::StringArray::fromTokens(tagsStr, ",", "");
      for (const auto &tag : tagArray)
        preset.tags.push_back(tag.trim().toStdString());
    }

    // Parameters
    auto paramsTree = tree.getChildWithName("Parameters");
    if (paramsTree.isValid()) {
      for (int i = 0; i < paramsTree.getNumChildren(); ++i) {
        auto paramTree = paramsTree.getChild(i);
        std::string paramId =
            paramTree.getProperty("id", "").toString().toStdString();
        float value = paramTree.getProperty("value", 0.5f);
        if (!paramId.empty())
          preset.parameters[paramId] = value;
      }
    }

    // Macros
    auto macrosTree = tree.getChildWithName("Macros");
    if (macrosTree.isValid()) {
      for (int i = 0; i < macrosTree.getNumChildren(); ++i) {
        auto macroTree = macrosTree.getChild(i);
        std::string macroId =
            macroTree.getProperty("id", "").toString().toStdString();
        float value = macroTree.getProperty("value", 0.5f);
        if (!macroId.empty())
          preset.macros[macroId] = value;
      }
    }

    return preset;
  }

  /**
   * @brief Save to JSON file
   */
  bool saveToFile(const juce::File &file) const {
    auto tree = toValueTree();
    std::unique_ptr<juce::XmlElement> xml(tree.createXml());
    if (xml != nullptr) {
      return xml->writeTo(file);
    }
    return false;
  }

  /**
   * @brief Load from JSON file
   */
  static ZenithInstrumentPreset loadFromFile(const juce::File &file) {
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml != nullptr) {
      auto tree = juce::ValueTree::fromXml(*xml);
      return fromValueTree(tree);
    }
    return ZenithInstrumentPreset();
  }

  /**
   * @brief Convert to JSON object (AI-friendly format)
   */
  juce::var toJson() const {
    auto *obj = new juce::DynamicObject();

    // Basic info
    obj->setProperty("id", juce::String(id));
    obj->setProperty("name", juce::String(name));
    obj->setProperty("instrumentId", juce::String(instrumentId));
    obj->setProperty("category", juce::String(category));
    obj->setProperty("author", juce::String(author));
    obj->setProperty("description", juce::String(description));
    obj->setProperty("version", juce::String(version));

    // Tags array
    juce::Array<juce::var> tagsArray;
    for (const auto &tag : tags)
      tagsArray.add(juce::String(tag));
    obj->setProperty("tags", juce::var(tagsArray));

    // Parameters object
    auto *paramsObj = new juce::DynamicObject();
    for (const auto &[paramId, value] : parameters)
      paramsObj->setProperty(juce::Identifier(paramId), value);
    obj->setProperty("params", juce::var(paramsObj));

    // Macros object (optional)
    if (!macros.empty()) {
      auto *macrosObj = new juce::DynamicObject();
      for (const auto &[macroId, value] : macros)
        macrosObj->setProperty(juce::Identifier(macroId), value);
      obj->setProperty("macros", juce::var(macrosObj));
    }

    return juce::var(obj);
  }

  /**
   * @brief Load from JSON object
   */
  static ZenithInstrumentPreset fromJson(const juce::var &json) {
    if (!json.isObject())
      return ZenithInstrumentPreset();

    auto obj = json.getDynamicObject();
    if (!obj)
      return ZenithInstrumentPreset();

    // Basic info
    std::string id = obj->getProperty("id").toString().toStdString();
    std::string name = obj->getProperty("name").toString().toStdString();
    std::string instrumentId =
        obj->getProperty("instrumentId").toString().toStdString();
    std::string author = obj->getProperty("author").toString().toStdString();

    // Generate ID if missing to ensure all presets have valid IDs
    if (id.empty()) {
      id = generateId(name);
    }

    ZenithInstrumentPreset preset(id, name, instrumentId, author);

    preset.category = obj->getProperty("category").toString().toStdString();
    preset.description =
        obj->getProperty("description").toString().toStdString();
    preset.version = obj->getProperty("version").toString().toStdString();

    // Tags array
    if (obj->hasProperty("tags")) {
      auto tagsVar = obj->getProperty("tags");
      if (tagsVar.isArray()) {
        auto *tagsArray = tagsVar.getArray();
        for (const auto &tag : *tagsArray)
          preset.tags.push_back(tag.toString().toStdString());
      }
    }

    // Parameters object (support both "params" and "parameters" for
    // compatibility)
    juce::var paramsVar;
    if (obj->hasProperty("params"))
      paramsVar = obj->getProperty("params");
    else if (obj->hasProperty("parameters"))
      paramsVar = obj->getProperty("parameters");

    if (paramsVar.isObject()) {
      auto paramsObj = paramsVar.getDynamicObject();
      for (const auto &prop : paramsObj->getProperties()) {
        std::string paramId = prop.name.toString().toStdString();
        float value = static_cast<float>(prop.value);
        // Clamp to [0, 1] range
        value = juce::jlimit(0.0f, 1.0f, value);
        preset.parameters[paramId] = value;
      }
    }

    // Macros object (optional)
    if (obj->hasProperty("macros")) {
      auto macrosVar = obj->getProperty("macros");
      if (macrosVar.isObject()) {
        auto macrosObj = macrosVar.getDynamicObject();
        for (const auto &prop : macrosObj->getProperties()) {
          std::string macroId = prop.name.toString().toStdString();
          float value = static_cast<float>(prop.value);
          value = juce::jlimit(0.0f, 1.0f, value);
          preset.macros[macroId] = value;
        }
      }
    }

    return preset;
  }

  /**
   * @brief Save to JSON file (AI-friendly format)
   */
  bool saveToJsonFile(const juce::File &file) const {
    auto json = toJson();
    juce::String jsonStr =
        juce::JSON::toString(json, true); // true = pretty print
    return file.replaceWithText(jsonStr);
  }

  /**
   * @brief Load multiple presets from a JSON file (handles single object or
   * bank)
   */
  static std::vector<ZenithInstrumentPreset>
  loadPresetsFromJsonFile(const juce::File &file) {
    std::vector<ZenithInstrumentPreset> presets;
    juce::String jsonStr = file.loadFileAsString();
    auto json = juce::JSON::parse(jsonStr);

    if (!json.isObject())
      return presets;

    auto obj = json.getDynamicObject();
    if (obj->hasProperty("presets") && obj->getProperty("presets").isArray()) {
      // It's a bank
      auto presetsArray = obj->getProperty("presets").getArray();
      for (const auto &presetVar : *presetsArray) {
        auto preset = fromJson(presetVar);
        // Inherit instrument_id from bank if missing in preset
        if (preset.instrumentId.empty() && obj->hasProperty("instrument_id")) {
          preset.instrumentId =
              obj->getProperty("instrument_id").toString().toStdString();
        }
        presets.push_back(preset);
      }
    } else {
      // It's a single preset
      presets.push_back(fromJson(json));
    }
    return presets;
  }

private:
  /**
   * @brief Generate unique preset ID
   */
  static std::string generateId(const std::string &name) {
    // Simple ID generation: sanitized name + timestamp
    juce::String sanitized = name;
    sanitized = sanitized.toLowerCase().replaceCharacter(' ', '_');
    sanitized =
        sanitized.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");

    auto timestamp = juce::Time::getCurrentTime().toMilliseconds();
    return sanitized.toStdString() + "_" + std::to_string(timestamp);
  }
};

//==============================================================================
/**
 * @brief Preset manager for loading and saving presets
 *
 * Manages:
 * - Factory preset discovery
 * - User preset storage
 * - Preset browsing and search
 */
/*
class ZenithPresetManager {
    // ... (Commented out to avoid redefinition)
};
*/

} // namespace zenith