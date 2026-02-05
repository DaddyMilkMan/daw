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

  ==============================================================================

    ExpressionLaneEditor.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Implementation of expression lane mouse interactions for MPE editing.
    Supports clicking to add points, dragging to edit, deleting, and multi-select.


  ==============================================================================
*/

#include "ExpressionLaneEditor.h"
#include "PianoRollComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include <algorithm>
#include <cmath>

namespace {

/**
 * Calculate squared distance between two points (avoids sqrt for comparison)
 */
float distanceSquared(const juce::Point<float>& p1, const juce::Point<float>& p2) {
  const float dx = p1.x - p2.x;
  const float dy = p1.y - p2.y;
  return dx * dx + dy * dy;
}

/**
 * Clamp value between min and max
 */
float clamp(float value, float min, float max) {
  return juce::jlimit(min, max, value);
}

} // anonymous namespace

namespace zenith {

ExpressionLaneEditor::ExpressionLaneEditor()
    : hoveredPoint_(nullptr),
      draggingPoint_(nullptr),
      hitTolerance_(10.0f),
      minPointSpacing_(5.0f),
      pianoRoll_(nullptr) {
}

ExpressionLaneEditor::~ExpressionLaneEditor() = default;

void ExpressionLaneEditor::setPointModifiedCallback(PointModifiedCallback callback) {
  onPointModified_ = std::move(callback);
}

void ExpressionLaneEditor::setPianoRoll(PianoRollComponent* pianoRoll) {
  pianoRoll_ = pianoRoll;
}

void ExpressionLaneEditor::setModifierKeyState(bool shiftIsDown, bool ctrlIsDown, bool altIsDown) {
  shiftIsDown_ = shiftIsDown;
  ctrlIsDown_ = ctrlIsDown;
  altIsDown_ = altIsDown;
}

bool ExpressionLaneEditor::mouseDown(const juce::Point<float>& position,
                                      const SkRect& laneRect,
                                      ExpressionType type,
                                      const std::vector<juce::String>& selectedNotes) {
  // Check if clicking on existing point
  ExpressionPointInteraction* clickedPoint = findPointAtPosition(position, hitTolerance_);

  if (clickedPoint != nullptr) {
    // Clicked on existing point
    draggingPoint_ = clickedPoint;
    draggingPoint_->isDragging = true;

    // Multi-select support
    if (shiftIsDown_) {
      // Toggle selection
      clickedPoint->isSelected = !clickedPoint->isSelected;
    } else {
      // Clear other selections, select this one
      clearSelection();
      clickedPoint->isSelected = true;
      clickedPoint->isHovered = true;
    }

    lastMousePosition_ = position;
    return true;
  }

  // Not clicking on existing point - add new point if note is selected
  if (!selectedNotes.empty()) {
    // Add point to all selected notes (multi-note editing)
    return addPointToAllSelectedNotes(position, laneRect, type, selectedNotes);
  }

  return false;
}

bool ExpressionLaneEditor::mouseDrag(const juce::Point<float>& position,
                                      const SkRect& laneRect) {
  if (draggingPoint_ != nullptr) {
    // Update the point's value based on mouse position
    updatePointValue(*draggingPoint_, position, laneRect);

    // Notify callback with all updated points for the note
    if (onPointModified_ && pianoRoll_ != nullptr) {
      auto points = pianoRoll_->getNoteExpression(draggingPoint_->noteId, draggingPoint_->type);
      onPointModified_(draggingPoint_->noteId, draggingPoint_->type, points);
    }

    lastMousePosition_ = position;
    return true;
  }

  return false;
}

bool ExpressionLaneEditor::mouseUp(const juce::Point<float>& position) {
  if (draggingPoint_ != nullptr) {
    draggingPoint_->isDragging = false;
    draggingPoint_ = nullptr;
    return true;
  }

  return false;
}

bool ExpressionLaneEditor::mouseMove(const juce::Point<float>& position,
                                      const SkRect& laneRect) {
  // Check if hovering over any point
  ExpressionPointInteraction* previouslyHovered = hoveredPoint_;
  hoveredPoint_ = findPointAtPosition(position, hitTolerance_);

  if (hoveredPoint_ != nullptr) {
    hoveredPoint_->isHovered = true;
  } else if (previouslyHovered != nullptr) {
    previouslyHovered->isHovered = false;
  }

  return hoveredPoint_ != nullptr;
}

bool ExpressionLaneEditor::deleteSelectedPoint() {
  // Delete hovered point if any
  if (hoveredPoint_ != nullptr) {
    bool removed = removeExpressionPoint(hoveredPoint_->noteId, hoveredPoint_->type, hoveredPoint_->pointIndex);
    if (removed) {
      hoveredPoint_ = nullptr;
    }
    return removed;
  }

  // Delete dragged point if any
  if (draggingPoint_ != nullptr) {
    bool removed = removeExpressionPoint(draggingPoint_->noteId, draggingPoint_->type, draggingPoint_->pointIndex);
    if (removed) {
      draggingPoint_ = nullptr;
    }
    return removed;
  }

  // Delete all selected points
  bool anyDeleted = false;
  auto it = activeInteractions_.begin();
  while (it != activeInteractions_.end()) {
    if ((*it)->isSelected) {
      if (removeExpressionPoint((*it)->noteId, (*it)->type, (*it)->pointIndex)) {
        anyDeleted = true;
      }
      it = activeInteractions_.erase(it);
    } else {
      ++it;
    }
  }

  return anyDeleted;
}

const ExpressionPointInteraction* ExpressionLaneEditor::getHoveredPoint() const {
  return hoveredPoint_;
}

std::vector<ExpressionPointInteraction> ExpressionLaneEditor::getSelectedPoints() const {
  std::vector<ExpressionPointInteraction> selected;

  for (const auto& interaction : activeInteractions_) {
    if (interaction->isSelected) {
      selected.push_back(*interaction);
    }
  }

  return selected;
}

void ExpressionLaneEditor::drawOverlay(SkCanvas* canvas, const SkRect& laneRect) {
  if (canvas == nullptr) return;

  // Draw hover highlight
  if (hoveredPoint_ != nullptr) {
    SkPaint hoverPaint;
    hoverPaint.setColor(design::colors::CYAN);  // Cyan highlight
    hoverPaint.setStyle(SkPaint::kStroke_Style);
    hoverPaint.setStrokeWidth(2.0f);
    hoverPaint.setAntiAlias(true);

    canvas->drawCircle(hoveredPoint_->screenPosition.x,
                       hoveredPoint_->screenPosition.y,
                       design::colors::mpe::POINT_RADIUS + 2.0f,  // Slightly larger
                       hoverPaint);
  }

  // Draw drag preview
  if (draggingPoint_ != nullptr) {
    SkPaint dragPaint;
    dragPaint.setColor(design::withAlpha(design::colors::CYAN, 0.3f));
    dragPaint.setStyle(SkPaint::kFill_Style);
    dragPaint.setAntiAlias(true);

    canvas->drawCircle(draggingPoint_->screenPosition.x,
                       draggingPoint_->screenPosition.y,
                       design::colors::mpe::POINT_RADIUS + 4.0f,  // Even larger for drag
                       dragPaint);
  }
}

void ExpressionLaneEditor::clearSelection() {
  for (auto& interaction : activeInteractions_) {
    interaction->isSelected = false;
    interaction->isHovered = false;
  }
  hoveredPoint_ = nullptr;
}

ExpressionPointInteraction* ExpressionLaneEditor::findPointAtPosition(
    const juce::Point<float>& position,
    float hitTolerance) {

  const float hitToleranceSquared = hitTolerance * hitTolerance;

  for (auto& interaction : activeInteractions_) {
    const float dist = distanceSquared(position, interaction->screenPosition);

    if (dist < hitToleranceSquared) {
      return interaction.get();
    }
  }

  return nullptr;
}

bool ExpressionLaneEditor::addPointAtPosition(const juce::Point<float>& position,
                                               const SkRect& laneRect,
                                               ExpressionType type,
                                               const juce::String& noteId) {
  if (pianoRoll_ == nullptr) {
    return false;
  }

  // Get note bounds for validation
  double noteStartBeats = 0.0;
  double noteEndBeats = 1.0;
  getNoteBounds(noteId, noteStartBeats, noteEndBeats);

  // Convert screen position to expression data
  const float value = screenYToValue(position.y, laneRect);
  const double timeOffset = screenXToTimeOffset(position.x, laneRect, noteId);

  // Clamp value to valid range
  const float clampedValue = clamp(value, 0.0f, 1.0f);

  // Ensure time offset is within note bounds
  const double maxTimeOffset = noteEndBeats - noteStartBeats;
  const double clampedTimeOffset = juce::jlimit(0.0, maxTimeOffset, timeOffset);

  // Create new expression point
  PianoRollComponent::ExpressionPoint newPoint;
  newPoint.timeOffset = clampedTimeOffset;
  newPoint.value = clampedValue;
  newPoint.tension = 0.0f;  // Linear by default

  // Get existing points
  auto existingPoints = pianoRoll_->getNoteExpression(noteId, type);

  // Add new point
  existingPoints.push_back(newPoint);

  // Sort by time offset
  std::sort(existingPoints.begin(), existingPoints.end(),
            [](const PianoRollComponent::ExpressionPoint& a,
               const PianoRollComponent::ExpressionPoint& b) {
              return a.timeOffset < b.timeOffset;
            });

  // Update note expression
  pianoRoll_->setNoteExpression(noteId, type, existingPoints);

  // Create interaction state for the new point
  auto interaction = std::make_unique<ExpressionPointInteraction>();
  interaction->noteId = noteId;
  interaction->type = type;
  interaction->pointIndex = existingPoints.size() - 1;
  interaction->screenPosition = position;
  interaction->isDragging = false;
  interaction->isHovered = false;
  interaction->isSelected = true; // Auto-select newly created points
  interaction->noteStartBeats = noteStartBeats;
  interaction->noteEndBeats = noteEndBeats;

  // Calculate hit region (circle around point)
  interaction->hitRegion = SkRect::MakeXYWH(
      position.x - hitTolerance_,
      position.y - hitTolerance_,
      hitTolerance_ * 2.0f,
      hitTolerance_ * 2.0f
  );

  activeInteractions_.push_back(std::move(interaction));

  // Trigger callback
  if (onPointModified_) {
    onPointModified_(noteId, type, existingPoints);
  }

  return true;
}

void ExpressionLaneEditor::updatePointValue(ExpressionPointInteraction& interaction,
                                             const juce::Point<float>& position,
                                             const SkRect& laneRect) {
  // Update screen position
  interaction.screenPosition = position;

  // Update hit region
  interaction.hitRegion = SkRect::MakeXYWH(
      position.x - hitTolerance_,
      position.y - hitTolerance_,
      hitTolerance_ * 2.0f,
      hitTolerance_ * 2.0f
  );

  if (pianoRoll_ == nullptr) return;

  // Get note bounds for validation (refresh in case they changed)
  double noteStartBeats = 0.0;
  double noteEndBeats = 1.0;
  getNoteBounds(interaction.noteId, noteStartBeats, noteEndBeats);
  interaction.noteStartBeats = noteStartBeats;
  interaction.noteEndBeats = noteEndBeats;

  // Convert screen position to expression data
  const float value = screenYToValue(position.y, laneRect);
  const double timeOffset = screenXToTimeOffset(position.x, laneRect, interaction.noteId);

  // Clamp value to valid range
  const float clampedValue = clamp(value, 0.0f, 1.0f);

  // Clamp time offset to note bounds
  const double maxTimeOffset = noteEndBeats - noteStartBeats;
  const double clampedTimeOffset = juce::jlimit(0.0, maxTimeOffset, timeOffset);

  // Get existing points
  auto points = pianoRoll_->getNoteExpression(interaction.noteId, interaction.type);

  // Validate point index
  if (interaction.pointIndex >= points.size()) {
    return;  // Point was deleted or index is invalid
  }

  // Update the point
  points[interaction.pointIndex].value = clampedValue;
  points[interaction.pointIndex].timeOffset = clampedTimeOffset;

  // Re-sort if time offset changed
  std::sort(points.begin(), points.end(),
            [](const PianoRollComponent::ExpressionPoint& a,
               const PianoRollComponent::ExpressionPoint& b) {
              return a.timeOffset < b.timeOffset;
            });

  // Update note expression
  pianoRoll_->setNoteExpression(interaction.noteId, interaction.type, points);

  // Trigger callback
  if (onPointModified_) {
    onPointModified_(interaction.noteId, interaction.type, points);
  }
}

float ExpressionLaneEditor::screenYToValue(float y, const SkRect& laneRect) const {
  // Lane coordinate system: bottom = 0.0, top = 1.0
  const float laneHeight = laneRect.height();
  const float relativeY = laneRect.bottom() - y;  // Invert so bottom is 0
  return juce::jlimit(0.0f, 1.0f, relativeY / laneHeight);
}

float ExpressionLaneEditor::valueToScreenY(float value, const SkRect& laneRect) const {
  // Invert the mapping: 0.0 → bottom, 1.0 → top
  const float laneHeight = laneRect.height();
  return laneRect.bottom() - (value * laneHeight);
}

double ExpressionLaneEditor::screenXToTimeOffset(float x, const SkRect& laneRect,
                                                  const juce::String& noteId) const {
  if (pianoRoll_ == nullptr) return 0.0;

  // Get the actual pixels per beat from the piano roll
  const double ppb = getPixelsPerBeat();

  // Convert lane-relative X to absolute X
  const float relativeX = x - laneRect.left();

  // Convert to time offset (beats from note start)
  return static_cast<double>(relativeX / ppb);
}

bool ExpressionLaneEditor::getNoteBounds(const juce::String& noteId,
                                          double& outStartBeats,
                                          double& outEndBeats) const {
  if (pianoRoll_ == nullptr) {
    outStartBeats = 0.0;
    outEndBeats = 1.0;
    return false;
  }

  // Search through the piano roll's note collection for the matching note ID
  for (const auto& note : pianoRoll_->noteRects) {
    if (note.id == noteId) {
      outStartBeats = note.startBeats;
      outEndBeats = note.startBeats + note.lengthBeats;
      return true;
    }
  }

  // Note not found - return defaults
  outStartBeats = 0.0;
  outEndBeats = 1.0;
  return false;
}

double ExpressionLaneEditor::getPixelsPerBeat() const {
  if (pianoRoll_ == nullptr) {
    return 20.0; // Default fallback
  }

  // Access the pixelsPerBeat member from PianoRollComponent
  // This is a public member variable
  return pianoRoll_->pixelsPerBeat;
}

bool ExpressionLaneEditor::removeExpressionPoint(const juce::String& noteId,
                                                  ExpressionType type,
                                                  size_t pointIndex) {
  if (pianoRoll_ == nullptr) return false;

  // Get current points for the note
  auto points = pianoRoll_->getNoteExpression(noteId, type);

  // Validate point index
  if (pointIndex >= points.size()) {
    return false;
  }

  // Remove the point
  points.erase(points.begin() + pointIndex);

  // Update the note expression
  pianoRoll_->setNoteExpression(noteId, type, points);

  // Remove from active interactions
  auto it = std::remove_if(activeInteractions_.begin(), activeInteractions_.end(),
    [&](const std::unique_ptr<ExpressionPointInteraction>& interaction) {
      return interaction->noteId == noteId &&
             interaction->type == type &&
             interaction->pointIndex == pointIndex;
    });
  activeInteractions_.erase(it, activeInteractions_.end());

  // Reindex remaining interactions for this note/type
  for (auto& interaction : activeInteractions_) {
    if (interaction->noteId == noteId && interaction->type == type) {
      if (interaction->pointIndex > pointIndex) {
        interaction->pointIndex--;
      }
    }
  }

  // Trigger callback
  if (onPointModified_) {
    onPointModified_(noteId, type, points);
  }

  return true;
}

bool ExpressionLaneEditor::addPointToAllSelectedNotes(const juce::Point<float>& position,
                                                       const SkRect& laneRect,
                                                       ExpressionType type,
                                                       const std::vector<juce::String>& selectedNotes) {
  if (pianoRoll_ == nullptr || selectedNotes.empty()) {
    return false;
  }

  bool anyAdded = false;

  // Add point to each selected note
  for (const auto& noteId : selectedNotes) {
    if (addPointAtPosition(position, laneRect, type, noteId)) {
      anyAdded = true;
    }
  }

  return anyAdded;
}

} // namespace zenith
