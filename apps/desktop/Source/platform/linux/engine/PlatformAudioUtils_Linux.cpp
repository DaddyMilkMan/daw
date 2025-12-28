/*
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
