/*
  ==============================================================================

    PluginScanner.cpp
    Created: 2026-02-03
    Author: Critical Fixes Implementation

    Standalone plugin scanner executable for out-of-process plugin loading.
    This prevents crashes in problematic plugins from taking down the main DAW.

    Usage: PluginScanner <plugin_path>
    
    Returns:
      0 = Success (JSON written to stdout)
      1 = Failed to load plugin
      2 = Invalid arguments

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include <iostream>
#include <chrono>

/** 
 * @brief Timeout handler for hung plugins
 * 
 * Some plugins can hang indefinitely during scanning.
 * This class provides a timeout mechanism.
 */
class TimeoutChecker : public juce::Timer
{
public:
    TimeoutChecker(int timeoutMs) : timeoutMs_(timeoutMs) {}
    
    void startTimer(int timeoutMs)
    {
        juce::Timer::startTimer(timeoutMs);
    }
    
    void timerCallback() override
    {
        std::cerr << "Plugin scan timeout (" << timeoutMs_ << "ms)" << std::endl;
        juce::JUCEApplicationBase::quit();
    }
    
private:
    int timeoutMs_;
};

/**
 * @brief Scan a single plugin file and output JSON description
 */
class PluginScannerApp
{
public:
    int run(int argc, char* argv[])
    {
        if (argc < 2)
        {
            std::cerr << "Usage: PluginScanner <plugin_path>" << std::endl;
            std::cerr << "Example: PluginScanner /path/to/plugin.vst3" << std::endl;
            return 2;
        }
        
        juce::String pluginPath = argv[1];
        juce::File pluginFile(pluginPath);
        
        if (!pluginFile.existsAsFile())
        {
            std::cerr << "Plugin file not found: " << pluginPath << std::endl;
            return 1;
        }
        
        // Initialize JUCE (minimal initialization)
        juce::initialiseJuce_GUI();
        
        // Set up timeout (5 seconds)
        TimeoutChecker timeout(5000);
        timeout.startTimer(5000);
        
        // Try to scan plugin
        bool success = false;
        juce::PluginDescription description;
        
        try
        {
            // Determine plugin format
            if (pluginPath.endsWithIgnoreCase(".vst3"))
            {
                juce::VST3PluginFormat vst3Format;
                success = vst3Format.findAllTypesForFile(description, pluginPath);
            }
            else if (pluginPath.endsWithIgnoreCase(".vst"))
            {
                // VST2 support (if enabled)
                #if JUCE_PLUGINHOST_VST
                juce::VSTPluginFormat vstFormat;
                success = vstFormat.findAllTypesForFile(description, pluginPath);
                #else
                std::cerr << "VST2 support not enabled" << std::endl;
                juce::shutdownJuce_GUI();
                return 1;
                #endif
            }
            else if (pluginPath.endsWithIgnoreCase(".component"))
            {
                // Audio Unit (macOS only)
                #if JUCE_PLUGINHOST_AU
                juce::AudioUnitPluginFormat auFormat;
                success = auFormat.findAllTypesForFile(description, pluginPath);
                #else
                std::cerr << "Audio Unit support not enabled" << std::endl;
                juce::shutdownJuce_GUI();
                return 1;
                #endif
            }
            else
            {
                std::cerr << "Unsupported plugin format: " << pluginPath << std::endl;
                juce::shutdownJuce_GUI();
                return 1;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception during plugin scan: " << e.what() << std::endl;
            juce::shutdownJuce_GUI();
            return 1;
        }
        catch (...)
        {
            std::cerr << "Unknown exception during plugin scan" << std::endl;
            juce::shutdownJuce_GUI();
            return 1;
        }
        
        // Stop timeout
        timeout.stopTimer();
        
        if (success)
        {
            // Build JSON output
            auto json = new juce::DynamicObject();
            json->setProperty("success", true);
            json->setProperty("name", description.name);
            json->setProperty("descriptiveName", description.descriptiveName);
            json->setProperty("pluginFormatName", description.pluginFormatName);
            json->setProperty("category", description.category);
            json->setProperty("manufacturerName", description.manufacturerName);
            json->setProperty("version", description.version);
            json->setProperty("fileOrIdentifier", description.fileOrIdentifier);
            json->setProperty("lastFileModTime", description.lastFileModTime.toMilliseconds());
            json->setProperty("lastInfoUpdateTime", description.lastInfoUpdateTime.toMilliseconds());
            json->setProperty("uid", description.uid);
            json->setProperty("isInstrument", description.isInstrument);
            json->setProperty("numInputChannels", description.numInputChannels);
            json->setProperty("numOutputChannels", description.numOutputChannels);
            json->setProperty("hasSharedContainer", description.hasSharedContainer);
            
            // Output JSON to stdout
            std::cout << juce::JSON::toString(juce::var(json), false) << std::endl;
            
            juce::shutdownJuce_GUI();
            return 0;
        }
        else
        {
            // Failed to scan
            auto json = new juce::DynamicObject();
            json->setProperty("success", false);
            json->setProperty("error", "Failed to scan plugin");
            json->setProperty("path", pluginPath);
            
            std::cout << juce::JSON::toString(juce::var(json), false) << std::endl;
            
            juce::shutdownJuce_GUI();
            return 1;
        }
    }
};

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[])
{
    PluginScannerApp app;
    
    try
    {
        return app.run(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fatal error: Unknown exception" << std::endl;
        return 1;
    }
}
