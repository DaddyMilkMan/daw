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

#include <JuceHeader.h>
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
struct ZenithInstrumentPreset
{
    // Identity
    std::string id;                     ///< Unique preset ID (generated)
    std::string name;                   ///< Preset name (e.g., "Warm Pad")
    std::string instrumentId;           ///< Target instrument (e.g., "zenith_poly_synth")

    // Metadata
    std::string author;                 ///< Preset author
    std::string description;            ///< Detailed description
    std::vector<std::string> tags;      ///< Tags for searching (e.g., "pad", "warm", "lush")
    std::string version;                ///< Preset version

    // Parameter values
    std::map<std::string, float> parameters;    ///< Parameter ID -> value
    std::map<std::string, float> macros;        ///< Macro ID -> value [0..1]

    ZenithInstrumentPreset() = default;

    /**
     * @brief Create preset with basic info
     */
    ZenithInstrumentPreset(
        const std::string& name_,
        const std::string& instrumentId_,
        const std::string& author_ = "Factory"
    ) : name(name_), instrumentId(instrumentId_), author(author_)
    {
        // Generate unique ID from name and timestamp
        id = generateId(name_);
    }

    /**
     * @brief Set parameter value
     */
    void setParameter(const std::string& paramId, float value)
    {
        parameters[paramId] = value;
    }

    /**
     * @brief Set macro value
     */
    void setMacro(const std::string& macroId, float value)
    {
        macros[macroId] = value;
    }

