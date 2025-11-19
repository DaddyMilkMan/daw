/*
  ==============================================================================

    PluginHost.h
    Created: 2025-11-11
    Author:  Zenith DAW

    VST3/AU/AAX plugin scanner and host manager

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Manages plugin scanning, loading, and instantiation.

    The PluginHost provides:
    - Plugin scanning (VST3, AU, AAX, LV2)
    - Plugin database management
    - Plugin instantiation
    - Plugin preset management
    - Thread-safe plugin list access

    This class is designed to be used as a singleton throughout the application.
*/
class PluginHost : public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    /** Get the singleton instance */
    static PluginHost& getInstance();

    /** Delete singleton instance */
    static void deleteInstance();

    //==============================================================================
    /** Plugin information structure */
    struct PluginInfo
    {
        juce::String name;
        juce::String manufacturer;
        juce::String category;
        juce::String fileOrIdentifier;
        juce::PluginDescription description;
        bool isInstrument = false;
        bool hasEditor = false;
        int numInputs = 0;
        int numOutputs = 0;
    };

    //==============================================================================
    /**
        Scan for plugins in default locations.
        This runs asynchronously and sends a change message when complete.
    */
    void scanForPlugins();

    /**
        Scan a specific directory for plugins.
    */
    void scanDirectory(const juce::File& directory);

    /**
        Check if a plugin scan is currently in progress.
    */
    bool isScanning() const { return scanning.load(); }

    /**
        Get the scan progress (0.0 to 1.0).
    */
    float getScanProgress() const { return scanProgress.load(); }

    /**
        Cancel an ongoing plugin scan.
    */
    void cancelScan();

    //==============================================================================
    /**
        Get the list of all known plugins.
    */
    juce::Array<PluginInfo> getPluginList() const;

    /**
        Get plugins filtered by type (instrument, effect).
    */
    juce::Array<PluginInfo> getInstrumentPlugins() const;
    juce::Array<PluginInfo> getEffectPlugins() const;

    /**
        Get plugins filtered by category.
    */
    juce::Array<PluginInfo> getPluginsByCategory(const juce::String& category) const;

    /**
        Search for plugins by name.
    */
    juce::Array<PluginInfo> searchPlugins(const juce::String& searchText) const;

    /**
        Get the total number of known plugins.
    */
    int getNumPlugins() const;

    //==============================================================================
    /**
        Load and instantiate a plugin.
        Returns nullptr if the plugin could not be loaded.
        The caller is responsible for managing the returned plugin's lifetime.
    */
    juce::AudioPluginInstance* loadPlugin(const PluginInfo& info, juce::String& errorMessage);

    /**
        Load a plugin by its description.
    */
    juce::AudioPluginInstance* loadPlugin(const juce::PluginDescription& description, juce::String& errorMessage);

    //==============================================================================
    /**
        Get the known plugin list (KnownPluginList).
    */
    juce::KnownPluginList& getKnownPluginList() { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }

    /**
        Get the plugin format manager.
    */
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    const juce::AudioPluginFormatManager& getFormatManager() const { return formatManager; }

    //==============================================================================
    /**
        Save the plugin list to the user's app data.
    */
    void savePluginList();

    /**
        Load the plugin list from the user's app data.
    */
    void loadPluginList();

    /**
        Get the default plugin list file location.
    */
    juce::File getPluginListFile() const;

    //==============================================================================
    /**
        Get the default VST3 plugin paths for the current platform.
    */
    static juce::StringArray getDefaultVST3Paths();

    /**
        Get the default Audio Unit paths (macOS only).
    */
    static juce::StringArray getDefaultAUPaths();

    /**
        Get the default AAX paths.
    */
    static juce::StringArray getDefaultAAXPaths();

    /**
        Get the default LV2 paths (Linux).
    */
    static juce::StringArray getDefaultLV2Paths();

private:
    //==============================================================================
    PluginHost();
    ~PluginHost();

    //==============================================================================
    static PluginHost* instance;

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

    std::atomic<bool> scanning{false};
    std::atomic<float> scanProgress{0.0f};
    std::atomic<bool> shouldCancelScan{false};

    juce::CriticalSection pluginListLock;

    //==============================================================================
    class ScannerThread : public juce::Thread
    {
    public:
        ScannerThread(PluginHost& host, const juce::File& directory);
        void run() override;

    private:
        PluginHost& owner;
        juce::File directoryToScan;
    };

    std::unique_ptr<ScannerThread> scannerThread;

    //==============================================================================
    void addPluginsFromFormat(juce::AudioPluginFormat& format, const juce::FileSearchPath& paths);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};
