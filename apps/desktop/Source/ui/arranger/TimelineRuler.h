/**
 * @file TimelineRuler.h
 * @brief Timeline ruler showing beat markers
 */

// POLISH: spacing normalized to 8px grid (labels at Typography.small)
// POLISH: typography now uses SkiaTheme::Typography (small)
// POLISH: flattened background (bg2, no gradients)

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


#ifdef ZENITH_USE_SKIA
#include "SkiaComponent.h"
#include "SkiaTheme.h"
#include "ZenithSkia.h"
#endif

/**
 * @class TimelineRuler
 * @brief Displays a horizontal timeline with beat markers
 *
 * Shows beat numbers and grid lines based on zoom level.
 * Works in beat units, independent of tempo/sample rate.
 * Features flat design with theme colors, hover feedback, and smooth
 * animations.
 */
#ifdef ZENITH_USE_SKIA
class TimelineRuler : public zenith::SkiaComponent
#else
class TimelineRuler : public juce::Component,
                      public juce::Timer
#endif
{
public:
  TimelineRuler();
  ~TimelineRuler() override = default;

  //==========================================================================
  // View control
  //==========================================================================

  /**
   * @brief Set the visible range in beats
   * @param start Start beat
   * @param length Number of beats visible
   */
  void setVisibleRange(double start, double length);

  /**
   * @brief Get pixels per beat ratio
   */
  double getPixelsPerBeat() const { return pixelsPerBeat; }

  /**
   * @brief Convert beats to pixels
   */
  int beatsToPixels(double beats) const;

  /**
   * @brief Convert pixels to beats
   */
  double pixelsToBeats(int pixels) const;

  /**
   * @brief Set callback for seek requests
   */
  std::function<void(double)> onSeek;

  //==========================================================================
  // Component interface
  //==========================================================================

#ifdef ZENITH_USE_SKIA
  void drawSkia(SkCanvas* canvas) override;
#else
  void paint(juce::Graphics &g) override;
#endif
  void resized() override;
  void mouseMove(const juce::MouseEvent &event) override;
  void mouseEnter(const juce::MouseEvent &event) override;
  void mouseExit(const juce::MouseEvent &event) override;
  void mouseDown(const juce::MouseEvent &event) override;
  void timerCallback() override;

private:
  // View state
  double viewStartBeat = 0.0;
  double viewLengthBeats = 32.0;
  double pixelsPerBeat = 20.0;

  // Hover state
  bool isHovered = false;
  int hoveredMeasure = -1;
  juce::Point<int> mousePosition;
  float hoverAnimation = 0.0f;

  // Helper methods
  void drawBackground(juce::Graphics &g, const juce::Rectangle<int> &bounds);
  void drawBeatMarkers(juce::Graphics &g, const juce::Rectangle<int> &bounds);
  void drawHoverFeedback(juce::Graphics &g, const juce::Rectangle<int> &bounds);
  void drawTooltip(juce::Graphics &g);
  juce::String formatTimePosition(double beat) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};
