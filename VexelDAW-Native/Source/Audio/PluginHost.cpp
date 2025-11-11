/*
  ==============================================================================

    PluginHost.cpp
    Created: 2025-11-11
    Author:  Vexel DAW

    Plugin host implementation

  ==============================================================================
*/

#include "PluginHost.h"

//==============================================================================
PluginHost* PluginHost::instance = nullptr;

PluginHost& PluginHost::getInstance()
{
    if (instance == nullptr)
    {
        instance = new PluginHost();
    }
    return *instance;
}

void PluginHost::deleteInstance()
{
    if (instance != nullptr)
    {
        delete instance;
        instance = nullptr;
    }
}

//==============================================================================
PluginHost::PluginHost()
{
    // Register all available plugin formats
   #if JUCE_PLUGINHOST_VST3
    formatManager.addDefaultFormats();
   #else
    formatManager.addFormat(new juce::AudioPluginFormat());
   #endif

    // Load saved plugin list
    loadPluginList();
}

PluginHost::~PluginHost()
{
    if (scannerThread != nullptr)
    {
        shouldCancelScan.store(true);
        scannerThread->stopThread(1000);
        scannerThread.reset();
    }

    savePluginList();
}

//==============================================================================
void PluginHost::scanForPlugins()
{
    if (scanning.load())
        return;

    scanning.store(true);
    scanProgress.store(0.0f);
    shouldCancelScan.store(false);

    // Scan in a background thread
    scannerThread.reset(new ScannerThread(*this, juce::File()));
    scannerThread->startThread();
}

void PluginHost::scanDirectory(const juce::File& directory)
{
    if (scanning.load())
        return;

    if (!directory.exists() || !directory.isDirectory())
        return;

    scanning.store(true);
    scanProgress.store(0.0f);
    shouldCancelScan.store(false);

    scannerThread.reset(new ScannerThread(*this, directory));
    scannerThread->startThread();
}

void PluginHost::cancelScan()
{
    shouldCancelScan.store(true);

    if (scannerThread != nullptr)
    {
        scannerThread->stopThread(1000);
        scannerThread.reset();
    }

    scanning.store(false);
}

//==============================================================================
juce::Array<PluginHost::PluginInfo> PluginHost::getPluginList() const
{
    const juce::ScopedLock sl(pluginListLock);

    juce::Array<PluginInfo> plugins;

    for (const auto& type : knownPluginList.getTypes())
    {
        PluginInfo info;
        info.name = type.name;
        info.manufacturer = type.manufacturerName;
        info.category = type.category;
        info.fileOrIdentifier = type.fileOrIdentifier;
        info.description = type;
        info.isInstrument = type.isInstrument;
        info.hasEditor = type.hasSharedContainer;
        info.numInputs = type.numInputChannels;
        info.numOutputs = type.numOutputChannels;

        plugins.add(info);
    }

    return plugins;
}

juce::Array<PluginHost::PluginInfo> PluginHost::getInstrumentPlugins() const
{
    auto allPlugins = getPluginList();
    juce::Array<PluginInfo> instruments;

    for (const auto& plugin : allPlugins)
    {
        if (plugin.isInstrument)
        {
            instruments.add(plugin);
        }
    }

    return instruments;
}

juce::Array<PluginHost::PluginInfo> PluginHost::getEffectPlugins() const
{
    auto allPlugins = getPluginList();
    juce::Array<PluginInfo> effects;

    for (const auto& plugin : allPlugins)
    {
        if (!plugin.isInstrument)
        {
            effects.add(plugin);
        }
    }

    return effects;
}

juce::Array<PluginHost::PluginInfo> PluginHost::getPluginsByCategory(const juce::String& category) const
{
    auto allPlugins = getPluginList();
    juce::Array<PluginInfo> filtered;

    for (const auto& plugin : allPlugins)
    {
        if (plugin.category.equalsIgnoreCase(category))
        {
            filtered.add(plugin);
        }
    }

    return filtered;
}

juce::Array<PluginHost::PluginInfo> PluginHost::searchPlugins(const juce::String& searchText) const
{
    auto allPlugins = getPluginList();
    juce::Array<PluginInfo> results;

    const juce::String searchLower = searchText.toLowerCase();

    for (const auto& plugin : allPlugins)
    {
        if (plugin.name.toLowerCase().contains(searchLower) ||
            plugin.manufacturer.toLowerCase().contains(searchLower) ||
            plugin.category.toLowerCase().contains(searchLower))
        {
            results.add(plugin);
        }
    }

    return results;
}

int PluginHost::getNumPlugins() const
{
    const juce::ScopedLock sl(pluginListLock);
    return knownPluginList.getTypes().size();
}

//==============================================================================
juce::AudioPluginInstance* PluginHost::loadPlugin(const PluginInfo& info, juce::String& errorMessage)
{
    return loadPlugin(info.description, errorMessage);
}

juce::AudioPluginInstance* PluginHost::loadPlugin(const juce::PluginDescription& description, juce::String& errorMessage)
{
    errorMessage.clear();

    juce::AudioPluginInstance* plugin = nullptr;

    // Try to instantiate the plugin
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);

        if (format->getName() == description.pluginFormatName)
        {
            plugin = format->createInstanceFromDescription(description, 44100.0, 512);

            if (plugin != nullptr)
            {
                break;
            }
            else
            {
                errorMessage = "Failed to instantiate plugin: " + description.name;
            }
        }
    }

    if (plugin == nullptr && errorMessage.isEmpty())
    {
        errorMessage = "Plugin format not found: " + description.pluginFormatName;
    }

    return plugin;
}

