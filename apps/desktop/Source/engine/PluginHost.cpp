/*
  ==============================================================================

    PluginHost.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 3: VST3 Plugin Hosting MVP

    Plugin hosting manager implementation

  ==============================================================================
*/

#include "PluginHost.h"
#include "../plugins/InternalPluginFormat.h"

namespace zenith {

//==============================================================================
PluginHost::PluginHost() {
  DBG("PluginHost: Initializing...");

  // Add VST3 format
  formatManager.addDefaultFormats();
  formatManager.addFormat(new InternalPluginFormat());

  // Get VST3 format pointer for later use
  for (int i = 0; i < formatManager.getNumFormats(); ++i) {
    auto *format = formatManager.getFormat(i);
    if (format->getName().contains("VST3")) {
      vst3Format = format;
      DBG("PluginHost: VST3 format registered");
      break;
    }
  }

  if (vst3Format == nullptr) {
    DBG("PluginHost: WARNING - VST3 format not available!");
  }

  DBG("PluginHost: Initialized");
}

PluginHost::~PluginHost() {
  DBG("PluginHost: Destructor");
  cancelScan();
  if (scanThread_.joinable())
    scanThread_.join();
}

//==============================================================================
// Plugin Scanning
//==============================================================================

// Internal scanning logic - runs on ANY thread
int PluginHost::scanInternal(
    std::function<void(const juce::String &)> onProgress) {
  if (vst3Format == nullptr)
    return 0;

  // Get default VST3 search paths
  auto searchPaths = vst3Format->getDefaultLocationsToSearch();

  // Add custom paths
  for (const auto &path : customSearchPaths) {
    searchPaths.add(path);
  }

  // Get all files to scan
  juce::Array<juce::File> filesToScan;
  for (int i = 0; i < searchPaths.getNumPaths(); ++i) {
      auto location = searchPaths[i];
      if (location.exists()) {
          location.findChildFiles(filesToScan, juce::File::findFiles, true, "*.vst3");
      }
  }

  int foundCount = 0;

  // Scan each file safely
  for (const auto& file : filesToScan) {
    if (shouldCancel_)
      break;

    if (isBlacklisted(file.getFullPathName())) {
        DBG("PluginHost: Skipping blacklisted plugin: " + file.getFileName());
        continue;
    }

    if (onProgress)
      onProgress("Scanning: " + file.getFileName());

    juce::Array<juce::PluginDescription> descriptions;
    if (scanPluginSafely(file, descriptions)) {
        for (const auto& desc : descriptions) {
            knownPlugins.addType(desc);
        }
    } else {
        DBG("PluginHost: Plugin scan failed or crashed: " + file.getFileName());
        addToBlacklist(file.getFullPathName());
    }

    foundCount = knownPlugins.getNumTypes();
  }

  return foundCount;
}

bool PluginHost::scanPluginSafely(const juce::File& file, juce::Array<juce::PluginDescription>& found) {
    juce::File scannerExe = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                .getSiblingFile("PluginScanner");
#if JUCE_WINDOWS
    if (!scannerExe.exists()) scannerExe = scannerExe.withFileExtension(".exe");
#endif

    if (!scannerExe.existsAsFile()) {
        DBG("PluginHost ERROR: PluginScanner executable not found at: " + scannerExe.getFullPathName());
        return false;
    }

    juce::StringArray args;
    args.add(scannerExe.getFullPathName());
    args.add(file.getFullPathName());

    juce::ChildProcess scanner;
    if (scanner.start(args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr)) {
        // Wait for process to finish with 10 second timeout
        if (scanner.waitForProcessToFinish(10000)) {
            if (scanner.getExitCode() != 0) return false;

            juce::String output = scanner.readAllProcessOutput();
            
            // Parse ZENITH_PLUGIN_START / ZENITH_PLUGIN_END blocks
            juce::StringArray lines;
            lines.addLines(output);

            bool inBlock = false;
            juce::String currentJson;

            for (const auto& line : lines) {
                if (line.trim() == "ZENITH_PLUGIN_START") {
                    inBlock = true;
                    currentJson = "";
                } else if (line.trim() == "ZENITH_PLUGIN_END") {
                    inBlock = false;
                    auto v = juce::JSON::parse(currentJson);
                    if (v.isObject()) {
                        juce::PluginDescription desc;
                        // Manual mapping from JSON back to PluginDescription
                        desc.name = v.getProperty("name", "").toString();
                        desc.descriptiveName = v.getProperty("descriptiveName", "").toString();
                        desc.manufacturerName = v.getProperty("manufacturerName", "").toString();
                        desc.version = v.getProperty("version", "").toString();
                        desc.fileOrIdentifier = v.getProperty("file", "").toString();
                        desc.isInstrument = v.getProperty("isInstrument", false);
                        desc.pluginFormatName = v.getProperty("pluginFormatName", "").toString();
                        desc.category = v.getProperty("category", "").toString();
                        desc.numInputChannels = v.getProperty("numInputChannels", 0);
                        desc.numOutputChannels = v.getProperty("numOutputChannels", 0);
                        desc.hasSharedContainer = v.getProperty("hasSharedContainer", false);
                        desc.uniqueId = v.getProperty("uniqueId", 0);
                        
                        found.add(desc);
                    }
                } else if (inBlock) {
                    currentJson += line + "\n";
                }
            }
            return !found.isEmpty();
        } else {
            // Timeout!
            scanner.kill();
            DBG("PluginHost: TIMEOUT scanning " + file.getFileName());
            return false;
        }
    }

    return false;
}

void PluginHost::addToBlacklist(const juce::String& pluginFileOrIdentifier) {
    if (!blacklist.contains(pluginFileOrIdentifier)) {
        blacklist.add(pluginFileOrIdentifier);
        // In a real app, we would save this to an XML file immediately
    }
}

bool PluginHost::isBlacklisted(const juce::String& pluginFileOrIdentifier) const {
    return blacklist.contains(pluginFileOrIdentifier);
}

void PluginHost::clearBlacklist() {
    blacklist.clear();
}

juce::StringArray PluginHost::getBlacklist() const {
    return blacklist;
}

int PluginHost::scanDefaultLocations(bool async) {
  if (async) {
    scanAsync([](int, int, const juce::String &) {});
    return 0;
  }

  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  DBG("PluginHost: Scanning default VST3 locations (Synchronous)...");
  int count = scanInternal([](const juce::String &msg) { DBG(msg); });
  DBG("PluginHost: Scan complete - found " + juce::String(count) + " plugins");

  return count;
}

void PluginHost::scanAsync(
    std::function<void(int, int, const juce::String &)> progressCallback) {
  if (isScanning_)
    return;

  isScanning_ = true;
  shouldCancel_ = false;

  scanThread_ = std::thread([this, progressCallback]() {
    DBG("PluginHost: Starting async scan...");

    int count = scanInternal([progressCallback](const juce::String &name) {
      juce::MessageManager::callAsync(
          [progressCallback, name]() { progressCallback(0, 0, name); });
    });

    isScanning_ = false;

    juce::MessageManager::callAsync(
        [progressCallback, count]() { progressCallback(100, count, "Done"); });

    DBG("PluginHost: Async scan complete.");
  });

  scanThread_.detach();
}

void PluginHost::cancelScan() { shouldCancel_ = true; }

bool PluginHost::isScanningPlugins() const { return isScanning_; }

bool PluginHost::scanPath(const juce::File &path) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (vst3Format == nullptr) {
    DBG("PluginHost: Cannot scan - VST3 format not available");
    return false;
  }

