/**
 * @file ClipComponent.h
 * @brief Flat clip component with theme colors and clean typography
 *
 * Features clean DAW aesthetics:
 * - Track-colored fills (muted)
 * - Typography.body for clip names
 * - Simple 1-2px selection border
 * - Rounded corners (4px)
 * - 60 Hz smooth animations
 */

// POLISH: spacing normalized to 8px grid (rounded corners 4px)
// POLISH: typography now uses ZenithDesignSystem
// POLISH: flattened visuals (track colors, no gradients)

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

#include "SkiaComponent.h"

/**
 * @class ClipComponent
 * @brief Flat clip display on the arranger timeline
 *
 * Clean design with:
 * - Track-colored backgrounds (muted, using theme clip colors)
 * - Typography.body for clip names
 * - Simple 1-2px selection border
 * - Rounded corners (4px, 8px grid)
 */
class ClipComponent : public zenith::SkiaComponent {
public:
  /**
   * @brief Constructor
   * @param clipNode ValueTree node for this clip
   */
  ClipComponent(juce::ValueTree clipNode);
  ~ClipComponent() override;

  //==========================================================================
  // Clip data
  //==========================================================================

  /**
   * @brief Get the clip's ValueTree node
   */
  juce::ValueTree getClipNode() const { return clip; }

  /**
   * @brief Get clip ID
   */
  juce::String getClipId() const;

  /**
   * @brief Get start position in beats
   */
  double getStartBeats() const;

  /**
   * @brief Get length in beats
   */
  double getLengthBeats() const;

  /**
   * @brief Update bounds from clip data and pixels-per-beat ratio
   */

  //==========================================================================
  // Smart Rendering Helpers
  //==========================================================================

  struct NeighborhoodState {
    bool leftConnected = false;
    bool rightConnected = false;
  };

  NeighborhoodState getNeighborhoodState() const;
  double getLoopLength() const;

  void updateBounds(double pixelsPerBeat, int yPosition, int height)
      [[maybe_unused]];

  //==========================================================================
  // Component interface
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;

  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;
  void mouseEnter(const juce::MouseEvent &event) override;
  void mouseExit(const juce::MouseEvent &event) override;

  //==========================================================================
  // State Management
  //==========================================================================
  
  void setPlaying(bool playing) { isPlaying = playing; repaint(); }
  void setRecording(bool recording) { isRecording = recording; repaint(); }
  void setMuted(bool muted) { isMuted = muted; repaint(); }
  
  bool getPlaying() const { return isPlaying; }
  bool getRecording() const { return isRecording; }
  bool getMuted() const { return isMuted; }

  //==========================================================================
  // Timer interface (for smooth animations)
  //==========================================================================

  void timerCallback() override;

private:
  juce::ValueTree clip;
  juce::Point<int> dragStartPos;
  double dragStartBeats = 0.0;

  // Animation state
  bool isHovered = false;
  bool isSelected = false;
  bool isPlaying = false;
  bool isRecording = false;
  bool isMuted = false;
  
  float hoverAnimation = 0.0f; // 0.0 to 1.0 for smooth hover animation
  float selectionPulse = 0.0f; // 0.0 to 1.0 for selection glow pulse
  float playheadAnimation = 0.0f; // 0.0 to 1.0 for active playhead
  
  // Drawing Helpers
  void drawDropShadow(SkCanvas* canvas, const SkRRect& rect);
  void drawClipBackground(SkCanvas* canvas, const SkRRect& rect, SkColor trackColor);
  void drawAudioContent(SkCanvas* canvas, const SkRect& rect, SkColor trackColor, int numLoops);
  void drawMidiContent(SkCanvas* canvas, const SkRect& rect, SkColor trackColor, int numLoops);
  void drawOverlayStates(SkCanvas* canvas, const SkRRect& rect);
  void drawClipName(SkCanvas* canvas);
  void drawFadeHandles(SkCanvas* canvas, float width);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
