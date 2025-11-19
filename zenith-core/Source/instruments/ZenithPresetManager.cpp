/**
 * @file ZenithPresetManager.cpp
 * @brief Implementation of ZenithPresetManager
 */

#include "ZenithPresetManager.h"
#include "Instrument.h"
#include "ContentPaths.h"
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
// Preset Implementation
//==============================================================================

juce::var Preset::toJson() const
{
    auto* obj = new juce::DynamicObject();

    // Metadata
    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("author", author);
    obj->setProperty("category", category);
    obj->setProperty("instrumentId", instrumentId);
    obj->setProperty("version", version);

    if (description.isNotEmpty())
        obj->setProperty("description", description);

    // Tags array
    if (!tags.empty())
    {
        juce::Array<juce::var> tagsArray;
        for (const auto& tag : tags)
            tagsArray.add(tag);
        obj->setProperty("tags", tagsArray);
    }

    // Parameters (compact object format)
    auto* paramsObj = new juce::DynamicObject();
    for (const auto& [paramId, value] : parameters)
    {
        paramsObj->setProperty(paramId, value);
    }
    obj->setProperty("parameters", juce::var(paramsObj));

    return juce::var(obj);
}

Preset Preset::fromJson(const juce::var& json)
{
    Preset preset;

    if (!json.isObject())
        return preset;

    auto* obj = json.getDynamicObject();
    if (!obj)
        return preset;

    // Metadata
    preset.id = obj->getProperty("id").toString();
    preset.name = obj->getProperty("name").toString();
    preset.author = obj->getProperty("author").toString();
    preset.category = obj->getProperty("category").toString();
    preset.instrumentId = obj->getProperty("instrumentId").toString();
    preset.version = obj->getProperty("version").toString();
    preset.description = obj->getProperty("description").toString();

    // Tags array
    if (obj->hasProperty("tags"))
    {
        auto tagsVar = obj->getProperty("tags");
        if (tagsVar.isArray())
        {
            auto* tagsArray = tagsVar.getArray();
            for (const auto& tag : *tagsArray)
                preset.tags.push_back(tag.toString());
        }
    }

    // Parameters
    if (obj->hasProperty("parameters"))
    {
        auto paramsVar = obj->getProperty("parameters");
        if (paramsVar.isObject())
        {
            auto* paramsObj = paramsVar.getDynamicObject();
            for (const auto& prop : paramsObj->getProperties())
            {
                juce::String paramId = prop.name.toString();
                float value = static_cast<float>(static_cast<double>(prop.value));
                // Clamp to [0, 1] range
                value = juce::jlimit(0.0f, 1.0f, value);
                preset.parameters[paramId] = value;
            }
        }
    }

    return preset;
}

juce::String Preset::generateId(const juce::String& name)
{
    // Sanitize name: lowercase, replace spaces with underscores, keep alphanumeric
    juce::String sanitized = name.toLowerCase()
                                 .replaceCharacter(' ', '_')
                                 .retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");

    // Add timestamp for uniqueness
    auto timestamp = juce::Time::getCurrentTime().toMilliseconds();
    return sanitized + "_" + juce::String(timestamp);
}

//==============================================================================
// PresetMetadata Implementation
//==============================================================================

juce::var PresetMetadata::toVar() const
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("author", author);
    obj->setProperty("category", category);
    obj->setProperty("instrumentId", instrumentId);
    obj->setProperty("filePath", filePath);

    if (!tags.empty())
    {
        juce::Array<juce::var> tagsArray;
        for (const auto& tag : tags)
            tagsArray.add(tag);
        obj->setProperty("tags", tagsArray);
    }

    return juce::var(obj);
}

//==============================================================================
// ZenithPresetManager Implementation
//==============================================================================

ZenithPresetManager& ZenithPresetManager::getInstance()
{
    static ZenithPresetManager instance;
    return instance;
}

