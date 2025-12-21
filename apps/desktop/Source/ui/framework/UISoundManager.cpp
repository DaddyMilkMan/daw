/*
  ==============================================================================

    UISoundManager.cpp
    Created: 2025-12-19
    Author:  Zenith DAW Team

    Implementation of minimalist UI sound feedback system.

  ==============================================================================
*/

#include "UISoundManager.h"
#include "../../engine/Engine.h"

namespace zenith::ui {

bool UISoundManager::shouldPlaySound() const {
  if (!enabled_)
    return false;
    
  if (engine_ == nullptr)
    return true; // No engine reference, allow sounds
    
  // CRITICAL: Suppress sounds during playback or recording
  if (engine_->isPlaying() || engine_->isRecording())
    return false;
    
  return true;
}

void UISoundManager::playKnobTick() {
  if (!shouldPlaySound())
    return;
    
  // NOTE: Actual audio playback would require a synthesized click or sample.
  // For now, this is a stub that can be hooked up to JUCE's
  // SystemAudioPlayer or a custom sample buffer.
  //
  // A real implementation might use:
  // - A tiny sine wave burst (1ms @ 2kHz)
  // - A pre-loaded WAV sample
  // - A direct call to the OS haptic feedback API on supported devices
  //
  // The architectural goal is achieved: we have suppression logic 
  // and a clear API for widgets to call.
  
  DBG("UISoundManager: playKnobTick (suppression check passed)");
}

} // namespace zenith::ui
