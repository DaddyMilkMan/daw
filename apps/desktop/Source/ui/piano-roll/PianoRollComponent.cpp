/**
 * @file PianoRollComponent.cpp
 * @brief Professional-grade MIDI Piano Roll Editor Implementation (Core)
 */

#include "PianoRollComponent.h"
#include "Engine.h"
#include "ZenithDesignSystem.h"
#include <algorithm>
#include <cmath>
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

using namespace zenith;

// Magic numbers moved to constants/theme
static constexpr float NOTE_CORNER_RADIUS = 3.0f;
static constexpr float SELECTION_STROKE_WIDTH = 2.0f;
static constexpr float HOVER_STROKE_WIDTH = 2.0f;
constexpr float RULER_HEIGHT = 30.0f;
constexpr float TOOLBAR_HEIGHT = 40.0f;
constexpr float PIANO_WIDTH = 80.0f;

// Toolbar Layout (Agent 1 Constants)
constexpr float TOOLBAR_BUTTON_START_X = 10.0f;
constexpr float TOOLBAR_BUTTON_WIDTH = 60.0f;
constexpr float TOOLBAR_BUTTON_HEIGHT = 30.0f;
constexpr float TOOLBAR_BUTTON_MARGIN = 5.0f;
constexpr int DEFAULT_PIANO_KEY_VELOCITY = 100;

//==============================================================================
// Constructor / Destructor
//==============================================================================

PianoRollComponent::PianoRollComponent(zenith::ProjectState &state,
                                       zenith::Engine &engine)
    : projectState(state), engine_(engine) {
  setWantsKeyboardFocus(true);
  setMouseCursor(juce::MouseCursor::NormalCursor);

  // Initialize visual resources
  using namespace zenith::design;
  rulerBarFont_ = typography::getMonoFont(11.0f, FontWeight::Bold);
  rulerBeatFont_ = typography::getMonoFont(9.0f, FontWeight::Regular);
  clipNameFont_ = typography::getSkFont(10.0f, FontWeight::Medium);

  textPaint_.setAntiAlias(true);
  textPaint_.setColor(colors::TEXT_PRIMARY);

  borderPaint_.setStyle(SkPaint::kStroke_Style);
  borderPaint_.setAntiAlias(true);

  generalPaint_.setAntiAlias(true);

  // Smooth playhead animation driven by VBlank
  vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(this, [this] {
    updatePlayheadPosition(engine_.getPlaybackPositionBeats());
  });
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

PianoRollComponent::NoteRect *
PianoRollComponent::findNoteById(const juce::String &id) {
  if (id.isEmpty())
    return nullptr;
  for (auto &note : noteRects) {
    if (note.id == id)
      return &note;
  }
  return nullptr;
}

void PianoRollComponent::mouseMove(const juce::MouseEvent &e) {
  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);

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
    repaint(0, 0, (int)PIANO_WIDTH, getHeight());
  }

  // Update cursor based on position
  auto newCursorType = getCursorForPosition(x, y);
  if (newCursorType != currentCursorType) {
    currentCursorType = newCursorType;
    repaint(); // Cursor changes might affect tooltips or global state, keep
               // simple for cursor
  }

  // Track hovered note
  auto *newHoveredNote = findNoteAtPosition(x, y);
  juce::String newHoveredId =
      newHoveredNote ? newHoveredNote->id : juce::String();

  if (newHoveredId != hoveredNoteId) {
    juce::Rectangle<float> dirtyRect;

    // Invalidate old hovered
    if (auto *oldHovered = findNoteById(hoveredNoteId)) {
      oldHovered->isHovered = false;
      dirtyRect = dirtyRect.getUnion(oldHovered->bounds);
    }

    hoveredNoteId = newHoveredId;

    // Invalidate new hovered
    if (auto *newHovered = findNoteById(hoveredNoteId)) {
      newHovered->isHovered = true;
      dirtyRect = dirtyRect.getUnion(newHovered->bounds);
    }

    if (!dirtyRect.isEmpty()) {
      // Expand slightly for strokes/shadows
      repaint(dirtyRect.expanded(2.0f).toNearestInt());
    }
  }
}

