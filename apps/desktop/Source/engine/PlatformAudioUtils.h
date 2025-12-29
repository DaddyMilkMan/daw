/*
  ==============================================================================

    PlatformAudioUtils.h
    Created: 2025-12-22

    Interface for platform-specific audio engine utility functions.

  ==============================================================================
*/

#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {

class PlatformAudioUtils {
public:
    /**
     * Initializes the platform-specific audio device setup and fallbacks.
     */
    static void initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager);
};

} // namespace zenith
