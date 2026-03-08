/*
  ==============================================================================

    PluginManagerIntegration.cpp
    Example: How to integrate OOMHandler with a real plugin manager

    This is WORKING CODE that you can adapt for Zenith DAW.

  ==============================================================================
*/

#include "../memory/OOMHandler.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <iostream>

using namespace zenith;

//==============================================================================
/**
 * @brief Example plugin manager with OOM integration
 *
 * This shows the PROFESSIONAL way to integrate memory safety with your
 * actual plugin management system.
 */
class PluginManagerWithOOM {
public:
    //==========================================================================
    PluginManagerWithOOM() {
        // Register OOM callbacks during initialization
        setupOOMCallbacks();
        std::cout << "PluginManager: Initialized with OOM protection" << std::endl;
    }

    //==========================================================================
    /**
     * @brief Load a plugin (with memory tracking)
     */
    juce::AudioPluginInstance* loadPlugin(const juce::PluginDescription& desc) {
        auto& oom = OOMHandler::getInstance();

        // Check memory before loading
        auto snapshot = oom.getMemorySnapshot();
        if (snapshot.memoryUsagePercent > 80.0) {
            std::cerr << "PluginManager: WARNING - High memory usage ("
                      << snapshot.memoryUsagePercent
                      << "%), consider closing plugins" << std::endl;
        }

        // Load the plugin
        juce::String errorMessage;
        auto* plugin = knownPlugins_.createPluginInstance(desc, 48000, 512, errorMessage);

        if (plugin != nullptr) {
            // Track the plugin
            activePlugins_[plugin] = {
                desc.name,
                getPluginMemoryUsage(plugin),  // Estimate
                juce::Time::getCurrentTime()
            };

            std::cout << "PluginManager: Loaded " << desc.name
                      << " (~" << (activePlugins_[plugin].estimatedMemoryBytes / (1024*1024))
                      << " MB)" << std::endl;
        }

        return plugin;
    }

    //==========================================================================
    /**
     * @brief Unload a plugin
     */
    void unloadPlugin(juce::AudioPluginInstance* plugin) {
        if (plugin == nullptr) return;

        auto it = activePlugins_.find(plugin);
        if (it != activePlugins_.end()) {
            std::cout << "PluginManager: Unloading " << it->second.name
                      << " (freed ~" << (it->second.estimatedMemoryBytes / (1024*1024))
                      << " MB)" << std::endl;

            activePlugins_.erase(it);
        }

        delete plugin;
    }

    //==========================================================================
    /**
     * @brief Get all active plugins
     */
    std::vector<juce::AudioPluginInstance*> getActivePlugins() const {
        std::vector<juce::AudioPluginInstance*> plugins;
        for (const auto& pair : activePlugins_) {
            plugins.push_back(pair.first);
        }
        return plugins;
    }

private:
    //==========================================================================
    struct PluginInfo {
        juce::String name;
        juce::uint64 estimatedMemoryBytes;
        juce::Time loadTime;
    };

    juce::KnownPluginList knownPlugins_;
    std::map<juce::AudioPluginInstance*, PluginInfo> activePlugins_;

    //==========================================================================
    /**
     * @brief Setup OOM callbacks
     *
     * THIS IS THE CRITICAL PART - shows how to properly integrate
     */
    void setupOOMCallbacks() {
        auto& oom = OOMHandler::getInstance();

        // Plugin close callback - OOMHandler calls this when needed
        oom.setPluginCloseCallback([this](juce::uint64 targetBytesToFree) -> juce::uint64 {
            std::cout << "PluginManager: OOM callback - Need to free "
                      << (targetBytesToFree / (1024*1024)) << " MB" << std::endl;

            juce::uint64 totalFreed = 0;

            // Sort plugins by memory usage (largest first)
            std::vector<std::pair<juce::AudioPluginInstance*, PluginInfo>> sorted;
            for (const auto& pair : activePlugins_) {
                sorted.push_back(pair);
            }

            std::sort(sorted.begin(), sorted.end(),
                [](const auto& a, const auto& b) {
                    return a.second.estimatedMemoryBytes > b.second.estimatedMemoryBytes;
                });

            // Close plugins until target reached
            for (const auto& pair : sorted) {
                if (totalFreed >= targetBytesToFree) break;

                std::cout << "  Closing: " << pair.second.name
                          << " (~" << (pair.second.estimatedMemoryBytes / (1024*1024))
                          << " MB)" << std::endl;

                totalFreed += pair.second.estimatedMemoryBytes;
                unloadPlugin(pair.first);
            }

            return totalFreed;
        });

        std::cout << "PluginManager: OOM callbacks registered" << std::endl;
    }

    //==========================================================================
    /**
     * @brief Estimate plugin memory usage
     *
     * Real implementation would query the actual plugin
     */
    juce::uint64 getPluginMemoryUsage(juce::AudioPluginInstance* plugin) {
        // Conservative estimates
        if (plugin->getPluginDescription().isInstrument) {
            return 200 * 1024 * 1024;  // 200MB for instruments
        } else {
            return 50 * 1024 * 1024;   // 50MB for effects
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManagerWithOOM)
};

//==============================================================================
/**
 * @brief Example usage
 */
void examplePluginManagerUsage() {
    PluginManagerWithOOM pluginManager;

    // Simulate loading plugins
    juce::PluginDescription desc;
    desc.name = "Test Plugin";
    desc.pluginFormatName = "VST";

    // In real usage, you'd load actual plugins
    // auto* plugin = pluginManager.loadPlugin(desc);

    std::cout << "\nExample: Plugin manager with OOM integration loaded" << std::endl;
}
