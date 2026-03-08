/*
  ==============================================================================

    MeterRenderer.h
    Created: 2025-12-31
    Author:  Zenith DAW

    Shared utility for rendering audio/CPU meters with consistent styling.
    Supports vertical and horizontal orientations, gradients, and peak hold.

  ==============================================================================
*/

#pragma once

#include "ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkRect.h>

namespace zenith::design {

class MeterRenderer {
public:
  struct Options {
    bool isHorizontal;
    bool showPeak;
    bool showLabel;
    float peakHoldDecay;
    juce::String labelText;

    Options() : isHorizontal(false), showPeak(true), showLabel(false), peakHoldDecay(0.95f) {}
  };

  /**
   * @brief Draws a standardized meter (audio level or CPU usage)
   * Overload with explicit options.
   */
  static void drawMeter(SkCanvas* canvas, const SkRect& bounds, 
                        float level, float peak, 
                        const Options& options);

  /**
   * @brief Draws a standardized meter with default options
   */
  static void drawMeter(SkCanvas* canvas, const SkRect& bounds, 
                        float level, float peak);

  /**
   * @brief Helper for vertical audio level meters (like in mixer)
   */
  static void drawVerticalLevelMeter(SkCanvas* canvas, const SkRect& bounds, 
                                     float level, float peak);

  /**
   * @brief Helper for horizontal CPU/Disk meters (like in transport)
   */
  static void drawHorizontalMeter(SkCanvas* canvas, const SkRect& bounds, 
                                  float value, const juce::String& label);

private:
  static void drawBar(SkCanvas* canvas, const SkRect& bounds, 
                      float level, bool isHorizontal);
};

} // namespace zenith::design
