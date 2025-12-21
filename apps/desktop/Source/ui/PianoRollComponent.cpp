/**
 * @file PianoRollComponent.cpp
 * @brief Professional-grade MIDI Piano Roll Editor Implementation (Core)
 */

#include "../../include/ui/PianoRollComponent.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"
#include <algorithm>
#include <cmath>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <core/SkRect.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>
#include <limits>

using namespace zenith;

constexpr float RULER_HEIGHT = 30.0f;
constexpr float TOOLBAR_HEIGHT = 40.0f;
constexpr float PIANO_WIDTH = 80.0f;

//==============================================================================
// Constructor / Destructor
//==============================================================================

PianoRollComponent::PianoRollComponent(zenith::ProjectState &state)
    : projectState(state) {
  setWantsKeyboardFocus(true);
  setMouseCursor(juce::MouseCursor::NormalCursor);
}

PianoRollComponent::~PianoRollComponent() {
  if (currentClip.isValid()) {
    auto [track, clip] = projectState.findClip(currentClip.clipId);
    if (clip.isValid()) {
      auto midiNotesNode =
          clip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (midiNotesNode.isValid())
        midiNotesNode.removeListener(this);
    }
  }
}

//==============================================================================
// Clip Management
//==============================================================================

void PianoRollComponent::setClipContext(const MidiClipContext &context) {
  if (currentClip.isValid()) {
    auto [oldTrack, oldClip] = projectState.findClip(currentClip.clipId);
    if (oldClip.isValid()) {
      auto midiNotesNode =
          oldClip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (midiNotesNode.isValid())
        midiNotesNode.removeListener(this);
    }
  }

  currentClip = context;

  if (currentClip.isValid()) {
    auto [track, clip] = projectState.findClip(currentClip.clipId);
    if (clip.isValid()) {
      auto midiNotesNode =
          clip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (!midiNotesNode.isValid()) {
        midiNotesNode = juce::ValueTree(zenith::ProjectState::ID_NOTES);
        clip.appendChild(midiNotesNode, nullptr);
      }
      midiNotesNode.addListener(this);
    }
    refreshNotesFromProjectState();
  }
  repaint();
}

void PianoRollComponent::refreshNotesFromProjectState() {
  if (!currentClip.isValid()) {
    noteRects.clear();
    repaint();
    return;
  }

  std::vector<juce::String> selectedIds;
  for (const auto &n : noteRects) {
    if (n.selected)
      selectedIds.push_back(n.id);
  }

  auto notes = projectState.getMidiNotesForClip(currentClip.clipId);
  noteRects.clear();
  noteRects.reserve(notes.size());

  for (const auto &n : notes) {
    NoteRect nr;
    nr.id = n.id;
    nr.pitch = n.pitch;
    nr.startBeats = n.startBeats;
    nr.lengthBeats = n.lengthBeats;
    nr.velocity = n.velocity;
    nr.muted = n.muted;
    nr.probability = n.probability;
    nr.condition = n.condition;
    nr.recurrence = n.recurrence;
    nr.articulationId = n.articulationId;

    bool isSelected = false;
    for (const auto &selId : selectedIds) {
      if (selId == n.id) {
        isSelected = true;
        break;
      }
    }
    nr.selected = isSelected;
    noteRects.push_back(nr);
  }

  // Multi-Clip
  for (const auto &ctx : multiClipContexts) {
    if (!ctx.isValid() || ctx.clipId == currentClip.clipId)
      continue;
    auto notes = projectState.getMidiNotesForClip(ctx.clipId);
    for (const auto &n : notes) {
      NoteRect nr;
      nr.id = n.id;
      nr.ownerClipId = ctx.clipId;
      nr.pitch = n.pitch;
      nr.startBeats = n.startBeats;
      nr.lengthBeats = n.lengthBeats;
      nr.velocity = n.velocity;
      nr.muted = n.muted;
      nr.selected = false;
      noteRects.push_back(nr);
    }
  }

  std::sort(noteRects.begin(), noteRects.end(),
            [](const NoteRect &a, const NoteRect &b) {
              if (std::abs(a.startBeats - b.startBeats) > 0.001)
                return a.startBeats < b.startBeats;
              return a.pitch < b.pitch;
            });

  updateVisiblePitches();
  updateNoteRectangles();
  detectNoteCollisions();
  repaint();
}

