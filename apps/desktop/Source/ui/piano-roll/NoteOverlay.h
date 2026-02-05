/*
  ==============================================================================

    NoteOverlay.h
    Created: 2026-01-29
    Author:  Zenith DAW

    Real-time visual feedback overlay for MPE performance.
    Draws glowing notes, pressure circles, timbre color shifts, and pitchbend
    visualization on notes during MPE playback.

  ==============================================================================
*/

#pragma once

#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <juce_core/juce_core.h>
#include <map>
#include <memory>

namespace zenith {

/**
 * Per-note MPE state for visualization
 */
struct NoteMPEState {
  float pressure = 0.0f;      // 0.0-1.0 (aftertouch)
  float timbre = 0.5f;        // 0.0-1.0 (CC74)
  float pitchbend = 0.5f;      // 0.0-1.0 (normalized)
  int midiChannel = 0;        // For identifying which note
  juce::int64 lastUpdateTime = 0;  // For fade-out

  bool isActive() const {
    return midiChannel >= 0;
  }
};

/**
 * Manages real-time visual feedback for MPE performance
 *
 * Features:
 * - Glowing notes based on pressure
 * - Circle size indicates pressure amount
 * - Color shift based on timbre
 * - Position shift based on pitchbend
 * - 60fps smooth updates
 */
class NoteOverlay {
public:
  NoteOverlay();
  ~NoteOverlay();

  /**
   * Update MPE state for a note
   * Called when MPE messages arrive
   */
  void updateNoteState(int midiChannel, int noteNumber,
                      float pressure, float timbre, float pitchbend);

  /**
   * Clear MPE state for a note (note off)
   */
  void clearNoteState(int midiChannel, int noteNumber);

  /**
   * Draw overlay for a specific note
   * @param canvas Skia canvas to draw to
   * @param noteRect Rectangle of the note
   * @param state MPE state for this note
   */
  void drawNoteOverlay(SkCanvas* canvas, const SkRect& noteRect,
                      const NoteMPEState& state);

  /**
   * Draw overlay for all active notes
   * @param canvas Skia canvas
   * @param notes Map of note ID -> state
   * @param noteRects Map of note ID -> screen rectangle
   */
  void drawAllOverlays(SkCanvas* canvas,
                       const std::map<juce::String, NoteMPEState>& notes,
                       const std::map<juce::String, SkRect>& noteRects);

  /**
   * Clear all state (e.g., on stop)
   */
  void clearAll();

private:
  /**
   * Calculate glow intensity based on pressure
   */
  float getGlowIntensity(float pressure) const;

  /**
   * Get color for timbre value
   */
  SkColor getTimbreColor(float timbre) const;

  /**
   * Calculate pitchbend offset in pixels
   */
  float getPitchbendOffset(float pitchbend) const;

  // State
  std::map<std::pair<int, int>, NoteMPEState> noteStates_;  // (channel, note) -> state

  // Configuration
  float maxGlowSize_ = 10.0f;      // Maximum glow radius in pixels
  float maxCircleSize_ = 12.0f;    // Maximum pressure circle size
  float maxPitchbendOffset_ = 12.0f; // Maximum pitchbend shift in pixels

  // Colors
  SkColor pressureColor_ = SkColorSetRGB(255, 80, 80);   // Red
  SkColor timbreBaseColor_ = SkColorSetRGB(100, 100, 255); // Blue-ish
  SkColor timbreHotColor_ = SkColorSetRGB(255, 100, 100); // Red-ish

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteOverlay)
};

} // namespace zenith
