/*
  ==============================================================================

    PlatformAudioUtils_Mac.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "engine/PlatformAudioUtils.h"

#ifdef __APPLE__
namespace zenith {

void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager) {
    // Mac implementation: CoreAudio
    deviceManager.initialiseWithDefaultDevices(2, 2);
}

} // namespace zenith
#endif
