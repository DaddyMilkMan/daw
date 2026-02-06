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
