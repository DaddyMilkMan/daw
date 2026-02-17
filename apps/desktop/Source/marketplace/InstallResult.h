/*
  ==============================================================================
    PluginMarketplace.h
    Plugin marketplace integration with download and management
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace zenith {
namespace marketplace {

// Plugin information
    struct InstallResult {
        bool success = false;
        juce::String message;
        juce::String installedPath;
    };

    PluginInstaller();
    ~PluginInstaller() = default;

    // Installation
    InstallResult installPlugin(const juce::File& installerFile, const juce::String& pluginId);
    bool uninstallPlugin(const juce::String& pluginId);

    // Validation
    bool validateInstaller(const juce::File& installerFile);
    bool verifySignature(const juce::File& file, const juce::String& signature);

    // Paths
    juce::File getVST3Path();
    juce::File getAUPath();
    juce::File getAAXPath();
    juce::File getStandalonePath();
    juce::File getCustomPath(const juce::String& pluginId);

private:
    bool runWindowsInstaller(const juce::File& installer);
    bool runMacInstaller(const juce::File& installer);
    bool runLinuxInstaller(const juce::File& installer);

    bool copyVST3Plugin(const juce::File& source, const juce::File& destination);
    bool copyAUPlugin(const juce::File& source, const juce::File& destination);

    void registerPlugin(const juce::String& pluginId, const juce::File& path);
    void unregisterPlugin(const juce::String& pluginId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInstaller)
};

} // namespace marketplace
} // namespace zenith