void PianoRollComponent::updateNoteRectangles() {
  float contentTop = RULER_HEIGHT + TOOLBAR_HEIGHT;
  // noteGridHeight is cached

  for (auto &note : noteRects) {
    float x = PIANO_WIDTH + beatsToPixels(note.startBeats);
    float y = RULER_HEIGHT + pitchToPixels(note.pitch);
    float width = beatsToPixels(note.lengthBeats);
    float height = pixelsPerPitch;

    note.bounds = juce::Rectangle<float>(x, y, width, height);

    float velocityBarY =
        RULER_HEIGHT + noteGridHeight + velocityToPixels(note.velocity);
    float velocityBarHeight =
        (RULER_HEIGHT + noteGridHeight + velocityLaneHeight) - velocityBarY;
    note.velocityBounds =
        juce::Rectangle<float>(x, velocityBarY, width, velocityBarHeight);
  }
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

int PianoRollComponent::pixelsToPitch(float y) const {
  float adjustedY = y - RULER_HEIGHT;
  int row = static_cast<int>((adjustedY + scrollOffsetY) / pixelsPerPitch);

  if (foldMode) {
    if (visiblePitches.empty())
      return 60;
    int maxRow = (int)visiblePitches.size() - 1;
    int resultRow = maxRow - row;
    resultRow = juce::jlimit(0, maxRow, resultRow);
    return visiblePitches[resultRow];
  } else {
    int pitch = 127 - row;
    return juce::jlimit(0, 127, pitch);
  }
}

float PianoRollComponent::pitchToPixels(int pitch) const {
  int row;
  if (foldMode) {
    row = mapPitchToRow(pitch);
    if (!visiblePitches.empty())
      row = ((int)visiblePitches.size() - 1) - row;
  } else {
    row = 127 - pitch;
  }
  return row * pixelsPerPitch - scrollOffsetY;
}

double PianoRollComponent::pixelsToBeats(float x) const {
  float adjustedX = x - PIANO_WIDTH;
  return (adjustedX + scrollOffsetX) / pixelsPerBeat;
}

float PianoRollComponent::beatsToPixels(double beats) const {
  return static_cast<float>(beats * pixelsPerBeat - scrollOffsetX);
}

double PianoRollComponent::snapToGrid(double beats) const {
  if (!snapEnabled || gridBeats <= 0.0)
    return beats;
  return std::round(beats / gridBeats) * gridBeats;
}

int PianoRollComponent::pixelsToVelocity(float y) const {
  auto bounds = getLocalBounds();
  // noteGridHeight is cached

  float yInLane = y - (TOOLBAR_HEIGHT + RULER_HEIGHT + noteGridHeight);
  float normalizedY = yInLane / velocityLaneHeight;
  int velocity = static_cast<int>((1.0f - normalizedY) * 127.0f);
  return juce::jlimit(1, 127, velocity);
}

float PianoRollComponent::velocityToPixels(int velocity) const {
  float normalized = velocity / 127.0f;
  return (1.0f - normalized) * velocityLaneHeight;
}

//==============================================================================
// Mouse Interaction Helpers
//==============================================================================

PianoRollComponent::NoteRect *PianoRollComponent::findNoteAtPosition(float x,
                                                                     float y) {
  for (auto it = noteRects.rbegin(); it != noteRects.rend(); ++it) {
    if (it->bounds.contains(x, y))
      return &(*it);
  }
  return nullptr;
}

PianoRollComponent::NoteRect *
PianoRollComponent::findNoteInVelocityLane(float x, float y) {
  auto bounds = getLocalBounds();
  float contentTop = RULER_HEIGHT + TOOLBAR_HEIGHT;
  // noteGridHeight is cached
  float velocityLaneTop = contentTop + noteGridHeight;

  if (y < velocityLaneTop || y > velocityLaneTop + velocityLaneHeight)
    return nullptr;

  for (auto it = noteRects.rbegin(); it != noteRects.rend(); ++it) {
    if (it->velocityBounds.contains(x, y))
      return &(*it);
  }
  return nullptr;
}

PianoRollComponent::DragMode
PianoRollComponent::detectNoteHitRegion(const NoteRect &note, float x,
                                        float y) const {
  if (!note.bounds.contains(x, y))
    return DragMode::None;
  if (x < note.bounds.getX() + resizeHandleWidth)
    return DragMode::ResizeLeft;
  if (x > note.bounds.getRight() - resizeHandleWidth)
    return DragMode::ResizeRight;
  return DragMode::MoveNote;
}

PianoRollComponent::CursorType
PianoRollComponent::getCursorForPosition(float x, float y) const {
  if (x < PIANO_WIDTH || y < RULER_HEIGHT)
    return CursorType::Normal;

  // Tool-aware cursor logic
  if (currentTool == Tool::Draw || currentTool == Tool::Erase ||
      currentTool == Tool::Slice) {
    return CursorType::Crosshair;
  }

  auto bounds = getLocalBounds();
  float contentTop = RULER_HEIGHT + TOOLBAR_HEIGHT;
  // noteGridHeight is cached
  if (y >= contentTop + noteGridHeight)
    return CursorType::Crosshair;

  for (const auto &note : noteRects) {
    if (note.bounds.contains(x, y)) {
      if (x < note.bounds.getX() + resizeHandleWidth)
        return CursorType::ResizeLeft;
      if (x > note.bounds.getRight() - resizeHandleWidth)
        return CursorType::ResizeRight;
      return CursorType::Hand;
    }
  }
  return CursorType::Crosshair;
}

juce::MouseCursor PianoRollComponent::getMouseCursor() {
  switch (currentCursorType) {
  case CursorType::Hand:
    return juce::MouseCursor::DraggingHandCursor;
  case CursorType::ResizeHorizontal:
  case CursorType::ResizeLeft:
  case CursorType::ResizeRight:
    return juce::MouseCursor::LeftRightResizeCursor;
  case CursorType::Crosshair:
    return juce::MouseCursor::CrosshairCursor;
  default:
    return juce::MouseCursor::NormalCursor;
  }
}

//==============================================================================
// ValueTree Listeners
//==============================================================================

void PianoRollComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                             juce::ValueTree &child) {
  if (parent.hasType(zenith::ProjectState::ID_NOTES)) {
    refreshNotesFromProjectState();
  }
}

void PianoRollComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                               juce::ValueTree &child,
                                               int index) {
  if (parent.hasType(zenith::ProjectState::ID_NOTES)) {
    refreshNotesFromProjectState();
  }
}

void PianoRollComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  refreshNotesFromProjectState();
}

void PianoRollComponent::mouseMove(const juce::MouseEvent &e) {
  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);

  auto bounds = getLocalBounds();
  float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
  // noteGridHeight is cached

  // Track hovered piano key
  int newHoveredKey = -1;
  if (x < PIANO_WIDTH && y >= RULER_HEIGHT &&
      y < RULER_HEIGHT + noteGridHeight) {
    newHoveredKey = pixelsToPitch(y);
    newHoveredKey = juce::jlimit(0, 127, newHoveredKey);
  }

  if (newHoveredKey != hoveredPianoKey) {
    hoveredPianoKey = newHoveredKey;
    repaint();
  }

  // Update cursor based on position
  auto newCursorType = getCursorForPosition(x, y);
  if (newCursorType != currentCursorType) {
    currentCursorType = newCursorType;
    repaint();
  }

  // Track hovered note
  auto *newHoveredNote = findNoteAtPosition(x, y);
  if (newHoveredNote != hoveredNote) {
    if (hoveredNote)
      hoveredNote->isHovered = false;
    hoveredNote = newHoveredNote;
    if (hoveredNote)
      hoveredNote->isHovered = true;
    repaint();
  }
}