void PianoRollComponent::mouseDown(const juce::MouseEvent &e) {
  if (!currentClip.isValid())
    return;

  if (stepSequencerMode) {
    // Step sequencer mode handled separately
    return;
  }

  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);
  float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;
  float velocityLaneTop = contentTop + noteGridHeight;

  // 1. Check Toolbar Clicks
  if (y < TOOLBAR_HEIGHT) {
    handleToolbarClick(e, x, y);
    return;
  }

  // 2. Check Piano Key Clicks
  if (x < PIANO_WIDTH && y >= contentTop) {
    if (y < contentTop + noteGridHeight) {
      handlePianoKeyClick(e, x, y);
      return;
    }
  }

  // Ruler area - not interactive for now
  if (y < RULER_HEIGHT)
    return;

  // Velocity lane interaction
  if (y >= velocityLaneTop) {
    handleVelocityLaneClick(e, x, y);
    return;
  }

  // Main note area - behavior depends on current tool
  if (x < PIANO_WIDTH)
    return;

  handleNoteMainAreaClick(e, x, y);
}

void PianoRollComponent::handleToolbarClick(const juce::MouseEvent &e, float x,
                                            float y) {

  float btnX = TOOLBAR_BUTTON_START_X;
  float btnY = (TOOLBAR_HEIGHT - TOOLBAR_BUTTON_HEIGHT) / 2.0f;

  Tool tools[] = {Tool::Select, Tool::Draw, Tool::Erase, Tool::Slice};
  for (const auto tool : tools) {
    if (x >= btnX && x < btnX + TOOLBAR_BUTTON_WIDTH && y >= btnY &&
        y < btnY + TOOLBAR_BUTTON_HEIGHT) {
      setCurrentTool(tool);
      return;
    }
    btnX += TOOLBAR_BUTTON_WIDTH + TOOLBAR_BUTTON_MARGIN;
  }
}

void PianoRollComponent::handlePianoKeyClick(const juce::MouseEvent &e, float x,
                                             float y) {
  int pitch = pixelsToPitch(y);
  pitch = juce::jlimit(0, 127, pitch);
  playPianoKey(pitch, DEFAULT_PIANO_KEY_VELOCITY);
}

void PianoRollComponent::handleVelocityLaneClick(const juce::MouseEvent &e,
                                                 float x, float y) {

  auto *note = findNoteInVelocityLane(x, y);
  if (note) {
    startEditingVelocity(note, e);
    activeNoteId = note->id;
  }
}

void PianoRollComponent::handleNoteMainAreaClick(const juce::MouseEvent &e,
                                                 float x, float y) {

  auto *note = findNoteAtPosition(x, y);

  switch (currentTool) {
  case Tool::Select:
    if (note) {
      DragMode mode = detectNoteHitRegion(*note, x, y);
      if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight) {
        startResizingNote(note, mode, e);
        activeNoteId = note->id;
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
        activeNoteId = note->id;
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
      activeNoteId = note->id;
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
  activeNoteId = juce::String();
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
  note.id = juce::Uuid().toString();
  note.pitch = pitch;
  note.startBeats = startBeats;
  note.lengthBeats = lengthBeats;
  note.velocity = 100;
  note.muted = false;

  projectState.addMidiNote(currentClip.clipId, note, "Create MIDI note");

  // Interaction Polish: Immediately select the new note
  // addMidiNote triggers listeners synchronously, so noteRects should be
  // updated.
  for (auto &n : noteRects) {
    if (n.id == note.id) {
      selectNote(&n, false);
      break;
    }
  }
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
      stateIndex++;
    }
  }
  dragStates.clear();
}

//==============================================================================
// Drag Operations (Refactored for Safety)
//==============================================================================

void PianoRollComponent::startResizingNote(NoteRect *note, DragMode mode,
                                           const juce::MouseEvent &e) {

  currentDragMode = mode;
  activeNoteId = note->id; // ID Safe
  dragStartPos = e.position;

  // Store initial state for all selected notes if this is a multi-edit
  dragStates.clear();
  for (const auto &n : noteRects) {
    if (n.selected) {
      NoteDragState s;
      s.id = n.id;
      s.originalPitch = n.pitch;
      s.originalStartBeats = n.startBeats;
      s.originalLengthBeats = n.lengthBeats;
      s.originalVelocity = n.velocity;
      dragStates.push_back(s);
    }
  }
}

