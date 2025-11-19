/**
 * @file ZenithPresetManager.h
 * @brief Factory preset system for Zenith instruments
 *
 * Provides a centralized preset management system with:
 * - Lightweight Preset struct for metadata + parameter blob
 * - Structured on-disk format (Presets/<InstrumentName>/<Category>/)
 * - JSON serialization (.zpreset.json files)
 * - Thread-safe API (all I/O on message thread)
 * - Integration with InstrumentRegistry
 *
 * Thread Safety:
 * - ALL file I/O operations MUST be called from the message thread
 * - NO audio thread operations are permitted
 * - Asserts enforce thread safety in debug builds
 */

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <functional>

namespace zenith {

// Forward declaration
class Instrument;

//==============================================================================
/**
 * @brief Lightweight preset structure
 *
 * Contains:
 * - Metadata (id, name, author, category, tags)
 * - Parameter values (compact normalized [0-1] blob)
 * - Instrument identifier for validation
 */
struct Preset
{
    // Metadata
    juce::String id;                            ///< Unique preset ID (e.g., "warm_pad_001")
    juce::String name;                          ///< Display name (e.g., "Warm Pad")
    juce::String author;                        ///< Author name (e.g., "Factory", "User")
    juce::String category;                      ///< Category for browsing (e.g., "Pad", "Bass", "Lead")
    std::vector<juce::String> tags;             ///< Search tags (e.g., ["warm", "lush", "ambient"])
    juce::String instrumentId;                  ///< Target instrument (e.g., "ZenithPolySynth")

    // Additional metadata
    juce::String description;                   ///< Detailed description (optional)
    juce::String version;                       ///< Preset version (e.g., "1.0.0")

    // Parameter data (normalized [0-1] values)
    std::map<juce::String, float> parameters;   ///< Parameter ID -> normalized value

    // Default constructor
    Preset() : version("1.0.0") {}

    /**
     * @brief Construct preset with basic info
     */
    Preset(const juce::String& name_,
           const juce::String& instrumentId_,
           const juce::String& category_ = "Uncategorized",
           const juce::String& author_ = "Factory")
        : name(name_)
        , author(author_)
        , category(category_)
        , instrumentId(instrumentId_)
        , version("1.0.0")
    {
        // Generate unique ID from name
        id = generateId(name_);
    }

    /**
     * @brief Convert to JSON representation
     * @return JSON var object
     */
    juce::var toJson() const;

    /**
     * @brief Load from JSON representation
     * @param json JSON var object
     * @return Parsed preset
     */
    static Preset fromJson(const juce::var& json);

    /**
     * @brief Generate unique preset ID from name
     */
    static juce::String generateId(const juce::String& name);
};

//==============================================================================
/**
 * @brief Preset metadata (lightweight, for browsing without loading full preset)
 */
struct PresetMetadata
{
    juce::String id;
    juce::String name;
    juce::String author;
    juce::String category;
    std::vector<juce::String> tags;
    juce::String instrumentId;
    juce::String filePath;                      ///< Absolute path to preset file

    /**
     * @brief Convert to JSON var for API responses
     */
    juce::var toVar() const;
};

//==============================================================================
/**
 * @brief Centralized preset manager for Zenith instruments
 *
 * Responsibilities:
 * - Discover presets from content directory structure
 * - Load/save presets in standardized JSON format
 * - Provide browsing API (by category, tags, instrument)
 * - Apply presets to instrument instances
 * - Capture current instrument state as preset
 *
 * Directory Structure:
 *   <ContentRoot>/Instruments/Presets/<InstrumentName>/<Category>/*.zpreset.json
 *
 * Example:
 *   ~/Music/Zenith/Instruments/Presets/ZenithPolySynth/Bass/sub_808.zpreset.json
 *   ~/Music/Zenith/Instruments/Presets/ZenithPolySynth/Pads/warm_pad.zpreset.json
 *
 * Thread Safety:
 * - ALL methods must be called from the message thread
 * - NO audio thread access permitted
 * - File I/O is synchronous (no background loading)
 */
class ZenithPresetManager
{
public:
    //==========================================================================
    // Singleton Access
    //==========================================================================

    /**
     * @brief Get singleton instance
     */
    static ZenithPresetManager& getInstance();

    //==========================================================================
    // Preset Discovery
    //==========================================================================

    /**
     * @brief Load all presets for a specific instrument
     * @param instrumentId Instrument identifier (e.g., "ZenithPolySynth")
     * @return Vector of all presets for this instrument
     * @note MESSAGE THREAD ONLY - performs file I/O
     */
    std::vector<Preset> loadAllPresetsForInstrument(const juce::String& instrumentId);

    /**
     * @brief Get preset metadata list (without loading full parameter data)
     * @param instrumentId Instrument identifier
     * @param category Optional category filter (empty = all categories)
     * @return Vector of preset metadata
     * @note MESSAGE THREAD ONLY - performs file I/O
     */
    std::vector<PresetMetadata> getPresetList(const juce::String& instrumentId,
                                              const juce::String& category = juce::String());

    /**
     * @brief Load a specific preset by ID
     * @param instrumentId Instrument identifier
     * @param presetId Preset unique ID
     * @return Preset, or empty preset if not found
     * @note MESSAGE THREAD ONLY - performs file I/O
     */
    Preset loadPreset(const juce::String& instrumentId, const juce::String& presetId);

