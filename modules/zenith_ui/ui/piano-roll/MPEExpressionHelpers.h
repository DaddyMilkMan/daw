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

#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkFont.h>
#include "../design-system/ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

namespace zenith {

// Forward declaration
class PianoRollComponent;

/**
 * MPE Expression Type enumeration (from PianoRollComponent)
 */
enum class ExpressionType {
  PitchBend,  // Per-note pitch bend
  Pressure,   // Aftertouch/Pressure
  Slide,      // Slide/Timbre (CC74)
  Expression  // Expression pedal (CC11)
};

/**
 * Get the display label for an expression type.
 *
 * @param type The expression type
 * @return Human-readable label string
 */
inline juce::String getExpressionLabel(ExpressionType type) {
  switch (type) {
    case ExpressionType::PitchBend:
      return "PITCH";
    case ExpressionType::Pressure:
      return "PRESSURE";
    case ExpressionType::Slide:
      return "SLIDE (MPE)";
    case ExpressionType::Expression:
      return "EXPRESSION";
  }
  return "";
}

/**
 * Get the color for an expression type.
 * Uses industry-standard color scheme that's colorblind-safe.
 *
 * @param type The expression type
 * @return SkColor for the expression type
 */
inline SkColor getExpressionColor(ExpressionType type) {
  using namespace design::colors;
  switch (type) {
    case ExpressionType::PitchBend:
      return MPE_PITCHBEND;  // Green
    case ExpressionType::Pressure:
      return MPE_PRESSURE;   // Red
    case ExpressionType::Slide:
      return MPE_TIMBRE;     // Blue
    case ExpressionType::Expression:
      return MPE_EXPRESSION; // Orange
  }
  return design::colors::CYAN; // Fallback to brand color
}

/**
 * Get an accessibility symbol for an expression type.
 * Provides non-visual distinction for screen readers and colorblind users.
 *
 * @param type The expression type
 * @return UTF-8 symbol string
 */
inline const char* getExpressionSymbol(ExpressionType type) {
  switch (type) {
    case ExpressionType::PitchBend:
      return "♫";      // Musical note
    case ExpressionType::Pressure:
      return "⬇";      // Down arrow (pressure)
    case ExpressionType::Slide:
      return "↔";      // Left-right arrow (slide)
    case ExpressionType::Expression:
      return "▶";      // Play/pedal symbol
  }
  return "";
}

/**
 * Configure a SkPaint object for expression curve rendering.
 * Sets color, width, anti-aliasing, and stroke style.
 *
 * @param paint The paint object to configure
 * @param type The expression type (determines color)
 */
inline void configureExpressionCurvePaint(SkPaint &paint, ExpressionType type) {
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(getExpressionColor(type));
  paint.setStrokeWidth(design::colors::mpe::CURVE_WIDTH);
  paint.setAntiAlias(true);
}

/**
 * Configure a SkPaint object for expression point rendering.
 * Sets color, anti-aliasing, and fills style.
 *
 * @param paint The paint object to configure
 * @param type The expression type (determines color)
 */
inline void configureExpressionPointPaint(SkPaint &paint, ExpressionType type) {
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(getExpressionColor(type));
  paint.setAntiAlias(true);
}

/**
 * Configure a SkPaint object for expression label rendering.
 * Sets color, alpha, and font for text labels.
 *
 * @param paint The paint object to configure
 * @param type The expression type (determines color)
 */
inline void configureExpressionLabelPaint(SkPaint &paint, ExpressionType type) {
  paint.setColor(design::withAlpha(getExpressionColor(type),
                                    design::colors::mpe::LABEL_ALPHA));
  paint.setAntiAlias(true);
}

/**
 * Get the lane height for expression lanes.
 * Clamped to min/max bounds for usability.
 *
 * @param userHeight User-specified height (may be outside bounds)
 * @return Clamped height in pixels
 */
inline float getClampedLaneHeight(float userHeight) {
  return juce::jlimit(design::colors::mpe::LANE_HEIGHT_MIN,
                      design::colors::mpe::LANE_HEIGHT_MAX,
                      userHeight);
}

} // namespace zenith
