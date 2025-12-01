/*
  ==============================================================================

    PluginHost.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 3: VST3 Plugin Hosting MVP

    Plugin hosting manager implementation

  ==============================================================================
*/

#include "PluginHost.h"

namespace zenith {

//==============================================================================
PluginHost::PluginHost()
{
    DBG("PluginHost: Initializing...");

    // Add VST3 format
    formatManager.addDefaultFormats();

    // Get VST3 format pointer for later use
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format->getName().contains("VST3"))
        {
            vst3Format = format;
            DBG("PluginHost: VST3 format registered");
            break;
        }
    }

    if (vst3Format == nullptr)
    {
        DBG("PluginHost: WARNING - VST3 format not available!");
    }

    DBG("PluginHost: Initialized");
}

PluginHost::~PluginHost()
{
    DBG("PluginHost: Destructor");
}

//==============================================================================
// Plugin Scanning
//==============================================================================

int PluginHost::scanDefaultLocations(bool async)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    if (async) {
        // CRITICAL FIX #2: Real async scanning
        DBG("PluginHost: Starting ASYNC scan...");
        
        scanThread_ = std::make_unique<juce::Thread>("PluginScanner");
        scanThread_->startThread([this]() {
            performScan();
            
            // Callback on message thread
            juce::MessageManager::callAsync([this]() {
                int count = knownPlugins.getNumTypes();
                DBG("PluginHost: Async scan complete - found " + juce::String(count) + " plugins");
                if (scanCompleteCallback_) {
                    scanCompleteCallback_(count);
                }
            });
        });
        
        return 0; // Return immediately, callback will provide count
    }
    
    // Synchronous scanning (original code)
    return performScan();
}

int PluginHost::performScan()
{
    if (vst3Format == nullptr)
    {
        DBG("PluginHost: Cannot scan - VST3 format not available");
        return 0;
    }

    DBG("PluginHost: Scanning default VST3 locations...");

    // Get default VST3 search paths
    auto defaultLocations = vst3Format->getDefaultLocationsToSearch();

    int foundCount = 0;

    // Scan each location
    for (int i = 0; i < defaultLocations.getNumPaths(); ++i)
    {
        auto location = defaultLocations[i];
        DBG("PluginHost: Scanning " + location.getFullPathName());

        if (!location.exists())
        {
            DBG("PluginHost: Location does not exist, skipping");
            continue;
        }

        // Use KnownPluginList to scan and add plugins
        juce::PluginDirectoryScanner scanner(
            knownPlugins,
            *vst3Format,
            defaultLocations,
            true,  // Search recursively
            juce::File()  // No dead-mans pedal file
        );

        juce::String pluginBeingScanned;

        while (scanner.scanNextFile(true, pluginBeingScanned))
        {
            DBG("PluginHost: Scanning " + pluginBeingScanned);
        }

        foundCount = knownPlugins.getNumTypes();
    }

    DBG("PluginHost: Scan complete - found " + juce::String(foundCount) + " plugins");

    return foundCount;
}

bool PluginHost::scanPath(const juce::File& path)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (vst3Format == nullptr)
    {
        DBG("PluginHost: Cannot scan - VST3 format not available");
        return false;
    }

    if (!path.exists())
    {
        DBG("PluginHost: Path does not exist: " + path.getFullPathName());
        return false;
    }

    DBG("PluginHost: Scanning path: " + path.getFullPathName());

    juce::FileSearchPath searchPath(path.getFullPathName());

    juce::PluginDirectoryScanner scanner(
        knownPlugins,
        *vst3Format,
        searchPath,
        true,  // Search recursively
        juce::File()  // No dead-mans pedal file
    );

    juce::String pluginBeingScanned;

    while (scanner.scanNextFile(true, pluginBeingScanned))
    {
        DBG("PluginHost: Scanning " + pluginBeingScanned);
    }

    DBG("PluginHost: Path scan complete - total plugins: " + juce::String(knownPlugins.getNumTypes()));

    return true;
}

void PluginHost::clearPluginList()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    DBG("PluginHost: Clearing plugin list");
    knownPlugins.clear();
}

//==============================================================================
// Plugin Access
//==============================================================================

juce::Array<juce::PluginDescription> PluginHost::getPluginDescriptions() const
{
    juce::Array<juce::PluginDescription> descriptions;

    for (const auto& desc : knownPlugins.getTypes())
    {
        descriptions.add(desc);
    }

    return descriptions;
}

bool PluginHost::findPluginDescription(const juce::String& identifier, juce::PluginDescription& outDescription) const
{
    for (const auto& desc : knownPlugins.getTypes())
    {
        if (desc.createIdentifierString() == identifier)
        {
            outDescription = desc;
            return true;
        }
    }

    return false;
}

//==============================================================================
// Plugin Instantiation
//==============================================================================

std::unique_ptr<juce::AudioPluginInstance> PluginHost::createInstance(
    const juce::PluginDescription& description,
    double sampleRate,
    int blockSize,
    juce::String& errorMessage)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    DBG("PluginHost: Creating instance of " + description.name);

    errorMessage.clear();

    // Create plugin instance (BLOCKING call)
    juce::String loadError;
    auto instance = formatManager.createPluginInstance(
        description,
        sampleRate,
        blockSize,
        loadError);

    if (instance == nullptr)
    {
        errorMessage = "Failed to load plugin: " + loadError;
        DBG("PluginHost: " + errorMessage);
        return nullptr;
    }

    // Prepare the plugin for playback
    instance->prepareToPlay(sampleRate, blockSize);
    instance->setNonRealtime(false);

    DBG("PluginHost: Plugin instance created successfully");

    return instance;
}

std::unique_ptr<juce::AudioPluginInstance> PluginHost::createInstance(
    const juce::String& identifier,
    double sampleRate,
    int blockSize,
    juce::String& errorMessage)
{
    errorMessage.clear();

    // Find the plugin description
    juce::PluginDescription description;
    if (!findPluginDescription(identifier, description))
    {
        errorMessage = "Plugin not found: " + identifier;
        DBG("PluginHost: " + errorMessage);
        return nullptr;
    }

    // Create instance using the description
    return createInstance(description, sampleRate, blockSize, errorMessage);
}

} // namespace zenith
