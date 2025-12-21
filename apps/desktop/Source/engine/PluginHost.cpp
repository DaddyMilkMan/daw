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
    cancelScan();
    if (scanThread_.joinable())
        scanThread_.join();
}

//==============================================================================
// Plugin Scanning
//==============================================================================

// Internal scanning logic - runs on ANY thread
int PluginHost::scanInternal(std::function<void(const juce::String&)> onProgress)
{
    if (vst3Format == nullptr)
        return 0;

    // Get default VST3 search paths
    auto searchPaths = vst3Format->getDefaultLocationsToSearch();
    
    // Add custom paths
    for (const auto& path : customSearchPaths)
    {
        searchPaths.add(path);
    }

    int foundCount = 0;

    // Scan each location
    for (int i = 0; i < searchPaths.getNumPaths(); ++i)
    {
        if (shouldCancel_) break;

        auto location = searchPaths[i];
        if (onProgress) onProgress("Scanning: " + location.getFullPathName());

        if (!location.exists()) continue;

        // Get all files in this location
        juce::Array<juce::File> files;
        location.findChildFiles(files, juce::File::findFiles, true, "*.vst3");

        for (const auto& file : files)
        {
            if (shouldCancel_) break;
            
            if (!knownPlugins.getBlacklistedFiles().contains(file.getFullPathName()))
            {
                if (onProgress) onProgress("Scanning: " + file.getFileName());

                juce::PluginDescription desc;
                if (scanPluginOutOfProcess(file, desc))
                {
                    knownPlugins.addType(desc);
                }
                else
                {
                    DBG("PluginHost: Blacklisting " + file.getFullPathName() + " due to scan failure");
                    knownPlugins.addToBlacklist(file.getFullPathName());
                }
            }
        }

        foundCount = knownPlugins.getNumTypes();
    }
    
    return foundCount;
}

int PluginHost::scanDefaultLocations(bool async)
{
    if (async) {
        scanAsync([](int, int, const juce::String&){});
        return 0;
    }

    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    DBG("PluginHost: Scanning default VST3 locations (Synchronous)...");
    int count = scanInternal([](const juce::String& msg) { DBG(msg); });
    DBG("PluginHost: Scan complete - found " + juce::String(count) + " plugins");

    return count;
}

void PluginHost::scanAsync(std::function<void(int, int, const juce::String&)> progressCallback)
{
    if (isScanning_) return;
    
    isScanning_ = true;
    shouldCancel_ = false;
    
    scanThread_ = std::thread([this, progressCallback]() {
        DBG("PluginHost: Starting async scan...");
        
        int count = scanInternal([progressCallback](const juce::String& name) {
            juce::MessageManager::callAsync([progressCallback, name]() {
                progressCallback(0, 0, name);
            });
        });
        
        isScanning_ = false;
        
        juce::MessageManager::callAsync([progressCallback, count]() {
            progressCallback(100, count, "Done");
        });
        
        DBG("PluginHost: Async scan complete.");
    });
    
    // Don't detach - destructor will join to ensure proper cleanup
    // scanThread_.detach() was causing use-after-free if PluginHost destroyed during scan
}

void PluginHost::cancelScan()
{
    shouldCancel_ = true;
}

bool PluginHost::isScanningPlugins() const
{
    return isScanning_;
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

    juce::Array<juce::File> files;
    if (path.isDirectory())
        path.findChildFiles(files, juce::File::findFiles, true, "*.vst3");
    else
        files.add(path);

    for (const auto& file : files)
    {
        if (!knownPlugins.getBlacklistedFiles().contains(file.getFullPathName()))
        {
            DBG("PluginHost: Out-of-process scan for " + file.getFileName());
            juce::PluginDescription desc;
            if (scanPluginOutOfProcess(file, desc))
            {
                knownPlugins.addType(desc);
            }
            else
            {
                DBG("PluginHost: Blacklisting " + file.getFullPathName() + " due to scan failure");
                knownPlugins.addToBlacklist(file.getFullPathName());
            }
        }
    }

    DBG("PluginHost: Path scan complete - total plugins: " + juce::String(knownPlugins.getNumTypes()));

    return true;
}