ZenithPresetManager::ZenithPresetManager()
{
    // Initialize factory and user preset directories
    // Factory: <ContentRoot>/Instruments/Presets/
    // User: <UserAppData>/Zenith/Presets/

    auto contentRoot = getDefaultContentRoot();
    factoryPresetsRoot_ = contentRoot.getChildFile("Instruments").getChildFile("Presets");

    auto userAppData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    userPresetsRoot_ = userAppData.getChildFile("Zenith").getChildFile("Presets");

    // Ensure user directory exists
    userPresetsRoot_.createDirectory();
}

std::vector<Preset> ZenithPresetManager::loadAllPresetsForInstrument(const juce::String& instrumentId)
{
    // THREAD SAFETY: This method performs file I/O and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    std::vector<Preset> presets;

    // Load from factory presets
    auto factoryDir = getPresetsDirectory(instrumentId, juce::String(), false);
    if (factoryDir.exists())
    {
        auto factoryPresets = scanDirectory(factoryDir, instrumentId);
        presets.insert(presets.end(), factoryPresets.begin(), factoryPresets.end());
    }

    // Load from user presets
    auto userDir = getPresetsDirectory(instrumentId, juce::String(), true);
    if (userDir.exists())
    {
        auto userPresets = scanDirectory(userDir, instrumentId);
        presets.insert(presets.end(), userPresets.begin(), userPresets.end());
    }

    return presets;
}

std::vector<PresetMetadata> ZenithPresetManager::getPresetList(const juce::String& instrumentId,
                                                                const juce::String& category)
{
    // THREAD SAFETY: This method performs file I/O and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    std::vector<PresetMetadata> metadataList;

    // Scan factory presets
    auto factoryDir = getPresetsDirectory(instrumentId, category, false);
    if (factoryDir.exists())
    {
        auto factoryMeta = scanDirectoryMetadata(factoryDir, instrumentId);
        metadataList.insert(metadataList.end(), factoryMeta.begin(), factoryMeta.end());
    }

    // Scan user presets
    auto userDir = getPresetsDirectory(instrumentId, category, true);
    if (userDir.exists())
    {
        auto userMeta = scanDirectoryMetadata(userDir, instrumentId);
        metadataList.insert(metadataList.end(), userMeta.begin(), userMeta.end());
    }

    return metadataList;
}

Preset ZenithPresetManager::loadPreset(const juce::String& instrumentId, const juce::String& presetId)
{
    // THREAD SAFETY: This method performs file I/O and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Try user presets first
    auto file = findPresetFile(instrumentId, presetId, true);
    if (file.existsAsFile())
        return loadPresetFromFile(file);

    // Try factory presets
    file = findPresetFile(instrumentId, presetId, false);
    if (file.existsAsFile())
        return loadPresetFromFile(file);

    // Not found
    return Preset();
}

juce::StringArray ZenithPresetManager::getCategories(const juce::String& instrumentId)
{
    // THREAD SAFETY: This method performs file I/O and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::StringArray categories;

    // Scan factory presets directory for category subdirectories
    auto factoryDir = getPresetsDirectory(instrumentId, juce::String(), false);
    if (factoryDir.exists())
    {
        auto subdirs = factoryDir.findChildFiles(juce::File::findDirectories, false);
        for (const auto& dir : subdirs)
        {
            categories.addIfNotAlreadyThere(dir.getFileName());
        }
    }

    // Scan user presets directory
    auto userDir = getPresetsDirectory(instrumentId, juce::String(), true);
    if (userDir.exists())
    {
        auto subdirs = userDir.findChildFiles(juce::File::findDirectories, false);
        for (const auto& dir : subdirs)
        {
            categories.addIfNotAlreadyThere(dir.getFileName());
        }
    }

    return categories;
}

