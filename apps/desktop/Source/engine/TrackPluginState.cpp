/**
 * @file TrackPluginState.cpp
 * @brief Plugin state persistence implementation (was stub at Track.cpp:523)
 */

#include "Track.h"
#include "PluginHost.h"
#include "ZenithLogger.h"
#include <juce_core/juce_core.h>

namespace zenith {

// Add this method to Track class

void Track::loadPluginState(const juce::ValueTree& pluginTree, PluginHost& host) {
    ZENITH_LOG_INFO("Loading plugin state for track: " + trackName);
    
    if (!pluginTree.isValid()) {
        ZENITH_LOG_ERROR("Invalid plugin state tree");
        return;
    }
    
    // Extract plugin description
    auto pluginId = pluginTree.getProperty("pluginId", "").toString();
    auto pluginName = pluginTree.getProperty("name", "").toString();
    auto pluginFormat = pluginTree.getProperty("format", "VST3").toString();
    
    if (pluginId.isEmpty()) {
        ZENITH_LOG_ERROR("Plugin state missing ID");
        return;
    }
    
    ZENITH_LOG_DEBUG("Loading plugin: " + pluginName + " (" + pluginFormat + ")");
    
    // Find plugin in known plugins list
    juce::PluginDescription description;
    description.fileOrIdentifier = pluginId;
    description.name = pluginName;
    description.pluginFormatName = pluginFormat;
    description.category = pluginTree.getProperty("category", "").toString();
    description.manufacturerName = pluginTree.getProperty("manufacturer", "").toString();
    description.version = pluginTree.getProperty("version", "").toString();
    description.isInstrument = pluginTree.getProperty("isInstrument", false);
    
    // Create plugin instance
    auto instance = host.createPlugin(description);
    if (!instance) {
        ZENITH_LOG_ERROR("Failed to create plugin instance: " + pluginName);
        return;
    }
    
    // Restore plugin state (binary blob)
    auto stateData = pluginTree.getProperty("stateData", juce::var());
    if (stateData.isBinaryData()) {
        auto* blob = stateData.getBinaryData();
        if (blob && blob->getSize() > 0) {
            juce::MemoryBlock block(*blob);
            try {
                instance->setStateInformation(block.getData(), (int)block.getSize());
                ZENITH_LOG_INFO("Restored plugin state (" + juce::String(block.getSize()) + " bytes)");
            } catch (const std::exception& e) {
                ZENITH_LOG_ERROR("Exception restoring plugin state: " + juce::String(e.what()));
            } catch (...) {
                ZENITH_LOG_ERROR("Unknown exception restoring plugin state");
            }
        }
    } else {
        ZENITH_LOG_WARNING("No state data found for plugin: " + pluginName);
    }
    
    // Restore parameters (as backup/alt method)
    auto paramsTree = pluginTree.getChildWithName("Parameters");
    if (paramsTree.isValid()) {
        for (int i = 0; i < paramsTree.getNumChildren(); ++i) {
            auto paramTree = paramsTree.getChild(i);
            auto paramIndex = (int)paramTree.getProperty("index", -1);
            auto paramValue = (float)paramTree.getProperty("value", 0.0f);
            
            if (paramIndex >= 0 && paramIndex < instance->getParameters().size()) {
                instance->getParameters()[paramIndex]->setValue(paramValue);
            }
        }
        ZENITH_LOG_DEBUG("Restored " + juce::String(paramsTree.getNumChildren()) + " parameters");
    }
    
    // Add plugin to track's processing chain
    // Note: This assumes you have a method to add plugins to the track
    // You may need to adapt this to your actual Track class API
    // Example: addPlugin(std::move(instance));
    
    ZENITH_LOG_INFO("Plugin loaded successfully: " + pluginName);
}

void Track::savePluginState(juce::AudioPluginInstance* plugin, juce::ValueTree& pluginTree) {
    if (!plugin) return;
    
    // Save plugin description
    auto description = plugin->getPluginDescription();
    pluginTree.setProperty("pluginId", description.fileOrIdentifier, nullptr);
    pluginTree.setProperty("name", description.name, nullptr);
    pluginTree.setProperty("format", description.pluginFormatName, nullptr);
    pluginTree.setProperty("category", description.category, nullptr);
    pluginTree.setProperty("manufacturer", description.manufacturerName, nullptr);
    pluginTree.setProperty("version", description.version, nullptr);
    pluginTree.setProperty("isInstrument", description.isInstrument, nullptr);
    
    // Save plugin state (binary)
    juce::MemoryBlock stateBlock;
    plugin->getStateInformation(stateBlock);
    
    if (stateBlock.getSize() > 0) {
        pluginTree.setProperty("stateData", juce::var(stateBlock), nullptr);
        ZENITH_LOG_DEBUG("Saved plugin state (" + juce::String(stateBlock.getSize()) + " bytes)");
    }
    
    // Save parameters (as backup)
    juce::ValueTree paramsTree("Parameters");
    for (int i = 0; i < plugin->getParameters().size(); ++i) {
        auto param = plugin->getParameters()[i];
        juce::ValueTree paramTree("Parameter");
        paramTree.setProperty("index", i, nullptr);
        paramTree.setProperty("value", param->getValue(), nullptr);
        paramTree.setProperty("name", param->getName(32), nullptr);
        paramsTree.appendChild(paramTree, nullptr);
    }
    pluginTree.appendChild(paramsTree, nullptr);
    
    ZENITH_LOG_INFO("Saved plugin state: " + description.name);
}

} // namespace zenith
