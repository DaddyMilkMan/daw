/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <thread>
#include <optional>

#include "RTSafetyChecks.h"

namespace zenith {

// Forward declaration
class PluginBlacklist;

//==============================================================================
/**
    Plugin hosting manager for Zenith DAW.

    This class manages VST3 plugin discovery and instantiation.
    All operations are designed to run on the message thread.

    Thread Safety:
    - ALL public methods are MESSAGE THREAD ONLY (ZENITH_NONRT_THREAD).
    - Plugin instances must be prepared on the message thread and then
      used on the audio thread without any further PluginHost calls.
    - Never call any PluginHost method from audioDeviceIOCallback or
      from code annotated ZENITH_RT_THREAD / ZENITH_RT_SAFE.
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
     * @param async If true, scan asynchronously (use scanAsync() for progress callbacks)
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

    //==============================================================================
    // Blacklist Management
    //==============================================================================
    
    /**
     * @brief Get the plugin blacklist manager
     */
    PluginBlacklist& getBlacklist() { return *blacklist_; }
    const PluginBlacklist& getBlacklist() const { return *blacklist_; }
    
    /**
     * @brief Scan results for UI feedback
     */
    struct ScanResult {
        juce::String filePath;
        bool success = false;
        juce::String errorType;  // "success", "crash", "timeout", "blacklisted", "parse_error"
        juce::String errorMessage;
        juce::PluginDescription description;  // Valid only if success=true
        
        bool wasBlacklisted() const { return errorType == "blacklisted"; }
        bool hadError() const { return !success && errorType != "blacklisted"; }
    };
    
    /**
     * @brief Scan with detailed result reporting
     * @return Vector of scan results for each plugin file attempted
     */
    std::vector<ScanResult> scanWithResults(const juce::File& path);
    
    /**
     * @brief Get the last scan results
     */
    const std::vector<ScanResult>& getLastScanResults() const { return lastScanResults_; }
    
    /**
     * @brief Clear the last scan results
     */
    void clearLastScanResults() { lastScanResults_.clear(); }
    
    //==============================================================================
    // Scan Statistics
    //==============================================================================
    
    struct ScanStatistics {
        int totalScanned = 0;
        int found = 0;
        int crashed = 0;
        int timedOut = 0;
        int blacklisted = 0;
        int parseErrors = 0;
        
        juce::String getSummary() const;
    };
    
    ScanStatistics getLastScanStatistics() const;

private:
    // Internal scanning logic
    int scanInternal(std::function<void(const juce::String&)> onProgress);
    
    // Out-of-process helper
    // Returns true if plugin was successfully scanned and added
    bool scanFileOutProcess(const juce::File& file, juce::PluginDescription& result);
    
    // Out-of-process with full result details
    ScanResult scanFileWithDetails(const juce::File& file);

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
    // Managed thread for plugin scanning
    std::thread scanThread_;
    
    // Custom search paths
    juce::StringArray customSearchPaths;
    
    // Blacklist manager
    std::unique_ptr<PluginBlacklist> blacklist_;
    
    // Last scan results for UI feedback
    std::vector<ScanResult> lastScanResults_;
    mutable std::mutex scanResultsMutex_;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
    JUCE_DECLARE_WEAK_REFERENCEABLE(PluginHost)
};

} // namespace zenith

