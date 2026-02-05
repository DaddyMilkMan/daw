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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    PluginBlacklist.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Persistent blacklist for plugins that crash during scanning.
    Prevents repeated crashes by skipping known-problematic plugins.


  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <mutex>
#include <set>

namespace zenith {

//==============================================================================
/**
    Manages a persistent blacklist of plugins that have crashed during scanning.
    
    The blacklist is stored in the application data directory as an XML file
    and persists across sessions. This prevents Zenith from repeatedly
    attempting to scan plugins known to cause crashes.
    
    Features:
    - Thread-safe access
    - Automatic persistence to disk
    - Crash count tracking (for retry logic)
    - User-managed whitelist/blacklist
    - Rich metadata (timestamp, error type, etc.)
*/
class PluginBlacklist
{
public:
    //==============================================================================
    PluginBlacklist();
    ~PluginBlacklist();
    
    //==============================================================================
    // Blacklist Management
    //==============================================================================
    
    /**
     * @brief Add a plugin to the blacklist
     * 
     * @param filePath Path to the plugin file
     * @param errorType Type of error (crash, timeout, exception, etc.)
     * @param errorMessage Detailed error message
     */
    void addToBlacklist(const juce::String& filePath, 
                        const juce::String& errorType = "crash",
                        const juce::String& errorMessage = "");
    
    /**
     * @brief Remove a plugin from the blacklist
     * 
     * @param filePath Path to the plugin file
     */
    void removeFromBlacklist(const juce::String& filePath);
    
    /**
     * @brief Check if a plugin is blacklisted
     * 
     * @param filePath Path to the plugin file
     * @return true if blacklisted
     */
    bool isBlacklisted(const juce::String& filePath) const;
    
    /**
     * @brief Record a failed scan attempt (for retry logic)
     * 
     * Increments the crash count for this plugin. After 3 failures,
     * the plugin is automatically blacklisted.
     * 
     * @param filePath Path to the plugin file
     * @param errorType Type of error
     * @param errorMessage Detailed error message
     * @return true if plugin should be blacklisted (3+ failures)
     */
    bool recordFailure(const juce::String& filePath,
                       const juce::String& errorType = "crash",
                       const juce::String& errorMessage = "");
    
    /**
     * @brief Clear all blacklist entries
     */
    void clearBlacklist();
    
    //==============================================================================
    // Statistics & Reporting
    //==============================================================================
    
    /**
     * @brief Get the number of blacklisted plugins
     */
    int getBlacklistCount() const;
    
    /**
     * @brief Get all blacklisted plugin paths
     */
    juce::StringArray getBlacklistedPaths() const;
    
    /**
     * @brief Get detailed info about a blacklisted plugin
     */
    struct BlacklistEntry {
        juce::String filePath;
        juce::String errorType;
        juce::String errorMessage;
        juce::Time timestamp;
        int failureCount = 0;
        bool userBlacklisted = false;
        
        juce::String getSummary() const;
    };
    
    /**
     * @brief Get detailed entry for a blacklisted plugin
     * @return Optional entry (empty if not blacklisted)
     */
    std::optional<BlacklistEntry> getEntry(const juce::String& filePath) const;
    
    /**
     * @brief Get all blacklist entries
     */
    std::vector<BlacklistEntry> getAllEntries() const;
    
    //==============================================================================
    // Persistence
    //==============================================================================
    
    /**
     * @brief Load blacklist from disk
     * Called automatically on construction
     */
    void loadFromDisk();
    
    /**
     * @brief Save blacklist to disk
     * Called automatically when modified
     */
    void saveToDisk();
    
    /**
     * @brief Get the blacklist file location
     */
    static juce::File getBlacklistFile();
    
    //==============================================================================
    // User Preferences
    //==============================================================================
    
    /**
     * @brief Temporarily skip blacklisting (for "Scan All" with retry)
     */
    void setIgnoreBlacklist(bool ignore) { ignoreBlacklist_ = ignore; }
    bool isIgnoringBlacklist() const { return ignoreBlacklist_; }
    
private:
    mutable std::mutex mutex_;
    std::map<juce::String, BlacklistEntry> entries_;
    bool ignoreBlacklist_ = false;
    bool needsSave_ = false;
    
    //==============================================================================
    // Serialization
    //==============================================================================
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBlacklist)
};

} // namespace zenith
