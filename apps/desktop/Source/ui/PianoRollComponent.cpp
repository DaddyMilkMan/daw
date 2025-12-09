/**
 * @file PianoRollComponent.cpp
 * @brief Professional-grade MIDI Piano Roll Editor Implementation (Core)
 */

#include "../../include/ui/PianoRollComponent.h"
#include <algorithm>
#include <cmath>
#include <limits>

using namespace zenith;

constexpr float RULER_HEIGHT = 30.0f;
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
  auto bounds = getLocalBounds();
  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;

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
  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
  float yInLane = y - (RULER_HEIGHT + noteGridHeight);
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
  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
  float velocityLaneTop = RULER_HEIGHT + noteGridHeight;

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

  auto bounds = getLocalBounds();
  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
  if (y >= RULER_HEIGHT + noteGridHeight)
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
  auto newCursorType =
      getCursorForPosition(static_cast<float>(e.x), static_cast<float>(e.y));
  if (newCursorType != currentCursorType) {
    currentCursorType = newCursorType;
    repaint();
  }

  auto *newHoveredNote =
      findNoteAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));
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
    // ... (Simplified for core, advanced logic in Advanced)
    return;
  }

  if (x < PIANO_WIDTH || y < RULER_HEIGHT)
    return;

  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
  float velocityLaneTop = RULER_HEIGHT + noteGridHeight;

  if (y >= velocityLaneTop) {
    auto *note = findNoteInVelocityLane(x, y);
    if (note) {
      startEditingVelocity(note, e);
      return;
    }
  }

  auto *note = findNoteAtPosition(x, y);
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
      createNoteAtPosition(x, y);
    }
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

} // namespace zenith