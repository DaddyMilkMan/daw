/*
  ==============================================================================

    BrowserData.h
    Created: 2025-12-05
    Author:  Zenith DAW

    Defines the data structures for the Universal Media Browser.
    All browser assets (files, plugins, presets) are normalized into these types.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <memory>
#include <map>

namespace zenith {

//==============================================================================
/**
 * Types of items that can appear in the browser
 */
enum class BrowserItemType
{
    Folder,         // Directory or logical grouping
    AudioFile,      // WAV, AIFF, MP3, etc.
    MidiFile,       // .mid files
    Plugin,         // VST3, AU, etc.
    Instrument,     // Internal Zenith Instrument
    Preset,         // Instrument or Effect Preset
    Project,        // .zenith project file
    Unknown
};

//==============================================================================
/**
 * Metadata for a browser item.
 * Used for searching, filtering, and displaying details.
 */
struct BrowserItemMetadata
{
    juce::String description;
    juce::String format;        // e.g. "VST3", "WAV", "44.1kHz"
    juce::String author;        // Manufacturer or Artist
    juce::String category;      // e.g. "Synth", "Bass", "Drums"
    std::vector<juce::String> tags;
    
    // Audio specific
    double duration = 0.0;
    int sampleRate = 0;
    int channels = 0;
    
    // Plugin specific
    juce::String version;
    bool isInstrument = false;
};

//==============================================================================
/**
 * A single item in the browser tree.
 * Can represent a file, a folder, a plugin, or a logical group.
 */
struct BrowserItem
{
    juce::String id;            // Unique identifier (path or internal ID)
    juce::String name;          // Display name
    BrowserItemType type = BrowserItemType::Unknown;
    
    BrowserItemMetadata metadata;
    
    // Hierarchy
    std::weak_ptr<BrowserItem> parent;
    std::vector<std::shared_ptr<BrowserItem>> children;
    
    // State
    bool isDirectory = false;   // Can have children
    bool isFavorite = false;
    
    // Icon / Visuals (Calculated type-safe icon helper could go here)
    
    BrowserItem() = default;
    
    BrowserItem(const juce::String& itemId, const juce::String& itemName, BrowserItemType itemType)
        : id(itemId), name(itemName), type(itemType)
    {
        isDirectory = (type == BrowserItemType::Folder);
    }
    
    // Helper to generic check
    bool isAudio() const { return type == BrowserItemType::AudioFile; }
    bool isMidi() const { return type == BrowserItemType::MidiFile; }
    bool isPlugin() const { return type == BrowserItemType::Plugin || type == BrowserItemType::Instrument; }
    
    // child management
    void addChild(std::shared_ptr<BrowserItem> child)
    {
        if (child)
        {
            children.push_back(child);
            // We need a shared_from_this mechanism if we want to set parent safely here
            // For now, parent setting is the caller's responsibility or we use raw pointers for parents
        }
    }
    
    bool hasChildren() const { return !children.empty(); }
    
    // Debug helper
    juce::String toString() const
    {
        return name + " (" + getEnumName(type) + ")";
    }
    
    static juce::String getEnumName(BrowserItemType t)
    {
        switch(t)
        {
            case BrowserItemType::Folder: return "Folder";
            case BrowserItemType::AudioFile: return "Audio";
            case BrowserItemType::MidiFile: return "MIDI";
            case BrowserItemType::Plugin: return "Plugin";
            case BrowserItemType::Instrument: return "Instrument";
            case BrowserItemType::Preset: return "Preset";
            case BrowserItemType::Project: return "Project";
            default: return "Unknown";
        }
    }
};

} // namespace zenith