void PianoRollComponent::updateNoteResize(const juce::MouseEvent &e) {
  auto *activeNote = findNoteById(activeNoteId);
  if (!activeNote || dragStates.empty())
    return;

  double deltaBeats =
      pixelsToBeats(e.position.x) - pixelsToBeats(dragStartPos.x);
  if (snapEnabled) {
    // Snap delta instead of absolute position for better feel
    deltaBeats = snapToGrid(deltaBeats);
  }

  for (auto &state : dragStates) {
    // Find note by ID (O(N) lookup but safe)
    auto *n = findNoteById(state.id);
    if (!n)
      continue;

    if (currentDragMode == DragMode::ResizeRight) {
      double newLen =
          std::max(gridBeats, state.originalLengthBeats + deltaBeats);
      n->lengthBeats = newLen;
    } else if (currentDragMode == DragMode::ResizeLeft) {
      double newStart = state.originalStartBeats + deltaBeats;
      double newLen = state.originalLengthBeats - deltaBeats;

      if (newLen < gridBeats) {
        newStart =
            state.originalStartBeats + (state.originalLengthBeats - gridBeats);
        newLen = gridBeats;
      }

      n->startBeats = newStart;
      n->lengthBeats = newLen;
    }
  }

  repaint();
}

void PianoRollComponent::finishNoteResize() {
  auto *activeNote = findNoteById(activeNoteId);
  if (!activeNote || dragStates.empty()) {
    activeNoteId = juce::String();
    return;
  }

  // Use first note to determine change type (simplified)
  bool startChanged = false;
  bool lengthChanged = false;

  // Commit changes via ProjectState
  projectState.getUndoManager().beginNewTransaction("Resize MIDI notes");

  for (const auto &state : dragStates) {
    auto *n = findNoteById(state.id);
    if (!n)
      continue;

    if (std::abs(n->startBeats - state.originalStartBeats) > 0.0001) {
      projectState.moveMidiNote(currentClip.clipId, n->id, n->startBeats,
                                n->pitch, "");
    }
    if (std::abs(n->lengthBeats - state.originalLengthBeats) > 0.0001) {
      projectState.setMidiNoteLength(currentClip.clipId, n->id, n->lengthBeats,
                                     "");
    }
  }

  activeNoteId = juce::String();
}

void PianoRollComponent::startEditingVelocity(NoteRect *note,
                                              const juce::MouseEvent &e) {
  currentDragMode = DragMode::VelocityEdit;
  activeNoteId = note->id;
  dragStartPos = e.position;

  dragStates.clear();
  for (const auto &n : noteRects) {
    if (n.selected) {
      NoteDragState s;
      s.id = n.id;
      s.originalPitch = n.pitch;
      s.originalStartBeats = n.startBeats;
      s.originalLengthBeats = n.lengthBeats;
      s.originalVelocity = n.velocity;
      dragStates.push_back(s);
    }
  }
}

