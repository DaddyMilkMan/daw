/*
  ==============================================================================

    PluginScanner.cpp
    Created: 2025-12-23
    Author:  Zenith DAW

    Standalone subprocess for safe plugin scanning.
    Takes a plugin path as an argument and outputs its description as JSON.

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <iostream>

/**
 * @brief Simple JSON serializer for PluginDescription
 */
juce::String descriptionToJSON(const juce::PluginDescription& desc)
{
    auto obj = new juce::DynamicObject();
    
    obj->setProperty("name", desc.name);
    obj->setProperty("descriptiveName", desc.descriptiveName);
    obj->setProperty("manufacturerName", desc.manufacturerName);
    obj->setProperty("version", desc.version);
    obj->setProperty("file", desc.fileOrIdentifier);
    obj->setProperty("lastFileModTime", desc.lastFileModTime.toMilliseconds());
    obj->setProperty("lastKnownEditTime", desc.lastKnownEditTime.toMilliseconds());
    obj->setProperty("isInstrument", desc.isInstrument);
    obj->setProperty("pluginFormatName", desc.pluginFormatName);
    obj->setProperty("category", desc.category);
    obj->setProperty("numInputChannels", desc.numInputChannels);
    obj->setProperty("numOutputChannels", desc.numOutputChannels);
    obj->setProperty("hasSharedContainer", desc.hasSharedContainer);
    obj->setProperty("uniqueId", desc.uniqueId);
    obj->setProperty("identifier", desc.createIdentifierString());
    
    return juce::JSON::toString(juce::var(obj));
}

int main(int argc, char* argv[])
{
    // Minimal JUCE environment
    juce::ScopedJuceInitialiser_GUI initialiser;
    
    if (argc < 2)
    {
        std::cerr << "Usage: PluginScanner <plugin_path>" << std::endl;
        return 1;
    }
    
    juce::String pluginPath = argv[1];
    juce::File pluginFile(pluginPath);
    
    if (!pluginFile.exists())
    {
        std::cerr << "Error: File does not exist: " << pluginPath.toStdString() << std::endl;
        return 1;
    }
    
    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();
    
    juce::OwnedArray<juce::PluginDescription> foundTypes;
    
    // We try to scan with each format
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        
        // Skip formats that don't match the file extension if possible
        if (!format->canHandleFile(pluginFile))
            continue;
            
        if (format->findAllTypesForFile(foundTypes, pluginFile))
        {
            // If we found any types, output them as JSON (one per line)
            for (auto* desc : foundTypes)
            {
                if (desc != nullptr)
                {
                    std::cout << "ZENITH_PLUGIN_START" << std::endl;
                    std::cout << descriptionToJSON(*desc).toStdString() << std::endl;
                    std::cout << "ZENITH_PLUGIN_END" << std::endl;
                }
            }
            return 0;
        }
    }
    
    std::cerr << "Error: No plugin found in: " << pluginPath.toStdString() << std::endl;
    return 1;
}
