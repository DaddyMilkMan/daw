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

  // JUCE 8.0.11: Use addHeadlessDefaultFormatsToManager() instead of deleted addDefaultFormats()
  juce::addHeadlessDefaultFormatsToManager(formatManager);
  
  // Add our internal plugin format
  formatManager.addFormat(std::make_unique<InternalPluginFormat>());

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
  if (scanThread && scanThread->isThreadRunning()) {
    scanThread->stopThread(5000); // Wait up to 5 seconds
  }
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

  // Scan each location
  for (int i = 0; i < searchPaths.getNumPaths(); ++i) {
    if (shouldCancel_)
      break;

    auto location = searchPaths[i];
    if (onProgress)
      onProgress("Scanning: " + location.getFullPathName());

    if (!location.exists())
      continue;

    // Recursive file find
    juce::Array<juce::File> filesToScan;
    location.findChildFiles(filesToScan, 
                            juce::File::findFiles, 
                            true, // recursive
                            "*.vst3"); // VST3 only for now

    for (const auto& file : filesToScan) {
        if (shouldCancel_) break;
        
        juce::String pluginName = file.getFileNameWithoutExtension(); // temp name
        if (onProgress) onProgress("Scanning: " + pluginName);

        juce::PluginDescription desc;
        if (scanFileOutProcess(file, desc)) {
            // Check if already known
            if (!knowsAboutPlugin(desc)) {
                addToKnownPlugins(desc);
                foundCount++;
            }
        }
    }
  }

  return foundCount;
}

bool PluginHost::scanFileOutProcess(const juce::File& file, juce::PluginDescription& result)
{
    juce::File currentApp = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    juce::File scannerExe = currentApp.getSiblingFile("ZenithPluginScanner");
    
    #if JUCE_WINDOWS
    if (!scannerExe.hasFileExtension("exe")) scannerExe = scannerExe.withFileExtension("exe");
    #endif

    if (!scannerExe.existsAsFile()) {
        // Fallback for Debug builds where it might be in same dir
        DBG("PluginHost: Scanner not found at " + scannerExe.getFullPathName());
        return false;
    }

    juce::ChildProcess process;
    juce::StringArray args;
    args.add(scannerExe.getFullPathName());
    args.add("--scan");
    args.add(file.getFullPathName());

    if (process.start(args))
    {
        // Fix: Wait for process to finish with a timeout BEFORE reading output.
        // Reading first would block indefinitely if the child process hangs.
        if (process.waitForProcessToFinish(10000)) // 10 second timeout
        {
            juce::String output = process.readAllProcessOutput();
            
            if (process.getExitCode() == 0)
            {
                // Parse JSON output
                // Output usually contains JSON on one line, but maybe headers.
                // We look for the last valid JSON lines or clean output.
                
                output = output.trim();
                int jsonStart = output.indexOf("{");
                int jsonEnd = output.lastIndexOf("}");
                
                if (jsonStart >= 0 && jsonEnd > jsonStart)
                {
                    juce::String jsonStr = output.substring(jsonStart, jsonEnd + 1);
                    auto json = juce::JSON::parse(jsonStr);
                    
                    if (!json.isVoid() && json.hasProperty("status"))
                    {
                       juce::String status = json["status"];
                       if (status == "success") {
                           result.fileOrIdentifier = file.getFullPathName();
                           result.name = json["name"];
                           result.manufacturerName = json["manufacturer"];
                           result.version = json["version"];
                           result.uniqueId = json["uid"].toString().getIntValue();
                           result.pluginFormatName = "VST3";
                           result.lastInfoUpdateTime = juce::Time::getCurrentTime();
                           
                           bool isInst = json["isInstrument"];
                           result.isInstrument = isInst;
                           
                           return true;
                       }
                    }
                }
            }
            else {
                 DBG("PluginHost: Detailed Crash detected scanning " + file.getFileName());
            }
        }
        else
        {
            process.kill();
            DBG("PluginHost: Scanner timed out (hung) scanning " + file.getFileName());
        }
    }
    
    return false;
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

  // Use the managed ScanThread class
  scanThread = std::make_unique<ScanThread>(*this);
  scanThread->startThread();
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

  // Use the robust out-of-process logic instead of the in-process juce::PluginDirectoryScanner
  juce::Array<juce::File> filesToScan;
  if (path.isDirectory()) {
      path.findChildFiles(filesToScan, juce::File::findFiles, true, "*.vst3");
  } else if (path.hasFileExtension(".vst3")) {
      filesToScan.add(path);
  }

  int foundCount = 0;
  for (const auto& file : filesToScan) {
      juce::PluginDescription desc;
      if (scanFileOutProcess(file, desc)) {
          if (!knowsAboutPlugin(desc)) {
              addToKnownPlugins(desc);
              foundCount++;
          }
      }
  }

  DBG("PluginHost: Path scan complete - found " + juce::String(foundCount) + 
      " new plugins. Total: " + juce::String(knownPlugins.getNumTypes()));

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

//==============================================================================
// Internal Helpers
//==============================================================================

bool PluginHost::knowsAboutPlugin(const juce::PluginDescription& desc) const {
    for (const auto& existing : knownPlugins.getTypes()) {
        if (existing.fileOrIdentifier == desc.fileOrIdentifier && existing.uniqueId == desc.uniqueId)
            return true;
    }
    return false;
}

void PluginHost::addToKnownPlugins(const juce::PluginDescription& desc) {
    knownPlugins.addType(desc);
}

} // namespace zenith

