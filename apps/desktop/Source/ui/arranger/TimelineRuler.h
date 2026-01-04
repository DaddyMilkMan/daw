/**
 * @file TimelineRuler.h
 * @brief Timeline ruler showing beat markers
 */

// POLISH: spacing normalized to 8px grid (labels at Typography.small)
// POLISH: typography now uses ZenithDesignSystem
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

#include "../framework/SkiaComponent.h"
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>

namespace zenith {

/**
 * @class TimelineRuler
 * @brief Displays a horizontal timeline with beat markers
 *
 * Shows beat numbers and grid lines based on zoom level.
 * Works in beat units, independent of tempo/sample rate.
 * Features flat design with theme colors, hover feedback, and smooth
 * animations.
 */
class TimelineRuler : public SkiaComponent
{
public:
  TimelineRuler();
  ~TimelineRuler() override = default;

  //==========================================================================
  // View control
  //==========================================================================
   * @param start Start beat

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

  /**
   * @brief Set loop region
   */
  void setLoopRange(double startBeat, double endBeat, bool enabled);

  /**
   * @brief Callback when loop region is changed by user
   */
  std::function<void(double start, double end)> onLoopChanged;

  //==========================================================================
  // Component interface
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseMove(const juce::MouseEvent &event) override;
  void mouseEnter(const juce::MouseEvent &event) override;
  void mouseUp(const juce::MouseEvent &event) override;
  void timerCallback() override;

private:
  // View state
  double viewStartBeat = 0.0;
  double viewLengthBeats = 32.0;
  double pixelsPerBeat = 20.0;

  // Loop state
  double loopStartBeat = 0.0;
  double loopEndBeat = 4.0;
  bool loopEnabled = false;

  // Hover state
  bool isHovered = false;
  int hoveredMeasure = -1;
  juce::Point<int> mousePosition;
  float hoverAnimation = 0.0f;

  enum class DragMode { None, Seek, MoveLoopStart, MoveLoopEnd, MoveLoopRegion };
  DragMode currentDragMode = DragMode::None;
  double dragStartBeat = 0.0;
  double initialLoopStart = 0.0;
  double initialLoopEnd = 0.0;

  // Helper methods
  void drawBackground(juce::Graphics &g, const juce::Rectangle<int> &bounds);
  void drawBeatMarkers(juce::Graphics &g, const juce::Rectangle<int> &bounds);
  void drawHoverFeedback(juce::Graphics &g, const juce::Rectangle<int> &bounds);
  void drawTooltip(juce::Graphics &g);
  juce::String formatTimePosition(double beat) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};

} // namespace zenith
