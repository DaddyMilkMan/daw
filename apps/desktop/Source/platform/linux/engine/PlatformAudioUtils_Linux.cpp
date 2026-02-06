/*
  ==============================================================================

    PlatformAudioUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "PlatformAudioUtils.h"
#include "zenith_core/engine/Settings.h"
#include <juce_gui_basics/juce_gui_basics.h> // For AlertWindow
#include <fstream>
#include <filesystem>

namespace zenith {

static bool isPipeWireRunning() {
#if JUCE_LINUX
    // Check for PipeWire socket in XDG_RUNTIME_DIR
    auto xdgRuntimeDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(".cache"); // Fallback
    const char* xdg_env = std::getenv("XDG_RUNTIME_DIR");
    if (xdg_env != nullptr) {
        juce::File runtimeDir(xdg_env);
        if (runtimeDir.getChildFile("pipewire-0").exists()) {
            return true;
        }
    }

    // Check processes as fallback
    juce::ChildProcess pw;
    if (pw.start("pgrep -x pipewire")) {
        return pw.readAllProcessOutput().trim().isNotEmpty();
    }
#endif
    return false;
}

void PlatformAudioUtils::initializeAudioDeviceSetup(
    juce::AudioDeviceManager &deviceManager) {
  auto& settings = Settings::getInstance();
  auto preferredBackend = settings.getLinuxAudioBackend();

  DBG("PlatformAudioUtils (Linux): Initializing audio device setup...");
  DBG("  Preferred backend (Settings): " + juce::String((int)preferredBackend));

  auto currentType = deviceManager.getCurrentAudioDeviceType();
  DBG("  Current backend: " + (currentType.isEmpty() ? "None" : currentType));

  auto trySwitchTo = [&](const juce::String& typeName, bool silent = false) -> bool {
      if (currentType == typeName) return true;

      const auto &availableTypes = deviceManager.getAvailableDeviceTypes();
      bool available = false;
      for (auto *type : availableTypes) {
          if (type->getTypeName() == typeName) {
              available = true;
              break;
          }
      }

      if (!available) {
          if (!silent) DBG("  " + typeName + " backend not available.");
          return false;
      }

      DBG("  Attempting to switch to " + typeName + "...");
      deviceManager.setCurrentAudioDeviceType(typeName, true);
      
      if (deviceManager.getCurrentAudioDeviceType() == typeName) {
          DBG("  Successfully switched to " + typeName + ".");
          return true;
      }
      
      if (!silent) DBG("  Failed to switch to " + typeName + ".");
      return false;
  };

  // 1. Manual Override from Settings
  if (preferredBackend != Settings::LinuxAudioBackend::Auto) {
      juce::String target = "";
      if (preferredBackend == Settings::LinuxAudioBackend::JACK) target = "JACK";
      else if (preferredBackend == Settings::LinuxAudioBackend::ALSA) target = "ALSA";
      else if (preferredBackend == Settings::LinuxAudioBackend::PipeWire) {
          // PipeWire is usually handled via JACK or ALSA, but we prioritize JACK if PipeWire is running
          target = isPipeWireRunning() ? "JACK" : "ALSA";
      }

      if (target.isNotEmpty() && trySwitchTo(target)) {
          return;
      }

      // If manual selection failed, show a warning and continue with Auto logic
      juce::AlertWindow::showMessageBoxAsync(
          juce::AlertWindow::WarningIcon, "Audio Setup Warning",
          "Failed to initialize your preferred audio backend (target: " + target + "). Falling back to auto-detection.", "OK");
  }

  // 2. Auto-Detection Logic
  // Priority: JACK (if PipeWire/JACK running) -> ALSA
  
  if (isPipeWireRunning()) {
      DBG("  PipeWire detected. Prioritizing JACK backend.");
      if (trySwitchTo("JACK", true)) return;
  }

  // Try JACK anyway as it's the pro-audio standard
  if (trySwitchTo("JACK", true)) return;

  // Final Fallback: ALSA
  if (deviceManager.getCurrentAudioDevice() == nullptr || currentType != "ALSA") {
      DBG("  Attempting ALSA fallback...");
      trySwitchTo("ALSA");
  }
}

} // namespace zenith
