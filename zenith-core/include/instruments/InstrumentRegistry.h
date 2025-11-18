/**
 * @file InstrumentRegistry.h
 * @brief Registry for built-in instruments
 *
 * Provides:
 * - Instrument registration and discovery
 * - Metadata access
 * - Instrument instantiation
 * - Preset management integration
 */

#pragma once

#include <JuceHeader.h>
#include "InstrumentMetadata.h"
#include "InstrumentPreset.h"
#include <map>
#include <memory>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Forward declaration of base instrument processor
 */
class ZenithInstrumentProcessor;

//==============================================================================
/**
 * @brief Factory function type for creating instruments
 */
using InstrumentFactory = std::function<std::unique_ptr<ZenithInstrumentProcessor>()>;

//==============================================================================
/**
 * @brief Registry entry for an instrument
 */
struct InstrumentRegistryEntry
{
    InstrumentMetadata metadata;        ///< Instrument metadata
    InstrumentFactory factory;          ///< Factory function to create instances
    bool isAvailable = true;            ///< Whether instrument is available

    InstrumentRegistryEntry() = default;

    InstrumentRegistryEntry(
        const InstrumentMetadata& metadata_,
        InstrumentFactory factory_
    ) : metadata(metadata_), factory(factory_)
    {}
};

//==============================================================================
/**
 * @brief Singleton registry for all built-in instruments
 *
 * This registry:
 * - Maintains a list of all available instruments
 * - Provides metadata for each instrument
 * - Creates instances on demand
 * - Integrates with preset management
 */
class InstrumentRegistry
{
public:
    /**
     * @brief Get singleton instance
     */
    static InstrumentRegistry& getInstance()
    {
        static InstrumentRegistry instance;
        return instance;
    }

    /**
     * @brief Register an instrument
     * @param metadata Instrument metadata
     * @param factory Factory function to create instances
     */
    void registerInstrument(
        const InstrumentMetadata& metadata,
        InstrumentFactory factory)
    {
        InstrumentRegistryEntry entry(metadata, factory);
        instruments_[metadata.instrumentId] = entry;
    }

    /**
     * @brief Check if instrument is registered
     * @param instrumentId Instrument ID
     * @return true if registered
     */
    bool isInstrumentRegistered(const std::string& instrumentId) const
    {
        return instruments_.find(instrumentId) != instruments_.end();
    }

    /**
     * @brief Get metadata for an instrument
     * @param instrumentId Instrument ID
     * @return Pointer to metadata, or nullptr if not found
     */
    const InstrumentMetadata* getMetadata(const std::string& instrumentId) const
    {
        auto it = instruments_.find(instrumentId);
        if (it != instruments_.end())
            return &it->second.metadata;
        return nullptr;
    }

    /**
     * @brief Create an instance of an instrument
     * @param instrumentId Instrument ID
     * @return Unique pointer to processor, or nullptr if not found
     */
    std::unique_ptr<ZenithInstrumentProcessor> createInstrument(
        const std::string& instrumentId) const
    {
        auto it = instruments_.find(instrumentId);
        if (it != instruments_.end() && it->second.factory)
        {
            return it->second.factory();
        }
        return nullptr;
    }

    /**
     * @brief Get all registered instrument IDs
     * @return Vector of instrument IDs
     */
    std::vector<std::string> getAllInstrumentIds() const
    {
        std::vector<std::string> ids;
        for (const auto& [id, entry] : instruments_)
        {
            if (entry.isAvailable)
                ids.push_back(id);
        }
        return ids;
    }

    /**
     * @brief Get all presets for an instrument
     * @param instrumentId Instrument ID
     * @return Vector of presets
     */
    std::vector<ZenithInstrumentPreset> getPresetsForInstrument(
        const std::string& instrumentId) const
    {
        return presetManager_.getPresetsForInstrument(instrumentId);
    }

    /**
     * @brief Get preset manager
     */
    ZenithPresetManager& getPresetManager()
    {
        return presetManager_;
    }

    /**
     * @brief Get preset manager (const)
     */
    const ZenithPresetManager& getPresetManager() const
    {
        return presetManager_;
    }

private:
    InstrumentRegistry() = default;
    ~InstrumentRegistry() = default;

    // Delete copy/move constructors
    InstrumentRegistry(const InstrumentRegistry&) = delete;
    InstrumentRegistry& operator=(const InstrumentRegistry&) = delete;
    InstrumentRegistry(InstrumentRegistry&&) = delete;
    InstrumentRegistry& operator=(InstrumentRegistry&&) = delete;

    std::map<std::string, InstrumentRegistryEntry> instruments_;
    mutable ZenithPresetManager presetManager_;
};

//==============================================================================
/**
 * @brief Base class for all Zenith built-in instruments
 *
 * Provides:
 * - Common interface for metadata
 * - Preset loading/saving
 * - Macro engine integration
 */
class ZenithInstrumentProcessor : public juce::AudioProcessor
{
public:
    ZenithInstrumentProcessor()
        : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    virtual ~ZenithInstrumentProcessor() = default;

    /**
     * @brief Get instrument metadata
     * @return Instrument metadata
     */
    virtual const InstrumentMetadata& getInstrumentMetadata() const = 0;

    /**
     * @brief Get macro engine
     * @return Reference to macro engine
     */
    virtual MacroEngine& getMacroEngine() = 0;

    /**
     * @brief Get macro engine (const)
     */
    virtual const MacroEngine& getMacroEngine() const = 0;

    /**
     * @brief Load preset
     * @param preset Preset to load
     * @return true if loaded successfully
     */
    virtual bool loadPreset(const ZenithInstrumentPreset& preset) = 0;

    /**
     * @brief Get current state as preset
     * @return Current preset
     */
    virtual ZenithInstrumentPreset getCurrentPreset() const = 0;

    //==========================================================================
    // AudioProcessor implementation
    //==========================================================================

    const juce::String getName() const override
    {
        return getInstrumentMetadata().name;
    }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override
    {
        auto preset = getCurrentPreset();
        auto tree = preset.toValueTree();
        std::unique_ptr<juce::XmlElement> xml(tree.createXml());
        if (xml != nullptr)
            copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
        if (xml != nullptr)
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            auto preset = ZenithInstrumentPreset::fromValueTree(tree);
            loadPreset(preset);
        }
    }

    bool hasEditor() const override { return true; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithInstrumentProcessor)
};

} // namespace zenith
