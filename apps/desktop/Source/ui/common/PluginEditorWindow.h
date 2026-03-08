/*
  ==============================================================================
    PluginEditorWindow.h
    Plugin editor window management
    
    Manages floating windows for VST3/AU plugin editors.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <map>
#include <string>

namespace zenith {

/**
    Manages plugin editor windows.
    
    Handles creation, tracking, and cleanup of floating windows
    that host VST3/AU plugin editors.
*/
class PluginEditorWindowManager {
public:
    PluginEditorWindowManager();
    ~PluginEditorWindowManager();
    
    /**
        Open an editor window for a plugin.
        
        @param processor The audio processor to create an editor for
        @param trackName Name of the track containing the plugin
        @param slotIndex Insert slot index on the track
    */
    void openEditor(juce::AudioProcessor* processor, 
                    const juce::String& trackName,
                    int slotIndex);
    
    /**
        Close an editor window.
        
        @param trackName Name of the track
        @param slotIndex Insert slot index
    */
    void closeEditor(const juce::String& trackName, int slotIndex);
    
    /**
        Close all open editor windows.
    */
    void closeAllWindows();
    
    /**
        Check if an editor is currently open.
        
        @param trackName Name of the track
        @param slotIndex Insert slot index
        @return true if the editor window is open
    */
    bool isEditorOpen(const juce::String& trackName, int slotIndex) const;
    
private:
    // Map of "trackName_slotIndex" -> open state (Skia-only mode)
    std::map<std::string, bool> openWindows_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindowManager)
};

} // namespace zenith
