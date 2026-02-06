/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "PluginHost.h"
#include "PluginBlacklist.h"
#include "../plugins/InternalPluginFormat.h"

namespace zenith {

juce::String PluginHost::ScanStatistics::getSummary() const
{
    juce::String summary;
    summary << "Scanned: " << totalScanned << ", ";
    summary << "Found: " << found << ", ";
    if (crashed > 0) summary << "Crashed: " << crashed << ", ";
    if (timedOut > 0) summary << "Timed out: " << timedOut << ", ";
    if (blacklisted > 0) summary << "Skipped (blacklisted): " << blacklisted << ", ";
    if (parseErrors > 0) summary << "Parse errors: " << parseErrors;
    return summary.trimCharactersAtEnd(", ");
}

//==============================================================================
PluginHost::PluginHost() {
  DBG("PluginHost: Initializing...");

  // Add VST3 format
#if JUCE_PLUGINHOST_VST3
  formatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());
#endif
#if JUCE_MAC && JUCE_PLUGINHOST_AU
  formatManager.addFormat(std::make_unique<juce::AudioUnitPluginFormat>());
#endif
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

  // Initialize blacklist
  blacklist_ = std::make_unique<PluginBlacklist>();
  DBG("PluginHost: Blacklist loaded with " + juce::String(blacklist_->getBlacklistCount()) + " entries");

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

PluginHost::ScanResult PluginHost::scanFileWithDetails(const juce::File& file)
{
    ScanResult result;
    result.filePath = file.getFullPathName();
    
    // Check blacklist first
    if (blacklist_ && blacklist_->isBlacklisted(file.getFullPathName()))
    {
        result.success = false;
        result.errorType = "blacklisted";
        result.errorMessage = "Plugin is in blacklist due to previous crashes";
        DBG("PluginHost: Skipping blacklisted plugin " + file.getFileName());
        return result;
    }

    juce::File currentApp = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    juce::File scannerExe = currentApp.getSiblingFile("ZenithPluginScanner");
    
    #if JUCE_WINDOWS
    if (!scannerExe.hasFileExtension("exe")) scannerExe = scannerExe.withFileExtension("exe");
    #endif

    if (!scannerExe.existsAsFile()) {
        DBG("PluginHost: Scanner not found at " + scannerExe.getFullPathName());
        result.success = false;
        result.errorType = "scanner_not_found";
        result.errorMessage = "Plugin scanner executable not found";
        return result;
    }

    juce::ChildProcess process;
    juce::StringArray args;
    args.add(scannerExe.getFullPathName());
    args.add(file.getFullPathName());  // Note: removed --scan flag, scanner takes path directly

    if (!process.start(args))
    {
        result.success = false;
        result.errorType = "process_start_failed";
        result.errorMessage = "Failed to start plugin scanner process";
        return result;
    }

    // Wait for process with timeout
    constexpr int kTimeoutMs = 8000;  // 8 second timeout (increased from 5)
    bool finished = process.waitForProcessToFinish(kTimeoutMs);
    
    if (!finished) {
        process.kill();
        process.waitForProcessToFinish(1000);
        
        result.success = false;
        result.errorType = "timeout";
        result.errorMessage = "Plugin scan timed out (plugin may be hung)";
        
        // Record failure and potentially blacklist
        if (blacklist_)
            blacklist_->recordFailure(file.getFullPathName(), "timeout", result.errorMessage);
        
        DBG("PluginHost: Scan timeout for " + file.getFileName());
        return result;
    }

    juce::String output = process.readAllProcessOutput();
    int exitCode = process.getExitCode();
    
    // Check for crash/non-zero exit
    if (exitCode != 0)
    {
        result.success = false;
        result.errorType = "crash";
        result.errorMessage = "Scanner process crashed or returned error code " + juce::String(exitCode);
        
        // Record failure and blacklist
        if (blacklist_)
            blacklist_->recordFailure(file.getFullPathName(), "crash", result.errorMessage);
        
        DBG("PluginHost: Crash detected scanning " + file.getFileName() + " (exit code " + juce::String(exitCode) + ")");
        return result;
    }

    // Parse YAML output
    output = output.trim();
    auto lines = juce::StringArray::fromLines(output);
    bool foundSuccess = false;
    
    for (const auto& line : lines) {
        auto trimmed = line.trim();
        
        if (trimmed.startsWith("status:")) {
            auto status = trimmed.fromFirstOccurrenceOf("status:", false, false).trim();
            if (status == "success") {
                foundSuccess = true;
            } else if (status == "error") {
                foundSuccess = false;
            }
        } else if (trimmed.startsWith("error_type:")) {
            result.errorType = trimmed.fromFirstOccurrenceOf("error_type:", false, false).trim();
        } else if (trimmed.startsWith("message:")) {
            result.errorMessage = trimmed.fromFirstOccurrenceOf("message:", false, false).trim();
        } else if (trimmed.startsWith("name:")) {
            result.description.name = trimmed.fromFirstOccurrenceOf("name:", false, false).trim();
        } else if (trimmed.startsWith("manufacturer:")) {
            result.description.manufacturerName = trimmed.fromFirstOccurrenceOf("manufacturer:", false, false).trim();
        } else if (trimmed.startsWith("version:")) {
            result.description.version = trimmed.fromFirstOccurrenceOf("version:", false, false).trim();
        } else if (trimmed.startsWith("uid:")) {
            result.description.uniqueId = trimmed.fromFirstOccurrenceOf("uid:", false, false).trim().getIntValue();
        } else if (trimmed.startsWith("is_instrument:")) {
            auto isInstStr = trimmed.fromFirstOccurrenceOf("is_instrument:", false, false).trim();
            result.description.isInstrument = (isInstStr == "true");
        } else if (trimmed.startsWith("format:")) {
            result.description.pluginFormatName = trimmed.fromFirstOccurrenceOf("format:", false, false).trim();
        }
    }
    
    if (foundSuccess) {
        result.success = true;
        result.errorType = "success";
        result.description.fileOrIdentifier = file.getFullPathName();
        result.description.lastInfoUpdateTime = juce::Time::getCurrentTime();
        return result;
    } else {
        result.success = false;
        if (result.errorType.isEmpty())
            result.errorType = "parse_error";
        if (result.errorMessage.isEmpty())
            result.errorMessage = "Failed to parse plugin description";
        
        DBG("PluginHost: Parse error for " + file.getFileName());
        return result;
    }
}

bool PluginHost::scanFileOutProcess(const juce::File& file, juce::PluginDescription& result)
{
    auto scanResult = scanFileWithDetails(file);
    
    if (scanResult.success) {
        result = scanResult.description;
        return true;
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

  if (scanThread_.joinable()) {
    scanThread_.join();
  }

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

std::vector<PluginHost::ScanResult> PluginHost::scanWithResults(const juce::File& path)
{
    std::vector<ScanResult> results;
    
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (vst3Format == nullptr) {
        DBG("PluginHost: Cannot scan - VST3 format not available");
        return results;
    }

    if (!path.exists()) {
        DBG("PluginHost: Path does not exist: " + path.getFullPathName());
        return results;
    }

    DBG("PluginHost: Scanning path with results: " + path.getFullPathName());

    // Collect files to scan
    juce::Array<juce::File> filesToScan;
    if (path.isDirectory()) {
        path.findChildFiles(filesToScan, juce::File::findFiles, true, "*.vst3");
    } else if (path.hasFileExtension(".vst3")) {
        filesToScan.add(path);
    }

    // Scan each file with detailed results
    for (const auto& file : filesToScan) {
        auto result = scanFileWithDetails(file);
        results.push_back(result);
        
        if (result.success && !knowsAboutPlugin(result.description)) {
            addToKnownPlugins(result.description);
        }
    }

    // Store results
    {
        std::lock_guard<std::mutex> lock(scanResultsMutex_);
        lastScanResults_ = results;
    }

    DBG("PluginHost: Scan complete - " + juce::String(results.size()) + " plugins attempted");
    
    return results;
}

PluginHost::ScanStatistics PluginHost::getLastScanStatistics() const
{
    ScanStatistics stats;
    
    std::lock_guard<std::mutex> lock(scanResultsMutex_);
    
    stats.totalScanned = static_cast<int>(lastScanResults_.size());
    
    for (const auto& result : lastScanResults_) {
        if (result.success) {
            stats.found++;
        } else if (result.errorType == "blacklisted") {
            stats.blacklisted++;
        } else if (result.errorType == "timeout") {
            stats.timedOut++;
        } else if (result.errorType == "crash") {
            stats.crashed++;
        } else {
            stats.parseErrors++;
        }
    }
    
    return stats;
}

} // namespace zenith
