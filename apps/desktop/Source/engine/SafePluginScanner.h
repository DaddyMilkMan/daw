/*
  ==============================================================================
    SafePluginScanner.h
    Created: 2026-02-23
    Author:  Zenith DAW
    Robust out-of-process VST3 plugin scanner with watchdog, blacklist, and crash recovery.
  ==============================================================================
*/
#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

struct BlacklistEntry {
    juce::String path;
    juce::String reason;
    juce::Time timestamp;
};

class PluginBlacklistManager {
public:
    PluginBlacklistManager();
    ~PluginBlacklistManager() = default;

    void addToBlacklist(const juce::String& pluginPath, const juce::String& reason);
    bool isBlacklisted(const juce::String& pluginPath) const;
    void removeFromBlacklist(const juce::String& pluginPath);
    void saveBlacklist();
    void loadBlacklist();

    juce::Array<BlacklistEntry> getEntries() const { return entries; }

private:
    juce::Array<BlacklistEntry> entries;
    juce::File getBlacklistFile() const;
};

class SafePluginScanner {
public:
    SafePluginScanner();
    ~SafePluginScanner();

    // Returns true if the plugin was successfully scanned.
    // If it crashes or times out, it is automatically blacklisted.
    bool scanPluginOutOfProcess(const juce::File& file, juce::PluginDescription& result);

    PluginBlacklistManager& getBlacklistManager() { return blacklistManager; }

    // Dead man's switch for handling complete DAW crashes during a scan
    void clearScanningState();
    void checkPreviousScanningState();

private:
    PluginBlacklistManager blacklistManager;
    juce::File getScanningStateFile() const;
    
    void setCurrentlyScanning(const juce::String& pluginPath);
    void clearCurrentlyScanning();
};

} // namespace zenith