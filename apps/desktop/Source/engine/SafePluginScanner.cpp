#include "SafePluginScanner.h"

namespace zenith {

PluginBlacklistManager::PluginBlacklistManager() {
    loadBlacklist();
}

juce::File PluginBlacklistManager::getBlacklistFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("PluginBlacklist.xml");
}

void PluginBlacklistManager::addToBlacklist(const juce::String& pluginPath, const juce::String& reason) {
    if (isBlacklisted(pluginPath)) return;
    
    BlacklistEntry entry { pluginPath, reason, juce::Time::getCurrentTime() };
    entries.add(entry);
    saveBlacklist();
    
    juce::Logger::writeToLog("Blacklisted plugin: " + pluginPath + " Reason: " + reason);
}

bool PluginBlacklistManager::isBlacklisted(const juce::String& pluginPath) const {
    for (const auto& entry : entries) {
        if (entry.path == pluginPath) return true;
    }
    return false;
}

void PluginBlacklistManager::removeFromBlacklist(const juce::String& pluginPath) {
    for (int i = 0; i < entries.size(); ++i) {
        if (entries[i].path == pluginPath) {
            entries.remove(i);
            saveBlacklist();
            return;
        }
    }
}

void PluginBlacklistManager::saveBlacklist() {
    juce::XmlElement xml("BLACKLIST");
    for (const auto& entry : entries) {
        auto* child = new juce::XmlElement("PLUGIN");
        child->setAttribute("path", entry.path);
        child->setAttribute("reason", entry.reason);
        child->setAttribute("timestamp", juce::String(entry.timestamp.toMilliseconds()));
        xml.addChildElement(child);
    }
    
    auto file = getBlacklistFile();
    file.getParentDirectory().createDirectory();
    xml.writeTo(file);
}

void PluginBlacklistManager::loadBlacklist() {
    entries.clear();
    auto file = getBlacklistFile();
    if (!file.existsAsFile()) return;
    
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
    if (xml != nullptr && xml->hasTagName("BLACKLIST")) {
        for (auto* child : xml->getChildIterator()) {
            if (child->hasTagName("PLUGIN")) {
                BlacklistEntry entry;
                entry.path = child->getStringAttribute("path");
                entry.reason = child->getStringAttribute("reason");
                entry.timestamp = juce::Time(child->getStringAttribute("timestamp").getLargeIntValue());
                entries.add(entry);
            }
        }
    }
}

// ==============================================================================

SafePluginScanner::SafePluginScanner() {
    checkPreviousScanningState();
}

SafePluginScanner::~SafePluginScanner() {
    clearScanningState();
}

juce::File SafePluginScanner::getScanningStateFile() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("ScanningState.xml");
}

void SafePluginScanner::checkPreviousScanningState() {
    auto file = getScanningStateFile();
    if (file.existsAsFile()) {
        juce::String crashedPlugin = file.loadFileAsString().trim();
        if (crashedPlugin.isNotEmpty()) {
            blacklistManager.addToBlacklist(crashedPlugin, "DAW crashed during scan");
            juce::Logger::writeToLog("Recovered from previous crash. Blacklisted: " + crashedPlugin);
        }
        file.deleteFile();
    }
}

void SafePluginScanner::setCurrentlyScanning(const juce::String& pluginPath) {
    auto file = getScanningStateFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(pluginPath);
}

void SafePluginScanner::clearCurrentlyScanning() {
    auto file = getScanningStateFile();
    if (file.existsAsFile()) {
        file.deleteFile();
    }
}

void SafePluginScanner::clearScanningState() {
    clearCurrentlyScanning();
}

bool SafePluginScanner::scanPluginOutOfProcess(const juce::File& file, juce::PluginDescription& result) {
    juce::String pluginPath = file.getFullPathName();
    
    if (blacklistManager.isBlacklisted(pluginPath)) {
        juce::Logger::writeToLog("Skipping blacklisted plugin: " + pluginPath);
        return false;
    }
    
    juce::File currentApp = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    juce::File scannerExe = currentApp.getSiblingFile("ZenithPluginScanner");
    
    #if JUCE_WINDOWS
    if (!scannerExe.hasFileExtension("exe")) scannerExe = scannerExe.withFileExtension("exe");
    #endif

    if (!scannerExe.existsAsFile()) {
        juce::Logger::writeToLog("Scanner executable not found at: " + scannerExe.getFullPathName());
        return false;
    }

    // Set dead man's switch
    setCurrentlyScanning(pluginPath);

    juce::ChildProcess process;
    juce::StringArray args;
    args.add(scannerExe.getFullPathName());
    args.add(pluginPath); // ZenithPluginScanner expects path as argv[1]

    if (process.start(args)) {
        juce::String output = process.readAllProcessOutput(); // This blocks until child exits
        
        if (process.getExitCode() == 0) {
            try {
                output = output.trim();
                auto lines = juce::StringArray::fromLines(output);
                bool foundSuccess = false;
                
                for (const auto& line : lines) {
                    auto trimmed = line.trim();
                    if (trimmed.startsWith("STATUS=")) {
                        if (trimmed.substring(7).trim() == "success") foundSuccess = true;
                    } else if (trimmed.startsWith("NAME=")) {
                        result.name = trimmed.substring(5).trim();
                    } else if (trimmed.startsWith("MANUFACTURER=")) {
                        result.manufacturerName = trimmed.substring(13).trim();
                    } else if (trimmed.startsWith("VERSION=")) {
                        result.version = trimmed.substring(8).trim();
                    } else if (trimmed.startsWith("UID=")) {
                        result.uniqueId = trimmed.substring(4).trim().getIntValue();
                    } else if (trimmed.startsWith("IS_INSTRUMENT=")) {
                        result.isInstrument = (trimmed.substring(14).trim() == "true");
                    } else if (trimmed.startsWith("FORMAT=")) {
                        result.pluginFormatName = trimmed.substring(7).trim();
                    }
                }
                
                if (foundSuccess) {
                    result.fileOrIdentifier = pluginPath;
                    result.lastInfoUpdateTime = juce::Time::getCurrentTime();
                    clearCurrentlyScanning();
                    return true;
                } else {
                    blacklistManager.addToBlacklist(pluginPath, "Invalid scanner output: " + output);
                }
            } catch (...) {
                blacklistManager.addToBlacklist(pluginPath, "Parse exception");
            }
        } else {
            blacklistManager.addToBlacklist(pluginPath, "Crashed (Exit code: " + juce::String(process.getExitCode()) + ")");
        }
    } else {
        juce::Logger::writeToLog("Failed to start scanner process.");
    }
    
    clearCurrentlyScanning();
    return false;
}

} // namespace zenith