    /**
     * @brief Get parameter value
     * @return Value, or defaultValue if not found
     */
    float getParameter(const std::string& paramId, float defaultValue = 0.5f) const
    {
        auto it = parameters.find(paramId);
        return (it != parameters.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Get macro value
     * @return Value [0..1], or 0.5 if not found
     */
    float getMacro(const std::string& macroId, float defaultValue = 0.5f) const
    {
        auto it = macros.find(macroId);
        return (it != macros.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Convert to ValueTree for serialization
     */
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree tree("InstrumentPreset");

        // Basic info
        tree.setProperty("id", id, nullptr);
        tree.setProperty("name", name, nullptr);
        tree.setProperty("instrumentId", instrumentId, nullptr);
        tree.setProperty("author", author, nullptr);
        tree.setProperty("description", description, nullptr);
        tree.setProperty("version", version, nullptr);

        // Tags
        if (!tags.empty())
        {
            juce::StringArray tagArray;
            for (const auto& tag : tags)
                tagArray.add(tag);
            tree.setProperty("tags", tagArray.joinIntoString(","), nullptr);
        }

        // Parameters
        juce::ValueTree paramsTree("Parameters");
        for (const auto& [paramId, value] : parameters)
        {
            juce::ValueTree paramTree("Param");
            paramTree.setProperty("id", paramId, nullptr);
            paramTree.setProperty("value", value, nullptr);
            paramsTree.appendChild(paramTree, nullptr);
        }
        tree.appendChild(paramsTree, nullptr);

        // Macros
        juce::ValueTree macrosTree("Macros");
        for (const auto& [macroId, value] : macros)
        {
            juce::ValueTree macroTree("Macro");
            macroTree.setProperty("id", macroId, nullptr);
            macroTree.setProperty("value", value, nullptr);
            macrosTree.appendChild(macroTree, nullptr);
        }
        tree.appendChild(macrosTree, nullptr);

        return tree;
    }

    /**
     * @brief Load from ValueTree
     */
    static ZenithInstrumentPreset fromValueTree(const juce::ValueTree& tree)
    {
        ZenithInstrumentPreset preset;

        // Basic info
        preset.id = tree.getProperty("id", "").toString().toStdString();
        preset.name = tree.getProperty("name", "Untitled").toString().toStdString();
        preset.instrumentId = tree.getProperty("instrumentId", "").toString().toStdString();
        preset.author = tree.getProperty("author", "Unknown").toString().toStdString();
        preset.description = tree.getProperty("description", "").toString().toStdString();
        preset.version = tree.getProperty("version", "1.0.0").toString().toStdString();

        // Tags
        juce::String tagsStr = tree.getProperty("tags", "").toString();
        if (tagsStr.isNotEmpty())
        {
            juce::StringArray tagArray = juce::StringArray::fromTokens(tagsStr, ",", "");
            for (const auto& tag : tagArray)
                preset.tags.push_back(tag.trim().toStdString());
        }

        // Parameters
        auto paramsTree = tree.getChildWithName("Parameters");
        if (paramsTree.isValid())
        {
            for (int i = 0; i < paramsTree.getNumChildren(); ++i)
            {
                auto paramTree = paramsTree.getChild(i);
                std::string paramId = paramTree.getProperty("id", "").toString().toStdString();
                float value = paramTree.getProperty("value", 0.5f);
                if (!paramId.empty())
                    preset.parameters[paramId] = value;
            }
        }

        // Macros
        auto macrosTree = tree.getChildWithName("Macros");
        if (macrosTree.isValid())
        {
            for (int i = 0; i < macrosTree.getNumChildren(); ++i)
            {
                auto macroTree = macrosTree.getChild(i);
                std::string macroId = macroTree.getProperty("id", "").toString().toStdString();
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
    bool saveToFile(const juce::File& file) const
    {
        auto tree = toValueTree();
        std::unique_ptr<juce::XmlElement> xml(tree.createXml());
        if (xml != nullptr)
        {
            return xml->writeTo(file);
        }
        return false;
    }

    /**
     * @brief Load from JSON file
     */
    static ZenithInstrumentPreset loadFromFile(const juce::File& file)
    {
        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml != nullptr)
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            return fromValueTree(tree);
        }
        return ZenithInstrumentPreset();
    }

private:
    /**
     * @brief Generate unique preset ID
     */
    static std::string generateId(const std::string& name)
    {
        // Simple ID generation: sanitized name + timestamp
        juce::String sanitized = name;
        sanitized = sanitized.toLowerCase().replaceCharacter(' ', '_');
        sanitized = sanitized.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");

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
class ZenithPresetManager
{
public:
    ZenithPresetManager()
    {
        // Get preset directories
        auto userDataDir = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory);

        factoryPresetsDir_ = userDataDir.getChildFile("Zenith/Instruments/Factory");
        userPresetsDir_ = userDataDir.getChildFile("Zenith/Instruments/User");

        // Ensure directories exist
        factoryPresetsDir_.createDirectory();
        userPresetsDir_.createDirectory();
    }

    /**
     * @brief Get all presets for an instrument
     * @param instrumentId Instrument ID (e.g., "zenith_poly_synth")
     * @return Vector of presets
     */
    std::vector<ZenithInstrumentPreset> getPresetsForInstrument(
        const std::string& instrumentId) const
    {
        std::vector<ZenithInstrumentPreset> presets;

        // Load factory presets
        auto factoryDir = getInstrumentPresetsDir(instrumentId, true);
        if (factoryDir.exists())
        {
            auto files = factoryDir.findChildFiles(
                juce::File::findFiles, false, "*.zpreset");
            for (const auto& file : files)
            {
                auto preset = ZenithInstrumentPreset::loadFromFile(file);
                if (preset.instrumentId == instrumentId)
                    presets.push_back(preset);
            }
        }

        // Load user presets
        auto userDir = getInstrumentPresetsDir(instrumentId, false);
        if (userDir.exists())
        {
            auto files = userDir.findChildFiles(
                juce::File::findFiles, false, "*.zpreset");
            for (const auto& file : files)
            {
                auto preset = ZenithInstrumentPreset::loadFromFile(file);
                if (preset.instrumentId == instrumentId)
                    presets.push_back(preset);
            }
        }

        return presets;
    }

    /**
     * @brief Save user preset
     * @param preset Preset to save
     * @return true if saved successfully
     */
    bool saveUserPreset(const ZenithInstrumentPreset& preset)
    {
        auto dir = getInstrumentPresetsDir(preset.instrumentId, false);
        dir.createDirectory();

        // Sanitize filename
        juce::String filename = preset.name;
        filename = filename.replaceCharacter('/', '_');
        filename = filename.replaceCharacter('\\', '_');

        auto file = dir.getChildFile(filename + ".zpreset");
        return preset.saveToFile(file);
    }

    /**
     * @brief Delete user preset
     * @param preset Preset to delete
     * @return true if deleted successfully
     */
    bool deleteUserPreset(const ZenithInstrumentPreset& preset)
    {
        auto dir = getInstrumentPresetsDir(preset.instrumentId, false);

        juce::String filename = preset.name;
        filename = filename.replaceCharacter('/', '_');
        filename = filename.replaceCharacter('\\', '_');

        auto file = dir.getChildFile(filename + ".zpreset");
        return file.deleteFile();
    }

    /**
     * @brief Create a factory preset (for built-in instruments)
     * @note Only call this during development/installation
     */
    bool saveFactoryPreset(const ZenithInstrumentPreset& preset)
    {
        auto dir = getInstrumentPresetsDir(preset.instrumentId, true);
        dir.createDirectory();

        juce::String filename = preset.name;
        filename = filename.replaceCharacter('/', '_');
        filename = filename.replaceCharacter('\\', '_');

        auto file = dir.getChildFile(filename + ".zpreset");
        return preset.saveToFile(file);
    }

private:
    /**
     * @brief Get presets directory for instrument
     * @param instrumentId Instrument ID
     * @param factory true for factory presets, false for user presets
     */
    juce::File getInstrumentPresetsDir(const std::string& instrumentId, bool factory) const
    {
        auto baseDir = factory ? factoryPresetsDir_ : userPresetsDir_;
        return baseDir.getChildFile(instrumentId);
    }

    juce::File factoryPresetsDir_;
    juce::File userPresetsDir_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPresetManager)
};

} // namespace zenith