bool PluginHost::scanPluginOutOfProcess(const juce::File& file, juce::PluginDescription& result)
{
    // Use the current executable itself as the scanner
    juce::File scannerExe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

    if (!scannerExe.existsAsFile())
    {
        DBG("PluginHost: ERROR - Current executable not found!");
        return false;
    }

    juce::StringArray args;
    args.add(scannerExe.getFullPathName());
    args.add("--scan-plugin");
    args.add(file.getFullPathName());

    juce::ChildProcess child;
    if (child.start(args))
    {
        if (child.waitForProcessToFinish(5000))
        {
            if (child.getExitCode() == 0)
            {
                juce::String output = child.readAllProcessOutput();
                auto var = juce::JSON::parse(output);
                
                if (var.isObject())
                {
                    result.name = var["name"];
                    result.descriptiveName = var["descriptiveName"];
                    result.pluginFormatName = var["pluginFormatName"];
                    result.category = var["category"];
                    result.manufacturerName = var["manufacturerName"];
                    result.version = var["version"];
                    result.fileOrIdentifier = var["fileOrIdentifier"];
                    result.lastFileModTime = juce::Time((juce::int64)var["lastFileModTime"]);
                    result.lastInfoUpdateTime = juce::Time((juce::int64)var["lastInfoUpdateTime"]);
                    result.uniqueId = (int)var["uniqueId"];
                    result.isInstrument = (bool)var["isInstrument"];
                    result.numInputChannels = (int)var["numInputChannels"];
                    result.numOutputChannels = (int)var["numOutputChannels"];
                    result.hasSharedContainer = (bool)var["hasSharedContainer"];
                    return true;
                }
            }
        }
        else
        {
            DBG("PluginHost: Scan TIMEOUT or CRASH for " + file.getFullPathName());
            child.kill();
        }
    }

    return false;
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

std::unique_ptr<juce::AudioPluginInstance> PluginHost::createPlugin(const juce::PluginDescription& description)
{
    juce::String errorMessage;
    // Use default sample rate and block size if not specified
    // Ideally these should come from the Engine, but for state restoration this is often acceptable initially
    return createInstance(description, 44100.0, 512, errorMessage);
}

//==============================================================================
// Custom Search Paths
//==============================================================================

void PluginHost::addSearchPath(const juce::String& path)
{
    if (!customSearchPaths.contains(path))
        customSearchPaths.add(path);
}

void PluginHost::removeSearchPath(int index)
{
    if (index >= 0 && index < customSearchPaths.size())
        customSearchPaths.remove(index);
}

juce::StringArray PluginHost::getSearchPaths() const
{
    return customSearchPaths;
}

int PluginHost::scanAll(bool async)
{
    return scanDefaultLocations(async);
}

//==============================================================================
// XML Caching
//==============================================================================

bool PluginHost::loadPluginList()
{
    juce::File cacheFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("plugin_cache.xml");

    if (!cacheFile.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(cacheFile);
    if (xml != nullptr && xml->hasTagName("KNOWN_PLUGINS"))
    {
        knownPlugins.recreateFromXml(*xml);
        DBG("PluginHost: Loaded " + juce::String(knownPlugins.getNumTypes()) + " plugins from cache");
        return true;
    }

    return false;
}

void PluginHost::savePluginList()
{
    juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW");

    if (!appDataDir.exists())
        appDataDir.createDirectory();

    juce::File cacheFile = appDataDir.getChildFile("plugin_cache.xml");

    auto xml = knownPlugins.createXml();
    if (xml != nullptr)
    {
        xml->writeTo(cacheFile);
        DBG("PluginHost: Saved plugin cache to " + cacheFile.getFullPathName());
    }
}

} // namespace zenith
