/*
  ==============================================================================

    PluginHost.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 3: VST3 Plugin Hosting MVP
    Updated: 2025-12-10 - Robust Plugin Management (Crash Recovery, Serialization)

    Plugin hosting manager implementation with:
    - Dead-man's pedal for crash recovery
    - XML serialization to avoid rescanning
    - Blacklist management for crashed plugins
    - Safe async scanning with progress callbacks

  ==============================================================================
*/

#include "PluginHost.h"

namespace zenith {

//==============================================================================
// Constants
//==============================================================================
static const char* KNOWN_PLUGINS_FILENAME = "known_plugins.xml";
static const char* BLACKLIST_FILENAME = "crashed_plugins.txt";
static const char* DEAD_MANS_PEDAL_FILENAME = "scan_in_progress.tmp";
static const char* CACHE_FOLDER_NAME = "ZenithAudio";

//==============================================================================
// Constructor / Destructor
//==============================================================================

PluginHost::PluginHost()
{
    DBG("PluginHost: Initializing...");

    // Add VST3 format (and AudioUnit on macOS)
    formatManager_.addDefaultFormats();

    // Get VST3 format pointer for later use
    for (int i = 0; i < formatManager_.getNumFormats(); ++i)
    {
        auto* format = formatManager_.getFormat(i);
        if (format->getName().contains("VST3"))
        {
            vst3Format_ = format;
            DBG("PluginHost: VST3 format registered");
            break;
        }
    }

    if (vst3Format_ == nullptr)
    {
        DBG("PluginHost: WARNING - VST3 format not available!");
    }

    DBG("PluginHost: Initialized");
}


PluginHost::~PluginHost()
{
    DBG("PluginHost: Destructor");
    cancelScan();
    
    // Wait for scan thread to finish
    if (scanThread_.joinable())
        scanThread_.join();
    
    // Save known plugins on exit
    if (loadedFromDisk_)
        saveToDisk();
}

//==============================================================================
// File Path Helpers
//==============================================================================

juce::File PluginHost::getPluginCacheDirectory() const
{
    // Use standard app data location
    // Windows: C:\Users\<user>\AppData\Roaming\ZenithAudio
    // macOS: ~/Library/Application Support/ZenithAudio
    // Linux: ~/.config/ZenithAudio
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile(CACHE_FOLDER_NAME);
}

juce::File PluginHost::getKnownPluginsFile() const
{
    return getPluginCacheDirectory().getChildFile(KNOWN_PLUGINS_FILENAME);
}

juce::File PluginHost::getBlacklistFile() const
{
    return getPluginCacheDirectory().getChildFile(BLACKLIST_FILENAME);
}

juce::File PluginHost::getDeadMansPedalFile() const
{
    return getPluginCacheDirectory().getChildFile(DEAD_MANS_PEDAL_FILENAME);
}

//==============================================================================
// Persistence: Load / Save
//==============================================================================

int PluginHost::loadFromDisk()
{
    DBG("PluginHost: Loading plugin data from disk...");
    
    // Ensure cache directory exists
    auto cacheDir = getPluginCacheDirectory();
    if (!cacheDir.exists())
    {
        cacheDir.createDirectory();
        DBG("PluginHost: Created cache directory: " + cacheDir.getFullPathName());
    }
    
    // Load blacklist first (needed for crash recovery check)
    loadBlacklist();
    
    // Check for crashed scan (dead-man's pedal)
    checkForCrashedScan();
    
    // Load known plugins from XML
    auto pluginsFile = getKnownPluginsFile();
    int loadedCount = 0;
    
    if (pluginsFile.existsAsFile())
    {
        auto xmlStr = pluginsFile.loadFileAsString();
        if (auto xml = juce::XmlDocument::parse(xmlStr))
        {
            knownPlugins_.recreateFromXml(*xml);
            loadedCount = knownPlugins_.getNumTypes();
            DBG("PluginHost: Loaded " + juce::String(loadedCount) + " plugins from cache");
        }
        else
        {
            DBG("PluginHost: Failed to parse known_plugins.xml");
        }
    }
    else
    {
        DBG("PluginHost: No cached plugin list found");
    }
    
    // Load custom search paths
    loadSearchPaths();
    
    loadedFromDisk_ = true;
    return loadedCount;
}

void PluginHost::saveToDisk()
{
    DBG("PluginHost: Saving plugin data to disk...");
    
    // Ensure cache directory exists
    auto cacheDir = getPluginCacheDirectory();
    if (!cacheDir.exists())
        cacheDir.createDirectory();
    
    // Save known plugins to XML
    auto pluginsFile = getKnownPluginsFile();
    if (auto xml = knownPlugins_.createXml())
    {
        if (pluginsFile.replaceWithText(xml->toString()))
        {
            DBG("PluginHost: Saved " + juce::String(knownPlugins_.getNumTypes()) + " plugins to cache");
        }
        else
        {
            DBG("PluginHost: Failed to write known_plugins.xml");
        }
    }
    
    // Save blacklist
    saveBlacklist();
    
    // Save custom search paths
    saveSearchPaths();
}

//==============================================================================
// Dead-Man's Pedal (Crash Recovery)
//==============================================================================

void PluginHost::checkForCrashedScan()
{
    auto pedalFile = getDeadMansPedalFile();
    
    if (pedalFile.existsAsFile())
    {
        // A scan was in progress when we crashed - read the plugin path
        auto crashedPlugin = pedalFile.loadFileAsString().trim();
        
        if (crashedPlugin.isNotEmpty())
        {
            DBG("PluginHost: CRASH DETECTED during scan of: " + crashedPlugin);
            DBG("PluginHost: Adding to blacklist to prevent future crashes");
            
            // Add to blacklist
            {
                std::lock_guard<std::mutex> lock(blacklistMutex_);
                blacklistedPlugins_.insert(crashedPlugin);
            }
            saveBlacklist();
        }
        
        // Clear the pedal file
        pedalFile.deleteFile();
    }
}

void PluginHost::writeDeadMansPedal(const juce::String& currentPlugin)
{
    auto pedalFile = getDeadMansPedalFile();
    pedalFile.replaceWithText(currentPlugin);
}

void PluginHost::clearDeadMansPedal()
{
    auto pedalFile = getDeadMansPedalFile();
    if (pedalFile.existsAsFile())
        pedalFile.deleteFile();
}

//==============================================================================
// Blacklist Management
//==============================================================================

void PluginHost::loadBlacklist()
{
    auto blacklistFile = getBlacklistFile();
    
    if (blacklistFile.existsAsFile())
    {
        juce::StringArray lines;
        blacklistFile.readLines(lines);
        
        std::lock_guard<std::mutex> lock(blacklistMutex_);
        blacklistedPlugins_.clear();
        
        for (const auto& line : lines)
        {
            auto trimmed = line.trim();
            if (trimmed.isNotEmpty())
                blacklistedPlugins_.insert(trimmed);
        }
        
        DBG("PluginHost: Loaded " + juce::String(blacklistedPlugins_.size()) + " blacklisted plugins");
    }
}

void PluginHost::saveBlacklist()
{
    auto blacklistFile = getBlacklistFile();
    
    juce::String content;
    
    {
        std::lock_guard<std::mutex> lock(blacklistMutex_);
        for (const auto& path : blacklistedPlugins_)
        {
            content += path + "\n";
        }
    }
    
    if (content.isNotEmpty())
    {
        blacklistFile.replaceWithText(content);
    }
    else if (blacklistFile.existsAsFile())
    {
        blacklistFile.deleteFile();
    }
}

juce::StringArray PluginHost::getBlacklistedPlugins() const
{
    std::lock_guard<std::mutex> lock(blacklistMutex_);
    
    juce::StringArray result;
    for (const auto& path : blacklistedPlugins_)
        result.add(path);
    
    return result;
}

void PluginHost::removeFromBlacklist(const juce::String& pluginPath)
{
    {
        std::lock_guard<std::mutex> lock(blacklistMutex_);
        blacklistedPlugins_.erase(pluginPath);
    }
    
    saveBlacklist();
    DBG("PluginHost: Removed from blacklist: " + pluginPath);
}

void PluginHost::clearBlacklist()
{
    {
        std::lock_guard<std::mutex> lock(blacklistMutex_);
        blacklistedPlugins_.clear();
    }
    
    saveBlacklist();
    DBG("PluginHost: Blacklist cleared");
}

bool PluginHost::isBlacklisted(const juce::String& pluginPath) const
{
    std::lock_guard<std::mutex> lock(blacklistMutex_);
    return blacklistedPlugins_.find(pluginPath) != blacklistedPlugins_.end();
}

//==============================================================================
// Plugin Scanning - Core Logic
//==============================================================================

int PluginHost::scanInternal(std::function<void(const juce::String&)> onProgress)
{
    if (vst3Format_ == nullptr)
    {
        DBG("PluginHost: No VST3 format available");
        return 0;
    }

    // Get default VST3 search paths
    auto searchPaths = vst3Format_->getDefaultLocationsToSearch();
    
    // Add custom paths
    for (const auto& path : customSearchPaths_)
    {
        searchPaths.add(path);
    }

    int foundCount = knownPlugins_.getNumTypes();
    
    // Collect all plugin files first for better progress reporting
    juce::Array<juce::File> pluginFiles;
    
    for (int i = 0; i < searchPaths.getNumPaths(); ++i)
    {
        if (shouldCancel_) break;

        auto location = searchPaths[i];
        if (!location.exists()) continue;
        
        // Find all .vst3 files/bundles
        auto files = location.findChildFiles(
            juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles,
            true, "*.vst3");
        
        for (const auto& file : files)
        {
            if (!isBlacklisted(file.getFullPathName()))
                pluginFiles.add(file);
        }
    }
    
    DBG("PluginHost: Found " + juce::String(pluginFiles.size()) + " plugin files to scan");
    
    // Scan each plugin file safely
    for (int i = 0; i < pluginFiles.size(); ++i)
    {
        if (shouldCancel_) break;
        
        const auto& pluginFile = pluginFiles[i];
        auto pluginPath = pluginFile.getFullPathName();
        
        // Update currently scanning
        {
            std::lock_guard<std::mutex> lock(scanMutex_);
            currentlyScanning_ = pluginFile.getFileName();
        }
        
        if (onProgress)
            onProgress("Scanning: " + pluginFile.getFileName());
        
        // Write dead-man's pedal BEFORE attempting scan
        writeDeadMansPedal(pluginPath);
        
        // Attempt to scan this plugin
        scanPluginFileSafely(pluginFile, onProgress);
        
        // Successfully scanned - clear dead-man's pedal
        clearDeadMansPedal();
    }
    
    foundCount = knownPlugins_.getNumTypes();
    
    // Clear currently scanning
    {
        std::lock_guard<std::mutex> lock(scanMutex_);
        currentlyScanning_.clear();
    }
    
    // Save results
    saveToDisk();
    
    return foundCount;
}

bool PluginHost::scanPluginFileSafely(const juce::File& pluginFile,
                                       std::function<void(const juce::String&)> onProgress)
{
    if (vst3Format_ == nullptr)
        return false;
    
    try
    {
        // Use OwnedArray to properly manage the descriptions
        juce::OwnedArray<juce::PluginDescription> descriptions;
        vst3Format_->findAllTypesForFile(descriptions, pluginFile.getFullPathName());
        
        for (auto* desc : descriptions)
        {
            if (desc != nullptr && !knowsAboutPlugin(*desc))
            {
                addToKnownPlugins(*desc);
                DBG("PluginHost: Found plugin - " + desc->name);
            }
        }
        
        return descriptions.size() > 0;
    }
    catch (const std::exception& e)
    {
        DBG("PluginHost: Exception scanning " + pluginFile.getFileName() + ": " + e.what());
        return false;
    }
    catch (...)
    {
        DBG("PluginHost: Unknown exception scanning " + pluginFile.getFileName());
        return false;
    }
}

//==============================================================================
// Synchronous Scanning
//==============================================================================

int PluginHost::scanDefaultLocations(bool async)
{
    if (async)
    {
        scanAsync([](int, int, const juce::String&){});
        return 0;
    }

    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    DBG("PluginHost: Scanning default VST3 locations (Synchronous)...");
    int count = scanInternal([](const juce::String& msg) { DBG(msg); });
    DBG("PluginHost: Scan complete - found " + juce::String(count) + " plugins");

    return count;
}

bool PluginHost::scanPath(const juce::File& path)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (vst3Format_ == nullptr)
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