bool ZenithPresetManager::applyPresetToInstrument(const Preset& preset, Instrument& instrument)
{
    // THREAD SAFETY: This method modifies instrument parameters and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Validate instrument ID match
    if (preset.instrumentId != instrument.getMetadata().instrumentId)
    {
        DBG("ZenithPresetManager: Instrument ID mismatch - preset for '"
            << preset.instrumentId << "' cannot be applied to '"
            << instrument.getMetadata().instrumentId << "'");
        return false;
    }

    // Check if custom deserializer is registered
    auto it = deserializers_.find(preset.instrumentId);
    if (it != deserializers_.end())
    {
        // Use custom deserializer
        it->second(instrument, preset.parameters);
        return true;
    }

    // Default implementation: apply each parameter
    for (const auto& [paramId, value] : preset.parameters)
    {
        if (!instrument.setParameter(paramId, value))
        {
            DBG("ZenithPresetManager: Failed to set parameter '" << paramId << "'");
        }
    }

    return true;
}

Preset ZenithPresetManager::capturePresetFromInstrument(Instrument& instrument,
                                                        const juce::String& name,
                                                        const juce::String& category,
                                                        const juce::String& author)
{
    // THREAD SAFETY: This method reads instrument parameters and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    Preset preset;
    preset.name = name;
    preset.category = category;
    preset.author = author;
    preset.instrumentId = instrument.getMetadata().instrumentId;
    preset.id = Preset::generateId(name);

    // Check if custom serializer is registered
    auto it = serializers_.find(preset.instrumentId);
    if (it != serializers_.end())
    {
        // Use custom serializer
        preset.parameters = it->second(instrument);
        return preset;
    }

    // Default implementation: capture all parameters from metadata
    const auto& metadata = instrument.getMetadata();
    for (const auto& param : metadata.parameters)
    {
        float value = instrument.getParameter(param.id);
        preset.parameters[param.id] = value;
    }

    return preset;
}

bool ZenithPresetManager::savePreset(const Preset& preset, bool userPreset)
{
    // THREAD SAFETY: This method performs file I/O and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Get target directory
    auto dir = getPresetsDirectory(preset.instrumentId, preset.category, userPreset);

    // Create directory if it doesn't exist
    if (!dir.exists())
    {
        if (!dir.createDirectory())
        {
            DBG("ZenithPresetManager: Failed to create directory: " << dir.getFullPathName());
            return false;
        }
    }

    // Generate filename from preset name
    juce::String filename = preset.name.replaceCharacter('/', '_')
                                      .replaceCharacter('\\', '_')
                                      .replaceCharacter(':', '_');
    auto file = dir.getChildFile(filename + ".zpreset.json");

    // Convert to JSON and write
    auto json = preset.toJson();
    juce::String jsonStr = juce::JSON::toString(json, true);  // true = pretty print

    return file.replaceWithText(jsonStr);
}

bool ZenithPresetManager::deletePreset(const juce::String& instrumentId,
                                       const juce::String& presetId,
                                       bool userPreset)
{
    // THREAD SAFETY: This method performs file I/O and MUST be called from the message thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto file = findPresetFile(instrumentId, presetId, userPreset);
    if (file.existsAsFile())
        return file.deleteFile();

    return false;
}

juce::File ZenithPresetManager::getPresetsDirectory(const juce::String& instrumentId,
                                                    const juce::String& category,
                                                    bool userPreset) const
{
    auto baseDir = userPreset ? userPresetsRoot_ : factoryPresetsRoot_;
    auto instrumentDir = baseDir.getChildFile(instrumentId);

    if (category.isEmpty())
        return instrumentDir;

    return instrumentDir.getChildFile(category);
}

void ZenithPresetManager::setContentRoot(const juce::File& root)
{
    customContentRoot_ = root;
    factoryPresetsRoot_ = root.getChildFile("Instruments").getChildFile("Presets");
}

juce::File ZenithPresetManager::getContentRoot() const
{
    if (customContentRoot_.exists())
        return customContentRoot_;

    return getDefaultContentRoot();
}

void ZenithPresetManager::registerSerializationHooks(const juce::String& instrumentId,
                                                     ParameterSerializerFn serializer,
                                                     ParameterDeserializerFn deserializer)
{
    serializers_[instrumentId] = serializer;
    deserializers_[instrumentId] = deserializer;
}

