#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <map>

namespace zenith {
namespace instruments {

/**
 * @brief Registry for built-in Zenith instruments
 *
 * Provides a factory pattern for creating instrument instances.
 * All instruments implement juce::AudioProcessor.
 *
 * Usage:
 *   auto sampler = InstrumentRegistry::createInstrument("zenith_sampler");
 *   auto patches = InstrumentRegistry::getInstrumentsByCategory("Sampler");
 */
class InstrumentRegistry
{
public:
    /**
     * @brief Information about a registered instrument
     */
    struct InstrumentInfo
    {
        juce::String id;           // Unique identifier (e.g., "zenith_sampler")
        juce::String name;         // Display name (e.g., "Zenith Sampler")
        juce::String category;     // Category (e.g., "Sampler", "Synth", "Drum")
        juce::String description;  // Brief description
        juce::String version;      // Version string

        std::function<std::unique_ptr<juce::AudioProcessor>()> factory;
    };

    /**
     * @brief Get the singleton instance
     */
    static InstrumentRegistry& getInstance()
    {
        static InstrumentRegistry instance;
        return instance;
    }

    /**
     * @brief Register a new instrument
     *
     * @param info Instrument information and factory function
     * @return bool True if registered successfully, false if ID already exists
     */
    bool registerInstrument(const InstrumentInfo& info)
    {
        if (info.id.isEmpty() || info.factory == nullptr)
            return false;

        if (instruments.find(info.id.toStdString()) != instruments.end())
            return false; // Already registered

        instruments[info.id.toStdString()] = info;
        return true;
    }

    /**
     * @brief Create an instrument instance by ID
     *
     * @param instrumentId Unique instrument identifier
     * @return std::unique_ptr<juce::AudioProcessor> New instrument instance, or nullptr if not found
     */
    std::unique_ptr<juce::AudioProcessor> createInstrument(const juce::String& instrumentId) const
    {
        auto it = instruments.find(instrumentId.toStdString());
        if (it == instruments.end())
            return nullptr;

        return it->second.factory();
    }

    /**
     * @brief Get information about a specific instrument
     *
     * @param instrumentId Unique instrument identifier
     * @return const InstrumentInfo* Pointer to info, or nullptr if not found
     */
    const InstrumentInfo* getInstrumentInfo(const juce::String& instrumentId) const
    {
        auto it = instruments.find(instrumentId.toStdString());
        if (it == instruments.end())
            return nullptr;

        return &it->second;
    }

    /**
     * @brief Get all registered instruments
     *
     * @return std::vector<InstrumentInfo> List of all instruments
     */
    std::vector<InstrumentInfo> getAllInstruments() const
    {
        std::vector<InstrumentInfo> result;
        for (const auto& [id, info] : instruments)
        {
            result.push_back(info);
        }
        return result;
    }

    /**
     * @brief Get instruments by category
     *
     * @param category Category filter (e.g., "Sampler", "Synth")
     * @return std::vector<InstrumentInfo> Filtered list
     */
    std::vector<InstrumentInfo> getInstrumentsByCategory(const juce::String& category) const
    {
        std::vector<InstrumentInfo> result;
        for (const auto& [id, info] : instruments)
        {
            if (info.category == category)
            {
                result.push_back(info);
            }
        }
        return result;
    }

    /**
     * @brief Get all unique categories
     *
     * @return juce::StringArray List of categories
     */
    juce::StringArray getCategories() const
    {
        juce::StringArray categories;
        for (const auto& [id, info] : instruments)
        {
            if (!categories.contains(info.category))
            {
                categories.add(info.category);
            }
        }
        return categories;
    }

    /**
     * @brief Check if an instrument is registered
     *
     * @param instrumentId Unique instrument identifier
     * @return bool True if registered
     */
    bool isRegistered(const juce::String& instrumentId) const
    {
        return instruments.find(instrumentId.toStdString()) != instruments.end();
    }

private:
    InstrumentRegistry() = default;
    ~InstrumentRegistry() = default;

    // Prevent copying
    InstrumentRegistry(const InstrumentRegistry&) = delete;
    InstrumentRegistry& operator=(const InstrumentRegistry&) = delete;

    std::map<std::string, InstrumentInfo> instruments;
};

/**
 * @brief Helper macro for registering instruments at startup
 *
 * Usage:
 *   REGISTER_INSTRUMENT("zenith_sampler", "Zenith Sampler", "Sampler",
 *                       "Sample-based instrument", "1.0.0",
 *                       []() { return std::make_unique<ZenithSampler>(); })
 */
#define REGISTER_ZENITH_INSTRUMENT(id, name, category, description, version, factory) \
    namespace { \
        struct InstrumentRegistrar_##id { \
            InstrumentRegistrar_##id() { \
                zenith::instruments::InstrumentRegistry::InstrumentInfo info; \
                info.id = id; \
                info.name = name; \
                info.category = category; \
                info.description = description; \
                info.version = version; \
                info.factory = factory; \
                zenith::instruments::InstrumentRegistry::getInstance().registerInstrument(info); \
            } \
        }; \
        static InstrumentRegistrar_##id registrar_##id; \
    }

} // namespace instruments
} // namespace zenith
