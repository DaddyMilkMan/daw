/*
  ==============================================================================

    PlatformAudioUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../engine/PlatformAudioUtils.h"

namespace zenith {

void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager) {
    // Linux implementation: Fallback strategy is handled by JUCE (ALSA/Pulse/JACK)
    // We just ensure it's initialized if not already
    // Note: Engine calls initialiseWithDefaultDevices(2, 2) before calling this.
    // So this is mostly for specific Linux tweaks if needed.
    
    // For now, no extra setup needed on Linux beyond what Engine does.
    // If Engine failed, we might try a different audio device type here.
    
    // Example: If ALSA failed, maybe try PulseAudio specifically if needed,
    // but JUCE's default usually tries them in order.
}

} // namespace zenith