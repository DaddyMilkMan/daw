/*
  ==============================================================================

    UISoundManager.h
    Created: 2025-12-19
    Author:  Zenith DAW Team

    Minimalist UI sound feedback system.
    - Plays subtle sounds for knobs and specific widgets.
    - SUPPRESSES sounds during playback or recording.
    - Respects user preference for UI sounds.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

// Forward declaration
namespace zenith {
class Engine;
}

namespace zenith::ui {

/**
 * @brief Manages minimalist UI sound effects.
 *
 * This is a lightweight system designed to provide tactile feedback
 * without being intrusive. Sounds are suppressed entirely during
 * playback or recording.
 *
 * Sounds are intentionally minimal:
 * - Knob clicks (tick on value change)
 */
class UISoundManager {
public:
  static UISoundManager &getInstance() {
    static UISoundManager instance;
    return instance;
  }

  /**
   * @brief Set the engine reference for checking playback state.
   */
  void setEngine(Engine *engine) { engine_ = engine; }

  /**
   * @brief Enable or disable UI sounds globally.
   */
  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }

  /**
   * @brief Play a knob tick sound.
   * Suppressed during playback/recording.
   */
  void playKnobTick();

private:
  UISoundManager() = default;
  ~UISoundManager() = default;

  bool shouldPlaySound() const;

  Engine *engine_ = nullptr;
  bool enabled_ = true;

  // Future: Could add AudioSampleBuffer for actual audio samples
};

} // namespace zenith::ui
