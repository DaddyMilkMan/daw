/*
  ==============================================================================

    PlatformAudioUtils_Windows.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "engine/PlatformAudioUtils.h"

#ifdef _WIN32
namespace zenith {

void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager) {
    // Windows implementation: ASIO -> WASAPI
    deviceManager.initialiseWithDefaultDevices(2, 2);
}

} // namespace zenith
#endif
