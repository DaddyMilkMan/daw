#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace zenith {

/**
 * @brief Describes a built-in instrument plugin
 *
 * This structure contains metadata and a factory function for creating
 * instances of built-in instruments. Built-in instruments are internal
 * AudioProcessors that ship with Zenith DAW.
 */
struct InstrumentDefinition
{
    /** Unique identifier (e.g., "zenith_poly_synth") */
    juce::String id;

    /** Human-readable name (e.g., "Zenith Poly Synth") */
    juce::String name;

    /** Category for UI grouping (e.g., "Synth", "Sampler", "Drum Machine") */
    juce::String category;

    /** Voice mode hint: "poly", "mono", "paraphonic", etc. */
    juce::String voiceMode;

    /** Short description of the instrument */
    juce::String description;

    /** Factory function to create an instance of this instrument */
    std::function<std::unique_ptr<juce::AudioProcessor>()> factory;

    InstrumentDefinition() = default;

    InstrumentDefinition(juce::String instrumentId,
                        juce::String instrumentName,
                        juce::String instrumentCategory,
                        juce::String instrumentVoiceMode,
                        juce::String instrumentDescription,
                        std::function<std::unique_ptr<juce::AudioProcessor>()> factoryFunc)
        : id(std::move(instrumentId))
        , name(std::move(instrumentName))
        , category(std::move(instrumentCategory))
        , voiceMode(std::move(instrumentVoiceMode))
        , description(std::move(instrumentDescription))
        , factory(std::move(factoryFunc))
    {
    }
};

/**
 * @brief Registry for built-in instrument plugins
 *
 * This singleton manages the collection of built-in instruments available
 * in Zenith DAW. Built-in instruments are registered at startup and can
 * be instantiated by ID.
 *
 * Thread-safety: This class is designed to be initialized once at startup
 * and then accessed read-only from multiple threads. Registration must
 * happen before any audio processing begins.
 */
class InstrumentRegistry
{
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the global InstrumentRegistry
     */
    static InstrumentRegistry& getInstance();

    /**
     * @brief Register a built-in instrument
     *
     * This should be called during application initialization, before
     * any audio processing begins. Registering an instrument with an
     * ID that already exists will replace the previous definition.
     *
     * @param definition The instrument definition to register
     */
    void registerInstrument(const InstrumentDefinition& definition);

    /**
     * @brief Create an instance of an instrument by ID
     *
     * This is thread-safe for read operations after initialization.
     *
     * @param instrumentId The unique ID of the instrument to create
     * @return A new AudioProcessor instance, or nullptr if ID not found
     */
    std::unique_ptr<juce::AudioProcessor> createInstrument(const juce::String& instrumentId) const;

    /**
     * @brief Get metadata for an instrument by ID
     *
     * @param instrumentId The unique ID of the instrument
     * @return Pointer to the definition, or nullptr if ID not found
     */
    const InstrumentDefinition* getDefinition(const juce::String& instrumentId) const;

    /**
     * @brief Get all registered instrument definitions
     *
     * @return Vector of all registered instrument definitions
     */
    std::vector<InstrumentDefinition> getAllDefinitions() const;

    /**
     * @brief Get all instrument IDs in a specific category
     *
     * @param category The category to filter by (e.g., "Synth")
     * @return Vector of instrument IDs in that category
     */
    std::vector<juce::String> getInstrumentsByCategory(const juce::String& category) const;

    /**
     * @brief Check if an instrument ID is registered
     *
     * @param instrumentId The ID to check
     * @return true if the instrument is registered
     */
    bool hasInstrument(const juce::String& instrumentId) const;

    /**
     * @brief Clear all registered instruments
     *
     * This is primarily for testing. Should not be called during normal operation.
     */
    void clear();

private:
    InstrumentRegistry() = default;
    ~InstrumentRegistry() = default;

    // Non-copyable, non-movable
    InstrumentRegistry(const InstrumentRegistry&) = delete;
    InstrumentRegistry& operator=(const InstrumentRegistry&) = delete;
    InstrumentRegistry(InstrumentRegistry&&) = delete;
    InstrumentRegistry& operator=(InstrumentRegistry&&) = delete;

    std::unordered_map<juce::String, InstrumentDefinition> instruments_;
};

} // namespace zenith
