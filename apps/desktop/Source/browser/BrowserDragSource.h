/*
  ==============================================================================

    BrowserDragSource.h
    Created: 2025-12-05
    Author:  Zenith DAW

    Drag-and-drop source for browser items.
    Supports dragging samples, instruments, and plugins to tracks.

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BrowserData.h"
#include <memory>

namespace zenith {

/**
 * @brief Custom drag container for browser items
 * 
 * Contains all information needed to instantiate the dragged asset
 * on the target (Track, Mixer, Sampler, etc.)
 */
class BrowserDragData : public juce::ReferenceCountedObject
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<BrowserDragData>;
    
    explicit BrowserDragData(std::shared_ptr<BrowserItem> item)
        : item_(item)
    {
    }
    
    std::shared_ptr<BrowserItem> getItem() const { return item_; }
    BrowserItemType getType() const { return item_ ? item_->type : BrowserItemType::Unknown; }
    juce::String getId() const { return item_ ? item_->id : ""; }
    juce::String getName() const { return item_ ? item_->name : ""; }
    
    // Convenience checks
    bool isAudioFile() const { return getType() == BrowserItemType::AudioFile; }
    bool isMidiFile() const { return getType() == BrowserItemType::MidiFile; }
    bool isPlugin() const { return getType() == BrowserItemType::Plugin; }
    bool isInstrument() const { return getType() == BrowserItemType::Instrument; }
    bool isPreset() const { return getType() == BrowserItemType::Preset; }
    
    // For Plugin/Instrument drops
    bool isPluginOrInstrument() const { return isPlugin() || isInstrument(); }
    
    // Get file path (for audio/midi)
    juce::File getFile() const 
    { 
        if (item_ && (isAudioFile() || isMidiFile()))
            return juce::File(item_->id);
        return {};
    }

private:
    std::shared_ptr<BrowserItem> item_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserDragData)
};

/**
 * @brief Helper class for initiating drags from the browser
 */
class BrowserDragSource
{
public:
    // Unique identifier for browser drag operations
    static constexpr const char* dragDescriptionPrefix = "ZenithBrowser::";
    
    /**
     * @brief Create a drag description string
     */
    static juce::String createDragDescription(BrowserItemType type)
    {
        return dragDescriptionPrefix + BrowserItem::getEnumName(type);
    }
    
    /**
     * @brief Check if a drag description is from the browser
     */
    static bool isBrowserDrag(const juce::String& description)
    {
        return description.startsWith(dragDescriptionPrefix);
    }
    
    /**
     * @brief Get the type from a drag description
     */
    static BrowserItemType getTypeFromDescription(const juce::String& description)
    {
        if (!isBrowserDrag(description))
            return BrowserItemType::Unknown;
        
        juce::String typeName = description.fromFirstOccurrenceOf(dragDescriptionPrefix, false, false);
        
        if (typeName == "Audio") return BrowserItemType::AudioFile;
        if (typeName == "MIDI") return BrowserItemType::MidiFile;
        if (typeName == "Plugin") return BrowserItemType::Plugin;
        if (typeName == "Instrument") return BrowserItemType::Instrument;
        if (typeName == "Preset") return BrowserItemType::Preset;
        if (typeName == "Folder") return BrowserItemType::Folder;
        
        return BrowserItemType::Unknown;
    }
    
    /**
     * @brief Start a drag operation from a component
     * @param sourceComponent Component initiating the drag
     * @param item BrowserItem being dragged
     * @param dragImage Optional custom drag image
     */
    static void startDrag(juce::Component* sourceComponent, 
                          std::shared_ptr<BrowserItem> item,
                          const juce::Image& dragImage = {})
    {
        if (!sourceComponent || !item)
            return;
        
        // Create drag description that includes item ID
        // We encode item info in the description since JUCE 8 doesn't support custom drag data
        juce::String description = createDragDescription(item->type) + "|" + item->id + "|" + item->name;
        
        // Create default drag image if none provided
        juce::Image image = dragImage;
        if (image.isNull())
        {
            image = createDragImage(item);
        }
        
        // Start the drag
        juce::DragAndDropContainer* container = 
            juce::DragAndDropContainer::findParentDragContainerFor(sourceComponent);
        
        if (container != nullptr)
        {
            container->startDragging(juce::var(description), sourceComponent, image, true);
            DBG("BrowserDrag: Started dragging " + item->name);
        }
        else
        {
            DBG("BrowserDrag: No DragAndDropContainer found!");
        }
    }
    
    /**
     * @brief Parse drag description to extract item info
     */
    static bool parseDragDescription(const juce::String& description,
                                     BrowserItemType& outType,
                                     juce::String& outId,
                                     juce::String& outName)
    {
        if (!isBrowserDrag(description))
            return false;
        
        juce::StringArray parts;
        parts.addTokens(description, "|", "");
        
        if (parts.size() >= 3)
        {
            outType = getTypeFromDescription(parts[0]);
            outId = parts[1];
            outName = parts[2];
            return true;
        }
        
        return false;
    }

private:
    /**
     * @brief Create a default drag image for an item
     */
    static juce::Image createDragImage(std::shared_ptr<BrowserItem> item)
    {
        int width = 180;
        int height = 30;
        
        juce::Image image(juce::Image::ARGB, width, height, true);
        juce::Graphics g(image);
        
        // Background
        g.setColour(juce::Colour(0xdd333344));
        g.fillRoundedRectangle(0, 0, width, height, 4.0f);
        
        // Border
        g.setColour(juce::Colour(0xff00aaff));
        g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, 4.0f, 1.0f);
        
        // Icon color based on type
        juce::Colour iconColour;
        switch (item->type)
        {
            case BrowserItemType::AudioFile: iconColour = juce::Colour(0xff66dd66); break;
            case BrowserItemType::MidiFile: iconColour = juce::Colour(0xffdd66dd); break;
            case BrowserItemType::Plugin: iconColour = juce::Colour(0xff6699ff); break;
            case BrowserItemType::Instrument: iconColour = juce::Colour(0xff66ccff); break;
            case BrowserItemType::Preset: iconColour = juce::Colour(0xffcc66dd); break;
            default: iconColour = juce::Colours::grey; break;
        }
        
        // Icon circle
        g.setColour(iconColour);
        g.fillEllipse(8, 7, 16, 16);
        
        // Text
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(13.0f));
        
        juce::String displayName = item->name;
        if (displayName.length() > 22)
            displayName = displayName.substring(0, 20) + "...";
        
        g.drawText(displayName, 30, 0, width - 35, height, juce::Justification::centredLeft);
        
        return image;
    }
};

} // namespace zenith
