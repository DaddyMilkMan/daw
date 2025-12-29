/*
  ==============================================================================

    FreezeProgressOverlay.h
    Created: 2025-12-19
    Author:  Zenith DAW

    Visual overlay for track freeze progress.
    Renders a neon-style circular progress indicator.

  ==============================================================================
*/

#pragma once

#include "../../ui/framework/SkiaComponent.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class FreezeProgressOverlay : public SkiaComponent {
public:
  FreezeProgressOverlay();
  ~FreezeProgressOverlay() override;

  void setProgress(float progress); // 0.0 to 1.0
  void setStatus(const juce::String &text);

  // Callback when cancel button is clicked
  std::function<void()> onCancel;

  void resized() override;
  void paint(juce::Graphics &g) override;
  void mouseDown(const juce::MouseEvent &e) override;

  // Skia rendering
  void drawSkia(SkCanvas *canvas) override;

private:
  float progress_ = 0.0f;
  juce::String statusText_ = "Freezing Track...";

  // Pulse animation state
  float pulsePhase_ = 0.0f;

  // Bounds for cancel button (virtual)
  juce::Rectangle<float> cancelButtonRect_;
  bool isCancelHovered_ = false;

  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreezeProgressOverlay)
};

} // namespace zenith
