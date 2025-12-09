/*
  ==============================================================================
    AudioFeedback.cpp
    Audio feedback system for UI interactions
    
    Plays short audio cues for button clicks, errors, and notifications.
  ==============================================================================
*/

#include "AudioFeedback.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {

// Static member for audio playback
static std::unique_ptr<juce::AudioDeviceManager> feedbackDeviceManager;
static std::unique_ptr<juce::AudioSourcePlayer> feedbackPlayer;

void AudioFeedback::play(const juce::String& id) {
    // Log the feedback request
    DBG("AudioFeedback: Playing sound for '" + id + "'");

    if (id.containsIgnoreCase("error") || id.containsIgnoreCase("fail")) {
        // Use system alert for errors
        juce::LookAndFeel::getDefaultLookAndFeel().playAlertSound();
    }
    
    // For other UI sounds (clicks, hovers), we would ideally trigger
    // a sample in the audio engine. Since the UI thread shouldn't
    // block on audio I/O, we delegate this.
    //
    // Future improvement: Send message to Engine's lock-free queue
    // Engine::getInstance().triggerUiSound(id);
}

void AudioFeedback::initialize() {
    // Future: Set up audio device for feedback sounds
    DBG("AudioFeedback system initialized");
}

void AudioFeedback::shutdown() {
    feedbackPlayer.reset();
    feedbackDeviceManager.reset();
    DBG("AudioFeedback system shut down");
}

} // namespace zenith