void PianoRollComponent::updateVelocityEdit(const juce::MouseEvent &e) {
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

void PianoRollComponent::updatePlayheadPosition(double engineBeats) {
  double currentTime = juce::Time::getMillisecondCounterHiRes() * 0.001;

  if (std::abs(engineBeats - lastEngineBeats_) > 0.0001) {
    lastEngineBeats_ = engineBeats;
    lastEngineTime_ = currentTime;
  }

  double interpolatedBeats = lastEngineBeats_;
  if (engine_.isPlaying()) {
    double bpm = projectState.getTempo();
    double elapsedSeconds = currentTime - lastEngineTime_;
    double elapsedBeats = elapsedSeconds * (bpm / 60.0);
    interpolatedBeats += elapsedBeats;
  }

  if (std::abs(interpolatedBeats - currentPlayheadBeats) > 0.0001) {
    currentPlayheadBeats = interpolatedBeats;
    markDirty();
  }
}

void PianoRollComponent::handleSprayPaint(float x, float y) {
  createNoteAtPosition(x, y);
}

void PianoRollComponent::detectNoteCollisions() {
  // Reset collision flags
  for (auto &note : noteRects) {
    note.hasCollision = false;
  }

  // O(n²) collision detection - acceptable for typical note counts
  for (size_t i = 0; i < noteRects.size(); ++i) {
    for (size_t j = i + 1; j < noteRects.size(); ++j) {
      auto &a = noteRects[i];
      auto &b = noteRects[j];

      // Same pitch and overlapping time?
      if (a.pitch == b.pitch) {
        double aEnd = a.startBeats + a.lengthBeats;
        double bEnd = b.startBeats + b.lengthBeats;

        // Check for time overlap
        if (a.startBeats < bEnd && aEnd > b.startBeats) {
          a.hasCollision = true;
          b.hasCollision = true;
        }
      }
    }
  }
}

void PianoRollComponent::updateVisiblePitches() {
  visiblePitches.clear();

  if (!foldMode) {
    // Normal mode: show all 128 pitches
    for (int i = 0; i < 128; ++i)
      visiblePitches.push_back(i);
  } else {
    // Fold mode: only show pitches that have notes
    std::set<int> usedPitches;
    for (const auto &note : noteRects) {
      usedPitches.insert(note.pitch);
    }

    // Also include ghost notes if enabled
    for (const auto &ghost : ghostNotes) {
      usedPitches.insert(ghost.pitch);
    }

    // Convert to sorted vector (high to low for display)
    for (int p : usedPitches) {
      visiblePitches.push_back(p);
    }

    // Sort descending (highest pitch at top)
    std::sort(visiblePitches.begin(), visiblePitches.end(),
              std::greater<int>());

    // If no notes, show default range (C3-C5)
    if (visiblePitches.empty()) {
      for (int i = 72; i >= 48; --i)
        visiblePitches.push_back(i);
    }
  }
}

int PianoRollComponent::mapPitchToRow(int pitch) const {
  if (!foldMode)
    return 127 - pitch; // Normal: just invert for top-to-bottom

  // Fold mode: find pitch in visible list
  for (size_t i = 0; i < visiblePitches.size(); ++i) {
    if (visiblePitches[i] == pitch)
      return static_cast<int>(i);
  }
  return -1; // Not visible
}

int PianoRollComponent::mapRowToPitch(int row) const {
  if (!foldMode)
    return 127 - row; // Normal: invert back

  // Fold mode: lookup pitch from row
  if (row >= 0 && row < static_cast<int>(visiblePitches.size()))
    return visiblePitches[row];
  return -1;
}

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

//==============================================================================
// Scale & Chord Helper Implementations
//==============================================================================

void PianoRollComponent::updateScaleHighlight() {
  // Update internal toggle state or parameters if needed
}

bool PianoRollComponent::isNoteInScale(int pitch) const {
  if (!scaleHighlight.enabled)
    return true;
  int note = pitch % 12;
  int root = scaleHighlight.rootNote;

  // Simple Major Scale Logic (W W H W W W H)
  // Intervals: 0, 2, 4, 5, 7, 9, 11
  static const int majorIntervals[] = {0, 2, 4, 5, 7, 9, 11};

  int interval = (note - root + 12) % 12;
  for (int i : majorIntervals) {
    if (interval == i)
      return true;
  }
  return false;
}

juce::String PianoRollComponent::getCurrentChordName() const {
  // Basic implementation: Analyze selected notes
  std::vector<int> pitches;
  for (const auto &note : noteRects) {
    if (note.selected) {
      pitches.push_back(note.pitch % 12);
    }
  }

  if (pitches.empty())
    return "";

  std::sort(pitches.begin(), pitches.end());
  pitches.erase(std::unique(pitches.begin(), pitches.end()), pitches.end());

  if (pitches.empty())
    return "";

  // Simple chord recognition for demo
  if (pitches.size() == 3) {
    if (pitches[1] - pitches[0] == 4 && pitches[2] - pitches[1] == 3)
      return "Major Triad";
    if (pitches[1] - pitches[0] == 3 && pitches[2] - pitches[1] == 4)
      return "Minor Triad";
  }
  return "Chord";
}

juce::String
PianoRollComponent::detectChord(const std::vector<int> &pitches) const {
  if (pitches.empty())
    return "";
  // Simple pass-through to getCurrentChordName for now
  return "Chord";
}

//==============================================================================
// Scale Lock
//==============================================================================

bool PianoRollComponent::getScaleLock() const { return scaleLockEnabled; }

void PianoRollComponent::setScaleLock(bool enabled) {
  scaleLockEnabled = enabled;
  repaint();
}

void PianoRollComponent::updateScaleLockNotes() {
  scaleLockNotes.resize(12, false);
  // Mark all notes in the current scale
  for (int i = 0; i < 12; ++i) {
    scaleLockNotes[i] = isNoteInScale(i);
  }
}