void PianoRollComponent::mouseDown(const juce::MouseEvent &e) {
  if (!currentClip.isValid())
    return;

  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);
  auto bounds = getLocalBounds();

  if (stepSequencerMode) {
    // Step sequencer mode handled separately
    return;
  }

  // 1. Check Toolbar Clicks
  if (y < TOOLBAR_HEIGHT) {
    float btnX = 10.0f;
    float btnSize = 30.0f;
    float btnMargin = 5.0f;
    float btnY = (TOOLBAR_HEIGHT - btnSize) / 2.0f;

    Tool tools[] = {Tool::Select, Tool::Draw, Tool::Erase, Tool::Slice};
    for (int i = 0; i < 4; ++i) {
      if (x >= btnX && x < btnX + 60.0f && y >= btnY && y < btnY + btnSize) {
        setCurrentTool(tools[i]);
        return;
      }
      btnX += 60.0f + btnMargin;
    }
    return;
  }

  float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;

  // 2. Check Piano Key Clicks
  if (x < PIANO_WIDTH && y >= contentTop) {
    // noteGridHeight is cached
    if (y < contentTop + noteGridHeight) {
      int pitch = pixelsToPitch(y);
      pitch = juce::jlimit(0, 127, pitch);
      playPianoKey(pitch, 100);
      return;
    }
  }

  // Ruler area - not interactive for now
  if (y < RULER_HEIGHT)
    return;

  // noteGridHeight is cached
  float velocityLaneTop = contentTop + noteGridHeight;

  // Velocity lane interaction
  if (y >= velocityLaneTop) {
    auto *note = findNoteInVelocityLane(x, y);
    if (note) {
      startEditingVelocity(note, e);
      return;
    }
  }

  // Main note area - behavior depends on current tool
  if (x < PIANO_WIDTH)
    return;

  auto *note = findNoteAtPosition(x, y);

  switch (currentTool) {
  case Tool::Select:
    if (note) {
      DragMode mode = detectNoteHitRegion(*note, x, y);
      if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight) {
        startResizingNote(note, mode, e);
      } else {
        bool isMultiSelectModifier = e.mods.isCommandDown();
        if (isMultiSelectModifier) {
          note->selected = !note->selected;
          repaint();
        } else if (!note->selected) {
          clearSelection();
          note->selected = true;
          repaint();
        }
        startMovingSelection(e);
      }
    } else {
      if (e.mods.isShiftDown()) {
        startMarqueeSelect(e);
      } else {
        // In select mode, clicking empty space clears selection
        clearSelection();
        repaint();
      }
    }
    break;

  case Tool::Draw:
    if (!note) {
      createNoteAtPosition(x, y);
    } else {
      // Clicking existing note in Draw mode selects it
      clearSelection();
      note->selected = true;
      startMovingSelection(e);
    }
    break;

  case Tool::Erase:
    if (note) {
      projectState.removeMidiNote(currentClip.clipId, note->id,
                                  "Erase MIDI note");
    }
    break;

  case Tool::Slice:
    if (note) {
      // Slice note at cursor position
      // Robust Implementation
      juce::String noteId = note->id;
      juce::String clipId = currentClip.clipId;
      double originalStart = note->startBeats;
      double originalLength = note->lengthBeats;
      int notePitch = note->pitch;
      int noteVelocity = note->velocity;
      bool noteMuted = note->muted;

      double sliceBeat = pixelsToBeats(x - PIANO_WIDTH);
      if (snapEnabled)
        sliceBeat = snapToGrid(sliceBeat);

      // Only slice if position is within note bounds
      if (sliceBeat > originalStart &&
          sliceBeat < originalStart + originalLength) {
        double leftLength = sliceBeat - originalStart;
        double rightLength = originalLength - leftLength;

        // Create right part first
        zenith::ProjectState::MidiNoteSpec rightNote;
        rightNote.pitch = notePitch;
        rightNote.startBeats = sliceBeat;
        rightNote.lengthBeats = rightLength;
        rightNote.velocity = noteVelocity;
        rightNote.muted = noteMuted;

        projectState.getUndoManager().beginNewTransaction("Slice MIDI note");

        // Shorten original
        projectState.setMidiNoteLength(clipId, noteId, leftLength, "");
        // Add new
        projectState.addMidiNote(clipId, rightNote, "");
      }
    }
    break;
  }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent &e) {
  if (sprayCanMode && currentDragMode == DragMode::None) {
    handleSprayPaint(e.position.x, e.position.y);
    return;
  }

  switch (currentDragMode) {
  case DragMode::MoveNote:
    updateSelectionMove(e);
    break;
  case DragMode::ResizeLeft:
  case DragMode::ResizeRight:
    updateNoteResize(e);
    break;
  case DragMode::VelocityEdit:
    updateVelocityEdit(e);
    break;
  case DragMode::MarqueeSelect:
    updateMarqueeSelect(e);
    break;
  default:
    break;
  }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent &e) {
  switch (currentDragMode) {
  case DragMode::MoveNote:
    finishSelectionMove();
    break;
  case DragMode::ResizeLeft:
  case DragMode::ResizeRight:
    finishNoteResize();
    break;
  case DragMode::VelocityEdit:
    finishVelocityEdit();
    break;
  case DragMode::MarqueeSelect:
    finishMarqueeSelect();
    break;
  default:
    break;
  }
  currentDragMode = DragMode::None;
  activeNote = nullptr;
}

void PianoRollComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  if (!currentClip.isValid())
    return;
  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);
  auto *note = findNoteAtPosition(x, y);
  if (note) {
    projectState.removeMidiNote(currentClip.clipId, note->id,
                                "Delete MIDI note");
  }
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent &e,
                                        const juce::MouseWheelDetails &wheel) {
  if (e.mods.isCommandDown()) {
    float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
    zoomHorizontal(zoomFactor, static_cast<float>(e.x));
  } else if (e.mods.isAltDown()) {
    float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
    zoomVertical(zoomFactor, static_cast<float>(e.y));
  } else if (e.mods.isShiftDown()) {
    scrollHorizontal(-wheel.deltaY * 50.0f);
  } else {
    scrollVertical(-wheel.deltaY * 50.0f);
  }
}

//==============================================================================
// Selection Management
//==============================================================================

void PianoRollComponent::clearSelection() {
  for (auto &note : noteRects)
    note.selected = false;
}

void PianoRollComponent::selectNote(NoteRect *note, bool addToSelection) {
  if (!addToSelection)
    clearSelection();
  if (note)
    note->selected = true;
  repaint();
}

void PianoRollComponent::selectNotesInRectangle(
    const juce::Rectangle<float> &rect) {
  for (auto &note : noteRects) {
    if (rect.intersects(note.bounds))
      note.selected = true;
  }
  repaint();
}

void PianoRollComponent::selectAll() {
  for (auto &note : noteRects)
    note.selected = true;
  repaint();
}

void PianoRollComponent::invertSelection() {
  for (auto &note : noteRects)
    note.selected = !note.selected;
  repaint();
}

int PianoRollComponent::getSelectedNoteCount() const {
  return static_cast<int>(
      std::count_if(noteRects.begin(), noteRects.end(),
                    [](const NoteRect &n) { return n.selected; }));
}

//==============================================================================
// Note Editing Operations
//==============================================================================

void PianoRollComponent::createNoteAtPosition(float x, float y) {
  int pitch = pixelsToPitch(y);
  double startBeats = pixelsToBeats(x - PIANO_WIDTH);
  if (snapEnabled)
    startBeats = snapToGrid(startBeats);
  startBeats = juce::jmax(0.0, startBeats);
  pitch = juce::jlimit(0, 127, pitch);
  double lengthBeats = gridBeats;

  zenith::ProjectState::MidiNoteSpec note;
  note.pitch = pitch;
  note.startBeats = startBeats;
  note.lengthBeats = lengthBeats;
  note.velocity = 100;
  note.muted = false;

  projectState.addMidiNote(currentClip.clipId, note, "Create MIDI note");
}

void PianoRollComponent::deleteSelectedNotes() {
  if (!currentClip.isValid() || getSelectedNoteCount() == 0)
    return;
  std::vector<juce::String> selectedIds;
  for (const auto &note : noteRects) {
    if (note.selected)
      selectedIds.push_back(note.id);
  }
  projectState.getUndoManager().beginNewTransaction("Delete MIDI notes");
  for (const auto &noteId : selectedIds) {
    projectState.removeMidiNote(currentClip.clipId, noteId, "");
  }
}

//==============================================================================
// Copy/Paste/Cut
//==============================================================================

void PianoRollComponent::copySelectedNotes() {
  clipboard.clear();
  if (getSelectedNoteCount() == 0)
    return;
  double earliestTime = std::numeric_limits<double>::max();
  for (const auto &note : noteRects) {
    if (note.selected)
      earliestTime = std::min(earliestTime, note.startBeats);
  }
  clipboardReferenceTime = earliestTime;
  for (const auto &note : noteRects) {
    if (note.selected) {
      ClipboardNote clipNote;
      clipNote.pitch = note.pitch;
      clipNote.startBeats = note.startBeats - earliestTime;
      clipNote.lengthBeats = note.lengthBeats;
      clipNote.velocity = note.velocity;
      clipNote.muted = note.muted;
      clipboard.push_back(clipNote);
    }
  }
}

void PianoRollComponent::pasteNotes() {
  if (!currentClip.isValid() || clipboard.empty())
    return;
  double pasteTime = viewStartBeats;
  projectState.getUndoManager().beginNewTransaction("Paste MIDI notes");
  clearSelection();
  for (const auto &clipNote : clipboard) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = clipNote.pitch;
    note.startBeats = pasteTime + clipNote.startBeats;
    note.lengthBeats = clipNote.lengthBeats;
    note.velocity = clipNote.velocity;
    note.muted = clipNote.muted;
    projectState.addMidiNote(currentClip.clipId, note, "");
  }
}

void PianoRollComponent::cutSelectedNotes() {
  copySelectedNotes();
  deleteSelectedNotes();
}

//==============================================================================
// Drag Operations
//==============================================================================

void PianoRollComponent::startMovingSelection(const juce::MouseEvent &e) {
  currentDragMode = DragMode::MoveNote;
  dragStartPos = e.position;
  dragStates.clear();
  for (const auto &note : noteRects) {
    if (note.selected) {
      NoteDragState state;
      state.id = note.id;
      state.originalPitch = note.pitch;
      state.originalStartBeats = note.startBeats;
      dragStates.push_back(state);
    }
  }
}

