/*
  ==============================================================================
    PlatformAudioUtils_Linux.cpp
    Linux-specific audio utilities implementation
  ==============================================================================
*/

#include "engine/Engine.h"
#include "engine/ZenithLogger.h"
#include "engine/PlatformAudioUtils.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {

void logAvailableAudioDevices(juce::AudioDeviceManager& deviceManager) {
    // Log all available audio device types on Linux
    ZENITH_LOG_INFO("=== Available Audio Device Types (Linux) ===");
    
    auto& types = deviceManager.getAvailableDeviceTypes();
    for (auto* type : types) {
        ZENITH_LOG_INFO("Device Type: " + type->getTypeName());
        
        juce::StringArray deviceNames = type->getDeviceNames();
        for (const auto& name : deviceNames) {
            ZENITH_LOG_INFO("  - " + name);
        }
    }
    
    // Log current device
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice()) {
        ZENITH_LOG_INFO("Current Audio Device: " + currentDevice->getName());
        ZENITH_LOG_INFO("  Sample Rate: " + juce::String(currentDevice->getCurrentSampleRate()));
        ZENITH_LOG_INFO("  Buffer Size: " + juce::String(currentDevice->getCurrentBufferSizeSamples()));
    }
}

bool initializeAudioWithFallback(juce::AudioDeviceManager& deviceManager) {
    // Try JACK first if available
    juce::String error;
    
    // Try to initialize with JACK
    for (auto* type : deviceManager.getAvailableDeviceTypes()) {
        if (type->getTypeName() == "JACK") {
            deviceManager.setCurrentAudioDeviceType("JACK", true);
            error = deviceManager.initialise(2, 2, nullptr, true);
            if (error.isEmpty()) {
                ZENITH_LOG_INFO("Audio initialized with JACK");
                return true;
            }
        }
    }
    
    // Fallback to ALSA
    deviceManager.setCurrentAudioDeviceType("ALSA", true);
    error = deviceManager.initialise(2, 2, nullptr, true);
    if (error.isEmpty()) {
        ZENITH_LOG_INFO("Audio initialized with ALSA");
        return true;
    }
    
    // Last resort: default device
    error = deviceManager.initialise(2, 2, nullptr, false);
    if (error.isEmpty()) {
        ZENITH_LOG_INFO("Audio initialized with default device");
        return true;
    }
    
    ZENITH_LOG_ERROR("Failed to initialize audio: " + error);
    return false;
}

void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager) {
    initializeAudioWithFallback(deviceManager);
}

} // namespace zenith
