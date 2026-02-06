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

#include <juce_core/juce_core.h>

namespace zenith {

/**
    Audio feedback system for UI interactions.
    
    Plays short audio cues to provide tactile feedback for user actions.
    All methods are thread-safe and non-blocking.
*/
class AudioFeedback {
public:
    /**
        Play an audio feedback sound.
        
        @param id Sound identifier:
               - "click"   - Button click
               - "error"   - Error notification
               - "success" - Success confirmation
               - "notify"  - General notification
    */
    static void play(const juce::String& id);
    
    /**
        Initialize the audio feedback system.
        Call once at application startup.
    */
    static void initialize();
    
    /**
        Shutdown the audio feedback system.
        Call at application shutdown.
    */
    static void shutdown();
};

} // namespace zenith