//==============================================================================
void PluginHost::savePluginList()
{
    const juce::ScopedLock sl(pluginListLock);

    auto file = getPluginListFile();

    if (auto xml = knownPluginList.createXml())
    {
        xml->writeTo(file);
    }
}

void PluginHost::loadPluginList()
{
    const juce::ScopedLock sl(pluginListLock);

    auto file = getPluginListFile();

    if (file.existsAsFile())
    {
        if (auto xml = juce::parseXML(file))
        {
            knownPluginList.recreateFromXml(*xml);
        }
    }
}

juce::File PluginHost::getPluginListFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto vexelDir = appDataDir.getChildFile("VexelDAW");

    if (!vexelDir.exists())
    {
        vexelDir.createDirectory();
    }

    return vexelDir.getChildFile("PluginList.xml");
}

//==============================================================================
juce::StringArray PluginHost::getDefaultVST3Paths()
{
    juce::StringArray paths;

   #if JUCE_WINDOWS
    paths.add("C:\\Program Files\\Common Files\\VST3");
    paths.add("C:\\Program Files (x86)\\Common Files\\VST3");
   #elif JUCE_MAC
    paths.add("/Library/Audio/Plug-Ins/VST3");
    paths.add("~/Library/Audio/Plug-Ins/VST3");
   #elif JUCE_LINUX
    paths.add("~/.vst3");
    paths.add("/usr/lib/vst3");
    paths.add("/usr/local/lib/vst3");
   #endif

    return paths;
}

juce::StringArray PluginHost::getDefaultAUPaths()
{
    juce::StringArray paths;

   #if JUCE_MAC
    paths.add("/Library/Audio/Plug-Ins/Components");
    paths.add("~/Library/Audio/Plug-Ins/Components");
   #endif

    return paths;
}

juce::StringArray PluginHost::getDefaultAAXPaths()
{
    juce::StringArray paths;

   #if JUCE_WINDOWS
    paths.add("C:\\Program Files\\Common Files\\Avid\\Audio\\Plug-Ins");
   #elif JUCE_MAC
    paths.add("/Library/Application Support/Avid/Audio/Plug-Ins");
   #endif

    return paths;
}

juce::StringArray PluginHost::getDefaultLV2Paths()
{
    juce::StringArray paths;

   #if JUCE_LINUX
    paths.add("~/.lv2");
    paths.add("/usr/lib/lv2");
    paths.add("/usr/local/lib/lv2");
   #endif

    return paths;
}

//==============================================================================
void PluginHost::addPluginsFromFormat(juce::AudioPluginFormat& format, const juce::FileSearchPath& paths)
{
    juce::PluginDirectoryScanner scanner(
        knownPluginList,
        format,
        paths,
        true,  // recursive
        juce::File());

    juce::String pluginBeingScanned;
    while (scanner.scanNextFile(true, pluginBeingScanned))
    {
        if (shouldCancelScan.load())
        {
            break;
        }

        scanProgress.store(scanner.getProgress());
    }
}

//==============================================================================
PluginHost::ScannerThread::ScannerThread(PluginHost& host, const juce::File& directory)
    : juce::Thread("Plugin Scanner"),
      owner(host),
      directoryToScan(directory)
{
}

void PluginHost::ScannerThread::run()
{
    const juce::ScopedLock sl(owner.pluginListLock);

    if (directoryToScan == juce::File())
    {
        // Scan all default locations
        for (int i = 0; i < owner.formatManager.getNumFormats(); ++i)
        {
            if (threadShouldExit() || owner.shouldCancelScan.load())
            {
                break;
            }

            auto* format = owner.formatManager.getFormat(i);

            juce::FileSearchPath searchPath;

            if (format->getName().contains("VST3"))
            {
                for (auto& path : PluginHost::getDefaultVST3Paths())
                {
                    searchPath.add(juce::File(path));
                }
            }
            else if (format->getName().contains("AudioUnit"))
            {
                for (auto& path : PluginHost::getDefaultAUPaths())
                {
                    searchPath.add(juce::File(path));
                }
            }
            else if (format->getName().contains("AAX"))
            {
                for (auto& path : PluginHost::getDefaultAAXPaths())
                {
                    searchPath.add(juce::File(path));
                }
            }
            else if (format->getName().contains("LV2"))
            {
                for (auto& path : PluginHost::getDefaultLV2Paths())
                {
                    searchPath.add(juce::File(path));
                }
            }

            if (searchPath.getNumPaths() > 0)
            {
                owner.addPluginsFromFormat(*format, searchPath);
            }
        }
    }
    else
    {
        // Scan specific directory
        for (int i = 0; i < owner.formatManager.getNumFormats(); ++i)
        {
            if (threadShouldExit() || owner.shouldCancelScan.load())
            {
                break;
            }

            auto* format = owner.formatManager.getFormat(i);
            juce::FileSearchPath searchPath(directoryToScan.getFullPathName());
            owner.addPluginsFromFormat(*format, searchPath);
        }
    }

    owner.scanning.store(false);
    owner.scanProgress.store(1.0f);
    owner.savePluginList();
    owner.sendChangeMessage();
}