    // Find all plugin files
    juce::Array<juce::File> pluginFiles;
    
    if (path.isDirectory())
    {
        pluginFiles = path.findChildFiles(
            juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles,
            true, "*.vst3");
    }
    else
    {
        pluginFiles.add(path);
    }
    
    int scannedCount = 0;
    
    for (const auto& pluginFile : pluginFiles)
    {
        if (!isBlacklisted(pluginFile.getFullPathName()))
        {
            writeDeadMansPedal(pluginFile.getFullPathName());
            
            if (scanPluginFileSafely(pluginFile, nullptr))
                ++scannedCount;
            
            clearDeadMansPedal();
        }
    }

    DBG("PluginHost: Path scan complete - found " + juce::String(scannedCount) + " new plugins, total: " + juce::String(knownPlugins_.getNumTypes()));
    
    saveToDisk();

    return true;
}

void PluginHost::clearPluginList()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    DBG("PluginHost: Clearing plugin list");
    knownPlugins_.clear();
    saveToDisk();
}

//==============================================================================
// Async Scanning
//==============================================================================

void PluginHost::scanAsync(std::function<void(int, int, const juce::String&)> progressCallback)
{
    if (isScanning_)
    {
        DBG("PluginHost: Scan already in progress");
        return;
    }
    
    // If we have a joinable thread from a previous scan, wait for it
    if (scanThread_.joinable())
        scanThread_.join();
    
    isScanning_ = true;
    shouldCancel_ = false;
    
    DBG("PluginHost: Starting async plugin scan...");
    
    scanThread_ = std::thread([this, progressCallback]() {
        int totalFound = 0;
        int progress = 0;
        
        // Collect plugin files for progress calculation
        juce::Array<juce::File> pluginFiles;
        
        if (vst3Format_ != nullptr)
        {
            auto searchPaths = vst3Format_->getDefaultLocationsToSearch();
            
            for (const auto& path : customSearchPaths_)
                searchPaths.add(path);
            
            for (int i = 0; i < searchPaths.getNumPaths(); ++i)
            {
                auto location = searchPaths[i];
                if (!location.exists()) continue;
                
                auto files = location.findChildFiles(
                    juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles,
                    true, "*.vst3");
                
                for (const auto& file : files)
                {
                    if (!isBlacklisted(file.getFullPathName()))
                        pluginFiles.add(file);
                }
            }
        }
        
        const int totalFiles = pluginFiles.size();
        
        // Report initial status
        juce::MessageManager::callAsync([progressCallback, totalFiles]() {
            progressCallback(0, 0, "Found " + juce::String(totalFiles) + " plugins to scan...");
        });
        
        // Scan each plugin
        for (int i = 0; i < totalFiles && !shouldCancel_; ++i)
        {
            const auto& pluginFile = pluginFiles[i];
            auto pluginPath = pluginFile.getFullPathName();
            auto pluginName = pluginFile.getFileName();
            
            // Update currently scanning
            {
                std::lock_guard<std::mutex> lock(scanMutex_);
                currentlyScanning_ = pluginName;
            }
            
            // Calculate progress percentage
            progress = static_cast<int>((i * 100) / std::max(1, totalFiles));
            
            // Report progress to UI thread
            juce::MessageManager::callAsync([progressCallback, progress, i, pluginName]() {
                progressCallback(progress, i, pluginName);
            });
            
            // Write dead-man's pedal
            writeDeadMansPedal(pluginPath);
            
            // Scan the plugin
            if (scanPluginFileSafely(pluginFile, nullptr))
            {
                totalFound = knownPlugins_.getNumTypes();
            }
            
            // Clear dead-man's pedal on success
            clearDeadMansPedal();
        }
        
        // Clear currently scanning
        {
            std::lock_guard<std::mutex> lock(scanMutex_);
            currentlyScanning_.clear();
        }
        
        isScanning_ = false;
        
        // Save results
        saveToDisk();
        
        // Report completion
        totalFound = knownPlugins_.getNumTypes();
        juce::MessageManager::callAsync([progressCallback, totalFound]() {
            progressCallback(100, totalFound, "Scan complete!");
        });
        
        DBG("PluginHost: Async scan complete - found " + juce::String(totalFound) + " plugins");
    });
    
    // Let the thread run independently
    // scanThread_.detach(); // Removed - joined in destructor or next scan
}