void PianoRollComponent::updateSelectionMove(const juce::MouseEvent &e) {
  if (dragStates.empty())
    return;
  float deltaX = e.position.x - dragStartPos.x;
  float deltaY = e.position.y - dragStartPos.y;
  double deltaBeats =
      pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
  int deltaPitch = -static_cast<int>(deltaY / pixelsPerPitch);

  size_t stateIndex = 0;
  for (auto &note : noteRects) {
    if (note.selected && stateIndex < dragStates.size()) {
      const auto &originalState = dragStates[stateIndex];
      double newStartBeats = originalState.originalStartBeats + deltaBeats;
      int newPitch = originalState.originalPitch + deltaPitch;
      if (snapEnabled)
        newStartBeats = snapToGrid(newStartBeats);
      newStartBeats = juce::jmax(0.0, newStartBeats);
      newPitch = juce::jlimit(0, 127, newPitch);
      note.startBeats = newStartBeats;
      note.pitch = newPitch;
      ++stateIndex;
    }
  }
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishSelectionMove() {
  if (dragStates.empty())
    return;
  projectState.getUndoManager().beginNewTransaction("Move MIDI notes");
  size_t stateIndex = 0;
  for (auto &note : noteRects) {
    if (note.selected && stateIndex < dragStates.size()) {
      const auto &originalState = dragStates[stateIndex];
      bool pitchChanged = note.pitch != originalState.originalPitch;
      bool startChanged =
          std::abs(note.startBeats - originalState.originalStartBeats) > 0.001;
      if (pitchChanged || startChanged) {
        projectState.moveMidiNote(currentClip.clipId, note.id, note.startBeats,
                                  note.pitch, "");
      }
      ++stateIndex;
    }
  }
  dragStates.clear();
}

void PianoRollComponent::startResizingNote(NoteRect *note, DragMode mode,
                                           const juce::MouseEvent &e) {
  currentDragMode = mode;
  activeNote = note;
  dragStartPos = e.position;
  dragStates.clear();
  NoteDragState state;
  state.id = note->id;
  state.originalStartBeats = note->startBeats;
  state.originalLengthBeats = note->lengthBeats;
  dragStates.push_back(state);
  clearSelection();
  note->selected = true;
  repaint();
}

void PianoRollComponent::updateNoteResize(const juce::MouseEvent &e) {
  if (!activeNote || dragStates.empty())
    return;
  float deltaX = e.position.x - dragStartPos.x;
  double deltaBeats =
      pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
  const auto &originalState = dragStates[0];

  if (currentDragMode == DragMode::ResizeLeft) {
    double newStartBeats = originalState.originalStartBeats + deltaBeats;
    if (snapEnabled)
      newStartBeats = snapToGrid(newStartBeats);
    newStartBeats = juce::jmax(0.0, newStartBeats);
    double newLengthBeats = originalState.originalLengthBeats -
                            (newStartBeats - originalState.originalStartBeats);
    newLengthBeats = juce::jmax(0.01, newLengthBeats);
    activeNote->startBeats = newStartBeats;
    activeNote->lengthBeats = newLengthBeats;
  } else if (currentDragMode == DragMode::ResizeRight) {
    double newLengthBeats = originalState.originalLengthBeats + deltaBeats;
    if (snapEnabled) {
      double endBeats =
          snapToGrid(originalState.originalStartBeats + newLengthBeats);
      newLengthBeats = endBeats - originalState.originalStartBeats;
    }
    newLengthBeats = juce::jmax(0.01, newLengthBeats);
    activeNote->lengthBeats = newLengthBeats;
  }
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishNoteResize() {
  if (!activeNote || dragStates.empty())
    return;
  const auto &originalState = dragStates[0];
  bool startChanged = std::abs(activeNote->startBeats -
                               originalState.originalStartBeats) > 0.001;
  bool lengthChanged = std::abs(activeNote->lengthBeats -
                                originalState.originalLengthBeats) > 0.001;
  if (startChanged || lengthChanged) {
    if (currentDragMode == DragMode::ResizeLeft) {
      projectState.getUndoManager().beginNewTransaction("Resize MIDI note");
      projectState.moveMidiNote(currentClip.clipId, activeNote->id,
                                activeNote->startBeats, activeNote->pitch, "");
      projectState.setMidiNoteLength(currentClip.clipId, activeNote->id,
                                     activeNote->lengthBeats, "");
    } else {
      projectState.setMidiNoteLength(currentClip.clipId, activeNote->id,
                                     activeNote->lengthBeats,
                                     "Resize MIDI note");
    }
  }
  dragStates.clear();
}

void PianoRollComponent::startEditingVelocity(NoteRect *note,
                                              const juce::MouseEvent &e) {
  currentDragMode = DragMode::VelocityEdit;
  activeNote = note;
  dragStartPos = e.position;
  dragStates.clear();
  NoteDragState state;
  state.id = note->id;
  state.originalVelocity = note->velocity;
  dragStates.push_back(state);
  clearSelection();
  note->selected = true;
  repaint();
}

void PianoRollComponent::updateVelocityEdit(const juce::MouseEvent &e) {
  if (!activeNote || dragStates.empty())
    return;
  int newVelocity = pixelsToVelocity(static_cast<float>(e.y));
  activeNote->velocity = newVelocity;
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishVelocityEdit() {
  if (!activeNote || dragStates.empty())
    return;
  const auto &originalState = dragStates[0];
  if (activeNote->velocity != originalState.originalVelocity) {
    projectState.setMidiNoteVelocity(currentClip.clipId, activeNote->id,
                                     activeNote->velocity,
                                     "Edit MIDI velocity");
  }
  dragStates.clear();
}

void PianoRollComponent::startMarqueeSelect(const juce::MouseEvent &e) {
  currentDragMode = DragMode::MarqueeSelect;
  dragStartPos = e.position;
  marqueeRect =
      juce::Rectangle<float>(dragStartPos.x, dragStartPos.y, 0.0f, 0.0f);
  if (!e.mods.isCommandDown())
    clearSelection();
  repaint();
}

void PianoRollComponent::updateMarqueeSelect(const juce::MouseEvent &e) {
  marqueeRect = juce::Rectangle<float>::leftTopRightBottom(
      juce::jmin(dragStartPos.x, e.position.x),
      juce::jmin(dragStartPos.y, e.position.y),
      juce::jmax(dragStartPos.x, e.position.x),
      juce::jmax(dragStartPos.y, e.position.y));
  repaint();
}

void PianoRollComponent::finishMarqueeSelect() {
  selectNotesInRectangle(marqueeRect);
  marqueeRect = juce::Rectangle<float>();
  repaint();
}

//==============================================================================
// Zoom & Scroll
//==============================================================================

void PianoRollComponent::zoomHorizontal(float factor, float centerX) {
  double centerBeats = pixelsToBeats(centerX);
  pixelsPerBeat *= factor;
  pixelsPerBeat = juce::jlimit(10.0, 800.0, pixelsPerBeat);
  viewStartBeats = centerBeats - ((centerX - PIANO_WIDTH) / pixelsPerBeat);
  viewStartBeats = juce::jmax(0.0, viewStartBeats);
  scrollOffsetX = static_cast<int>(viewStartBeats * pixelsPerBeat);
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::zoomVertical(float factor, float centerY) {
  pixelsPerPitch *= factor;
  pixelsPerPitch = juce::jlimit(4.0, 96.0, pixelsPerPitch);
  scrollOffsetY = static_cast<int>(viewLowestPitch * pixelsPerPitch);
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::scrollHorizontal(float delta) {
  scrollOffsetX += static_cast<int>(delta);
  scrollOffsetX = juce::jmax(0, scrollOffsetX);
  viewStartBeats = scrollOffsetX / pixelsPerBeat;
  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::scrollVertical(float delta) {
  scrollOffsetY += static_cast<int>(delta);
  scrollOffsetY =
      juce::jlimit(0, 127 * static_cast<int>(pixelsPerPitch), scrollOffsetY);
  viewLowestPitch = scrollOffsetY / static_cast<int>(pixelsPerPitch);
  updateNoteRectangles();
  repaint();
}

//==============================================================================
// Keyboard Shortcuts
//==============================================================================

bool PianoRollComponent::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    deleteSelectedNotes();
    return true;
  }
  if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
    projectState.undo();
    return true;
  }
  if (key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0)) {
    projectState.redo();
    return true;
  }
  if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0)) {
    copySelectedNotes();
    return true;
  }
  if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0)) {
    pasteNotes();
    return true;
  }
  if (key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0)) {
    cutSelectedNotes();
    return true;
  }
  if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)) {
    selectAll();
    return true;
  }
  if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0)) {
    smartDuplicate();
    return true;
  }
  if (key == juce::KeyPress('q', 0, 0)) {
    quantizeSelected(gridBeats, 1.0f, 0.0f);
    return true;
  }

  // ... (keep simplified, delegates handle complex stuff if needed)

  return false;
}

