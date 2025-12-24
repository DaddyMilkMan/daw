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

  // Setup blacklist file
  blacklistFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("ZenithDAW")
          .getChildFile("blacklisted.xml");
  loadBlacklist();

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

  int foundCount = 0;

  // Collect all files to scan
  juce::Array<juce::File> filesToScan;
  for (int i = 0; i < searchPaths.getNumPaths(); ++i) {
    auto location = searchPaths[i];
    if (location.isDirectory()) {
      location.findChildFiles(filesToScan, juce::File::findFiles, true,
                              "*.vst3");
    } else if (location.exists()) {
      filesToScan.add(location);
    }
  }

  // Scan each file
  for (int i = 0; i < filesToScan.size(); ++i) {
    if (shouldCancel_)
      break;

    auto file = filesToScan[i];
    if (onProgress)
      onProgress("Scanning (" + juce::String(i + 1) + "/" +
                 juce::String(filesToScan.size()) + "): " + file.getFileName());

    if (isBlacklisted(file.getFullPathName())) {
      DBG("PluginHost: Skipping blacklisted plugin: " + file.getFullPathName());
      continue;
    }

    if (scanOutOfProcess(file, onProgress)) {
      foundCount++;
    }
  }

  return knownPlugins.getNumTypes();
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

  juce::Array<juce::File> filesToScan;
  if (path.isDirectory()) {
    path.findChildFiles(filesToScan, juce::File::findFiles, true, "*.vst3");
  } else {
    filesToScan.add(path);
  }

  for (const auto &file : filesToScan) {
    if (isBlacklisted(file.getFullPathName()))
      continue;
    scanOutOfProcess(file, nullptr);
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

bool PluginHost::isBlacklisted(const juce::String &filePath) const {
  return blacklist.contains(filePath);
}

void PluginHost::blacklistPlugin(const juce::String &filePath) {
  if (!blacklist.contains(filePath)) {
    blacklist.add(filePath);
    saveBlacklist();
  }
}

void PluginHost::clearBlacklist() {
  blacklist.clear();
  saveBlacklist();
}

juce::StringArray PluginHost::getBlacklistedPlugins() const {
  return blacklist;
}

void PluginHost::loadBlacklist() {
  if (blacklistFile.existsAsFile()) {
    if (auto xml = juce::XmlDocument::parse(blacklistFile)) {
      blacklist.clear();
      for (auto *child : xml->getChildIterator()) {
        if (child->hasTagName("PLUGIN")) {
          blacklist.add(child->getStringAttribute("path"));
        }
      }
    }
  }
}

void PluginHost::saveBlacklist() {
  juce::XmlElement xml("BLACKLIST");
  for (const auto &path : blacklist) {
    auto *el = xml.createNewChildElement("PLUGIN");
    el->setAttribute("path", path);
  }
  xml.writeTo(blacklistFile);
}

bool PluginHost::scanOutOfProcess(
    const juce::File &file,
    std::function<void(const juce::String &)> onProgress) {
  // Path to the PluginScanner executable
  // In a real app, this would be bundled. Here we look in the build directory.
  juce::File scannerExe =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getSiblingFile("PluginScanner");

#if JUCE_WINDOWS
  if (!scannerExe.exists())
    scannerExe = scannerExe.withFileExtension(".exe");
#endif

  if (!scannerExe.exists()) {
    DBG("PluginHost: Scanner not found at " + scannerExe.getFullPathName());
    return false;
  }

  juce::StringArray args;
  args.add(scannerExe.getFullPathName());
  args.add(file.getFullPathName());

  juce::ChildProcess process;
  if (process.start(args)) {
    // Wait for process to finish with a 5s timeout
    if (process.waitForProcessToFinish(5000)) { // 5000ms = 5s
      int exitCode = process.getExitCode();
      if (exitCode == 0) {
        juce::String output = process.readAllProcessOutput();
        if (auto xml = juce::XmlDocument::parse(output)) {
          bool anyAdded = false;
          for (auto *child : xml->getChildIterator()) {
            juce::PluginDescription desc;
            if (desc.loadFromXml(*child)) {
              knownPlugins.addType(desc);
              anyAdded = true;
            }
          }
          return anyAdded;
        }
      } else {
        DBG("PluginHost: Scanner failed with exit code " +
            juce::String(exitCode) + " for " + file.getFileName());
        blacklistPlugin(file.getFullPathName());
      }
    } else {
      // Timeout!
      DBG("PluginHost: Scanner timed out for " + file.getFileName());
      process.kill();
      blacklistPlugin(file.getFullPathName());
    }
  } else {
    DBG("PluginHost: Failed to start scanner for " + file.getFileName());
  }

  return false;
}

} // namespace zenith