void PluginHost::cancelScan()
{
    shouldCancel_ = true;
    DBG("PluginHost: Scan cancelled");
}

bool PluginHost::isScanningPlugins() const
{
    return isScanning_;
}

juce::String PluginHost::getCurrentlyScanning() const
{
    std::lock_guard<std::mutex> lock(scanMutex_);
    return currentlyScanning_;
}

//==============================================================================
// Plugin Access
//==============================================================================

juce::Array<juce::PluginDescription> PluginHost::getPluginDescriptions() const
{
    juce::Array<juce::PluginDescription> descriptions;

    for (const auto& desc : knownPlugins_.getTypes())
    {
        descriptions.add(desc);
    }

    return descriptions;
}

bool PluginHost::findPluginDescription(const juce::String& identifier, juce::PluginDescription& outDescription) const
{
    for (const auto& desc : knownPlugins_.getTypes())
    {
        if (desc.createIdentifierString() == identifier)
        {
            outDescription = desc;
            return true;
        }
    }

    return false;
}

bool PluginHost::knowsAboutPlugin(const juce::PluginDescription& desc) const
{
    for (const auto& existing : knownPlugins_.getTypes())
    {
        if (existing.createIdentifierString() == desc.createIdentifierString())
            return true;
    }
    return false;
}