void PianoRollComponent::resized() {
  auto bounds = getLocalBounds();
  float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
  noteGridHeight = bounds.getHeight() - contentTop - velocityLaneHeight;
  updateNoteRectangles();
}

void PianoRollComponent::quantizeSelected(double grid, float strength,
                                          float swing) {
  if (grid <= 0.0)
    return;

  // Start undo transaction
  projectState.getUndoManager().beginNewTransaction("Quantize");

  bool anyChanged = false;

  for (auto &note : noteRects) {
    if (!note.selected)
      continue;

    double originalStart = note.startBeats;

    // Simple quantization
    double quantizedStart = std::round(originalStart / grid) * grid;

    // Apply strength (0.0 to 1.0)
    double newStart =
        originalStart + (quantizedStart - originalStart) * strength;

    // Note: Swing logic omitted for brevity

    if (std::abs(newStart - originalStart) > 0.0001) {
      projectState.moveMidiNote(currentClip.clipId, note.id, newStart,
                                note.pitch, "");
      anyChanged = true;
    }
  }

  if (anyChanged)
    repaint();
}

void PianoRollComponent::drawSkia(SkCanvas *canvas) {
  if (!canvas)
    return;
  using namespace zenith::design;

  // Background (Deep Slate)
  canvas->clear(colors::BG_DARKEST);

  SkPaint paint;
  paint.setAntiAlias(true);

  auto localBounds = getLocalBounds();
  float width = (float)localBounds.getWidth();
  float height = (float)localBounds.getHeight();
  float notesHeight = height - RULER_HEIGHT - velocityLaneHeight;

  // 1. Piano Keys Area Background
  SkRect pianoRect =
      SkRect::MakeXYWH(0, RULER_HEIGHT, PIANO_WIDTH, notesHeight);
  paint.setColor(colors::BG_DARKER);
  canvas->drawRect(pianoRect, paint);

  // 2. Grid Lines (Vertical) - Added for "Real" feel
  canvas->save();
  SkRect noteAreaRect = SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT,
                                         width - PIANO_WIDTH, notesHeight);
  canvas->clipRect(noteAreaRect);

  paint.setColor(colors::BORDER_SUBTLE);
  paint.setStrokeWidth(1.0f);
  // Draw vertical lines for beats
  // We iterate visible beat range
  double startBeat = std::floor(pixelsToBeats(PIANO_WIDTH));
  double endBeat = pixelsToBeats(width);

  // Optimization: Don't draw too many lines if zoomed out
  double beatStep = (pixelsPerBeat < 15.0) ? 4.0 : 1.0;

  for (double b = startBeat; b <= endBeat; b += beatStep) {
    float x = PIANO_WIDTH + beatsToPixels(b);
    if (x >= PIANO_WIDTH) {
      canvas->drawLine(x, RULER_HEIGHT, x, RULER_HEIGHT + notesHeight, paint);
    }
  }
  canvas->restore();

  // 3. Draw Keys and Horizontal Grid Lines
  int topPitch = pixelsToPitch(RULER_HEIGHT);
  int bottomPitch = pixelsToPitch(RULER_HEIGHT + notesHeight);

  topPitch = juce::jlimit(0, 127, topPitch);
  bottomPitch = juce::jlimit(0, 127, bottomPitch);

  for (int p = bottomPitch; p <= topPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;
    float h = pixelsPerPitch;

    if (y < RULER_HEIGHT - h || y >= RULER_HEIGHT + notesHeight)
      continue;

    int noteInOctave = p % 12;
    bool black = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                  noteInOctave == 8 || noteInOctave == 10);

    // Draw Key
    static constexpr float kKeyLabelMinZoom = 12.0f;
    static constexpr float kKeyLabelDetailZoom = 18.0f;
    static constexpr float kKeyLabelSmallFontZoom = 11.0f;
    static constexpr float kKeyLabelTinyFontZoom = 9.0f;
    static constexpr float kKeyLabelOffset = -24.0f;
    static constexpr float kKeyLabelDetailOffset = -18.0f;

    SkRect keyRect = SkRect::MakeXYWH(0, y, PIANO_WIDTH, h);
    paint.setStyle(SkPaint::kFill_Style);

    // Check if this key is hovered or playing
    bool isHovered = (p == hoveredPianoKey);
    bool isPlaying = (p == playingPianoKey);

    if (black) {
      // Black key
      if (isPlaying) {
        paint.setColor(colors::MAGENTA);
      } else if (isHovered) {
        paint.setColor(colors::BG_MEDIUM);
      } else {
        paint.setColor(colors::BG_DARKEST);
      }
      canvas->drawRect(keyRect, paint);
    } else {
      // White key (actually light grey)
      if (isPlaying) {
        paint.setColor(colors::CYAN);
      } else if (isHovered) {
        paint.setColor(colors::TEXT_PRIMARY);
      } else {
        paint.setColor(colors::TEXT_SECONDARY); // #A1A1AA
      }
      canvas->drawRect(keyRect, paint);

      // Shadow for depth
      SkPaint shadow;
      shadow.setColor(SkColorSetARGB(50, 0, 0, 0));
      canvas->drawRect(SkRect::MakeXYWH(0, y + h - 1, PIANO_WIDTH, 1), shadow);
    }

    // Key Label
    if (pixelsPerPitch > kKeyLabelMinZoom) {
      static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                        "F#", "G",  "G#", "A",  "A#", "B"};
      SkPaint textPaint;
      textPaint.setAntiAlias(true);

      if (noteInOctave == 0) {
        // C notes get octave number
        textPaint.setColor(black ? colors::TEXT_SECONDARY : colors::BG_DARKEST);
        SkFont font = getMonoFont(
            juce::jmin(kKeyLabelSmallFontZoom, (float)(pixelsPerPitch * 0.7f)),
            FontWeight::Bold);
        juce::String label = "C" + juce::String(p / 12 - 2);
        canvas->drawString(label.toStdString().c_str(),
                           PIANO_WIDTH + kKeyLabelOffset, y + h * 0.7f, font,
                           textPaint);
      } else if (pixelsPerPitch > kKeyLabelDetailZoom) {
        textPaint.setColor(colors::TEXT_TERTIARY);
        SkFont font = getMonoFont(
            juce::jmin(kKeyLabelTinyFontZoom, (float)(pixelsPerPitch * 0.5f)),
            FontWeight::Regular);
        canvas->drawString(noteNames[noteInOctave],
                           PIANO_WIDTH + kKeyLabelDetailOffset, y + h * 0.7f,
                           font, textPaint);
      }
    }

    // Horizontal Grid Line
    paint.setColor(colors::BORDER_SUBTLE);
    paint.setStrokeWidth(1.0f);
    canvas->drawLine(PIANO_WIDTH, y + h, width, y + h, paint);
  }

  // 4. Notes - Use VIOLET for notes, CYAN for selected (per requirements)
  canvas->save();
  canvas->clipRect(noteAreaRect);

  SkPaint selectedGlowPaint;
  selectedGlowPaint.setColor(colors::CYAN);
  selectedGlowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kSolid_SkBlurStyle, 4.0f));

  for (const auto &note : noteRects) {
    // Culling
    if (note.bounds.getY() > height || note.bounds.getBottom() < 0)
      continue;
    if (note.bounds.getX() > width || note.bounds.getRight() < PIANO_WIDTH)
      continue;

    SkRect r =
        SkRect::MakeXYWH(note.bounds.getX(), note.bounds.getY(),
                         note.bounds.getWidth(), note.bounds.getHeight());

    // Inner Rect for pseudo-3D
    SkRect inner = r.makeInset(1.0f, 1.0f);
    SkRRect rr = SkRRect::MakeRectXY(inner, 2.0f, 2.0f);

    // Color based on selection and velocity
    SkColor noteColor;
    if (note.selected) {
      // Glow for selected notes
      SkRect outsetRect = rr.rect().makeOutset(2.0f, 2.0f);
      canvas->drawRect(outsetRect, selectedGlowPaint);
      noteColor = colors::CYAN; // CYAN for selected (per requirements)
    } else {
      // VIOLET for notes (per requirements)
      // Velocity shown as color intensity
      float velocityFactor = note.velocity / 127.0f;
      // Interpolate between darker and brighter VIOLET based on velocity
      noteColor =
          interpolateColor(darken(colors::VIOLET, 0.3f),
                           lighten(colors::VIOLET, 0.1f), velocityFactor);
    }

    paint.setColor(noteColor);

    // Gradient for note
    SkPoint pts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
    SkColor nColors[2] = {lighten(noteColor, 0.1f), darken(noteColor, 0.1f)};
    paint.setShader(SkGradientShader::MakeLinear(pts, nColors, nullptr, 2,
                                                 SkTileMode::kClamp));

    canvas->drawRRect(rr, paint);
    paint.setShader(nullptr);

    // Border
    SkPaint border;
    border.setStyle(SkPaint::kStroke_Style);
    border.setColor(SkColorSetARGB(100, 0, 0, 0));
    border.setAntiAlias(true);
    canvas->drawRRect(rr, border);

    // Hover highlight
    if (note.isHovered && !note.selected) {
      SkPaint hoverPaint;
      hoverPaint.setStyle(SkPaint::kStroke_Style);
      hoverPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.5f));
      hoverPaint.setStrokeWidth(1.5f);
      hoverPaint.setAntiAlias(true);
      canvas->drawRRect(rr, hoverPaint);
    }
  }
  canvas->restore();

  // 5. Velocity Lane Background
  SkRect velocityRect =
      SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT + notesHeight,
                       width - PIANO_WIDTH, velocityLaneHeight);
  paint.setColor(colors::BG_DARK);
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRect(velocityRect, paint);

  // Velocity Bars - Draw vertical bars for each note
  canvas->save();
  canvas->clipRect(velocityRect);

  // Separator line
  paint.setColor(colors::BORDER_DEFAULT);
  canvas->drawLine(0, RULER_HEIGHT + notesHeight, width,
                   RULER_HEIGHT + notesHeight, paint);

  float velocityLaneTop = RULER_HEIGHT + notesHeight;
  float velocityLaneBottom = velocityLaneTop + velocityLaneHeight;

  for (const auto &note : noteRects) {
    // Skip notes outside visible horizontal range
    if (note.bounds.getX() > width || note.bounds.getRight() < PIANO_WIDTH)
      continue;

    float barWidth = std::max(2.0f, std::min(8.0f, note.bounds.getWidth()));
    float barX =
        note.bounds.getX() + (note.bounds.getWidth() - barWidth) * 0.5f;

    // Height based on velocity (0-127 maps to 0 to velocityLaneHeight)
    float normalizedVelocity = note.velocity / 127.0f;
    float barHeight = normalizedVelocity * (velocityLaneHeight - 4.0f);
    float barY = velocityLaneBottom - barHeight - 2.0f;

    // Color - selected uses CYAN, unselected uses VIOLET with velocity
    // intensity
    SkColor barColor;
    if (note.selected) {
      barColor = colors::CYAN;
    } else {
      barColor = interpolateColor(darken(colors::VIOLET, 0.2f), colors::VIOLET,
                                  normalizedVelocity);
    }

    // Draw velocity bar
    SkRect barRect = SkRect::MakeXYWH(barX, barY, barWidth, barHeight);
    SkRRect barRR = SkRRect::MakeRectXY(barRect, 1.0f, 1.0f);

    paint.setColor(barColor);
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRRect(barRR, paint);

    // Glow for selected
    if (note.selected) {
      SkPaint glowPaint;
      glowPaint.setColor(colors::CYAN);
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 2.0f));
      canvas->drawRRect(barRR, glowPaint);
    }
  }
  canvas->restore();

  // 6. Playhead - follows transport
  {
    float playheadX = PIANO_WIDTH + beatsToPixels(currentPlayheadBeats);
    if (playheadX >= PIANO_WIDTH && playheadX <= width) {
      // Playhead line
      SkPaint playheadPaint;
      playheadPaint.setColor(colors::TEXT_PRIMARY);
      playheadPaint.setStrokeWidth(2.0f);
      playheadPaint.setAntiAlias(true);
      canvas->drawLine(playheadX, RULER_HEIGHT, playheadX, height,
                       playheadPaint);

      // Triangle marker in ruler/toolbar area
      static constexpr float kPlayheadMarkerHalfWidth = 5.0f;
      static constexpr float kPlayheadMarkerHeight = 8.0f;
      float contentTop = (float)RULER_HEIGHT;

      SkPath trianglePath;
      trianglePath.moveTo(playheadX, contentTop);
      trianglePath.lineTo(playheadX - kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.lineTo(playheadX + kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.close();
    }
  }

  // 7. Marquee Selection
  if (!marqueeRect.isEmpty()) {
    SkRect m =
        SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                         marqueeRect.getWidth(), marqueeRect.getHeight());
    paint.setColor(withAlpha(colors::CYAN, 0.2f));
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(m, paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(colors::CYAN);
    canvas->drawRect(m, paint);
  }
}

void PianoRollComponent::smartDuplicate() {
  copySelectedNotes();
  if (clipboard.empty())
    return;

  double start = std::numeric_limits<double>::max();
  double end = std::numeric_limits<double>::lowest();
  bool hasSelection = false;

  for (const auto &note : noteRects) {
    if (note.selected) {
      hasSelection = true;
      start = std::min(start, note.startBeats);
      end = std::max(end, note.startBeats + note.lengthBeats);
    }
  }

  if (!hasSelection)
    return;

  double length = end - start;
  if (length < 0.001)
    length = gridBeats;

  projectState.getUndoManager().beginNewTransaction("Smart Duplicate");
  clearSelection();

  for (const auto &clipNote : clipboard) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = clipNote.pitch;
    note.startBeats = start + length + clipNote.startBeats;
    note.lengthBeats = clipNote.lengthBeats;
    note.velocity = clipNote.velocity;
    note.muted = clipNote.muted;

    projectState.addMidiNote(currentClip.clipId, note, "");
  }
}

