/*
  ==============================================================================
    PlatformAudioUtils_Mac.mm
    macOS-specific audio utilities implementation
  ==============================================================================
*/

#ifdef __APPLE__
#include "../../../engine/PlatformAudioUtils.h"
#include "../../../engine/ZenithLogger.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

void logAvailableAudioDevices(juce::AudioDeviceManager& deviceManager) {
    ZENITH_LOG_INFO("=== Available Audio Device Types (macOS) ===");
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

    // Try Core Audio (native macOS)
    for (auto* type : deviceManager.getAvailableDeviceTypes()) {
        if (type->getTypeName() == "CoreAudio") {
            deviceManager.setCurrentAudioDeviceType("CoreAudio", true);
            error = deviceManager.initialise(2, 2, nullptr, true);
            if (error.isEmpty()) {
                ZENITH_LOG_INFO("Audio initialized with Core Audio (Native macOS)");
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

    // Prefer Core Audio for lowest latency on macOS
    if (trySwitchTo("CoreAudio")) return;

    ZENITH_LOG_WARNING("Could not switch to preferred Core Audio backend; using default.");
}

} // namespace zenith
#endif // __APPLE__
