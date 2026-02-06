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

// TrackPluginState.cpp - Plugin state persistence implementation

#include "Track.h"
#include "PluginHost.h"
#include "ZenithLogger.h"

#include <limits>

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
                const auto size = block.getSize();
                if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
                    ZENITH_LOG_ERROR("Plugin state is too large to restore: " + juce::String(size) + " bytes");
                } else {
                    instance->setStateInformation(block.getData(), static_cast<int>(size));
                    ZENITH_LOG_INFO("Restored plugin state (" + juce::String(size) + " bytes)");
                }
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
            auto paramIndex = static_cast<int>(paramTree.getProperty("index", -1));
            auto paramValue = static_cast<float>(paramTree.getProperty("value", 0.0f));
            
            if (paramIndex >= 0 && paramIndex < instance->getParameters().size()) {
                instance->getParameters()[paramIndex]->setValue(paramValue);
            }
        }
        ZENITH_LOG_DEBUG("Restored " + juce::String(paramsTree.getNumChildren()) + " parameters");
    }
    
    // Add plugin to track's processing chain
    addPlugin(std::move(instance));
    
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
