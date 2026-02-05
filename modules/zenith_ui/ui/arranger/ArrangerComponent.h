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

 * @file ArrangerComponent.h
 * @brief Timeline/Arranger view component for Zenith DAW
 *
 * The ArrangerComponent is the main timeline view that displays tracks and
 * clips. It delegates to specialized helper classes for different concerns:
 * - ArrangerGridUtils: Coordinate conversion and waveform caching
 * - ArrangerClipManager: Clip lifecycle and selection
 * - ArrangerInputHandler: Mouse and keyboard input
 * - ArrangerRenderer: Skia drawing (when ZENITH_USE_SKIA is defined)

 */
#pragma once

#include "Engine.h"
#include "MiniMapComponent.h"
#include "ProjectState.h"
#include "SkiaComponent.h"
#include "TimelineRuler.h"
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include <core/SkCanvas.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../controls/FreezeProgressOverlay.h"
#include "MacroToolbar.h"
#include "ArrangerTypes.h"

// Forward declaration for browser drag
namespace zenith {
class BrowserDragData;
}

namespace zenith {

// Forward declarations for helper classes
class ArrangerGridUtils;
class ArrangerClipManager;
class ArrangerInputHandler;
class ArrangerTrackComponent;
class GridResolutionDropdown;
struct ClipView;

#ifdef ZENITH_USE_SKIA
class ArrangerRenderer;
#endif



//==============================================================================

//==============================================================================
/**
 * @class ArrangerComponent
 * @brief Main timeline/arranger view for the DAW
 *
 * Displays tracks and clips in a horizontal timeline. Supports:
 * - Clip selection, movement, and resizing
 * - Zoom and scroll navigation
 * - Drag-and-drop from browser
 * - Keyboard shortcuts for editing
 * - Premium glassmorphic Skia rendering
 */
class ArrangerComponent : public SkiaComponent,
                          public juce::ValueTree::Listener,
                          public juce::DragAndDropTarget {
public:
  /**
   * @brief Construct arranger component
   * @param engine Reference to the audio engine
   * @param ps Reference to the project state
   */
  ArrangerComponent(Engine &engine, ProjectState &ps);

  ~ArrangerComponent() override;

  //==========================================================================
  // Component Interface
  //==========================================================================

  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

  //==========================================================================
  // Callbacks
  //==========================================================================

  /** @brief Callback when a clip is double-clicked (for opening editor) */
  std::function<void(const juce::String &trackId, const juce::String &clipId)>
      onClipDoubleClicked;

  //==========================================================================
  // ValueTree::Listener Interface
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;

  //==========================================================================
  // Skia Rendering
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;

  //==========================================================================
  // Tooltip
  //==========================================================================

  juce::String getTooltip();

  //==========================================================================
  // DragAndDropTarget Interface
  //==========================================================================

  bool isInterestedInDragSource(
      const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDropped(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragEnter(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragExit(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragMove(const juce::DragAndDropTarget::SourceDetails &details) override;

  //==========================================================================
  // Grid Resolution Control
  //==========================================================================

  void setGridResolution(GridResolution res);
  GridResolution getGridResolution() const { return gridResolution_; }

  void setTool(ArrangerTool t) { currentTool_ = t; }
  ArrangerTool getTool() const { return currentTool_; }

  /** @brief Get access to the clip manager for collaboration features */
  ArrangerClipManager* getClipManager() { return clipManager_.get(); }
  const ArrangerClipManager* getClipManager() const { return clipManager_.get(); }

  //==========================================================================
  // Timer Interface
  //==========================================================================

  void timerCallback() override;

private:
  // Allow helper classes to access private members
  friend class ArrangerGridUtils;
  friend class ArrangerClipManager;
  friend class ArrangerInputHandler;
#ifdef ZENITH_USE_SKIA
  friend class ArrangerRenderer;
#endif

  //==========================================================================
  // Core References
  //==========================================================================

  std::unique_ptr<GridResolutionDropdown> gridDropdown;
  Engine &engine_;
  ProjectState &projectState;

  //==========================================================================
  // Helper Module Objects
  //==========================================================================

  std::unique_ptr<ArrangerGridUtils> gridUtils_;
  std::unique_ptr<ArrangerClipManager> clipManager_;
  std::unique_ptr<ArrangerInputHandler> inputHandler_;
#ifdef ZENITH_USE_SKIA
  std::unique_ptr<ArrangerRenderer> renderer_;
#endif

  //==========================================================================
  // Child Components
  //==========================================================================

  MiniMapComponent miniMap;
  TimelineRuler timelineRuler;
  std::unique_ptr<MacroToolbar> macroToolbar;
  std::unique_ptr<FreezeProgressOverlay> freezeOverlay;
  std::unique_ptr<ArrangerTrackComponent> sectionTrack;
  std::vector<std::unique_ptr<ArrangerTrackComponent>> trackComponents;

  //==========================================================================
  // View State
  //==========================================================================

  double pixelsPerBeat = 50.0;
  double viewStartBeats = 0.0;
  int firstVisibleTrackIndex = 0;

  double gridSnapBeats = 1.0;
  GridResolution gridResolution_ = GridResolution::Beat_1;
  ArrangerTool currentTool_ = ArrangerTool::Select;

  //==========================================================================
  // Playhead State
  //==========================================================================

  double playheadBeats_ = 0.0;
  bool isPlaying_ = false;
  bool followPlayhead_ = true;

  //==========================================================================
  // Loop Region State
  //==========================================================================

  bool loopEnabled_ = false;
  double loopStartBeats_ = 0.0;
  double loopEndBeats_ = 8.0;

  //==========================================================================
  // Drop Zone State
  //==========================================================================

  bool isDropTargetActive_ = false;
  int dropTargetTrackIndex_ = -1;
  double dropTargetBeats_ = 0.0;

  //==========================================================================
  // Private Methods
  //==========================================================================

  bool keyPressed(const juce::KeyPress &key) override;
  void updatePlayheadFromEngine();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
