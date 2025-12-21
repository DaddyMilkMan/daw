/*
  ==============================================================================
    AudioFeedback.h
    Audio feedback system for UI interactions
    
    Provides audio cues for button clicks, errors, and notifications.
  ==============================================================================
*/

#pragma once
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
