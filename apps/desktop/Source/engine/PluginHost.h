/*
  ==============================================================================

    PluginHost.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 3: VST3 Plugin Hosting MVP

    Engine-level plugin hosting manager

    Responsibilities:
    - Scan for VST3 plugins (and AudioUnit on macOS in future)
    - Maintain KnownPluginList of available plugins
    - Create plugin instances on demand
    - All operations on MESSAGE THREAD only

    Thread Safety:
    - All methods are MESSAGE THREAD ONLY
    - Plugin scanning is synchronous (blocking) for MVP
    - Plugin instantiation is synchronous (blocking) for MVP

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
/**
    Plugin hosting manager for Zenith DAW.

    This class manages VST3 plugin discovery and instantiation.
    All operations are designed to run on the message thread.
*/
class PluginHost
{
public:
    //==============================================================================
    PluginHost();
    ~PluginHost();

    //==============================================================================
    // Plugin Scanning (MESSAGE THREAD ONLY)
    //==============================================================================

    /**
     * @brief Scan default VST3 locations
     *
     * This will scan:
     * - Windows: C:\Program Files\Common Files\VST3
     * - macOS: ~/Library/Audio/Plug-Ins/VST3, /Library/Audio/Plug-Ins/VST3
     * - Linux: ~/.vst3, /usr/lib/vst3, /usr/local/lib/vst3
     *
     * @param async If true, scan asynchronously (not implemented in MVP)
     * @return Number of plugins found
     */
    int scanDefaultLocations(bool async = false);

    /**
     * @brief Scan a specific file or directory
     *
     * @param path File or directory to scan
     * @return true if scan completed successfully
     */
    bool scanPath(const juce::File& path);

    /**
     * @brief Clear the known plugins list
     */
    void clearPluginList();

    //==============================================================================
    // Plugin Access (MESSAGE THREAD ONLY)
    //==============================================================================

    /**
     * @brief Get the known plugins list (read-only)
     *
     * @return Const reference to known plugins
     */
    const juce::KnownPluginList& getKnownPlugins() const { return knownPlugins; }
    juce::KnownPluginList& getKnownPlugins() { return knownPlugins; }

    /**
     * @brief Get the plugin format manager
     */
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    /**
     * @brief Get plugin descriptions as an array
     *
     * @return Array of plugin descriptions
     */
    juce::Array<juce::PluginDescription> getPluginDescriptions() const;

    /**
     * @brief Find a plugin description by identifier
     *
     * @param identifier Plugin identifier (from PluginDescription::createIdentifierString())
     * @param outDescription Output parameter to receive the description
     * @return true if found, false otherwise
     */
    bool findPluginDescription(const juce::String& identifier, juce::PluginDescription& outDescription) const;

    //==============================================================================
    // Plugin Instantiation (MESSAGE THREAD ONLY)
    //==============================================================================

    /**
     * @brief Create a plugin instance from a description
     *
     * This is a BLOCKING call that may take a few seconds.
     *
     * @param description Plugin description
     * @param sampleRate Sample rate to prepare plugin at
     * @param blockSize Block size to prepare plugin at
     * @param errorMessage Output parameter for error message
     * @return Plugin instance, or nullptr on failure
     */
    std::unique_ptr<juce::AudioPluginInstance> createInstance(
        const juce::PluginDescription& description,
        double sampleRate,
        int blockSize,
        juce::String& errorMessage);

    /**
     * @brief Create a plugin instance from an identifier string
     *
     * @param identifier Plugin identifier string
     * @param sampleRate Sample rate to prepare plugin at
     * @param blockSize Block size to prepare plugin at
     * @param errorMessage Output parameter for error message
     * @return Plugin instance, or nullptr on failure
     */
    std::unique_ptr<juce::AudioPluginInstance> createInstance(
        const juce::String& identifier,
        double sampleRate,
        int blockSize,
        juce::String& errorMessage);

    //==============================================================================
    // Async Scanning
    //==============================================================================
    void scanAsync(std::function<void(int, int, const juce::String&)> progressCallback);
    void cancelScan();
    bool isScanningPlugins() const;

    // Internal helpers
    bool knowsAboutPlugin(const juce::PluginDescription& desc) const;
    void addToKnownPlugins(const juce::PluginDescription& desc);

    // Convenience wrapper
    std::unique_ptr<juce::AudioPluginInstance> createPlugin(const juce::PluginDescription& description);

    //==============================================================================
    // Custom Search Paths
    //==============================================================================
    void addSearchPath(const juce::String& path);
    void removeSearchPath(int index);
    juce::StringArray getSearchPaths() const;
    
    /**
     * @brief Scan both default locations and custom paths
     */
    int scanAll(bool async = false);

private:
    // Internal scanning logic
    int scanInternal(std::function<void(const juce::String&)> onProgress);

    //==============================================================================
    // Member Variables
    //==============================================================================

    // Plugin format manager (owns the VST3 format)
    juce::AudioPluginFormatManager formatManager;

    // Known plugins list (populated by scanning)
    juce::KnownPluginList knownPlugins;

    // VST3 format (raw pointer owned by formatManager)
    juce::AudioPluginFormat* vst3Format = nullptr;

    // Scanning state
    std::atomic<bool> isScanning_{false};
    std::atomic<bool> shouldCancel_{false};
    std::thread scanThread_;
    
    // Custom search paths
    juce::StringArray customSearchPaths;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
    JUCE_DECLARE_WEAK_REFERENCEABLE(PluginHost)
};

} // namespace zenith

