/*
  ==============================================================================

    PlatformAudioUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../engine/PlatformAudioUtils.h"
#include "../../../Settings.h"
#include <juce_gui_basics/juce_gui_basics.h> // For AlertWindow
#include <unistd.h> // For getuid()
#include <sys/types.h>

namespace zenith {

static bool isPipeWireRunning() {
#if JUCE_LINUX
    static bool hasChecked = false;
    static bool isRunning = false;

    // Return cached result if we've already checked
    if (hasChecked)
        return isRunning;

    // Method 1: Check XDG_RUNTIME_DIR
    // This is the standard definition for user-specific runtime files.
    // We utilize juce::SystemStats to get the environment variable robustly.
    juce::File runtimeDir;
    auto xdgEnv = juce::SystemStats::getEnvironmentVariable("XDG_RUNTIME_DIR", "");
    
    if (xdgEnv.isNotEmpty()) {
        runtimeDir = juce::File(xdgEnv);
    } else {
        // Method 2: Fallback to standard /run/user/<uid>
        // Use standard systemd location if XDG env var is missing
        runtimeDir = juce::File("/run/user").getChildFile(juce::String(getuid()));
    }

    // Check for the native PipeWire socket (usually "pipewire-0")
    // If the socket exists, the daemon is active and accepting connections.
    if (runtimeDir.exists() && runtimeDir.isDirectory()) {
         if (runtimeDir.getChildFile("pipewire-0").exists()) {
             isRunning = true;
             hasChecked = true;
             return true;
         }
    }

    // We intentionally avoid 'pgrep' here to ensure non-blocking behavior.
    // If the socket isn't open, we can't connect anyway, so it doesn't matter 
    // if the daemon process is technically running.
    
    isRunning = false;
    hasChecked = true;
    return false;
#else
    return false;
#endif
}

void PlatformAudioUtils::initializeAudioDeviceSetup(
    juce::AudioDeviceManager &deviceManager) {
  auto& settings = Settings::getInstance();
  auto preferredBackend = settings.getLinuxAudioBackend();

  // Reduced logging to avoid spam during startup.
  // Using atomic backend switching logic.
  
  auto currentType = deviceManager.getCurrentAudioDeviceType();
  
  // Helper to switch backend if available
  auto trySwitchTo = [&](const juce::String& typeName) -> bool {
      // Already on the desired backend
      if (currentType == typeName) return true;

      // Check availability
      const auto &availableTypes = deviceManager.getAvailableDeviceTypes();
      bool available = false;
      for (auto *type : availableTypes) {
          if (type->getTypeName() == typeName) {
              available = true;
              break;
          }
      }

      if (!available) return false;

      DBG("PlatformAudioUtils: Switching to " + typeName + " backend...");
      deviceManager.setCurrentAudioDeviceType(typeName, true);
      
      // confirm switch
      return deviceManager.getCurrentAudioDeviceType() == typeName;
  };

  // 1. Manual Override from Settings
  if (preferredBackend != Settings::LinuxAudioBackend::Auto) {
      juce::String target = "";
      if (preferredBackend == Settings::LinuxAudioBackend::JACK) target = "JACK";
      else if (preferredBackend == Settings::LinuxAudioBackend::ALSA) target = "ALSA";
      else if (preferredBackend == Settings::LinuxAudioBackend::PipeWire) {
          // PipeWire often shimmed via JACK, check simple presence.
          // If PipeWire is preferred, we try JACK if the socket is there, otherwise fallback to ALSA.
          target = isPipeWireRunning() ? "JACK" : "ALSA";
      }

      if (target.isNotEmpty() && trySwitchTo(target)) {
          return;
      }

      // Warn only if manual selection failed
      DBG("PlatformAudioUtils: Warning - Preferred backend " + target + " unavailable. Falling back to Auto.");
  }

  // 2. Auto-Detection Logic
  // Priority: JACK (if PipeWire detected or standard JACK) -> ALSA
  
  if (isPipeWireRunning()) {
      // If PipeWire is confirmed via socket, we aggressively prefer JACK
      if (trySwitchTo("JACK")) return;
  }

  // Try JACK anyway (e.g., standard JACK2 without PipeWire)
  if (trySwitchTo("JACK")) return;

  // Final Fallback: ALSA
  // Only try ALSA if we are not already on it or we have no device
  if (deviceManager.getCurrentAudioDevice() == nullptr || currentType != "ALSA") {
     if (!trySwitchTo("ALSA")) {
         DBG("PlatformAudioUtils: Critical - Failed to initialize ALSA fallback.");
     }
  }
}

} // namespace zenith