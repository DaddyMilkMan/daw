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

/*
    ==============================================================================
    Original file header:
*/

 * @file ArrangementComponent.h
 * @brief Arrangement view with automation lanes (Phase 14)
 *
 * Displays:
 * - Tracks with headers
 * - Clips on timeline
 * - Automation lanes (volume/pan/mute) per track
 *
 * Features:

 * - Per-track automation lane selection (V/P/M buttons)
 * - Visual automation curve display
 * - Add/move/delete automation points with mouse
 * - Full undo/redo support via ProjectState
 * - Live updates from CommandAPI/Wingman
 * - Skia GPU-accelerated rendering
 */

#pragma once

#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>

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
#include "../design-system/ZenithDesignSystem.h"

#include <memory>
#include <vector>

namespace zenith {

class ProjectState;
class Engine;

//==============================================================================
/**
 * @struct TrackUIState
 * @brief Per-track UI state (not stored in ProjectState)
 *
 * This is ephemeral UI state that doesn't need to be saved.
 */
struct TrackUIState {
  juce::String trackId;
  juce::String visibleAutomationParam; // "volume", "pan", "mute", or empty
  int trackIndex = -1;
};

//==============================================================================
/**
 * @struct AutomationPointSelection
 * @brief Currently selected automation point
 */
struct AutomationPointSelection {
  juce::String trackId;
  juce::String paramId;
  juce::String pointId;
  double originalTimeBeats = 0.0;
  double originalValue = 0.0;
  bool isDragging = false;

  void clear() {
    trackId = "";
    paramId = "";
    pointId = "";
    isDragging = false;
  }

  bool isValid() const { return !pointId.isEmpty(); }
};

//==============================================================================
/**
 * @struct AutomationPointView
 * @brief Lightweight view of an automation point for rendering
 */
struct AutomationPointView {
  juce::String pointId;
  double timeBeats;
  double value;
};

//==============================================================================
/**
 * @class ArrangementComponent
 * @brief Main timeline/arrangement view with automation support
 *
 * Layout:
 * - Left: Track headers (names, automation lane buttons)
 * - Right: Timeline view (clips + automation lanes)
 *
 * Thread safety:
 * - All methods run on MESSAGE THREAD only
 * - Reads from ProjectState ValueTree (message thread safe)
 * - Modifies ProjectState via undoable API calls
 */
class ArrangementComponent : public zenith::SkiaComponent,
                             public juce::ValueTree::Listener {
public:
  //==========================================================================
  ArrangementComponent(ProjectState &projectState, Engine &engine);
  ~ArrangementComponent() override;

  //==========================================================================
  // SkiaComponent interface
  //==========================================================================

  void drawSkia(SkCanvas* canvas) override;
  void resized() override;

  //==========================================================================
  // Mouse interaction
  //==========================================================================

  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;
  void mouseUp(const juce::MouseEvent &event) override;

  //==========================================================================
  // Keyboard interaction
  //==========================================================================

  bool keyPressed(const juce::KeyPress &key) override;

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
                                  int newIndex) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override;

private:
  //==========================================================================
  // Timer callback (for UI updates)
  //==========================================================================

  void timerCallback() override;

  //==========================================================================
  // Skia drawing helpers
  //==========================================================================

  void drawTrackHeaders(SkCanvas* canvas, SkRect area);
  void drawTimeline(SkCanvas* canvas, SkRect area);
  void drawTrack(SkCanvas* canvas, int trackIndex, SkRect area);
  void drawAutomationLane(SkCanvas* canvas, const juce::String &trackId,
                          const juce::String &paramId, SkRect area);
  void drawGridLines(SkCanvas* canvas, SkRect area);
  void drawPlayhead(SkCanvas* canvas, SkRect area);

  //==========================================================================
  // Coordinate mapping
  //==========================================================================

  double pixelsToBeats(int pixels) const;
  int beatsToPixels(double beats) const;
  double pixelsToAutomationValue(int y, juce::Rectangle<int> laneArea,
                                 const juce::String &paramId) const;
  int automationValueToPixels(double value, juce::Rectangle<int> laneArea,
                              const juce::String &paramId) const;

  //==========================================================================
  // Hit testing
  //==========================================================================

  bool isPointInAutomationLane(juce::Point<int> pos, int trackIndex) const;
  juce::String findAutomationPointAtPosition(juce::Point<int> pos,
                                             int trackIndex,
                                             juce::String &outParamId,
                                             juce::String &outPointId);
  juce::Rectangle<int> getTrackArea(int trackIndex) const;
  juce::Rectangle<int> getAutomationLaneArea(int trackIndex) const;
  juce::Rectangle<int> getTrackHeaderArea(int trackIndex) const;

  //==========================================================================
  // Automation editing
  //==========================================================================

  void addAutomationPoint(const juce::String &trackId,
                          const juce::String &paramId, double timeBeats,
                          double value);
  void startDraggingPoint(const juce::String &trackId,
                          const juce::String &paramId,
                          const juce::String &pointId, double timeBeats,
                          double value);
  void updateDraggingPoint(double newTimeBeats, double newValue);
  void finishDraggingPoint();
  void deleteSelectedPoint();

  //==========================================================================
  // Track UI state management
  //==========================================================================

  void rebuildTrackUIState();
  void setVisibleAutomationParam(int trackIndex, const juce::String &paramId);
  juce::String getVisibleAutomationParam(int trackIndex) const;

  //==========================================================================
  // Helpers
  //==========================================================================

  juce::Array<AutomationPointView>
  getAutomationPoints(const juce::String &trackId,
                      const juce::String &paramId) const;
  SkColor getAutomationColor(const juce::String &paramId) const;
  double snapToGrid(double beats) const;

  //==========================================================================
  // Member variables
  //==========================================================================

  ProjectState &projectState;
  Engine &engine;

  // UI state
  std::vector<TrackUIState> trackUIStates;
  AutomationPointSelection selectedPoint;

  // Layout constants
  static constexpr int TRACK_HEIGHT = 100;
  static constexpr int TRACK_HEADER_WIDTH = 200;
  static constexpr int AUTOMATION_LANE_HEIGHT_RATIO = 30; // 30% of track height
  static constexpr int GRID_SNAP_BEATS = 1; // Snap to 1/16 beats (0.25)

  // View state
  double pixelsPerBeat = 50.0;
  double viewOffsetBeats = 0.0;
  int lastTrackCount = 0;
  double playheadBeats_ = 0.0;

  // Font for text rendering
  SkFont textFont_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementComponent)
};

} // namespace zenith