    /**
     * @brief Get list of categories for an instrument
     * @param instrumentId Instrument identifier
     * @return Array of category names
     * @note MESSAGE THREAD ONLY - performs file I/O
     */
    juce::StringArray getCategories(const juce::String& instrumentId);

    //==========================================================================
    // Preset Application
    //==========================================================================

    /**
     * @brief Apply preset to an instrument instance
     * @param preset Preset to apply
     * @param instrument Target instrument
     * @return true if successful, false if instrument ID mismatch or error
     * @note MESSAGE THREAD ONLY - modifies instrument parameters
     */
    bool applyPresetToInstrument(const Preset& preset, Instrument& instrument);

    /**
     * @brief Capture current instrument state as a preset
     * @param instrument Source instrument
     * @param name Preset name
     * @param category Preset category
     * @param author Author name (default: "User")
     * @return Captured preset
     * @note MESSAGE THREAD ONLY - reads instrument parameters
     */
    Preset capturePresetFromInstrument(Instrument& instrument,
                                       const juce::String& name,
                                       const juce::String& category,
                                       const juce::String& author = "User");

    //==========================================================================
    // Preset Storage
    //==========================================================================

    /**
     * @brief Save preset to disk
     * @param preset Preset to save
     * @param userPreset If true, save to user directory; if false, save to factory directory
     * @return true if successful
     * @note MESSAGE THREAD ONLY - performs file I/O
     */
    bool savePreset(const Preset& preset, bool userPreset = true);

    /**
     * @brief Delete preset from disk
     * @param instrumentId Instrument identifier
     * @param presetId Preset unique ID
     * @param userPreset If true, delete from user directory; if false, from factory
     * @return true if successful
     * @note MESSAGE THREAD ONLY - performs file I/O
     */
    bool deletePreset(const juce::String& instrumentId,
                      const juce::String& presetId,
                      bool userPreset = true);

    //==========================================================================
    // Path Management
    //==========================================================================

    /**
     * @brief Get presets directory for an instrument
     * @param instrumentId Instrument identifier
     * @param category Optional category (empty = root presets directory)
     * @param userPreset If true, user directory; if false, factory directory
     * @return Directory path
     */
    juce::File getPresetsDirectory(const juce::String& instrumentId,
                                   const juce::String& category = juce::String(),
                                   bool userPreset = false) const;

    /**
     * @brief Set custom content root (for testing or custom installations)
     * @param root Custom content root directory
     */
    void setContentRoot(const juce::File& root);

    /**
     * @brief Get current content root
     */
    juce::File getContentRoot() const;

    //==========================================================================
    // Instrument Integration
    //==========================================================================

    /**
     * @brief Callback type for parameter serialization
     * @param instrument Source instrument
     * @return Map of parameter ID -> normalized value [0-1]
     */
    using ParameterSerializerFn = std::function<std::map<juce::String, float>(Instrument&)>;

    /**
     * @brief Callback type for parameter deserialization
     * @param instrument Target instrument
     * @param parameters Map of parameter ID -> normalized value [0-1]
     */
    using ParameterDeserializerFn = std::function<void(Instrument&, const std::map<juce::String, float>&)>;

    /**
     * @brief Register parameter serialization hooks for an instrument
     * @param instrumentId Instrument identifier
     * @param serializer Function to serialize parameters
     * @param deserializer Function to deserialize parameters
     * @note This allows each instrument to control its own parameter format
     */
    void registerSerializationHooks(const juce::String& instrumentId,
                                    ParameterSerializerFn serializer,
                                    ParameterDeserializerFn deserializer);

private:
    ZenithPresetManager();
    ~ZenithPresetManager() = default;

    // Prevent copying
    ZenithPresetManager(const ZenithPresetManager&) = delete;
    ZenithPresetManager& operator=(const ZenithPresetManager&) = delete;

    /**
     * @brief Scan directory for preset files
     * @param directory Directory to scan
     * @param instrumentId Instrument ID to filter (empty = all)
     * @return Vector of presets
     */
    std::vector<Preset> scanDirectory(const juce::File& directory,
                                      const juce::String& instrumentId);

    /**
     * @brief Scan directory for preset metadata only
     * @param directory Directory to scan
     * @param instrumentId Instrument ID to filter (empty = all)
     * @return Vector of preset metadata
     */
    std::vector<PresetMetadata> scanDirectoryMetadata(const juce::File& directory,
                                                      const juce::String& instrumentId);

    /**
     * @brief Get default content root
     */
    juce::File getDefaultContentRoot() const;

    /**
     * @brief Load preset from file
     * @param file Preset file
     * @return Loaded preset, or empty preset if error
     */
    Preset loadPresetFromFile(const juce::File& file);

    /**
     * @brief Load preset metadata from file (without full parameter data)
     * @param file Preset file
     * @return Preset metadata, or empty if error
     */
    PresetMetadata loadMetadataFromFile(const juce::File& file);

    /**
     * @brief Find preset file by ID
     * @param instrumentId Instrument identifier
     * @param presetId Preset unique ID
     * @param userPreset Search user or factory directory
     * @return File path, or invalid file if not found
     */
    juce::File findPresetFile(const juce::String& instrumentId,
                             const juce::String& presetId,
                             bool userPreset);

    // Content directories
    juce::File customContentRoot_;
    juce::File factoryPresetsRoot_;
    juce::File userPresetsRoot_;

    // Serialization hooks
    std::map<juce::String, ParameterSerializerFn> serializers_;
    std::map<juce::String, ParameterDeserializerFn> deserializers_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPresetManager)
};

} // namespace zenith
