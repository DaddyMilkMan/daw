/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux
#pragma once
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

/*
    ==============================================================================
    PluginEditorWindow.h
    Plugin editor window management
    
    Manages floating windows for VST3/AU plugin editors.
    ==============================================================================
*/



#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
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
    // Map of "trackName_slotIndex" -> window
    std::map<std::string, std::unique_ptr<juce::DocumentWindow>> openWindows_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindowManager)
};

} // namespace zenith
