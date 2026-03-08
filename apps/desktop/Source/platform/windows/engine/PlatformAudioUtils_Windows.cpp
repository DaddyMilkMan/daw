/*
  ==============================================================================
    PlatformAudioUtils_Windows.cpp
    Windows-specific audio utilities implementation
  ==============================================================================
*/

#ifdef _WIN32
#include "../../../engine/PlatformAudioUtils.h"
#include "../../../engine/ZenithLogger.h"
#include "../../../Settings.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

void logAvailableAudioDevices(juce::AudioDeviceManager& deviceManager) {
    ZENITH_LOG_INFO("=== Available Audio Device Types (Windows) ===");
    auto& types = deviceManager.getAvailableDeviceTypes();
    for (auto* type : types) {
        ZENITH_LOG_INFO("Device Type: " + type->getTypeName());
        juce::StringArray deviceNames = type->getDeviceNames();
        for (const auto& name : deviceNames) {
            ZENITH_LOG_INFO("  - " + name);
        }
    }
}

bool initializeAudioWithFallback(juce::AudioDeviceManager& deviceManager) {
    juce::String error;

    // Try ASIO first (Professional priority)
    for (auto* type : deviceManager.getAvailableDeviceTypes()) {
        if (type->getTypeName() == "ASIO") {
            deviceManager.setCurrentAudioDeviceType("ASIO", true);
            error = deviceManager.initialise(2, 2, nullptr, true);
            if (error.isEmpty()) {
                ZENITH_LOG_INFO("Audio initialized with ASIO (Professional Low Latency)");
                return true;
            }
        }
    }

    // Try WASAPI second (Modern Windows priority)
    for (auto* type : deviceManager.getAvailableDeviceTypes()) {
        if (type->getTypeName() == "Windows Audio") {
            deviceManager.setCurrentAudioDeviceType("Windows Audio", true);
            error = deviceManager.initialise(2, 2, nullptr, true);
            if (error.isEmpty()) {
                ZENITH_LOG_INFO("Audio initialized with Windows Audio (WASAPI)");
                return true;
            }
        }
    }

    // Try DirectSound as fallback
    for (auto* type : deviceManager.getAvailableDeviceTypes()) {
        if (type->getTypeName() == "DirectSound") {
            deviceManager.setCurrentAudioDeviceType("DirectSound", true);
            error = deviceManager.initialise(2, 2, nullptr, true);
            if (error.isEmpty()) {
                ZENITH_LOG_INFO("Audio initialized with DirectSound");
                return true;
            }
        }
    }

    // Last resort: any available device
    error = deviceManager.initialise(2, 2, nullptr, false);
    if (error.isEmpty()) {
        ZENITH_LOG_INFO("Audio initialized with default device");
        return true;
    }

    ZENITH_LOG_ERROR("Failed to initialize audio: " + error);
    return false;
}

void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager& deviceManager) {
    auto currentType = deviceManager.getCurrentAudioDeviceType();

    auto trySwitchTo = [&](const juce::String& typeName) -> bool {
        if (currentType == typeName) return true;
        const auto& availableTypes = deviceManager.getAvailableDeviceTypes();
        bool available = false;
        for (auto* type : availableTypes) {
            if (type->getTypeName() == typeName) {
                available = true;
                break;
            }
        }
        if (!available) return false;
        deviceManager.setCurrentAudioDeviceType(typeName, true);
        return deviceManager.getCurrentAudioDeviceType() == typeName;
    };

    // Prefer ASIO (Professional)
    if (trySwitchTo("ASIO")) return;

    // Prefer WASAPI (Modern Windows)
    if (trySwitchTo("Windows Audio")) return;

    // Fall back to DirectSound
    if (trySwitchTo("DirectSound")) return;

    ZENITH_LOG_WARNING("Could not switch to preferred Windows audio backend; using default.");
}

} // namespace zenith
#endif // _WIN32
