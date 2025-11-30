/**
 * @file PianoRollEditor.h
 * @brief MIDI note editor with piano roll grid
 */

// POLISH: spacing normalized to 8px grid (note labels at Typography.tiny)
// POLISH: typography now uses SkiaTheme::Typography (tiny)
// POLISH: flattened visuals (bg1/bg2 grid, borderSubtle lines)

#pragma once

#include "../ProjectState.h"
#include "TimelineRuler.h"
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
// #include "../../src/ui/skia/SkiaCanvasComponent.h"  // TEMP DISABLED: File doesn't exist
#include "../../src/ui/skia/SkiaTheme.h"
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#endif

/**
 * @class PianoRollEditor
 * @brief Piano roll editor for MIDI notes
 *
 * Displays:
 * - Piano keyboard on the left
 * - Grid of beats vs pitch (bg1/bg2, borderSubtle)
 * - MIDI notes as rounded rectangles
 *
 * Allows:
 * - Click to add note
 * - Drag to move note
 * - Double-click to delete note
 *
 * Operates on a single CLIP ValueTree node.
 */
#ifdef ZENITH_USE_SKIA
class PianoRollEditor : public zenith::SkiaCanvasComponent,
#else
class PianoRollEditor : public juce::Component,
#endif
                        public juce::TooltipClient,
                        private juce::ValueTree::Listener {
public:
  /**
   * @brief Constructor
   * @param projectState Reference to project state
   * @param trackId ID of track containing the clip
   * @param clipId ID of clip to edit
   */
  PianoRollEditor(ProjectState &projectState, const juce::String &trackId,
                  const juce::String &clipId);
  ~PianoRollEditor() override;

  //==========================================================================
  // Component interface
  //==========================================================================

#ifdef ZENITH_USE_SKIA
  void drawSkia(SkCanvas* canvas) override;
#else
  void paint(juce::Graphics &g) override;
#endif
  void resized() override;
  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;
  void mouseDoubleClick(const juce::MouseEvent &event) override;
  juce::String getTooltip() override;

private:
  //==========================================================================
  // ValueTree::Listener interface
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override {}
  void valueTreeParentChanged(juce::ValueTree &tree) override {}

  //==========================================================================
  // Helper methods
  //==========================================================================

  /**
   * @brief Get note number (0-127) at Y position
   */
  int getNoteAtY(int y) const;

  /**
   * @brief Get beat position at X position
   */
  double getBeatAtX(int x) const;

  /**
   * @brief Get Y position for a note number
   */
  int getYForNote(int noteNumber) const;

  /**
   * @brief Get X position for a beat
   */
  int getXForBeat(double beat) const;

  /**
   * @brief Find note at position
   */
  juce::ValueTree findNoteAtPosition(int x, int y);

  //==========================================================================
  // Member variables
  //==========================================================================

  ProjectState &projectState;
  juce::String trackId;
  juce::String clipId;
  juce::ValueTree clipNode;

  TimelineRuler ruler;

  double viewStartBeat = 0.0;
  double viewLengthBeats = 8.0;
  double pixelsPerBeat = 40.0;

  int lowestNote = 36;  // C2
  int highestNote = 96; // C7
  int noteHeight = 16;  // 8px grid: 12->16

  static constexpr int PIANO_WIDTH = 64;  // 8px grid: 60->64
  static constexpr int RULER_HEIGHT = 32; // 8px grid: 30->32

  // Dragging state
  juce::ValueTree draggedNote;
  juce::Point<int> dragStartPos;
  double dragStartBeats = 0.0;
  int dragStartNoteNumber = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};
