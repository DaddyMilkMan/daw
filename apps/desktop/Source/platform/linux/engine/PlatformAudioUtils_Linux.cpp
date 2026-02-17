/*
  ==============================================================================
    PlatformAudioUtils_Linux.cpp
    Linux-specific audio utilities implementation
  ==============================================================================
*/

#include "../../../engine/PlatformAudioUtils.h"
#include "../../../engine/ZenithLogger.h"
#include "../../../Settings.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <unistd.h>
#include <sys/types.h>

namespace zenith {

static bool isPipeWireRunning() {
#if JUCE_LINUX
    static bool hasChecked = false;
    static bool isRunning = false;

    if (hasChecked) return isRunning;

    juce::File runtimeDir;
    auto xdgEnv = juce::SystemStats::getEnvironmentVariable("XDG_RUNTIME_DIR", "");
    
    if (xdgEnv.isNotEmpty()) {
        runtimeDir = juce::File(xdgEnv);
    } else {
        runtimeDir = juce::File("/run/user").getChildFile(juce::String(getuid()));
    }

    if (runtimeDir.exists() && runtimeDir.isDirectory()) {
         if (runtimeDir.getChildFile("pipewire-0").exists()) {
             isRunning = true;
             hasChecked = true;
             return true;
         }
    }
    
    isRunning = false;
    hasChecked = true;
    return false;
#else
    return false;
#endif
}

void logAvailableAudioDevices(juce::AudioDeviceManager& deviceManager) {
    ZENITH_LOG_INFO("=== Available Audio Device Types (Linux) ===");
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
    deviceManager.setCurrentAudioDeviceType("ALSA", true);
    error = deviceManager.initialise(2, 2, nullptr, true);
    if (error.isEmpty()) {
        ZENITH_LOG_INFO("Audio initialized with ALSA");
        return true;
    }
    error = deviceManager.initialise(2, 2, nullptr, false);
    if (error.isEmpty()) {
        ZENITH_LOG_INFO("Audio initialized with default device");
        return true;
    }
    ZENITH_LOG_ERROR("Failed to initialize audio: " + error);
    return false;
}

void PlatformAudioUtils::initializeAudioDeviceSetup(juce::AudioDeviceManager &deviceManager) {
  auto& settings = Settings::getInstance();
  auto preferredBackend = settings.getLinuxAudioBackend();
  auto currentType = deviceManager.getCurrentAudioDeviceType();
  
  auto trySwitchTo = [&](const juce::String& typeName) -> bool {
      if (currentType == typeName) return true;
      const auto &availableTypes = deviceManager.getAvailableDeviceTypes();
      bool available = false;
      for (auto *type : availableTypes) {
          if (type->getTypeName() == typeName) {
              available = true;
              break;
          }
      }
      if (!available) return false;
      deviceManager.setCurrentAudioDeviceType(typeName, true);
      return deviceManager.getCurrentAudioDeviceType() == typeName;
  };

  if (preferredBackend != Settings::LinuxAudioBackend::Auto) {
      juce::String target = "";
      if (preferredBackend == Settings::LinuxAudioBackend::JACK) target = "JACK";
      else if (preferredBackend == Settings::LinuxAudioBackend::ALSA) target = "ALSA";
      else if (preferredBackend == Settings::LinuxAudioBackend::PipeWire) {
          target = isPipeWireRunning() ? "JACK" : "ALSA";
      }
      if (target.isNotEmpty() && trySwitchTo(target)) return;
  }

  if (isPipeWireRunning()) {
      if (trySwitchTo("JACK")) return;
  }
  if (trySwitchTo("JACK")) return;
  if (deviceManager.getCurrentAudioDevice() == nullptr || currentType != "ALSA") {
     trySwitchTo("ALSA");
  }
}


} // namespace zenith
