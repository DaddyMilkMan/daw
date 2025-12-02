/**
 * @file PluginHostAsync.cpp
 * @brief Implements async plugin scanning (was TODO at line 57)
 * @author Marcus "The Craftsman" - Operation Polish A+ Grade
 */

#include "../engine/PluginHost.h"
#include "../engine/ZenithLogger.h"
#include <thread>

namespace zenith {

void PluginHost::scanAsync(std::function<void(int, int, const juce::String&)> progressCallback) {
    if (isScanning_) {
        ZENITH_LOG_WARNING("Plugin scan already in progress");
        return;
    }
    
    isScanning_ = true;
    ZENITH_LOG_INFO("Starting async plugin scan");
    
    // Launch scan on background thread
    std::thread scanThread([this, progressCallback]() {
        jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread());
        
        juce::Array<juce::File> defaultLocations;
        
        #ifdef JUCE_WINDOWS
            defaultLocations.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
            defaultLocations.add(juce::File(juce::File::getSpecialLocation(
                juce::File::globalApplicationsDirectory).getFullPathName() + "\\Common Files\\VST3"));
        #elif JUCE_MAC
            defaultLocations.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
            defaultLocations.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
        #endif
        
        int totalPlugins = 0;
        int scannedCount = 0;
        
        // First pass: count total plugins
        for (const auto& location : defaultLocations) {
            if (location.exists() && location.isDirectory()) {
                totalPlugins += location.findChildFiles(
                    juce::File::findFiles, 
                    true, 
                    "*.vst3").size();
            }
        }
        
        ZENITH_LOG_INFO("Found " + juce::String(totalPlugins) + " potential plugins");
        
        // Second pass: scan each plugin
        for (const auto& location : defaultLocations) {
            if (!location.exists() || !location.isDirectory()) continue;
            
            auto pluginFiles = location.findChildFiles(
                juce::File::findFiles, 
                true, 
                "*.vst3");
            
            for (const auto& pluginFile : pluginFiles) {
                if (!isScanning_) {
                    ZENITH_LOG_WARNING("Plugin scan cancelled");
                    return;
                }
                
                // Report progress on message thread
                juce::MessageManager::callAsync([progressCallback, scannedCount, totalPlugins, pluginFile]() {
                    if (progressCallback) {
                        progressCallback(scannedCount, totalPlugins, pluginFile.getFileName());
                    }
                });
                
                // Scan the plugin
                juce::OwnedArray<juce::PluginDescription> typesFound;
                
                if (vst3Format_) {
                    vst3Format_->findAllTypesForFile(typesFound, pluginFile.getFullPathName());
                    
                    for (auto* desc : typesFound) {
                        if (!knowsAboutPlugin(*desc)) {
                            addToKnownPlugins(*desc);
                            ZENITH_LOG_DEBUG("Added plugin: " + desc->name);
                        }
                    }
                }
                
                scannedCount++;
            }
        }
        
        // Final callback
        juce::MessageManager::callAsync([this, progressCallback, scannedCount]() {
            isScanning_ = false;
            ZENITH_LOG_INFO("Plugin scan complete: " + juce::String(scannedCount) + " plugins scanned");
            ZENITH_LOG_INFO("Total known plugins: " + juce::String(knownPlugins.getNumTypes()));
            
            if (progressCallback) {
                progressCallback(scannedCount, scannedCount, "Complete");
            }
        });
    });
    
    scanThread.detach(); // Let it run independently
}

void PluginHost::cancelScan() {
    if (isScanning_) {
        ZENITH_LOG_INFO("Cancelling plugin scan");
        isScanning_ = false;
    }
}

bool PluginHost::isScanningPlugins() const {
    return isScanning_;
}

bool PluginHost::knowsAboutPlugin(const juce::PluginDescription& desc) const {
    for (int i = 0; i < knownPlugins.getNumTypes(); ++i) {
        auto* existing = knownPlugins.getType(i);
        if (existing->fileOrIdentifier == desc.fileOrIdentifier &&
            existing->pluginFormatName == desc.pluginFormatName) {
            return true;
        }
    }
    return false;
}

void PluginHost::addToKnownPlugins(const juce::PluginDescription& desc) {
    knownPlugins.addType(desc);
}

} // namespace zenith
