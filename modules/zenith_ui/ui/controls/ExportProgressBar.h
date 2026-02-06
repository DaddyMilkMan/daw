/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