//==============================================================================
// Private Implementation
//==============================================================================

std::vector<Preset> ZenithPresetManager::scanDirectory(const juce::File& directory,
                                                        const juce::String& instrumentId)
{
    std::vector<Preset> presets;

    if (!directory.exists())
        return presets;

    // Find all .zpreset.json files recursively
    auto files = directory.findChildFiles(juce::File::findFiles, true, "*.zpreset.json");

    for (const auto& file : files)
    {
        auto preset = loadPresetFromFile(file);
        if (preset.instrumentId.isNotEmpty() &&
            (instrumentId.isEmpty() || preset.instrumentId == instrumentId))
        {
            presets.push_back(preset);
        }
    }

    return presets;
}

std::vector<PresetMetadata> ZenithPresetManager::scanDirectoryMetadata(const juce::File& directory,
                                                                        const juce::String& instrumentId)
{
    std::vector<PresetMetadata> metadataList;

    if (!directory.exists())
        return metadataList;

    // Find all .zpreset.json files recursively
    auto files = directory.findChildFiles(juce::File::findFiles, true, "*.zpreset.json");

    for (const auto& file : files)
    {
        auto meta = loadMetadataFromFile(file);
        if (meta.instrumentId.isNotEmpty() &&
            (instrumentId.isEmpty() || meta.instrumentId == instrumentId))
        {
            metadataList.push_back(meta);
        }
    }

    return metadataList;
}

juce::File ZenithPresetManager::getDefaultContentRoot() const
{
    // Use ContentPaths if available, otherwise use default
    return ContentPaths::getInstance().getContentRoot();
}

Preset ZenithPresetManager::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return Preset();

    juce::String jsonStr = file.loadFileAsString();
    auto json = juce::JSON::parse(jsonStr);

    return Preset::fromJson(json);
}

PresetMetadata ZenithPresetManager::loadMetadataFromFile(const juce::File& file)
{
    PresetMetadata meta;

    if (!file.existsAsFile())
        return meta;

    // Load JSON and extract only metadata fields (skip parameters for efficiency)
    juce::String jsonStr = file.loadFileAsString();
    auto json = juce::JSON::parse(jsonStr);

    if (!json.isObject())
        return meta;

    auto* obj = json.getDynamicObject();
    if (!obj)
        return meta;

    meta.id = obj->getProperty("id").toString();
    meta.name = obj->getProperty("name").toString();
    meta.author = obj->getProperty("author").toString();
    meta.category = obj->getProperty("category").toString();
    meta.instrumentId = obj->getProperty("instrumentId").toString();
    meta.filePath = file.getFullPathName();

    // Tags
    if (obj->hasProperty("tags"))
    {
        auto tagsVar = obj->getProperty("tags");
        if (tagsVar.isArray())
        {
            auto* tagsArray = tagsVar.getArray();
            for (const auto& tag : *tagsArray)
                meta.tags.push_back(tag.toString());
        }
    }

    return meta;
}

juce::File ZenithPresetManager::findPresetFile(const juce::String& instrumentId,
                                               const juce::String& presetId,
                                               bool userPreset)
{
    auto baseDir = getPresetsDirectory(instrumentId, juce::String(), userPreset);

    if (!baseDir.exists())
        return juce::File();

    // Search recursively for preset with matching ID
    auto files = baseDir.findChildFiles(juce::File::findFiles, true, "*.zpreset.json");

    for (const auto& file : files)
    {
        // Quick check: load just the ID field
        juce::String jsonStr = file.loadFileAsString();
        auto json = juce::JSON::parse(jsonStr);

        if (json.isObject())
        {
            auto* obj = json.getDynamicObject();
            if (obj && obj->getProperty("id").toString() == presetId)
            {
                return file;
            }
        }
    }

    return juce::File();
}

} // namespace zenith
