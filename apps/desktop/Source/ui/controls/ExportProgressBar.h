/*
  ==============================================================================

    ExportProgressBar.h
    Created: 2025-12-25
    Author:  Zenith DAW

    Skia-based progress bar for export operations.
    Features animated gradient fill with glow effect.

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @class ExportProgressBar
 * @brief Animated progress bar for export operations
 *
 * Displays export progress with:
 * - Animated gradient fill
 * - Subtle glow effect
 * - Percentage text overlay
 * - Status message display
 */
class ExportProgressBar : public SkiaComponent {
public:
  ExportProgressBar();
  ~ExportProgressBar() override;

  /// Set progress (0.0 to 1.0)
  void setProgress(float progress);
  float getProgress() const { return progress_; }

  /// Set status message
  void setStatusMessage(const juce::String& message);
  juce::String getStatusMessage() const { return statusMessage_; }

  /// Show/hide percentage
  void setShowPercentage(bool show) { showPercentage_ = show; repaint(); }

  /// Animation control
  void startAnimation();
  void stopAnimation();

  // SkiaComponent override
  void drawSkia(SkCanvas* canvas) override;

private:
  void timerCallback() override;

  float progress_ = 0.0f;
  float animatedProgress_ = 0.0f; // Smooth animated value
  float glowPhase_ = 0.0f;        // For animated glow
  juce::String statusMessage_ = "";
  bool showPercentage_ = true;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportProgressBar)
};

} // namespace zenith
