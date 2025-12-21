/*
  ==============================================================================

    PluginHost.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 3: VST3 Plugin Hosting MVP
    Updated: 2025-12-10 - Robust Plugin Management (Crash Recovery, Serialization)

    Engine-level plugin hosting manager

    Responsibilities:
    - Scan for VST3 plugins (and AudioUnit on macOS in future)
    - Maintain KnownPluginList of available plugins
    - Create plugin instances on demand
    - Safe scanning with crash recovery (dead-man's pedal)
    - Persist known plugins to avoid rescanning every startup
    - Blacklist crashed plugins

    Thread Safety:
    - All methods are MESSAGE THREAD ONLY except where noted
    - Plugin scanning runs on a background thread
    - UI updates are marshalled to message thread

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
#include <set>
#include <mutex>
#include <thread>

namespace zenith {

//==============================================================================
/**
    Plugin hosting manager for Zenith DAW.

    This class manages VST3 plugin discovery and instantiation with robust
    crash recovery. Key features:
    
    1. **Dead-Man's Pedal**: Before scanning each plugin, we write its path
       to a temp file. If we crash, that file tells us which plugin caused it.
    
    2. **Blacklist**: Plugins that crash during scan are added to a blacklist
       and skipped on future scans.
    
    3. **Persistence**: Known plugins are saved to XML so we don't rescan
       every startup.
    
    4. **Async Scanning**: Scanning runs on a background thread with progress
       callbacks to keep the UI responsive.
*/
class PluginHost
{
public:
    //==============================================================================
    PluginHost();
    ~PluginHost();

    //==============================================================================
    // Initialization & Persistence
    //==============================================================================
    
    /**
     * @brief Load known plugins from disk (call on startup)
     * 
     * This loads:
     * - known_plugins.xml - Previously scanned plugins
     * - crashed_plugins.txt - Blacklisted plugins that crashed during scan
     * - scan_in_progress.tmp - Dead-man's pedal to detect previous crash
     * 
     * @return Number of plugins loaded from cache
     */
    int loadFromDisk();
    
    /**
     * @brief Save known plugins to disk
     * 
     * Saves the current known plugins list to known_plugins.xml
     */
    void saveToDisk();

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
     * @param async If true, scan asynchronously (recommended)
     * @return Number of plugins found (0 if async)
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
    const juce::KnownPluginList& getKnownPlugins() const { return knownPlugins_; }
    juce::KnownPluginList& getKnownPlugins() { return knownPlugins_; }

    /**
     * @brief Get the plugin format manager
     */
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager_; }

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
    // Async Scanning with Progress
    //==============================================================================
    
    /**
     * @brief Start asynchronous plugin scan
     * 
     * @param progressCallback Called with (percentComplete, pluginsFound, currentPluginName)
     *                         percentComplete = 100 means scan is complete
     */
    void scanAsync(std::function<void(int, int, const juce::String&)> progressCallback);
    
    /**
     * @brief Cancel an ongoing async scan
     */
    void cancelScan();
    
    /**
     * @brief Check if a scan is currently in progress
     */
    bool isScanningPlugins() const;
    
    /**
     * @brief Get the name of the plugin currently being scanned
     */
    juce::String getCurrentlyScanning() const;

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
     * @brief Save custom search paths to settings
     */
    void saveSearchPaths();
    
    /**
     * @brief Load custom search paths from settings
     */
    void loadSearchPaths();
    
    /**
     * @brief Scan both default locations and custom paths
     */
    int scanAll(bool async = false);

    //==============================================================================
    // Blacklist Management
    //==============================================================================
    
    /**
     * @brief Get list of blacklisted (crashed) plugins
     */
    juce::StringArray getBlacklistedPlugins() const;
    
    /**
     * @brief Remove a plugin from the blacklist (to retry scanning)
     */
    void removeFromBlacklist(const juce::String& pluginPath);
    
    /**
     * @brief Clear all blacklisted plugins
     */
    void clearBlacklist();
    
    /**
     * @brief Check if a plugin path is blacklisted
     */
    bool isBlacklisted(const juce::String& pluginPath) const;

private:
    //==============================================================================
    // Internal Implementation
    //==============================================================================
    
    // Get app data directory for plugin cache files
    juce::File getPluginCacheDirectory() const;
    
    // File paths for persistence
    juce::File getKnownPluginsFile() const;
    juce::File getBlacklistFile() const;
    juce::File getDeadMansPedalFile() const;
    
    // Handle dead-man's pedal (crash recovery)
    void checkForCrashedScan();
    void writeDeadMansPedal(const juce::String& currentPlugin);
    void clearDeadMansPedal();
    
    // Load/save blacklist
    void loadBlacklist();
    void saveBlacklist();
    
    // Internal scanning logic (runs on background thread)
    int scanInternal(std::function<void(const juce::String&)> onProgress);
    
    // Safe scan a single plugin file
    bool scanPluginFileSafely(const juce::File& pluginFile,
                               std::function<void(const juce::String&)> onProgress);

    //==============================================================================
    // Member Variables
    //==============================================================================

    // Plugin format manager (owns the VST3 format)
    juce::AudioPluginFormatManager formatManager_;

    // Known plugins list (populated by scanning)
    juce::KnownPluginList knownPlugins_;

    // VST3 format (raw pointer owned by formatManager)
    juce::AudioPluginFormat* vst3Format_ = nullptr;

    // Scanning state
    std::atomic<bool> isScanning_{false};
    std::atomic<bool> shouldCancel_{false};
    std::thread scanThread_;
    juce::String currentlyScanning_;
    mutable std::mutex scanMutex_;
    
    // Blacklist of crashed plugins (paths)
    std::set<juce::String> blacklistedPlugins_;
    mutable std::mutex blacklistMutex_;
    
    // Custom search paths
    juce::StringArray customSearchPaths_;
    
    // Track if we've loaded from disk
    bool loadedFromDisk_ = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
    JUCE_DECLARE_WEAK_REFERENCEABLE(PluginHost)
};

} // namespace zenith