void PianoRollComponent::timerCallback() { SkiaComponent::timerCallback(); }

void PianoRollComponent::handleSprayPaint(float x, float y) {
  createNoteAtPosition(x, y);
}

void PianoRollComponent::detectNoteCollisions() {}

void PianoRollComponent::updateVisiblePitches() {
  visiblePitches.clear();
  for (int i = 0; i < 128; ++i)
    visiblePitches.push_back(i);
}

int PianoRollComponent::mapPitchToRow(int pitch) const { return pitch; }

int PianoRollComponent::mapRowToPitch(int row) const { return row; }

//==============================================================================
// Tool System
//==============================================================================

void PianoRollComponent::setCurrentTool(Tool tool) {
  currentTool = tool;

  // Update cursor based on tool
  switch (tool) {
  case Tool::Select:
    setMouseCursor(juce::MouseCursor::NormalCursor);
    break;
  case Tool::Draw:
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    break;
  case Tool::Erase:
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    break;
  case Tool::Slice:
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    break;
  }
  repaint();
}

//==============================================================================
// Piano Key Interaction
//==============================================================================

void PianoRollComponent::playPianoKey(int pitch, int velocity) {
  pitch = juce::jlimit(0, 127, pitch);
  velocity = juce::jlimit(1, 127, velocity);

  playingPianoKey = pitch;

  // Trigger note preview callback if set
  if (notePreviewCallback) {
    notePreviewCallback(pitch, velocity, true);
  }

  repaint();
}

void PianoRollComponent::stopPianoKey(int pitch) {
  if (playingPianoKey == pitch) {
    if (notePreviewCallback) {
      notePreviewCallback(pitch, 0, false);
    }
    playingPianoKey = -1;
    repaint();
  }
}