  if (!path.exists()) {
    DBG("PluginHost: Path does not exist: " + path.getFullPathName());
    return false;
  }

  DBG("PluginHost: Scanning path: " + path.getFullPathName());

  juce::FileSearchPath searchPath(path.getFullPathName());

  juce::PluginDirectoryScanner scanner(knownPlugins, *vst3Format, searchPath,
                                       true,        // Search recursively
                                       juce::File() // No dead-mans pedal file
  );

  juce::String pluginBeingScanned;

  while (scanner.scanNextFile(true, pluginBeingScanned)) {
    DBG("PluginHost: Scanning " + pluginBeingScanned);
  }

  DBG("PluginHost: Path scan complete - total plugins: " +
      juce::String(knownPlugins.getNumTypes()));

  return true;
}

void PluginHost::clearPluginList() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("PluginHost: Clearing plugin list");
  knownPlugins.clear();
}

//==============================================================================
// Plugin Access
//==============================================================================

juce::Array<juce::PluginDescription> PluginHost::getPluginDescriptions() const {
  juce::Array<juce::PluginDescription> descriptions;

  for (const auto &desc : knownPlugins.getTypes()) {
    descriptions.add(desc);
  }

  return descriptions;
}

bool PluginHost::findPluginDescription(
    const juce::String &identifier,
    juce::PluginDescription &outDescription) const {
  for (const auto &desc : knownPlugins.getTypes()) {
    if (desc.createIdentifierString() == identifier) {
      outDescription = desc;
      return true;
    }
  }

  return false;
}

//==============================================================================
// Plugin Instantiation
//==============================================================================

std::unique_ptr<juce::AudioPluginInstance>
PluginHost::createInstance(const juce::PluginDescription &description,
                           double sampleRate, int blockSize,
                           juce::String &errorMessage) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("PluginHost: Creating instance of " + description.name);

  errorMessage.clear();

  // Create plugin instance (BLOCKING call)
  juce::String loadError;
  auto instance = formatManager.createPluginInstance(description, sampleRate,
                                                     blockSize, loadError);

  if (instance == nullptr) {
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

std::unique_ptr<juce::AudioPluginInstance>
PluginHost::createInstance(const juce::String &identifier, double sampleRate,
                           int blockSize, juce::String &errorMessage) {
  errorMessage.clear();

  // Find the plugin description
  juce::PluginDescription description;
  if (!findPluginDescription(identifier, description)) {
    errorMessage = "Plugin not found: " + identifier;
    DBG("PluginHost: " + errorMessage);
    return nullptr;
  }

  // Create instance using the description
  return createInstance(description, sampleRate, blockSize, errorMessage);
}

std::unique_ptr<juce::AudioPluginInstance>
PluginHost::createPlugin(const juce::PluginDescription &description) {
  juce::String errorMessage;
  // Use default sample rate and block size if not specified
  // Ideally these should come from the Engine, but for state restoration this
  // is often acceptable initially
  return createInstance(description, 44100.0, 512, errorMessage);
}

//==============================================================================
// Custom Search Paths
//==============================================================================

void PluginHost::addSearchPath(const juce::String &path) {
  if (!customSearchPaths.contains(path))
    customSearchPaths.add(path);
}

void PluginHost::removeSearchPath(int index) {
  if (index >= 0 && index < customSearchPaths.size())
    customSearchPaths.remove(index);
}

juce::StringArray PluginHost::getSearchPaths() const {
  return customSearchPaths;
}

int PluginHost::scanAll(bool async) { return scanDefaultLocations(async); }

} // namespace zenith
