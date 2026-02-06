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

#include <core/SkCanvas.h>
#include <core/SkRect.h>
#include <juce_core/juce_core.h>
#include "MPEExpressionHelpers.h"
#include <functional>
#include <memory>
#include <vector>

namespace zenith {

class PianoRollComponent;

/**
 * State for tracking mouse interactions with expression lane points
 */
struct ExpressionPointInteraction {
  juce::String noteId;              // Note this point belongs to
  ExpressionType type;              // Expression type (Pressure, Timbre, etc.)
  size_t pointIndex;                // Index in expression points array
  juce::Point<float> screenPosition; // Current screen position
  bool isDragging = false;          // Currently being dragged
  bool isHovered = false;           // Mouse is hovering over point
  bool isSelected = false;          // Point is selected (for multi-select operations)
  SkRect hitRegion;                 // Clickable region around point

  /** Note bounds for time offset validation (set during interaction) */
  double noteStartBeats = 0.0;
  double noteEndBeats = 0.0;
};

/**
 * Manages expression lane editing interactions
 *
 * Features:
 * - Click to add points
 * - Drag to edit values
 * - Hover to highlight
 * - Delete selected points
 * - Multi-select with Shift+click
 * - Copy/paste support
 */
class ExpressionLaneEditor {
public:
  using PointModifiedCallback = std::function<void(
      const juce::String& noteId,
      ExpressionType type,
      const std::vector<PianoRollComponent::ExpressionPoint>& points
  )>;

  ExpressionLaneEditor();
  ~ExpressionLaneEditor();

  /**
   * Handle mouse down events in expression lane area
   *
   * @param position Mouse position in lane coordinates
   * @param laneRect The lane's screen rectangle
   * @param type The expression type of this lane
   * @param selectedNotes Currently selected notes (to limit editing)
   * @return true if event was handled
   */
  bool mouseDown(const juce::Point<float>& position,
                 const SkRect& laneRect,
                 ExpressionType type,
                 const std::vector<juce::String>& selectedNotes);

  /**
   * Handle mouse drag events
   *
   * @param position Current mouse position
   * @param laneRect The lane's screen rectangle
   * @return true if dragging a point
   */
  bool mouseDrag(const juce::Point<float>& position,
                 const SkRect& laneRect);

  /**
   * Handle mouse up events
   *
   * @param position Mouse position
   * @return true if interaction completed
   */
  bool mouseUp(const juce::Point<float>& position);

  /**
   * Handle mouse move events (for hover effects)
   *
   * @param position Mouse position
   * @param laneRect The lane's screen rectangle
   * @return true if hovering over a point
   */
  bool mouseMove(const juce::Point<float>& position,
                 const SkRect& laneRect);

  /**
   * Delete selected/hovered point
   *
   * @return true if a point was deleted
   */
  bool deleteSelectedPoint();

  /**
   * Get the currently hovered point (for cursor changes)
   *
   * @return Hovered point info, or nullptr if none
   */
  const ExpressionPointInteraction* getHoveredPoint() const;

  /**
   * Get all currently selected points
   */
  std::vector<ExpressionPointInteraction> getSelectedPoints() const;

  /**
   * Set callback for when points are modified
   */
  void setPointModifiedCallback(PointModifiedCallback callback);

  /**
   * Set the piano roll component reference
   * Must be called before any interaction operations
   */
  void setPianoRoll(PianoRollComponent* pianoRoll);

  /**
   * Update modifier key state (for multi-select operations)
   */
  void setModifierKeyState(bool shiftIsDown, bool ctrlIsDown = false, bool altIsDown = false);

  /**
   * Get the piano roll component
   */
  PianoRollComponent* getPianoRoll() const { return pianoRoll_; }

  /**
   * Draw interactive elements (hover highlights, drag previews)
   *
   * @param canvas Skia canvas to draw to
   * @param laneRect Lane rectangle
   */
  void drawOverlay(SkCanvas* canvas, const SkRect& laneRect);

  /**
   * Clear all selections
   */
  void clearSelection();

private:
  /**
   * Find a point at the given screen position
   *
   * @param position Mouse position to check
   * @param hitTolerance Maximum distance to consider a hit (pixels)
   * @return Pointer to interaction state, or nullptr if no hit
   */
  ExpressionPointInteraction* findPointAtPosition(
      const juce::Point<float>& position,
      float hitTolerance = 10.0f);

  /**
   * Add a new expression point at the given position
   *
   * @param position Screen position in lane
   * @param laneRect Lane bounds
   * @param type Expression type
   * @param noteId Note to add point to (must be selected)
   * @return true if point was added
   */
  bool addPointAtPosition(const juce::Point<float>& position,
                         const SkRect& laneRect,
                         ExpressionType type,
                         const juce::String& noteId);

  /**
   * Update the value of a point based on screen position
   *
   * @param interaction Point to update
   * @param position New screen position
   * @param laneRect Lane bounds for coordinate conversion
   */
  void updatePointValue(ExpressionPointInteraction& interaction,
                       const juce::Point<float>& position,
                       const SkRect& laneRect);

  /**
   * Convert screen Y position to expression value (0.0 - 1.0)
   */
  float screenYToValue(float y, const SkRect& laneRect) const;

  /**
   * Convert expression value to screen Y position
   */
  float valueToScreenY(float value, const SkRect& laneRect) const;

  /**
   * Convert screen X position to time offset
   */
  double screenXToTimeOffset(float x, const SkRect& laneRect,
                             const juce::String& noteId) const;

  /**
   * Get note bounds (start and end beats) for a note ID
   * Returns true if note was found
   */
  bool getNoteBounds(const juce::String& noteId, double& outStartBeats, double& outEndBeats) const;

  /**
   * Get pixels per beat from piano roll
   * Returns default value if piano roll is not available
   */
  double getPixelsPerBeat() const;

  /**
   * Remove an expression point from a note
   * Properly updates the piano roll and triggers callback
   */
  bool removeExpressionPoint(const juce::String& noteId, ExpressionType type, size_t pointIndex);

  /**
   * Add expression point to all selected notes
   */
  bool addPointToAllSelectedNotes(const juce::Point<float>& position,
                                   const SkRect& laneRect,
                                   ExpressionType type,
                                   const std::vector<juce::String>& selectedNotes);

  // State
  std::vector<std::unique_ptr<ExpressionPointInteraction>> activeInteractions_;
  ExpressionPointInteraction* hoveredPoint_ = nullptr;
  ExpressionPointInteraction* draggingPoint_ = nullptr;
  juce::Point<float> lastMousePosition_;
  bool shiftIsDown_ = false;  // For multi-select (add to selection)
  bool ctrlIsDown_ = false;  // For copy operations
  bool altIsDown_ = false;   // For fine-tuning

  // Configuration
  float hitTolerance_ = 10.0f;  // Pixels
  float minPointSpacing_ = 5.0f; // Minimum pixels between points

  // Callbacks
  PointModifiedCallback onPointModified_;

  // Access to piano roll for reading/writing expression data
  PianoRollComponent* pianoRoll_;  // Non-owning pointer
};

} // namespace zenith