void PluginHost::addToKnownPlugins(const juce::PluginDescription& desc)
{
    knownPlugins_.addType(desc);
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
    auto instance = formatManager_.createPluginInstance(
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
    if (!customSearchPaths_.contains(path))
    {
        customSearchPaths_.add(path);
        saveSearchPaths();
    }
}

void PluginHost::removeSearchPath(int index)
{
    if (index >= 0 && index < customSearchPaths_.size())
    {
        customSearchPaths_.remove(index);
        saveSearchPaths();
    }
}

juce::StringArray PluginHost::getSearchPaths() const
{
    return customSearchPaths_;
}

void PluginHost::saveSearchPaths()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "ZenithDAW";
    options.filenameSuffix = ".settings";
    options.folderName = CACHE_FOLDER_NAME;
    options.osxLibrarySubFolder = "Application Support";
    
    juce::ApplicationProperties props;
    props.setStorageParameters(options);
    
    if (auto* userSettings = props.getUserSettings())
    {
        userSettings->setValue("customPluginPaths", customSearchPaths_.joinIntoString("|"));
        userSettings->saveIfNeeded();
    }
}

void PluginHost::loadSearchPaths()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "ZenithDAW";
    options.filenameSuffix = ".settings";
    options.folderName = CACHE_FOLDER_NAME;
    options.osxLibrarySubFolder = "Application Support";
    
    juce::ApplicationProperties props;
    props.setStorageParameters(options);
    
    if (auto* userSettings = props.getUserSettings())
    {
        auto pathsStr = userSettings->getValue("customPluginPaths", "");
        if (pathsStr.isNotEmpty())
        {
            customSearchPaths_.clear();
            customSearchPaths_.addTokens(pathsStr, "|", "");
            DBG("PluginHost: Loaded " + juce::String(customSearchPaths_.size()) + " custom search paths");
        }
    }
}

int PluginHost::scanAll(bool async)
{
    return scanDefaultLocations(async);
}

} // namespace zenith
