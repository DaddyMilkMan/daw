/*
  ==============================================================================

    PlatformAudioUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-specific audio device initialization.

  ==============================================================================
*/

#include "PlatformAudioUtils.h"

namespace zenith {

#ifndef __linux__
void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager)
{
    // Basic initialization - can be expanded for WASAPI/ASIO specific logic on Windows
    // or CoreAudio on macOS.
    
    // For now, we rely on JUCE's default behavior which is usually sufficient 
    // for standard desktop apps unless specific routing is needed.
    // Specifying 2 inputs and 2 outputs as a safe default.
    deviceManager.initialise(2, 2, nullptr, true);
}
#endif

} // namespace zenith
