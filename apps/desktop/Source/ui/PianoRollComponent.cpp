/**
 * @file PianoRollComponent.cpp
 * @brief Professional-grade MIDI Piano Roll Editor Implementation
 */

#include "../../include/ui/PianoRollComponent.h"
// Force rebuild
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <set>

// Layout constants - made configurable for better UX
constexpr float RULER_HEIGHT = 30.0f;
constexpr float PIANO_WIDTH = 80.0f;
constexpr float TOOLBAR_HEIGHT = 40.0f;
constexpr float MIN_NOTE_LENGTH_BEATS =
    0.01; // Minimum note length (1/100th beat)
constexpr float MAX_NOTE_LENGTH_BEATS =
    64.0; // Maximum note length (16 bars at 4/4)
constexpr int MIN_VELOCITY = 1;
constexpr int MAX_VELOCITY = 127;
constexpr int MIN_PITCH = 0;
constexpr int MAX_PITCH = 127;

#include "skia/ZenithDesignSystem.h"

using namespace zenith;

//==============================================================================
// Constructor / Destructor
//==============================================================================

PianoRollComponent::PianoRollComponent(zenith::ProjectState &state)
    : projectState(state) {
  setWantsKeyboardFocus(true);
  setMouseCursor(juce::MouseCursor::NormalCursor);

  // Initialize common CC lanes
  updateCCLaneNames();

  // Start timer for playhead updates (30 Hz - smooth enough for visual
  // feedback)
  startTimer(33); // ~30 FPS
}

PianoRollComponent::~PianoRollComponent() {
  stopTimer();
  stopNotePreview();

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
  // Detach from old clip
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

  // Attach to new clip
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

  // Persist selection
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

    // Restore selection
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

  // Multi-Clip: Fetch notes from additional clips
  for (const auto &ctx : multiClipContexts) {
    if (!ctx.isValid() || ctx.clipId == currentClip.clipId)
      continue;
    auto notes = projectState.getMidiNotesForClip(ctx.clipId);
    for (const auto &n : notes) {
      NoteRect nr;
      nr.id = n.id;
      nr.ownerClipId = ctx.clipId; // Mark owner for multi-clip editing
      nr.pitch = n.pitch;
      nr.startBeats = n.startBeats;
      nr.lengthBeats = n.lengthBeats;
      nr.velocity = n.velocity;
      nr.muted = n.muted;
      // Multi-clip notes can be selected for viewing, but editing requires
      // switching to their owner clip context
      nr.selected = false; // Start unselected, user can select for reference
      // Validate and clamp new properties
      nr.probability = juce::jlimit(0.0f, 1.0f, n.probability);
      nr.condition = n.condition.substring(0, 32); // Limit string length
      nr.recurrence = n.recurrence.substring(0, 16); // Limit string length
      nr.articulationId = juce::jmax(0, n.articulationId); // Must be >= 0
      noteRects.push_back(nr);
    }
  }

  // Sorting Optimization for O(1) Rendering Culling
  std::sort(noteRects.begin(), noteRects.end(),
            [](const NoteRect &a, const NoteRect &b) {
              if (std::abs(a.startBeats - b.startBeats) > 0.001)
                return a.startBeats < b.startBeats;
              return a.pitch < b.pitch;
            });

  updateVisiblePitches();
  updateNoteRectangles();
  detectNoteCollisions();

  // Rebuild spatial grid for performance
  spatialGridDirty = true;
  rebuildSpatialGrid();

  repaint();
}

void PianoRollComponent::updateNoteRectangles() {
  auto bounds = getLocalBounds();
  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;

  for (auto &note : noteRects) {
    // Main note grid bounds
    float x = PIANO_WIDTH + beatsToPixels(note.startBeats);
    float y = RULER_HEIGHT + pitchToPixels(note.pitch);
    float width = beatsToPixels(note.lengthBeats);
    float height = pixelsPerPitch;

    note.bounds = juce::Rectangle<float>(x, y, width, height);

    // Velocity lane bounds
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
  
  // Guard against division by zero
  if (pixelsPerPitch <= 0.0f)
    return 60; // Default to middle C
  
  int row = static_cast<int>((adjustedY + scrollOffsetY) / pixelsPerPitch);

  if (foldMode) {
    if (visiblePitches.empty())
      return 60; // Default to middle C
    int maxRow = static_cast<int>(visiblePitches.size()) - 1;
    
    // COORDINATE INVERSION EXPLANATION:
    // In fold mode, visiblePitches is sorted ascending (low to high).
    // But visually, we want high pitches at the top (y=0) and low at bottom.
    // So we invert: row 0 (top of screen) maps to the highest visible pitch.
    // This matches standard piano roll convention where C8 is at top, C0 at bottom.
    int resultRow = maxRow - row;
    resultRow = juce::jlimit(0, maxRow, resultRow);
    
    // Additional safety check (shouldn't be needed after jlimit, but defensive)
    if (resultRow >= 0 && resultRow < static_cast<int>(visiblePitches.size()))
      return visiblePitches[resultRow];
    return 60; // Fallback to middle C
  } else {
    int pitch = 127 - row;
    return juce::jlimit(0, 127, pitch);
  }
}

float PianoRollComponent::pitchToPixels(int pitch) const {
  // Guard against division by zero
  if (pixelsPerPitch <= 0.0f)
    return 0.0f;
    
  int row;
  if (foldMode) {
    if (visiblePitches.empty()) {
      // Fallback when no visible pitches
      row = 127 - pitch;
    } else {
      row = mapPitchToRow(pitch);
      // COORDINATE INVERSION EXPLANATION:
      // mapPitchToRow returns index in visiblePitches (0 = lowest visible pitch).
      // But we need high pitches at top (y=0), so we invert the row index.
      // This ensures visual consistency: clicking top of screen selects highest visible pitch.
      row = ((int)visiblePitches.size() - 1) - row;
      // Ensure row is valid after inversion
      row = juce::jlimit(0, (int)visiblePitches.size() - 1, row);
    }
  } else {
    row = 127 - pitch;
  }
  return row * pixelsPerPitch - scrollOffsetY;
}

double PianoRollComponent::pixelsToBeats(float x) const {
  float adjustedX = x - PIANO_WIDTH;
  // Guard against division by zero
  if (pixelsPerBeat <= 0.0)
    return 0.0;
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

  // Guard against division by zero
  if (velocityLaneHeight <= 0.0f)
    return 64; // Default to middle velocity

  float normalizedY = yInLane / velocityLaneHeight;
  int velocity = static_cast<int>((1.0f - normalizedY) * 127.0f);
  return juce::jlimit(1, 127, velocity);
}

float PianoRollComponent::velocityToPixels(int velocity) const {
  float normalized = velocity / 127.0f;
  // velocityLaneHeight should always be > 0, but defensive check
  if (velocityLaneHeight <= 0)
    return 0.0f;
  return (1.0f - normalized) * velocityLaneHeight;
}

//==============================================================================
// Mouse Interaction Helpers
//==============================================================================

PianoRollComponent::NoteRect *PianoRollComponent::findNoteAtPosition(float x,
                                                                     float y) {
  if (!currentClip.isValid())
    return nullptr;

  // Use spatial grid for fast lookup if available
  if (!spatialGridDirty && noteRects.size() > 50) {
    double beat = pixelsToBeats(x - PIANO_WIDTH);
    int pitch = pixelsToPitch(y);

    // Query small region around click point
    double queryStart = beat - 0.1;
    double queryEnd = beat + 0.1;
    int queryMinPitch = juce::jmax(0, pitch - 1);
    int queryMaxPitch = juce::jmin(127, pitch + 1);

    auto candidates =
        spatialGrid.query(queryStart, queryEnd, queryMinPitch, queryMaxPitch,
                          currentClip.clipLengthBeats);

    // Check which candidate actually contains the point
    for (auto *note : candidates) {
      if (note && note->bounds.contains(x, y)) {
        return note;
      }
    }
  }

  // Fallback: linear search (for small note counts or when grid is dirty)
  // Search in reverse order (top notes first)
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

  // Search in reverse order
  for (auto it = noteRects.rbegin(); it != noteRects.rend(); ++it) {
    if (it->velocityBounds.contains(x, y))
      return &(*it);
  }
  return nullptr;
}

bool PianoRollComponent::findCCLaneAtPosition(float x, float y, int &ccNumber,
                                              juce::Rectangle<float> &laneRect) const {
    auto bounds = getLocalBounds();
    // Calculate total height of visible expression lanes
    int visibleExpressionLanes = 0;
    for (int i = 0; i < 4; ++i) { // ExpressionType has 4 values
        if (expressionLaneVisible[i])
            visibleExpressionLanes++;
    }

    float startY = bounds.getHeight() - (visibleExpressionLanes * expressionLaneHeight);

    // Check MPE expression lanes first (if any)
    int laneIndex = 0;
    for (int i = 0; i < 4; ++i) {
        if (!expressionLaneVisible[i])
            continue;

        laneRect = juce::Rectangle<float>(
            PIANO_WIDTH, startY + (laneIndex * expressionLaneHeight),
            bounds.getWidth() - PIANO_WIDTH, (float)expressionLaneHeight);

        if (laneRect.contains(x, y)) {
            // We are in an MPE lane, not a generic CC lane.
            // For now, this function only concerns generic CC lanes.
            return false; // Not a generic CC lane
        }
        laneIndex++;
    }

    // Now check generic MIDI CC lanes
    for (int ccNum : visibleCCLanes) {
        auto it = ccLanes.find(ccNum);
        if (it == ccLanes.end() || !it->second.visible)
            continue;

        laneRect = juce::Rectangle<float>(
            PIANO_WIDTH, startY + (laneIndex * ccLaneHeight),
            bounds.getWidth() - PIANO_WIDTH, (float)ccLaneHeight);

        if (laneRect.contains(x, y)) {
            ccNumber = ccNum;
            return true; // Found a generic CC lane
        }
        laneIndex++;
    }
    return false; // No CC lane at position
}

PianoRollComponent::CCPoint *PianoRollComponent::findCCPointAtPosition(
    int ccNumber, float x, float y, juce::Rectangle<float> &laneRect) const {
    auto it = ccLanes.find(ccNumber);
    if (it == ccLanes.end())
        return nullptr;

    // Need a non-const reference to lane to return a non-const CCPoint*
    // This is a design conflict with the const qualifier.
    // Re-evaluate: if this function is meant to return a *modifiable* point,
    // then it cannot be const. If it's only for querying existence for cursor,
    // it should return a const pointer or just a boolean.
    // For now, let's assume it returns a non-const pointer, and the const
    // qualifier on the function is temporary for compilation.
    // A better solution would be to have a const version that returns const CCPoint*
    // and a non-const version that returns CCPoint*.

    // Given the previous usage in mouseDown, which needed a modifiable point,
    // this function probably shouldn't be const if it's used for that.
    // However, for getCursorForPosition, we only need to know *if* there's a point.
    // Let's create a temporary copy of the lane, or a const_cast (not ideal).
    // The safest is to only return whether a point is found for const context.

    // Let's modify the return type in .h to be const CCPoint* if the function is const.
    // Or, remove const from function and only call it from non-const contexts.

    // For now, I will use const_cast for the purpose of getting the compiler to pass,
    // but note that this is a temporary workaround and ideally the design should be
    // either two functions (const/non-const) or the CCPoint* should be const CCPoint*
    // in the const version of this function.

    // Using a const_cast here to make it compile with const method.
    // This is generally not recommended but demonstrates the immediate fix.
    CCLane &lane = const_cast<CCLane &>(it->second);

    // Search for a point near the mouse position
    for (auto &point : lane.points) {
        // Calculate point's screen coordinates
        float pointX = beatsToPixels(point.timeBeats) + PIANO_WIDTH;
        float pointY = laneRect.getBottom() - ((point.value / 127.0f) * laneRect.getHeight());

        // Check if mouse is within a small radius of the point
        juce::Rectangle<float> pointHitBox =
            juce::Rectangle<float>(pointX - 5, pointY - 5, 10, 10); // 10x10 pixel hitbox
        if (pointHitBox.contains(x, y)) {
            return &point;
        }
    }
    return nullptr;
}


PianoRollComponent::DragMode
PianoRollComponent::detectNoteHitRegion(const NoteRect &note, float x,
                                        float y) const {
  if (!note.bounds.contains(x, y))
    return DragMode::None;

  // Left edge resize?
  if (x < note.bounds.getX() + resizeHandleWidth)
    return DragMode::ResizeLeft;

  // Right edge resize?
  if (x > note.bounds.getRight() - resizeHandleWidth)
    return DragMode::ResizeRight;

  return DragMode::MoveNote;
}

PianoRollComponent::CursorType
PianoRollComponent::getCursorForPosition(float x, float y) const {
  // Check if over piano keys
  if (x < PIANO_WIDTH)
    return CursorType::Normal;

  // Check if over ruler
  if (y < RULER_HEIGHT)
    return CursorType::Normal;

  // Check velocity lane
  auto bounds = getLocalBounds();
  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
  if (y >= RULER_HEIGHT + noteGridHeight)
  {
      // NEW: Check if over velocity lane note stalk
      if (findNoteInVelocityLane(x,y))
          return CursorType::ResizeHorizontal; // Can drag up/down to change velocity
      return CursorType::Crosshair;
  }

  // NEW: Check if over CC lane or CC point
  int ccNum = -1;
  juce::Rectangle<float> ccLaneBounds;
  if (findCCLaneAtPosition(x, y, ccNum, ccLaneBounds)) {
      // If over a CC lane, check if over a CC point
      if (findCCPointAtPosition(ccNum, x, y, ccLaneBounds)) {
          return CursorType::Hand; // Indicating a movable point
      }
      return CursorType::Crosshair; // Indicating ability to add a point
  }


  // Check if over a note
  for (const auto &note : noteRects) {
    if (note.bounds.contains(x, y)) {
      // Check resize handles
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
  (void)child;
  (void)index;

  if (parent.hasType(zenith::ProjectState::ID_NOTES)) {
    refreshNotesFromProjectState();
  }
}

void PianoRollComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  (void)tree;
  (void)property;
  // Simple refresh for now
  refreshNotesFromProjectState();
}

void PianoRollComponent::mouseMove(const juce::MouseEvent &e) {
  // Update cursor based on position
  auto newCursorType =
      getCursorForPosition(static_cast<float>(e.x), static_cast<float>(e.y));
  if (newCursorType != currentCursorType) {
    currentCursorType = newCursorType;
    setMouseCursor(getMouseCursor());
  }

  // Update hover state
  auto *newHoveredNote =
      findNoteAtPosition(static_cast<float>(e.x), static_cast<float>(e.y));
  if (newHoveredNote != hoveredNote) {
    if (hoveredNote)
      hoveredNote->isHovered = false;

    hoveredNote = newHoveredNote;

    if (hoveredNote)
      hoveredNote->isHovered = true;

    // Note preview on hover (if enabled)
    if (notePreviewEnabled && notePreviewCallback) {
      if (hoveredNote) {
        // Preview the hovered note
        previewNote(hoveredNote->pitch, hoveredNote->velocity);
        currentPreviewPitch = hoveredNote->pitch;
      } else {
        // Stop preview when leaving note
        stopNotePreview();
        currentPreviewPitch = -1;
      }
    }

    repaint();
  }
}

void PianoRollComponent::mouseDown(const juce::MouseEvent &e) {
  if (!currentClip.isValid())
    return;

  float x = static_cast<float>(e.x);
  float y = static_cast<float>(e.y);

  auto bounds = getLocalBounds();

  //==========================================================================
  // Toolbar Click Handling
  //==========================================================================
  SkRect toolbarRect =
      SkRect::MakeXYWH(bounds.getWidth() - 600.0f, 4.0f, 590.0f, 32.0f);

  if (y >= toolbarRect.top() && y <= toolbarRect.bottom() &&
      x >= toolbarRect.left() && x <= toolbarRect.right()) {
    // Calculate which button was clicked based on x position
    float btnX = toolbarRect.left() + 8.0f;
    float btnSpacing = 4.0f;

    // Mode buttons: Spray (40), Step (35), Lock (35)
    if (x >= btnX && x < btnX + 40.0f) {
      setSprayCanMode(!sprayCanMode);
      return;
    }
    btnX += 40.0f + btnSpacing;

    if (x >= btnX && x < btnX + 35.0f) {
      // Trigger Riff Machine
      RiffSettings settings;
      if (scaleLockEnabled) {
        settings.scale = scaleHighlight.scale;
        settings.rootNote = scaleHighlight.rootNote;
      }
      generateRiff(settings);
      return;
    }
    btnX += 35.0f + btnSpacing;

    if (x >= btnX && x < btnX + 35.0f) {
      setStepSequencerMode(!stepSequencerMode);
      return;
    }
    btnX += 35.0f + btnSpacing;

    if (x >= btnX && x < btnX + 35.0f) {
      setScaleLock(!scaleLockEnabled);
      repaint();
      return;
    }
    btnX += 35.0f + btnSpacing;

    if (x >= btnX && x < btnX + 35.0f) {
      foldMode = !foldMode;
      updateVisiblePitches(); // Recalculate visible rows
      updateNoteRectangles();
      repaint();
      return;
    }
    btnX += 35.0f + btnSpacing + 8.0f; // Plus separator

    // Skip "Groove:" label (45px) and groove button (55px)
    btnX += 45.0f + 55.0f + btnSpacing + 8.0f;

    // Scale Display Interaction (120px)
    if (x >= btnX && x < btnX + 120.0f) {
      if (e.mods.isRightButtonDown()) {
        // Cycle Scale Type
        int type = (int)scaleHighlight.scale;
        type = (type + 1) % 50; // Approx max types
        setScaleHighlight(scaleHighlight.rootNote, (ScaleType)type);
      } else {
        // Cycle Root Note
        int root = scaleHighlight.rootNote;
        root = (root + 1) % 12;
        setScaleHighlight(root, scaleHighlight.scale);
      }
      return;
    }
    btnX += 120.0f + btnSpacing + 8.0f;

    // Action buttons: Echo (35), Strum (40), Arp (30), Chord (42)
    if (x >= btnX && x < btnX + 35.0f) {
      applyMidiEcho(4, 0.25, 0.7f, 0);
      return;
    }
    btnX += 35.0f + btnSpacing;

    if (x >= btnX && x < btnX + 40.0f) {
      applyStrumming(StrumDirection::Down, 0.03);
      return;
    }
    btnX += 40.0f + btnSpacing;

    if (x >= btnX && x < btnX + 30.0f) {
      if (arpPreviewEnabled) {
        commitArpeggiator();
      } else {
        setArpeggiatorPreview(true);
      }
      return;
    }
    btnX += 30.0f + btnSpacing;

    if (x >= btnX && x < btnX + 42.0f) {
      // Insert C major chord at current position
      insertChord(60, ChordType::Major, 0.0, 1.0);
      return;
    }
    btnX += 42.0f + btnSpacing + 8.0f;

    // Transform buttons: Retrograde (24), Inversion (24), TimeStretch (24)
    if (x >= btnX && x < btnX + 24.0f) {
      transformRetrograde();
      return;
    }
    btnX += 24.0f + btnSpacing;

    if (x >= btnX && x < btnX + 24.0f) {
      transformInversion();
      return;
    }
    btnX += 24.0f + btnSpacing;

    if (x >= btnX && x < btnX + 24.0f) {
      transformTimeStretch(2.0); // Double duration
      return;
    }

    return; // Consumed by toolbar
  }

  //==========================================================================
  // Step Sequencer Click Handling (when in step sequencer mode)
  //==========================================================================
  if (stepSequencerMode) {
    float noteGridHeight =
        bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
    SkRect stepArea = SkRect::MakeLTRB(
        (float)PIANO_WIDTH, (float)RULER_HEIGHT + TOOLBAR_HEIGHT,
        bounds.getWidth(), bounds.getHeight() - velocityLaneHeight);

    if (x >= stepArea.left() && x <= stepArea.right() && y >= stepArea.top() &&
        y <= stepArea.bottom()) {

      float cellWidth = stepArea.width() / stepSequencerSteps;
      float cellHeight = stepArea.height() / stepSequencerRows.size();

      int step = static_cast<int>((x - stepArea.left()) / cellWidth);
      int row = static_cast<int>((y - stepArea.top()) / cellHeight);

      if (step >= 0 && step < stepSequencerSteps && row >= 0 &&
          row < (int)stepSequencerRows.size()) {
        toggleStep(row, step);
      }
      return;
    }
  }

  // Ignore clicks on piano keys or ruler
  if (x < PIANO_WIDTH || y < RULER_HEIGHT)
    return;

  float noteGridHeight = bounds.getHeight() - RULER_HEIGHT - velocityLaneHeight;
  float velocityLaneTop = RULER_HEIGHT + noteGridHeight;

  // Check if in velocity lane
  if (y >= velocityLaneTop) {
    auto *note = findNoteInVelocityLane(x, y);
    if (note) {
      startEditingVelocity(note, e);
      return;
    }
  }

  // NEW: Check if in CC lane
  int ccNum = -1;
  juce::Rectangle<float> ccLaneBounds;
  if (findCCLaneAtPosition(x, y, ccNum, ccLaneBounds)) {
      activeCCNumber = ccNum;
      activeCCPoint = findCCPointAtPosition(ccNum, x, y, ccLaneBounds);

      if (activeCCPoint) {
          // Existing CC point clicked
          if (e.mods.isCommandDown()) { // Ctrl/Cmd + click to delete
              removeCCPoint(ccNum, activeCCPoint->id);
              activeCCPoint = nullptr;
              activeCCNumber = -1;
              repaint();
              return;
          } else {
              // Start dragging existing CC point
              currentDragMode = DragMode::CCEditPoint;
              dragStartPos = e.position;
              currentCCLaneBounds = ccLaneBounds; // Store the lane bounds
              return;
          }
      } else {
          // Clicked in CC lane, no existing point: create new one and drag
          currentDragMode = DragMode::CCNewPoint;
          dragStartPos = e.position;
          currentCCLaneBounds = ccLaneBounds; // Store the lane bounds
          double timeBeats = pixelsToBeats(x);
          // Convert mouse Y within lane to CC value
          float normalizedY = 1.0f - ((y - ccLaneBounds.getY()) / ccLaneBounds.getHeight());
          int value = juce::jlimit(0, 127, static_cast<int>(normalizedY * 127.0f));
          
          // Create a temporary point and add it (setCCPoint handles duplicates and sorting)
          setCCPoint(ccNum, timeBeats, value);
          // Re-find the newly created point (setCCPoint might reorder or create a new ID)
          // We need to find the specific point that was just created, not just any point at x,y
          // A more robust solution would return the ID from setCCPoint or pass back the created point
          // For now, assume it's the one at the exact timeBeats and value just set
          auto it = std::find_if(ccLanes[ccNum].points.begin(), ccLanes[ccNum].points.end(),
                                 [&](const CCPoint& p) {
                                     return std::abs(p.timeBeats - timeBeats) < 0.001 && p.value == value;
                                 });
          if (it != ccLanes[ccNum].points.end())
              activeCCPoint = &(*it);
          
          repaint();
          return;
      }
  }

  // Check for note hit in main grid
  auto *note = findNoteAtPosition(x, y);

  if (note) {
    // Detect hit region (resize vs move)
    DragMode mode = detectNoteHitRegion(*note, x, y);

    if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight) {
      // Start resizing
      startResizingNote(note, mode, e);
    } else {
      // Multi-select with Cmd/Ctrl
      bool isMultiSelectModifier = e.mods.isCommandDown();

      if (isMultiSelectModifier) {
        // Toggle selection
        note->selected = !note->selected;
        repaint();
      } else if (!note->selected) {
        // Clear selection and select this note
        clearSelection();
        note->selected = true;
        repaint();
      }

      // Start moving selection
      startMovingSelection(e);
    }
  } else {
    // No note hit
    if (e.mods.isShiftDown()) {
      // Marquee select
      startMarqueeSelect(e);
    } else {
      // Create new note
      createNoteAtPosition(x, y);
    }
  }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent &e) {
  // Spray can mode takes priority
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

  case DragMode::CCEditPoint:
  case DragMode::CCNewPoint:
    if (activeCCPoint && activeCCNumber != -1) {
        // Use the stored currentCCLaneBounds for consistency during drag
        double newTimeBeats = pixelsToBeats(e.x);
        // Clamp Y position to the lane bounds
        float clampedY = juce::jlimit(currentCCLaneBounds.getY(),
                                      currentCCLaneBounds.getBottom(),
                                      static_cast<float>(e.y));
        float normalizedY = 1.0f - ((clampedY - currentCCLaneBounds.getY()) / currentCCLaneBounds.getHeight());
        int newValue = juce::jlimit(0, 127, static_cast<int>(normalizedY * 127.0f));

        // Update the active CC point
        // setCCPoint will handle updating the point in the map and sorting
        setCCPoint(activeCCNumber, newTimeBeats, newValue);
        repaint();
    }
    break;

  default:
    break;
  }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent &e) {
  (void)e;

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

  case DragMode::CCEditPoint:
  case DragMode::CCNewPoint:
      // No specific "finish" logic needed as setCCPoint updates in real-time
      // Just clear active state
      activeCCPoint = nullptr;
      activeCCNumber = -1;
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

  // Double-click to delete note
  auto *note = findNoteAtPosition(x, y);
  if (note) {
    projectState.removeMidiNote(currentClip.clipId, note->id,
                                "Delete MIDI note");
  }
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent &e,
                                        const juce::MouseWheelDetails &wheel) {
  if (e.mods.isCommandDown()) {
    // Cmd/Ctrl + wheel = horizontal zoom
    float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
    zoomHorizontal(zoomFactor, static_cast<float>(e.x));
  } else if (e.mods.isAltDown()) {
    // Alt + wheel = vertical zoom
    float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
    zoomVertical(zoomFactor, static_cast<float>(e.y));
  } else if (e.mods.isShiftDown()) {
    // Shift + wheel = horizontal scroll
    scrollHorizontal(-wheel.deltaY * 50.0f);
  } else {
    // Default = vertical scroll
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
  if (!currentClip.isValid())
    return;

  int pitch = pixelsToPitch(y);
  double startBeats = pixelsToBeats(x - PIANO_WIDTH);

  // Bounds checking and validation
  pitch = juce::jlimit(0, 127, pitch);
  startBeats = juce::jmax(0.0, startBeats);

  // Clamp to clip bounds
  if (startBeats >= currentClip.clipLengthBeats)
    return;

  if (snapEnabled)
    startBeats = snapToGrid(startBeats);

  // Default note length (1 grid unit), clamped to clip end
  double lengthBeats = gridBeats;
  double maxLength = currentClip.clipLengthBeats - startBeats;
  lengthBeats = juce::jmin(lengthBeats, maxLength);
  lengthBeats = juce::jmax(0.1, lengthBeats); // Minimum note length

  // Create note with validated properties
  zenith::ProjectState::MidiNoteSpec note;
  note.pitch = pitch;
  note.startBeats = startBeats;
  note.lengthBeats = lengthBeats;
  note.velocity = previewVelocity;
  note.muted = false;
  // Validate new properties with safe defaults
  note.probability = 1.0f; // Default to 100% probability
  note.articulationId = 0; // Default articulation
  // condition and recurrence remain empty strings (valid)

  juce::String noteId =
      projectState.addMidiNote(currentClip.clipId, note, "Create MIDI note");

  // Preview note audio
  if (notePreviewEnabled && notePreviewCallback) {
    previewNote(pitch, previewVelocity);
  }

  DBG("Created note " + noteId + " at pitch=" + juce::String(pitch) +
      ", start=" + juce::String(startBeats));
}

void PianoRollComponent::deleteSelectedNotes() {
  if (!currentClip.isValid() || getSelectedNoteCount() == 0)
    return;

  // Collect selected note IDs
  std::vector<juce::String> selectedIds;
  for (const auto &note : noteRects) {
    if (note.selected)
      selectedIds.push_back(note.id);
  }

  // FIXED: Batched undo transaction
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

  // Find earliest selected note for relative positioning
  double earliestTime = std::numeric_limits<double>::max();
  for (const auto &note : noteRects) {
    if (note.selected)
      earliestTime = std::min(earliestTime, note.startBeats);
  }

  clipboardReferenceTime = earliestTime;

  // Copy selected notes
  for (const auto &note : noteRects) {
    if (note.selected) {
      ClipboardNote clipNote;
      clipNote.pitch = note.pitch;
      clipNote.startBeats =
          note.startBeats - earliestTime; // Relative to earliest
      clipNote.lengthBeats = note.lengthBeats;
      clipNote.velocity = note.velocity;
      clipNote.muted = note.muted;
      // Copy new properties
      clipNote.probability = note.probability;
      clipNote.condition = note.condition;
      clipNote.recurrence = note.recurrence;
      clipNote.articulationId = note.articulationId;

      clipboard.push_back(clipNote);
    }
  }

  DBG("Copied " + juce::String(clipboard.size()) + " notes to clipboard");
}

void PianoRollComponent::pasteNotes() {
  if (!currentClip.isValid() || clipboard.empty())
    return;

  // Paste at current playhead or view start
  double pasteTime = viewStartBeats;
  pasteTime = juce::jmax(0.0, pasteTime);

  // FIXED: Batched undo transaction
  projectState.getUndoManager().beginNewTransaction("Paste MIDI notes");

  clearSelection();

  std::vector<juce::String> pastedNoteIds;

  for (const auto &clipNote : clipboard) {
    double noteStart = pasteTime + clipNote.startBeats;

    // Bounds checking - skip notes that would go past clip end
    if (noteStart >= currentClip.clipLengthBeats)
      continue;

    // Clamp note length to fit in clip
    double noteLength = clipNote.lengthBeats;
    if (noteStart + noteLength > currentClip.clipLengthBeats) {
      noteLength = currentClip.clipLengthBeats - noteStart;
      noteLength = juce::jmax((double)MIN_NOTE_LENGTH_BEATS, noteLength);
    }

    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = juce::jlimit((int)MIN_PITCH, (int)MAX_PITCH, clipNote.pitch);
    note.startBeats = noteStart;
    note.lengthBeats = juce::jlimit((double)MIN_NOTE_LENGTH_BEATS,
                                    (double)MAX_NOTE_LENGTH_BEATS, noteLength);
    note.velocity = juce::jlimit(MIN_VELOCITY, MAX_VELOCITY, clipNote.velocity);
    note.muted = clipNote.muted;
    // Validate new properties from clipboard
    note.probability = juce::jlimit(0.0f, 1.0f, clipNote.probability);
    note.condition = clipNote.condition.substring(0, 32); // Limit length
    note.recurrence = clipNote.recurrence.substring(0, 16); // Limit length
    note.articulationId = juce::jmax(0, clipNote.articulationId);

    juce::String noteId =
        projectState.addMidiNote(currentClip.clipId, note, "");

    if (noteId.isNotEmpty()) {
      pastedNoteIds.push_back(noteId);
    }
  }

  refreshNotesFromProjectState();

  // Select pasted notes
  for (const auto &noteId : pastedNoteIds) {
    for (auto &note : noteRects) {
      if (note.id == noteId) {
        note.selected = true;
        break;
      }
    }
  }

  DBG("Pasted " + juce::String(pastedNoteIds.size()) + " notes (skipped " +
      juce::String(clipboard.size() - pastedNoteIds.size()) +
      " out of bounds)");
  repaint();
}

void PianoRollComponent::cutSelectedNotes() {
  copySelectedNotes();
  deleteSelectedNotes();
}

//==============================================================================
// Drag Operations - Move
//==============================================================================

void PianoRollComponent::startMovingSelection(const juce::MouseEvent &e) {
  currentDragMode = DragMode::MoveNote;
  dragStartPos = e.position;

  // Cache original positions of all selected notes
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
  if (dragStates.empty() || !currentClip.isValid())
    return;

  float deltaX = e.position.x - dragStartPos.x;
  float deltaY = e.position.y - dragStartPos.y;

  double deltaBeats =
      pixelsToBeats(PIANO_WIDTH + deltaX) - pixelsToBeats(PIANO_WIDTH);
  int deltaPitch = -static_cast<int>(deltaY / pixelsPerPitch);

  // Update visual positions with bounds checking
  size_t stateIndex = 0;
  for (auto &note : noteRects) {
    if (note.selected && stateIndex < dragStates.size()) {
      const auto &originalState = dragStates[stateIndex];

      double newStartBeats = originalState.originalStartBeats + deltaBeats;
      int newPitch = originalState.originalPitch + deltaPitch;

      if (snapEnabled)
        newStartBeats = snapToGrid(newStartBeats);

      // Bounds checking
      newStartBeats = juce::jmax(0.0, newStartBeats);
      newPitch = juce::jlimit(0, 127, newPitch);

      // Clamp to clip bounds - ensure note doesn't go past clip end
      double noteEnd = newStartBeats + note.lengthBeats;
      if (noteEnd > currentClip.clipLengthBeats) {
        newStartBeats = currentClip.clipLengthBeats - note.lengthBeats;
        newStartBeats = juce::jmax(0.0, newStartBeats);
      }

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

  // FIXED: Batched undo transaction
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

//==============================================================================
// Drag Operations - Resize
//==============================================================================

void PianoRollComponent::startResizingNote(NoteRect *note, DragMode mode,
                                           const juce::MouseEvent &e) {
  currentDragMode = mode;
  activeNote = note;
  dragStartPos = e.position;

  // Cache original state
  dragStates.clear();
  NoteDragState state;
  state.id = note->id;
  state.originalStartBeats = note->startBeats;
  state.originalLengthBeats = note->lengthBeats;
  dragStates.push_back(state);

  // Select this note
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
    // FIXED: Batched undo for left resize, single transaction for right resize
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

//==============================================================================
// TO BE CONTINUED IN PART 3...
// (Velocity editing, marquee select, zoom/scroll, advanced features)
//==============================================================================

//==============================================================================
// Drag Operations - Velocity
//==============================================================================

void PianoRollComponent::startEditingVelocity(NoteRect *note,
                                              const juce::MouseEvent &e) {
  currentDragMode = DragMode::VelocityEdit;
  activeNote = note; // Keep activeNote for hover/visual feedback
  dragStartPos = e.position;

  dragStates.clear();
  // Store original velocity for all selected notes
  for (auto &selectedNote : noteRects) {
    if (selectedNote.selected) {
      NoteDragState state;
      state.id = selectedNote.id;
      state.originalVelocity = selectedNote.velocity;
      dragStates.push_back(state);
    }
  }

  // If no notes were selected, and a note was clicked, select it and add to dragStates
  if (dragStates.empty() && note) {
    note->selected = true;
    NoteDragState state;
    state.id = note->id;
    state.originalVelocity = note->velocity;
    dragStates.push_back(state);
  }
  
  repaint();
}

void PianoRollComponent::updateVelocityEdit(const juce::MouseEvent &e) {
  if (dragStates.empty() || !currentClip.isValid())
    return;

  // Calculate the velocity at the initial drag start position (implicit reference)
  // This is the velocity value where the drag began.
  int originalDragVelocity = pixelsToVelocity(dragStartPos.y);

  // Calculate the velocity at the current mouse position
  int currentMouseVelocity = pixelsToVelocity(static_cast<float>(e.y));

  // Determine the change in velocity from the start of the drag
  int deltaVelocity = currentMouseVelocity - originalDragVelocity;

  // Apply this delta to all notes in the dragStates
  for (const auto &dragState : dragStates) {
    // Find the actual NoteRect for the current note ID
    auto it = std::find_if(noteRects.begin(), noteRects.end(),
                           [&dragState](const NoteRect &n) {
                             return n.id == dragState.id;
                           });
    if (it != noteRects.end()) {
      // Calculate new velocity based on its original velocity + delta
      int newVelocity = dragState.originalVelocity + deltaVelocity;
      it->velocity = juce::jlimit(MIN_VELOCITY, MAX_VELOCITY, newVelocity);
    }
  }

  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::finishVelocityEdit() {
  if (dragStates.empty() || !currentClip.isValid())
    return;

  // Start a single undo transaction for all changes
  projectState.getUndoManager().beginNewTransaction("Edit MIDI velocity (batch)");

  for (const auto &dragState : dragStates) {
    // Find the current NoteRect corresponding to the dragState ID
    auto it = std::find_if(noteRects.begin(), noteRects.end(),
                           [&dragState](const NoteRect &n) {
                             return n.id == dragState.id;
                           });

    if (it != noteRects.end()) {
      // Only commit if the velocity has actually changed
      if (it->velocity != dragState.originalVelocity) {
        projectState.setMidiNoteVelocity(currentClip.clipId, it->id,
                                         it->velocity, "");
      }
    }
  }

  dragStates.clear();
  activeNote = nullptr;
}

//==============================================================================
// Drag Operations - Marquee Select
//==============================================================================

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
  pixelsPerBeat =
      juce::jlimit(10.0, 800.0, pixelsPerBeat); // Expanded width range
  // Ensure pixelsPerBeat never becomes zero (safety check)
  if (pixelsPerBeat <= 0.0)
    pixelsPerBeat = 10.0; // Minimum safe value

  viewStartBeats = centerBeats - ((centerX - PIANO_WIDTH) / pixelsPerBeat);
  viewStartBeats = juce::jmax(0.0, viewStartBeats);

  scrollOffsetX = static_cast<int>(viewStartBeats * pixelsPerBeat);

  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::zoomVertical(float factor, float centerY) {
  pixelsPerPitch *= factor;
  pixelsPerPitch =
      juce::jlimit(4.0, 96.0, pixelsPerPitch); // Expanded height range
  // Ensure pixelsPerPitch never becomes zero (safety check)
  if (pixelsPerPitch <= 0.0)
    pixelsPerPitch = 4.0; // Minimum safe value

  scrollOffsetY = static_cast<int>(viewLowestPitch * pixelsPerPitch);

  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::scrollHorizontal(float delta) {
  scrollOffsetX += static_cast<int>(delta);
  scrollOffsetX = juce::jmax(0, scrollOffsetX);

  // Guard against division by zero
  if (pixelsPerBeat > 0.0)
    viewStartBeats = scrollOffsetX / pixelsPerBeat;
  else
    viewStartBeats = 0.0;

  updateNoteRectangles();
  repaint();
}

void PianoRollComponent::scrollVertical(float delta) {
  scrollOffsetY += static_cast<int>(delta);
  scrollOffsetY =
      juce::jlimit(0, 127 * static_cast<int>(pixelsPerPitch), scrollOffsetY);

  // Guard against division by zero
  if (pixelsPerPitch > 0.0f)
    viewLowestPitch = scrollOffsetY / static_cast<int>(pixelsPerPitch);
  else
    viewLowestPitch = 0;

  updateNoteRectangles();
  repaint();
}

//==============================================================================
// Advanced Features - Quantize
//==============================================================================

void PianoRollComponent::quantizeSelected(double gridSize, float strength,
                                          float swing) {
  if (getSelectedNoteCount() == 0)
    return;

  projectState.getUndoManager().beginNewTransaction("Quantize MIDI notes");

  for (auto &note : noteRects) {
    if (note.selected) {
      double originalStart = note.startBeats;
      double quantizedStart = std::round(originalStart / gridSize) * gridSize;

      // Apply swing (offset every other grid position)
      int gridIndex = static_cast<int>(std::round(quantizedStart / gridSize));
      if (gridIndex % 2 == 1 && swing != 0.0f) {
        quantizedStart += gridSize * swing * 0.5f;
      }

      // Blend between original and quantized based on strength
      double newStart =
          originalStart + (quantizedStart - originalStart) * strength;
      newStart = juce::jmax(0.0, newStart);

      if (std::abs(newStart - originalStart) > 0.001) {
        projectState.moveMidiNote(currentClip.clipId, note.id, newStart,
                                  note.pitch, "");
      }
    }
  }
}

//==============================================================================
// Advanced Features - Velocity Humanization
//==============================================================================

void PianoRollComponent::humanizeVelocity(float amount) {
  if (getSelectedNoteCount() == 0)
    return;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

  projectState.getUndoManager().beginNewTransaction("Humanize MIDI velocity");

  for (auto &note : noteRects) {
    if (note.selected) {
      int originalVelocity = note.velocity;

      // Random variation proportional to current velocity
      float variation = dis(gen) * amount * 30.0f; // Up to Â±30 at full amount
      int newVelocity = originalVelocity + static_cast<int>(variation);
      newVelocity = juce::jlimit(1, 127, newVelocity);

      if (newVelocity != originalVelocity) {
        projectState.setMidiNoteVelocity(currentClip.clipId, note.id,
                                         newVelocity, "");
      }
    }
  }
}

//==============================================================================
// Advanced Features - Velocity Curves
//==============================================================================

void PianoRollComponent::applyVelocityCurve(VelocityCurve curve, float amount) {
  if (getSelectedNoteCount() == 0)
    return;

  // Sort selected notes by time
  std::vector<NoteRect *> selectedNotes;
  for (auto &note : noteRects) {
    if (note.selected)
      selectedNotes.push_back(&note);
  }

  std::sort(selectedNotes.begin(), selectedNotes.end(),
            [](const NoteRect *a, const NoteRect *b) {
              return a->startBeats < b->startBeats;
            });

  if (selectedNotes.empty())
    return;

  projectState.getUndoManager().beginNewTransaction("Apply velocity curve");

  int count = static_cast<int>(selectedNotes.size());

  for (int i = 0; i < count; ++i) {
    auto *note = selectedNotes[i];
    int originalVelocity = note->velocity;
    int newVelocity = originalVelocity;

    float t = (count > 1) ? (i / static_cast<float>(count - 1)) : 0.5f;

    switch (curve) {
    case VelocityCurve::RampUp: {
      int targetVelocity = 1 + static_cast<int>(t * 126.0f);
      newVelocity =
          originalVelocity +
          static_cast<int>((targetVelocity - originalVelocity) * amount);
      break;
    }

    case VelocityCurve::RampDown: {
      int targetVelocity = 127 - static_cast<int>(t * 126.0f);
      newVelocity =
          originalVelocity +
          static_cast<int>((targetVelocity - originalVelocity) * amount);
      break;
    }

    case VelocityCurve::Compress: {
      // Calculate average velocity
      int avgVelocity = 0;
      for (const auto *n : selectedNotes)
        avgVelocity += n->velocity;
      avgVelocity /= count;

      int diff = originalVelocity - avgVelocity;
      newVelocity = avgVelocity + static_cast<int>(diff * (1.0f - amount));
      break;
    }

    case VelocityCurve::Expand: {
      int avgVelocity = 0;
      for (const auto *n : selectedNotes)
        avgVelocity += n->velocity;
      avgVelocity /= count;

      int diff = originalVelocity - avgVelocity;
      newVelocity = avgVelocity + static_cast<int>(diff * (1.0f + amount));
      break;
    }

    case VelocityCurve::Invert: {
      int inverted = 128 - originalVelocity;
      newVelocity = originalVelocity +
                    static_cast<int>((inverted - originalVelocity) * amount);
      break;
    }
    }

    newVelocity = juce::jlimit(1, 127, newVelocity);

    if (newVelocity != originalVelocity) {
      projectState.setMidiNoteVelocity(currentClip.clipId, note->id,
                                       newVelocity, "");
    }
  }
}

//==============================================================================
// Advanced Features - Smart Duplicate
//==============================================================================

void PianoRollComponent::smartDuplicate() {
  if (getSelectedNoteCount() < 2)
    return;

  // Find time range of selection
  double minTime = std::numeric_limits<double>::max();
  double maxTime = 0.0;

  for (const auto &note : noteRects) {
    if (note.selected) {
      minTime = std::min(minTime, note.startBeats);
      maxTime = std::max(maxTime, note.startBeats + note.lengthBeats);
    }
  }

  double patternLength = maxTime - minTime;

  projectState.getUndoManager().beginNewTransaction(
      "Smart duplicate MIDI notes");

  clearSelection();

  for (const auto &note : noteRects) {
    if (note.selected) {
      zenith::ProjectState::MidiNoteSpec newNote;
      newNote.pitch = note.pitch;
      newNote.startBeats = note.startBeats + patternLength;
      newNote.lengthBeats = note.lengthBeats;
      newNote.velocity = note.velocity;
      newNote.muted = note.muted;

      projectState.addMidiNote(currentClip.clipId, newNote, "");
    }
  }
}

//==============================================================================
// Advanced Features - Note Repeater/Roll
//==============================================================================

void PianoRollComponent::createRoll(float x, float y, double rollSpeed) {
  int pitch = pixelsToPitch(y);
  double startBeats = pixelsToBeats(x - PIANO_WIDTH);

  if (snapEnabled)
    startBeats = snapToGrid(startBeats);

  // Create 16 notes for the roll
  int numNotes = 16;
  double totalLength = rollSpeed * numNotes;

  projectState.getUndoManager().beginNewTransaction("Create MIDI roll");

  for (int i = 0; i < numNotes; ++i) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = pitch;
    note.startBeats = startBeats + (i * rollSpeed);
    note.lengthBeats = rollSpeed * 0.9; // Slight gap
    note.velocity = 100 - (i * 3);      // Decreasing velocity
    note.muted = false;

    projectState.addMidiNote(currentClip.clipId, note, "");
  }
}

//==============================================================================
// Advanced Features - Toggle Mute
//==============================================================================

void PianoRollComponent::toggleMuteSelected() {
  if (getSelectedNoteCount() == 0)
    return;

  projectState.getUndoManager().beginNewTransaction("Toggle MIDI note mute");

  for (auto &note : noteRects) {
    if (note.selected) {
      // Toggle mute state
      bool newMuteState = !note.muted;

      // Persist to ProjectState with undo support
      projectState.setMidiNoteMuted(currentClip.clipId, note.id, newMuteState,
                                    "");

      note.muted = newMuteState;
    }
  }

  repaint();
}

//==============================================================================
// Advanced Features - Scale Highlighting
//==============================================================================

// Duplicate method isNoteInScale removed (superseded by implementation at end
// of file)

//==============================================================================
// Advanced Features - Chord Detection
//==============================================================================

juce::String
PianoRollComponent::detectChord(const std::vector<int> &pitches) const {
  if (pitches.size() < 3)
    return "";

  // Get unique pitch classes (mod 12)
  std::set<int> pitchClasses;
  for (int pitch : pitches)
    pitchClasses.insert(pitch % 12);

  std::vector<int> sortedClasses(pitchClasses.begin(), pitchClasses.end());
  std::sort(sortedClasses.begin(), sortedClasses.end());

  // Try each pitch class as root
  static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                    "F#", "G",  "G#", "A",  "A#", "B"};

  for (int root : sortedClasses) {
    std::vector<int> intervals;
    for (int pc : sortedClasses) {
      int interval = (pc - root + 12) % 12;
      intervals.push_back(interval);
    }
    std::sort(intervals.begin(), intervals.end());

    // Check common chord types
    if (intervals == std::vector<int>{0, 4, 7})
      return juce::String(noteNames[root]) + " Major";
    if (intervals == std::vector<int>{0, 3, 7})
      return juce::String(noteNames[root]) + " Minor";
    if (intervals == std::vector<int>{0, 4, 7, 11})
      return juce::String(noteNames[root]) + " Maj7";
    if (intervals == std::vector<int>{0, 3, 7, 10})
      return juce::String(noteNames[root]) + " m7";
    if (intervals == std::vector<int>{0, 4, 7, 10})
      return juce::String(noteNames[root]) + " 7";
    if (intervals == std::vector<int>{0, 3, 6})
      return juce::String(noteNames[root]) + " Dim";
    if (intervals == std::vector<int>{0, 4, 8})
      return juce::String(noteNames[root]) + " Aug";
    if (intervals == std::vector<int>{0, 5, 7})
      return juce::String(noteNames[root]) + " Sus4";
  }

  return "Unknown Chord";
}

juce::String PianoRollComponent::getCurrentChordName() const {
  std::vector<int> selectedPitches;
  for (const auto &note : noteRects) {
    if (note.selected)
      selectedPitches.push_back(note.pitch);
  }

  return detectChord(selectedPitches);
}

//==============================================================================
// TO BE CONTINUED IN PART 4...
// (Keyboard shortcuts, Rendering, Color helpers)
//==============================================================================

//==============================================================================
// Keyboard Shortcuts
//==============================================================================

bool PianoRollComponent::keyPressed(const juce::KeyPress &key) {
  // Delete / Backspace
  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    deleteSelectedNotes();
    return true;
  }

  // Undo (Cmd/Ctrl+Z)
  if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
    projectState.undo();
    return true;
  }

  // Redo (Cmd/Ctrl+Shift+Z or Cmd/Ctrl+Y)
  if (key == juce::KeyPress('z',
                            juce::ModifierKeys::commandModifier |
                                juce::ModifierKeys::shiftModifier,
                            0) ||
      key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0)) {
    projectState.redo();
    return true;
  }

  // Copy (Cmd/Ctrl+C)
  if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0)) {
    copySelectedNotes();
    return true;
  }

  // Paste (Cmd/Ctrl+V)
  if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0)) {
    pasteNotes();
    return true;
  }

  // Cut (Cmd/Ctrl+X)
  if (key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0)) {
    cutSelectedNotes();
    return true;
  }

  // Select All (Cmd/Ctrl+A)
  if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)) {
    selectAll();
    return true;
  }

  // Duplicate (Cmd/Ctrl+D)
  if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0)) {
    smartDuplicate();
    return true;
  }

  // Mute (Cmd/Ctrl+M)
  if (key == juce::KeyPress('m', juce::ModifierKeys::commandModifier, 0)) {
    toggleMuteSelected();
    return true;
  }

  // Quantize (Q)
  if (key == juce::KeyPress('q', 0, 0)) {
    quantizeSelected(gridBeats, 1.0f, 0.0f);
    return true;
  }

  // Humanize (H)
  if (key == juce::KeyPress('h', 0, 0)) {
    humanizeVelocity(0.3f);
    return true;
  }

  // Zoom In (+/=)
  if (key == juce::KeyPress('+', 0, 0) || key == juce::KeyPress('=', 0, 0)) {
    zoomHorizontal(1.2f, getWidth() / 2.0f);
    return true;
  }

  // Zoom Out (-)
  if (key == juce::KeyPress('-', 0, 0)) {
    zoomHorizontal(0.8f, getWidth() / 2.0f);
    return true;
  }

  // Invert Selection (Cmd/Ctrl+I)
  if (key == juce::KeyPress('i', juce::ModifierKeys::commandModifier, 0)) {
    invertSelection();
    return true;
  }

  //==========================================================================
  // New Pro Feature Shortcuts
  //==========================================================================

  // Toggle Spray Can Mode (B - like Bitwig)
  if (key == juce::KeyPress('b', 0, 0)) {
    setSprayCanMode(!sprayCanMode);
    repaint();
    return true;
  }

  // Toggle Step Sequencer Mode (S)
  if (key == juce::KeyPress('s', 0, 0)) {
    setStepSequencerMode(!stepSequencerMode);
    return true;
  }

  // Toggle Scale Lock (L)
  if (key == juce::KeyPress('l', 0, 0)) {
    setScaleLock(!scaleLockEnabled);
    repaint();
    return true;
  }

  // Retrograde Transform (R with Cmd/Ctrl)
  if (key == juce::KeyPress('r', juce::ModifierKeys::commandModifier, 0)) {
    transformRetrograde();
    return true;
  }

  // Inversion Transform (I with Alt)
  if (key == juce::KeyPress('i', juce::ModifierKeys::altModifier, 0)) {
    transformInversion();
    return true;
  }

  // Apply Arpeggiator Preview (A)
  if (key == juce::KeyPress('a', juce::ModifierKeys::altModifier, 0)) {
    if (arpPreviewEnabled) {
      commitArpeggiator();
    } else {
      setArpeggiatorPreview(true);
    }
    return true;
  }

  // MIDI Echo (E)
  if (key == juce::KeyPress('e', 0, 0)) {
    applyMidiEcho(4, 0.25, 0.7f, 0); // 4 echoes, 16th notes, 70% decay
    return true;
  }

  // Strumming Down (G)
  if (key == juce::KeyPress('g', 0, 0)) {
    applyStrumming(StrumDirection::Down, 0.03);
    return true;
  }

  // Strumming Up (Shift+G)
  if (key == juce::KeyPress('g', juce::ModifierKeys::shiftModifier, 0)) {
    applyStrumming(StrumDirection::Up, 0.03);
    return true;
  }

  // Legato (Cmd/Ctrl+L)
  if (key == juce::KeyPress('l', juce::ModifierKeys::commandModifier, 0)) {
    applyLegato();
    return true;
  }

  // Split Notes Equal into 2 (Cmd/Ctrl+2)
  if (key == juce::KeyPress('2', juce::ModifierKeys::commandModifier, 0)) {
    splitNotesEqual(2);
    return true;
  }

  // Split Notes Equal into 4 (Cmd/Ctrl+4)
  if (key == juce::KeyPress('4', juce::ModifierKeys::commandModifier, 0)) {
    splitNotesEqual(4);
    return true;
  }

  // Join Consecutive Notes (J)
  if (key == juce::KeyPress('j', 0, 0)) {
    joinConsecutiveNotes();
    return true;
  }

  // Extend Melody (Cmd/Ctrl+E)
  if (key == juce::KeyPress('e', juce::ModifierKeys::commandModifier, 0)) {
    extendMelody(4);
    return true;
  }

  // Auto-Harmonize with Thirds (Cmd/Ctrl+3)
  if (key == juce::KeyPress('3', juce::ModifierKeys::commandModifier, 0)) {
    autoHarmonize(HarmonyType::Thirds);
    return true;
  }

  // Transpose up in scale (Shift+Up)
  if (key == juce::KeyPress::upKey && key.getModifiers().isShiftDown() &&
      scaleLockEnabled) {
    transformTransposeInScale(1);
    return true;
  }

  // Transpose down in scale (Shift+Down)
  if (key == juce::KeyPress::downKey && key.getModifiers().isShiftDown() &&
      scaleLockEnabled) {
    transformTransposeInScale(-1);
    return true;
  }

  return false;
}
// Orphan code removed

//==============================================================================
// Note Split / Join Implementation (Competition Feature)
//==============================================================================

void PianoRollComponent::splitNotesAtBeat(double beatPosition) {
  if (getSelectedNoteCount() == 0)
    return;

  projectState.getUndoManager().beginNewTransaction("Split notes at beat");

  std::vector<NoteRect> toSplit;
  for (const auto &note : noteRects) {
    if (!note.selected)
      continue;

    double noteEnd = note.startBeats + note.lengthBeats;
    if (beatPosition > note.startBeats && beatPosition < noteEnd) {
      toSplit.push_back(note);
    }
  }

  for (const auto &note : toSplit) {
    // Shorten original note
    double firstLength = beatPosition - note.startBeats;
    projectState.setMidiNoteLength(currentClip.clipId, note.id, firstLength,
                                   "");

    // Create second note
    zenith::ProjectState::MidiNoteSpec secondNote;
    secondNote.pitch = note.pitch;
    secondNote.startBeats = beatPosition;
    secondNote.lengthBeats =
        (note.startBeats + note.lengthBeats) - beatPosition;
    secondNote.velocity = note.velocity;
    secondNote.muted = note.muted;

    projectState.addMidiNote(currentClip.clipId, secondNote, "");
  }

  refreshNotesFromProjectState();
}

void PianoRollComponent::splitNotesEqual(int divisions) {
  if (getSelectedNoteCount() == 0 || divisions < 2)
    return;

  projectState.getUndoManager().beginNewTransaction("Split notes equal");

  std::vector<NoteRect> toSplit;
  for (const auto &note : noteRects) {
    if (note.selected)
      toSplit.push_back(note);
  }

  for (const auto &note : toSplit) {
    double divLength = note.lengthBeats / divisions;

    // Shorten original to first division
    projectState.setMidiNoteLength(currentClip.clipId, note.id, divLength, "");

    // Create remaining divisions
    for (int i = 1; i < divisions; ++i) {
      zenith::ProjectState::MidiNoteSpec newNote;
      newNote.pitch = note.pitch;
      newNote.startBeats = note.startBeats + i * divLength;
      newNote.lengthBeats = divLength;
      newNote.velocity = note.velocity;
      newNote.muted = note.muted;

      projectState.addMidiNote(currentClip.clipId, newNote, "");
    }
  }

  refreshNotesFromProjectState();
}

void PianoRollComponent::joinConsecutiveNotes() {
  if (getSelectedNoteCount() < 2)
    return;

  // Group selected notes by pitch
  std::map<int, std::vector<NoteRect *>> notesByPitch;
  for (auto &note : noteRects) {
    if (note.selected)
      notesByPitch[note.pitch].push_back(&note);
  }

  projectState.getUndoManager().beginNewTransaction("Join consecutive notes");

  for (auto &[pitch, notes] : notesByPitch) {
    if (notes.size() < 2)
      continue;

    // Sort by start time
    std::sort(notes.begin(), notes.end(),
              [](const NoteRect *a, const NoteRect *b) {
                return a->startBeats < b->startBeats;
              });

    // Find consecutive pairs (gap < small threshold)
    const double gapThreshold = 0.1; // 10% of a beat

    for (size_t i = 0; i < notes.size() - 1; ++i) {
      double note1End = notes[i]->startBeats + notes[i]->lengthBeats;
      double gap = notes[i + 1]->startBeats - note1End;

      if (gap < gapThreshold) {
        // Extend first note to cover second
        double newLength =
            (notes[i + 1]->startBeats + notes[i + 1]->lengthBeats) -
            notes[i]->startBeats;
        projectState.setMidiNoteLength(currentClip.clipId, notes[i]->id,
                                       newLength, "");

        // Delete second note
        projectState.removeMidiNote(currentClip.clipId, notes[i + 1]->id, "");
      }
    }
  }

  refreshNotesFromProjectState();
}

void PianoRollComponent::applyLegato() {
  if (getSelectedNoteCount() < 2)
    return;

  // Group by pitch
  std::map<int, std::vector<NoteRect *>> notesByPitch;
  for (auto &note : noteRects) {
    if (note.selected)
      notesByPitch[note.pitch].push_back(&note);
  }

  projectState.getUndoManager().beginNewTransaction("Apply legato");

  for (auto &[pitch, notes] : notesByPitch) {
    if (notes.size() < 2)
      continue;

    // Sort by start time
    std::sort(notes.begin(), notes.end(),
              [](const NoteRect *a, const NoteRect *b) {
                return a->startBeats < b->startBeats;
              });

    // Extend each note to meet the next
    for (size_t i = 0; i < notes.size() - 1; ++i) {
      double newLength = notes[i + 1]->startBeats - notes[i]->startBeats;
      if (newLength > 0) {
        projectState.setMidiNoteLength(currentClip.clipId, notes[i]->id,
                                       newLength, "");
      }
    }
  }

  refreshNotesFromProjectState();
}

//==============================================================================
// Arpeggiator Preview Implementation (Competition Feature)
//==============================================================================

void PianoRollComponent::setArpeggiatorPreview(bool enabled, ArpPattern pattern,
                                               double rate, int octaves) {
  arpPreviewEnabled = enabled;
  arpPattern = pattern;
  arpRate = rate;
  arpOctaves = juce::jlimit(1, 4, octaves);

  if (enabled)
    generateArpPreview();
  else
    arpPreviewNotes.clear();

  repaint();
}

void PianoRollComponent::generateArpPreview() {
  arpPreviewNotes.clear();

  if (!arpPreviewEnabled || getSelectedNoteCount() == 0)
    return;

  // Collect selected pitches
  std::vector<int> pitches;
  double startBeat = std::numeric_limits<double>::max();
  double endBeat = 0.0;
  int avgVelocity = 0;
  int count = 0;

  for (const auto &note : noteRects) {
    if (note.selected) {
      pitches.push_back(note.pitch);
      startBeat = std::min(startBeat, note.startBeats);
      endBeat = std::max(endBeat, note.startBeats + note.lengthBeats);
      avgVelocity += note.velocity;
      count++;
    }
  }

  if (pitches.empty())
    return;

  avgVelocity /= count;

  // Sort pitches
  std::sort(pitches.begin(), pitches.end());

  // Add octaves
  size_t originalSize = pitches.size();
  for (int oct = 1; oct < arpOctaves; ++oct) {
    for (size_t i = 0; i < originalSize; ++i) {
      int newPitch = pitches[i] + oct * 12;
      if (newPitch <= 127)
        pitches.push_back(newPitch);
    }
  }

  // Order based on pattern
  std::vector<int> orderedPitches = pitches;
  switch (arpPattern) {
  case ArpPattern::Up:
    // Already sorted ascending
    break;
  case ArpPattern::Down:
    std::reverse(orderedPitches.begin(), orderedPitches.end());
    break;
  case ArpPattern::UpDown:
    for (auto it = pitches.rbegin() + 1; it != pitches.rend() - 1; ++it)
      orderedPitches.push_back(*it);
    break;
  case ArpPattern::DownUp:
    std::reverse(orderedPitches.begin(), orderedPitches.end());
    for (auto it = pitches.begin() + 1; it != pitches.end() - 1; ++it)
      orderedPitches.push_back(*it);
    break;
  case ArpPattern::Random: {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(orderedPitches.begin(), orderedPitches.end(), g);
  } break;
  case ArpPattern::Order:
    // Keep as-is (as selected)
    break;
  }

  // Generate arp notes
  double currentBeat = startBeat;
  size_t pitchIndex = 0;

  while (currentBeat < endBeat) {
    NoteRect arpNote;
    arpNote.pitch = orderedPitches[pitchIndex % orderedPitches.size()];
    arpNote.startBeats = currentBeat;
    arpNote.lengthBeats = arpRate * 0.9; // Slightly shorter than rate
    arpNote.velocity = avgVelocity;
    arpNote.selected = false;
    arpNote.muted = false;

    arpPreviewNotes.push_back(arpNote);

    currentBeat += arpRate;
    pitchIndex++;
  }
}

void PianoRollComponent::commitArpeggiator() {
  if (!arpPreviewEnabled || arpPreviewNotes.empty() || !currentClip.isValid())
    return;

  projectState.getUndoManager().beginNewTransaction("Commit arpeggiator");

  // Delete original selected notes
  std::vector<juce::String> toDelete;
  for (const auto &note : noteRects) {
    if (note.selected)
      toDelete.push_back(note.id);
  }

  for (const auto &id : toDelete) {
    projectState.removeMidiNote(currentClip.clipId, id, "");
  }

  // Add arp notes
  for (const auto &arpNote : arpPreviewNotes) {
    zenith::ProjectState::MidiNoteSpec newNote;
    newNote.pitch = arpNote.pitch;
    newNote.startBeats = arpNote.startBeats;
    newNote.lengthBeats = arpNote.lengthBeats;
    newNote.velocity = arpNote.velocity;
    newNote.muted = false;

    projectState.addMidiNote(currentClip.clipId, newNote, "");
  }

  arpPreviewEnabled = false;
  arpPreviewNotes.clear();
  refreshNotesFromProjectState();
}

//==============================================================================
// Pattern Library Implementation (Competition Feature)
//==============================================================================

PianoRollComponent::MidiPattern
PianoRollComponent::saveAsPattern(const juce::String &name,
                                  const juce::String &category) {
  MidiPattern pattern;
  pattern.name = name;
  pattern.category = category;
  pattern.lengthBeats = 0.0;

  if (getSelectedNoteCount() == 0)
    return pattern;

  // Find earliest note
  double earliestStart = std::numeric_limits<double>::max();
  for (const auto &note : noteRects) {
    if (note.selected)
      earliestStart = std::min(earliestStart, note.startBeats);
  }

  // Convert to relative positions
  for (const auto &note : noteRects) {
    if (!note.selected)
      continue;

    zenith::ProjectState::MidiNoteSpec spec;
    spec.pitch = note.pitch;
    spec.startBeats = note.startBeats - earliestStart;
    spec.lengthBeats = note.lengthBeats;
    spec.velocity = note.velocity;
    spec.muted = note.muted;

    pattern.notes.push_back(spec);

    double noteEnd = spec.startBeats + spec.lengthBeats;
    if (noteEnd > pattern.lengthBeats)
      pattern.lengthBeats = noteEnd;
  }

  return pattern;
}

void PianoRollComponent::loadPattern(const MidiPattern &pattern,
                                     double startBeat, int transposition) {
  if (pattern.notes.empty() || !currentClip.isValid())
    return;

  projectState.getUndoManager().beginNewTransaction("Load pattern");

  for (const auto &noteSpec : pattern.notes) {
    zenith::ProjectState::MidiNoteSpec newNote = noteSpec;
    newNote.startBeats += startBeat;
    newNote.pitch = juce::jlimit(0, 127, newNote.pitch + transposition);
    newNote.id = ""; // Will be auto-generated

    if (newNote.startBeats < currentClip.clipLengthBeats) {
      projectState.addMidiNote(currentClip.clipId, newNote, "");
    }
  }

  refreshNotesFromProjectState();
}

std::vector<PianoRollComponent::MidiPattern>
PianoRollComponent::getBuiltInPatterns() {
  std::vector<MidiPattern> patterns;

  // Basic drum pattern
  {
    MidiPattern drumBasic;
    drumBasic.name = "Basic Drum Beat";
    drumBasic.category = "Drums";
    drumBasic.lengthBeats = 4.0;

    // Kick on 1 and 3
    drumBasic.notes.push_back({/*id*/ "", 36, 0.0, 0.25, 100, false});
    drumBasic.notes.push_back({/*id*/ "", 36, 2.0, 0.25, 100, false});
    // Snare on 2 and 4
    drumBasic.notes.push_back({/*id*/ "", 38, 1.0, 0.25, 100, false});
    drumBasic.notes.push_back({/*id*/ "", 38, 3.0, 0.25, 100, false});
    // Hi-hats every 8th
    for (int i = 0; i < 8; ++i) {
      drumBasic.notes.push_back({/*id*/ "", 42, i * 0.5, 0.25, 80, false});
    }

    patterns.push_back(drumBasic);
  }

  // Arpeggio pattern
  {
    MidiPattern arpUp;
    arpUp.name = "Arpeggio Up";
    arpUp.category = "Melodic";
    arpUp.lengthBeats = 2.0;

    arpUp.notes.push_back({/*id*/ "", 60, 0.0, 0.25, 100, false});  // C
    arpUp.notes.push_back({/*id*/ "", 64, 0.25, 0.25, 90, false});  // E
    arpUp.notes.push_back({/*id*/ "", 67, 0.5, 0.25, 85, false});   // G
    arpUp.notes.push_back({/*id*/ "", 72, 0.75, 0.25, 80, false});  // C+octave
    arpUp.notes.push_back({/*id*/ "", 67, 1.0, 0.25, 85, false});   // G
    arpUp.notes.push_back({/*id*/ "", 64, 1.25, 0.25, 90, false});  // E
    arpUp.notes.push_back({/*id*/ "", 60, 1.5, 0.25, 100, false});  // C
    arpUp.notes.push_back({/*id*/ "", 55, 1.75, 0.25, 100, false}); // G-octave

    patterns.push_back(arpUp);
  }

  // Bass pattern
  {
    MidiPattern bassGroove;
    bassGroove.name = "Funk Bass";
    bassGroove.category = "Bass";
    bassGroove.lengthBeats = 4.0;

    bassGroove.notes.push_back({/*id*/ "", 36, 0.0, 0.375, 110, false});
    bassGroove.notes.push_back({/*id*/ "", 36, 0.75, 0.125, 80, false});
    bassGroove.notes.push_back({/*id*/ "", 38, 1.0, 0.25, 100, false});
    bassGroove.notes.push_back({/*id*/ "", 36, 1.5, 0.25, 90, false});
    bassGroove.notes.push_back({/*id*/ "", 43, 2.0, 0.5, 100, false});
    bassGroove.notes.push_back({/*id*/ "", 41, 2.75, 0.125, 70, false});
    bassGroove.notes.push_back({/*id*/ "", 38, 3.0, 0.25, 100, false});
    bassGroove.notes.push_back({/*id*/ "", 36, 3.5, 0.375, 90, false});

    patterns.push_back(bassGroove);
  }

  return patterns;
}

//==============================================================================
// Pattern File Persistence
//==============================================================================

juce::File PianoRollComponent::getUserPatternsDirectory() {
  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
  auto patternsDir = appDataDir.getChildFile("Zenith").getChildFile("Patterns");
  patternsDir.createDirectory();
  return patternsDir;
}

bool PianoRollComponent::savePatternToFile(const MidiPattern &pattern,
                                           const juce::File &file) {
  // Use juce::var to manage DynamicObject lifetime automatically
  juce::var patternData(new juce::DynamicObject());
  auto *patternObj = patternData.getDynamicObject();
  if (patternObj == nullptr)
    return false;

  patternObj->setProperty("name", pattern.name);
  patternObj->setProperty("category", pattern.category);
  patternObj->setProperty("lengthBeats", pattern.lengthBeats);

  juce::var notesArray;
  for (const auto &note : pattern.notes) {
    juce::var noteData(new juce::DynamicObject());
    auto *noteObj = noteData.getDynamicObject();
    if (noteObj == nullptr)
      continue;

    noteObj->setProperty("pitch", note.pitch);
    noteObj->setProperty("startBeats", note.startBeats);
    noteObj->setProperty("lengthBeats", note.lengthBeats);
    noteObj->setProperty("velocity", note.velocity);
    noteObj->setProperty("muted", note.muted);
    notesArray.append(noteData);
  }
  patternObj->setProperty("notes", notesArray);

  juce::String jsonString = juce::JSON::toString(patternData);
  return file.replaceWithText(jsonString);
}

PianoRollComponent::MidiPattern
PianoRollComponent::loadPatternFromFile(const juce::File &file) {
  MidiPattern pattern;

  if (!file.existsAsFile())
    return pattern;

  juce::String jsonString = file.loadFileAsString();
  juce::var patternData = juce::JSON::parse(jsonString);

  if (patternData.isVoid())
    return pattern;

  pattern.name = patternData.getProperty("name", "").toString();
  pattern.category = patternData.getProperty("category", "").toString();
  pattern.lengthBeats = patternData.getProperty("lengthBeats", 4.0);

  juce::var notesArray = patternData.getProperty("notes", juce::var());
  if (notesArray.isArray()) {
    for (int i = 0; i < notesArray.size(); ++i) {
      juce::var noteData = notesArray[i];
      zenith::ProjectState::MidiNoteSpec note;
      note.pitch = noteData.getProperty("pitch", 60);
      note.startBeats = noteData.getProperty("startBeats", 0.0);
      note.lengthBeats = noteData.getProperty("lengthBeats", 0.25);
      note.velocity = noteData.getProperty("velocity", 100);
      note.muted = noteData.getProperty("muted", false);
      pattern.notes.push_back(note);
    }
  }

  return pattern;
}

//==============================================================================
// MIDI Echo Implementation (Competition Feature)
//==============================================================================

void PianoRollComponent::applyMidiEcho(int repeats, double delayBeats,
                                       float velocityDecay,
                                       int pitchShiftPerRepeat) {
  if (getSelectedNoteCount() == 0 || repeats < 1)
    return;

  velocityDecay = juce::jlimit(0.0f, 1.0f, velocityDecay);

  projectState.getUndoManager().beginNewTransaction("MIDI echo");

  // Copy selected notes
  std::vector<NoteRect> original;
  for (const auto &note : noteRects) {
    if (note.selected)
      original.push_back(note);
  }

  // Create echoes
  for (int rep = 1; rep <= repeats; ++rep) {
    float velMultiplier = std::pow(velocityDecay, static_cast<float>(rep));

    for (const auto &note : original) {
      zenith::ProjectState::MidiNoteSpec echoNote;
      echoNote.pitch =
          juce::jlimit(0, 127, note.pitch + pitchShiftPerRepeat * rep);
      echoNote.startBeats = note.startBeats + delayBeats * rep;
      echoNote.lengthBeats = note.lengthBeats;
      echoNote.velocity =
          juce::jlimit(1, 127, static_cast<int>(note.velocity * velMultiplier));
      echoNote.muted = false;

      if (echoNote.startBeats < currentClip.clipLengthBeats) {
        projectState.addMidiNote(currentClip.clipId, echoNote, "");
      }
    }
  }

  refreshNotesFromProjectState();
}

//==============================================================================
// Chord Presets Implementation (Competition Feature)
//==============================================================================

std::vector<int> PianoRollComponent::getChordIntervals(ChordType type) {
  switch (type) {
  case ChordType::Major:
    return {0, 4, 7};
  case ChordType::Minor:
    return {0, 3, 7};
  case ChordType::Diminished:
    return {0, 3, 6};
  case ChordType::Augmented:
    return {0, 4, 8};
  case ChordType::Major7:
    return {0, 4, 7, 11};
  case ChordType::Minor7:
    return {0, 3, 7, 10};
  case ChordType::Dominant7:
    return {0, 4, 7, 10};
  case ChordType::Diminished7:
    return {0, 3, 6, 9};
  case ChordType::Sus2:
    return {0, 2, 7};
  case ChordType::Sus4:
    return {0, 5, 7};
  case ChordType::Add9:
    return {0, 4, 7, 14};
  case ChordType::Minor9:
    return {0, 3, 7, 10, 14};
  case ChordType::Power:
    return {0, 7};
  case ChordType::Sixth:
    return {0, 4, 7, 9};
  case ChordType::Minor6:
    return {0, 3, 7, 9};
  default:
    return {0, 4, 7};
  }
}

void PianoRollComponent::insertChord(int rootPitch, ChordType type,
                                     double startBeat, double lengthBeats,
                                     int velocity) {
  if (!currentClip.isValid())
    return;

  auto intervals = getChordIntervals(type);

  projectState.getUndoManager().beginNewTransaction("Insert chord");

  for (int interval : intervals) {
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = juce::jlimit(0, 127, rootPitch + interval);
    note.startBeats = startBeat;
    note.lengthBeats = lengthBeats;
    note.velocity = velocity;
    note.muted = false;

    if (note.startBeats < currentClip.clipLengthBeats) {
      projectState.addMidiNote(currentClip.clipId, note, "");
    }
  }

  refreshNotesFromProjectState();
}

//==============================================================================
// Spray Can Tool Implementation (Competition Feature)
//==============================================================================

void PianoRollComponent::setSprayCanMode(bool enabled) {
  sprayCanMode = enabled;
  repaint();
}

void PianoRollComponent::setSprayVelocityRange(int min, int max) {
  sprayVelocityMin = juce::jlimit(1, 127, std::min(min, max));
  sprayVelocityMax = juce::jlimit(1, 127, std::max(min, max));
}

void PianoRollComponent::handleSprayPaint(float x, float y) {
  if (!sprayCanMode || !currentClip.isValid())
    return;

  // Convert position to beat/pitch
  double beat = pixelsToBeats((int)x - PIANO_WIDTH);
  int pitch = pixelsToPitch((int)y - RULER_HEIGHT);

  if (beat < 0 || beat >= currentClip.clipLengthBeats)
    return;
  if (pitch < 0 || pitch > 127)
    return;

  // Random variations
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> velDist(sprayVelocityMin,
                                             sprayVelocityMax);
  std::uniform_int_distribution<int> pitchDist(-sprayPitchRange,
                                               sprayPitchRange);

  zenith::ProjectState::MidiNoteSpec note;
  note.pitch = juce::jlimit(0, 127, pitch + pitchDist(gen));
  note.startBeats = beat;
  note.lengthBeats = gridBeats; // Use current grid size
  note.velocity = velDist(gen);
  note.muted = false;

  // Snap to scale if enabled
  if (scaleLockEnabled) {
    note.pitch = snapPitchToScale(note.pitch);
  }

  projectState.addMidiNote(currentClip.clipId, note, "Spray note");

  lastSprayPosition = juce::Point<float>(x, y);
  refreshNotesFromProjectState();
}

//==============================================================================
// Scripting API Implementation (Competition Feature)
//==============================================================================

void PianoRollComponent::setScriptCallback(const juce::String &name,
                                           ScriptCallback callback) {
  scriptCallbacks[name] = callback;
}

void PianoRollComponent::executeScript(const juce::String &name) {
  auto it = scriptCallbacks.find(name);
  if (it == scriptCallbacks.end())
    return;

  projectState.getUndoManager().beginNewTransaction("Execute script: " + name);

  // Execute the callback
  it->second(noteRects);

  // Sync changes back to project state
  // (This is a simplified version - a full impl would track changes)
  refreshNotesFromProjectState();
  repaint();
}

void PianoRollComponent::runScriptFromFile(const juce::File &file) {
  if (!file.existsAsFile())
    return;

  // Simple Line-based Script Parser
  // Supported commands:
  // transpose <semitones>
  // velocity <delta>
  // quantize <grid>
  // reverse
  // invert

  juce::StringArray lines;
  file.readLines(lines);

  if (lines.isEmpty())
    return;

  projectState.getUndoManager().beginNewTransaction(
      "Run Script: " + file.getFileNameWithoutExtension());

  for (const auto &line : lines) {
    juce::String cmd = line.trim();
    if (cmd.isEmpty() || cmd.startsWith("#") || cmd.startsWith("//"))
      continue;

    juce::StringArray parts;
    parts.addTokens(cmd, " ", "\"");

    if (parts.size() == 0)
      continue;

    juce::String command = parts[0].toLowerCase();

    if (command == "transpose" && parts.size() > 1) {
      int semitones = parts[1].getIntValue();
      for (const auto &note : noteRects) {
        if (note.selected)
          projectState.moveMidiNote(
              currentClip.clipId, note.id, note.startBeats,
              juce::jlimit(0, 127, note.pitch + semitones), "");
      }
    } else if (command == "velocity" && parts.size() > 1) {
      int delta = parts[1].getIntValue();
      for (const auto &note : noteRects) {
        if (note.selected)
          projectState.setMidiNoteVelocity(
              currentClip.clipId, note.id,
              juce::jlimit(1, 127, note.velocity + delta), "");
      }
    } else if (command == "quantize" && parts.size() > 1) {
      float grid = parts[1].getFloatValue();
      quantizeSelected(grid, 1.0f, 0.0f);
    } else if (command == "reverse") {
      transformRetrograde();
    } else if (command == "invert") {
      transformInversion();
    }
  }

  refreshNotesFromProjectState();
}

//==============================================================================
//==============================================================================
// Note Collision Detection Implementation
//==============================================================================

void PianoRollComponent::detectNoteCollisions() {
  // Reset all collision flags
  for (auto &note : noteRects)
    note.hasCollision = false;

  // Optimization: Use spatial grid for faster collision detection if available
  if (!spatialGridDirty && noteRects.size() > 20) {
    // Group notes by pitch first (most collisions are same pitch)
    std::map<int, std::vector<NoteRect *>> notesByPitch;
    for (auto &note : noteRects) {
      notesByPitch[note.pitch].push_back(&note);
    }

    // Only check collisions within same pitch groups
    for (auto &[pitch, notes] : notesByPitch) {
      if (notes.size() < 2)
        continue;

      for (size_t i = 0; i < notes.size(); ++i) {
        for (size_t j = i + 1; j < notes.size(); ++j) {
          auto *noteA = notes[i];
          auto *noteB = notes[j];

          double startA = noteA->startBeats;
          double endA = noteA->startBeats + noteA->lengthBeats;
          double startB = noteB->startBeats;
          double endB = noteB->startBeats + noteB->lengthBeats;

          if (startA < endB && endA > startB) {
            noteA->hasCollision = true;
            noteB->hasCollision = true;
          }
        }
      }
    }
  } else {
    // Fallback: O(n²) check for small note counts
    for (size_t i = 0; i < noteRects.size(); ++i) {
      for (size_t j = i + 1; j < noteRects.size(); ++j) {
        auto &noteA = noteRects[i];
        auto &noteB = noteRects[j];

        if (noteA.pitch == noteB.pitch) {
          double startA = noteA.startBeats;
          double endA = noteA.startBeats + noteA.lengthBeats;
          double startB = noteB.startBeats;
          double endB = noteB.startBeats + noteB.lengthBeats;

          if (startA < endB && endA > startB) {
            noteA.hasCollision = true;
            noteB.hasCollision = true;
          }
        }
      }
    }
  }
}

int PianoRollComponent::getCollisionCount() const {
  int count = 0;
  for (const auto &note : noteRects) {
    if (note.hasCollision)
      ++count;
  }
  return count;
}

//==============================================================================
// MIDI Input & Preview
//==============================================================================

void PianoRollComponent::setMidiInputEnabled(bool enabled) {
  midiInputEnabled = enabled;
  if (!enabled) {
    activeInputNotes.clear();
  }
}

void PianoRollComponent::handleMidiNoteOn(int pitch, int velocity) {
  if (!midiInputEnabled || !currentClip.isValid())
    return;

  // Bounds checking
  pitch = juce::jlimit(0, 127, pitch);
  velocity = juce::jlimit(1, 127, velocity);

  // Record start time (clamped to clip bounds)
  double startTime =
      juce::jlimit(0.0, currentClip.clipLengthBeats, currentPlayheadBeats);
  activeInputNotes[pitch] = startTime;

  // Visual feedback / add note immediately
  zenith::ProjectState::MidiNoteSpec newNote;
  newNote.pitch = pitch;
  newNote.startBeats = startTime;
  newNote.lengthBeats = 0.25; // Default, updated on release
  newNote.velocity = velocity;
  newNote.muted = false;

  // Ensure note doesn't exceed clip bounds
  if (startTime + newNote.lengthBeats > currentClip.clipLengthBeats) {
    newNote.lengthBeats = currentClip.clipLengthBeats - startTime;
    newNote.lengthBeats = juce::jmax(0.1, newNote.lengthBeats);
  }

  projectState.addMidiNote(currentClip.clipId, newNote, "Record MIDI Note");
  refreshNotesFromProjectState();
}

void PianoRollComponent::handleMidiNoteOff(int pitch) {
  if (!midiInputEnabled || !currentClip.isValid())
    return;

  pitch = juce::jlimit(0, 127, pitch);

  auto it = activeInputNotes.find(pitch);
  if (it == activeInputNotes.end())
    return;

  double startTime = it->second;
  double length = currentPlayheadBeats - startTime;

  // Bounds checking
  length = juce::jmax(0.1, length);
  double noteEnd = startTime + length;
  if (noteEnd > currentClip.clipLengthBeats) {
    length = currentClip.clipLengthBeats - startTime;
    length = juce::jmax(0.1, length);
  }

  // Update note length - find the most recent note at this pitch/time
  juce::String noteIdToUpdate;
  double closestTime = std::numeric_limits<double>::max();

  for (const auto &note : noteRects) {
    if (note.pitch == pitch && std::abs(note.startBeats - startTime) < 0.1) {
      double timeDiff = std::abs(note.startBeats - startTime);
      if (timeDiff < closestTime) {
        closestTime = timeDiff;
        noteIdToUpdate = note.id;
      }
    }
  }

  if (noteIdToUpdate.isNotEmpty()) {
    projectState.setMidiNoteLength(currentClip.clipId, noteIdToUpdate, length,
                                   "");
  }

  activeInputNotes.erase(it);
  refreshNotesFromProjectState();
}

void PianoRollComponent::previewNote(int pitch, int velocity) {
  if (notePreviewCallback)
    notePreviewCallback(pitch, velocity, true);
}

void PianoRollComponent::stopNotePreview() {
  if (notePreviewCallback && currentPreviewPitch >= 0) {
    notePreviewCallback(currentPreviewPitch, 0, false);
    currentPreviewPitch = -1;
  }
}

void PianoRollComponent::updatePlayheadPosition(double beats) {
  // Update playhead for MIDI recording
  currentPlayheadBeats = juce::jmax(0.0, beats);

  // Clamp to clip bounds if recording
  if (midiInputEnabled && currentClip.isValid()) {
    currentPlayheadBeats =
        juce::jlimit(0.0, currentClip.clipLengthBeats, currentPlayheadBeats);
  }
}

//==============================================================================
// Skia Rendering Implementation
//==============================================================================

void PianoRollComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect fullRect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Define areas
  float rulerH = RULER_HEIGHT;
  float pianoW = PIANO_WIDTH;
  float velH = (float)velocityLaneHeight;

  SkRect contentArea = SkRect::MakeLTRB(pianoW, rulerH, bounds.getWidth(),
                                        bounds.getHeight() - velH);
  SkRect pianoArea =
      SkRect::MakeLTRB(0, rulerH, pianoW, bounds.getHeight() - velH);
  SkRect velocityArea = SkRect::MakeLTRB(pianoW, bounds.getHeight() - velH,
                                         bounds.getWidth(), bounds.getHeight());
  SkRect rulerArea = SkRect::MakeLTRB(pianoW, 0, bounds.getWidth(), rulerH);

  // 1. Background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(18, 18, 22));
  canvas->drawRect(fullRect, bgPaint);

  // 2. Grid & Content
  canvas->save();
  canvas->clipRect(contentArea);

  if (stepSequencerMode) {
    drawStepSequencer(canvas, contentArea);
  } else {
    drawGrid(canvas, contentArea);

    if (ghostNotesEnabled) {
      drawGhostNotes(canvas, contentArea);
    }

    drawExpressionLanes(canvas, contentArea);
    drawNotes(canvas, contentArea);

    if (arpPreviewEnabled) {
      drawArpPreview(canvas, contentArea);
    }

    // Selection marquee
    if (currentDragMode == DragMode::MarqueeSelect) {
      SkPaint marqueePaint;
      marqueePaint.setColor(SkColorSetARGB(40, 0, 255, 255));
      marqueePaint.setStyle(SkPaint::kFill_Style);
      SkRect mRect =
          SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                           marqueeRect.getWidth(), marqueeRect.getHeight());
      canvas->drawRect(mRect, marqueePaint);

      marqueePaint.setColor(SkColorSetRGB(0, 255, 255));
      marqueePaint.setStyle(SkPaint::kStroke_Style);
      canvas->drawRect(mRect, marqueePaint);
    }
  }
  canvas->restore();

  // 3. Sidebars & Overlays
  drawPianoKeys(canvas, pianoArea);
  drawVelocityLane(canvas, velocityArea);

  // Ruler Background
  SkPaint rulerPaint;
  rulerPaint.setColor(SkColorSetRGB(25, 25, 30));
  canvas->drawRect(rulerArea, rulerPaint);

  // 4. Toolbar
  drawModernToolbar(canvas, fullRect);

  // 5. Overlays
  drawChordName(canvas);

  // Clip Name Overlay
  if (currentClip.isValid()) {
    SkFont titleFont;
    titleFont.setSize(12.0f);
    SkPaint titlePaint;
    titlePaint.setColor(SkColorSetARGB(200, 255, 255, 255));
    canvas->drawString(currentClip.clipName.toRawUTF8(), PIANO_WIDTH + 10.0f,
                       20.0f, titleFont, titlePaint);
  }
}

void PianoRollComponent::drawPianoKeys(SkCanvas *canvas, const SkRect &area) {
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(10, 10, 12));
  canvas->drawRect(area, bgPaint);

  SkPaint keyPaint;
  SkPaint textPaint;
  textPaint.setColor(SkColorSetARGB(180, 255, 255, 255));
  SkFont font;
  font.setSize(10.0f);
  font.setSubpixel(true);

  int startPitch = pixelsToPitch(area.top());
  int endPitch = pixelsToPitch(area.bottom());
  startPitch = juce::jlimit(0, 127, startPitch + 1);
  endPitch = juce::jlimit(0, 127, endPitch - 1);

  for (int p = endPitch; p <= startPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;
    float h = pixelsPerPitch;

    bool isBlackKey = (p % 12 == 1 || p % 12 == 3 || p % 12 == 6 ||
                       p % 12 == 8 || p % 12 == 10);
    SkRect keyRect = SkRect::MakeXYWH(area.left(), y, area.width(), h);

    if (isBlackKey) {
      keyPaint.setColor(SkColorSetRGB(30, 30, 35));
      keyPaint.setStyle(SkPaint::kFill_Style);
      canvas->drawRect(keyRect, keyPaint);

      SkPaint bevel;
      bevel.setColor(SkColorSetRGB(50, 50, 60));
      canvas->drawLine(area.left(), y, area.right(), y, bevel);
    } else {
      keyPaint.setColor(SkColorSetRGB(60, 60, 70));
      keyPaint.setStyle(SkPaint::kFill_Style);
      canvas->drawRect(keyRect, keyPaint);

      SkPaint sep;
      sep.setColor(SkColorSetRGB(20, 20, 25));
      sep.setStrokeWidth(1.0f);
      canvas->drawLine(area.left(), y + h, area.right(), y + h,
                       sep); // Bottom separator

      if (p % 12 == 0) {
        juce::String label = "C" + juce::String(p / 12 - 2);
        canvas->drawString(label.toRawUTF8(), area.left() + 35.0f, y + h - 3.0f,
                           font, textPaint);
      }
    }
  }
}

void PianoRollComponent::drawGrid(SkCanvas *canvas, const SkRect &area) {
  SkPaint gridPaint;
  gridPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
  gridPaint.setStrokeWidth(1.0f);

  int startPitch = pixelsToPitch(area.top() - RULER_HEIGHT);
  int endPitch = pixelsToPitch(area.bottom() - RULER_HEIGHT);

  for (int p = endPitch; p <= startPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;

    if (p % 12 == 0)
      gridPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
    else
      gridPaint.setColor(SkColorSetARGB(20, 255, 255, 255));

    if (scaleLockEnabled && scaleHighlight.enabled && !isNoteInScale(p)) {
      SkPaint dimPaint;
      dimPaint.setColor(SkColorSetARGB(100, 10, 10, 12));
      canvas->drawRect(
          SkRect::MakeXYWH(area.left(), y, area.width(), pixelsPerPitch),
          dimPaint);
    }

    canvas->drawLine(area.left(), y, area.right(), y, gridPaint);
  }

  double startBeat = pixelsToBeats(area.left() - PIANO_WIDTH);
  double endBeat = pixelsToBeats(area.right() - PIANO_WIDTH);

  double beatStep = 1.0;
  if (pixelsPerBeat > 100)
    beatStep = 0.25;
  else if (pixelsPerBeat < 20)
    beatStep = 4.0;

  for (double b = std::floor(startBeat); b <= endBeat; b += beatStep) {
    float x = beatsToPixels(b) + PIANO_WIDTH;
    if (std::abs(std::fmod(b, 4.0)) < 0.001)
      gridPaint.setColor(SkColorSetARGB(60, 255, 255, 255));
    else if (std::abs(std::fmod(b, 1.0)) < 0.001)
      gridPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
    else
      gridPaint.setColor(SkColorSetARGB(20, 255, 255, 255));

    canvas->drawLine(x, area.top(), x, area.bottom(), gridPaint);
  }
}

//==============================================================================
// MPE Expression Lane Visualization
//==============================================================================

void PianoRollComponent::updateCCLaneNames() {
  // Set display names for common CC numbers
  static const std::map<int, juce::String> ccNames = {
      {CC_MODULATION, "Modulation"},
      {CC_VOLUME, "Volume"},
      {CC_PAN, "Pan"},
      {CC_EXPRESSION, "Expression"},
      {CC_SUSTAIN, "Sustain"},
      {CC_PORTAMENTO, "Portamento"},
      {CC_SOSTENUTO, "Sostenuto"},
      {CC_SOFT_PEDAL, "Soft Pedal"},
      {CC_FILTER_RESONANCE, "Resonance"},
      {CC_RELEASE, "Release"},
      {CC_ATTACK, "Attack"},
      {CC_CUTOFF, "Cutoff"},
      {CC_REVERB, "Reverb"},
      {CC_CHORUS, "Chorus"}};

  for (auto &[ccNum, lane] : ccLanes) {
    auto it = ccNames.find(ccNum);
    if (it != ccNames.end()) {
      lane.name = it->second;
    } else {
      lane.name = "CC " + juce::String(ccNum);
    }
  }
}

juce::String PianoRollComponent::getCCLaneName(int ccNumber) const {
  auto it = ccLanes.find(ccNumber);
  if (it != ccLanes.end() && it->second.name.isNotEmpty()) {
    return it->second.name;
  }
  return "CC " + juce::String(ccNumber);
}

void PianoRollComponent::setCCLaneVisible(int ccNumber, bool visible) {
  ccNumber = juce::jlimit(0, 127, ccNumber);

  if (visible) {
    if (ccLanes.find(ccNumber) == ccLanes.end()) {
      ccLanes[ccNumber] = CCLane{ccNumber, true, {}, getCCLaneName(ccNumber)};
    } else {
      ccLanes[ccNumber].visible = true;
    }
    visibleCCLanes.insert(ccNumber);
  } else {
    if (ccLanes.find(ccNumber) != ccLanes.end()) {
      ccLanes[ccNumber].visible = false;
    }
    visibleCCLanes.erase(ccNumber);
  }

  updateCCLaneNames();
  repaint();
}

bool PianoRollComponent::getCCLaneVisible(int ccNumber) const {
  auto it = ccLanes.find(ccNumber);
  return it != ccLanes.end() && it->second.visible;
}

void PianoRollComponent::setCCPoint(int ccNumber, double timeBeats, int value) {
  if (!currentClip.isValid())
    return;

  ccNumber = juce::jlimit(0, 127, ccNumber);
  value = juce::jlimit(0, 127, value);
  timeBeats = juce::jmax(0.0, timeBeats);

  // Clamp to clip bounds
  if (timeBeats > currentClip.clipLengthBeats)
    return;

  // Ensure lane exists
  if (ccLanes.find(ccNumber) == ccLanes.end()) {
    ccLanes[ccNumber] = CCLane{ccNumber, false, {}, getCCLaneName(ccNumber)};
  }

  auto &lane = ccLanes[ccNumber];

  // Check if point already exists at this time (update it)
  for (auto &point : lane.points) {
    if (std::abs(point.timeBeats - timeBeats) < 0.001) {
      point.value = value;
      repaint();
      return;
    }
  }

  // Add new point
  CCPoint newPoint;
  newPoint.timeBeats = timeBeats;
  newPoint.value = value;
  // Generate unique ID using ProjectState's ID generator
  // Note: We use a simple timestamp-based approach since CC points aren't
  // stored in ProjectState yet
  static int ccPointCounter = 0;
  newPoint.id = "cc_" + juce::String(++ccPointCounter) + "_" +
                juce::String(juce::Time::currentTimeMillis());
  lane.points.push_back(newPoint);

  // Sort by time
  std::sort(lane.points.begin(), lane.points.end(),
            [](const CCPoint &a, const CCPoint &b) {
              return a.timeBeats < b.timeBeats;
            });

  repaint();
}

void PianoRollComponent::removeCCPoint(int ccNumber,
                                       const juce::String &pointId) {
  auto it = ccLanes.find(ccNumber);
  if (it == ccLanes.end())
    return;

  auto &lane = it->second;
  lane.points.erase(
      std::remove_if(lane.points.begin(), lane.points.end(),
                     [&pointId](const CCPoint &p) { return p.id == pointId; }),
      lane.points.end());

  repaint();
}

std::vector<PianoRollComponent::CCPoint>
PianoRollComponent::getCCPoints(int ccNumber, double startBeats,
                                double endBeats) const {
  std::vector<CCPoint> result;

  auto it = ccLanes.find(ccNumber);
  if (it == ccLanes.end())
    return result;

  const auto &lane = it->second;
  for (const auto &point : lane.points) {
    if (point.timeBeats >= startBeats && point.timeBeats <= endBeats) {
      result.push_back(point);
    }
  }

  return result;
}

std::vector<PianoRollComponent::CCPoint>
PianoRollComponent::getAllCCPoints(int ccNumber) const {
  auto it = ccLanes.find(ccNumber);
  if (it == ccLanes.end())
    return {};

  return it->second.points;
}

void PianoRollComponent::drawExpressionLanes(SkCanvas *canvas,
                                             const SkRect &area) {
  // Calculate total height of visible lanes
  int visibleLanes = 0;
  for (int i = 0; i < 4; ++i) {
    if (expressionLaneVisible[i])
      visibleLanes++;
  }

  if (visibleLanes == 0)
    return;

  float startY = area.bottom() - (visibleLanes * expressionLaneHeight);

  SkPaint laneBgPaint;
  laneBgPaint.setColor(SkColorSetARGB(255, 30, 30, 30));

  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(255, 60, 60, 60));
  borderPaint.setStyle(SkPaint::kStroke_Style);

  SkPaint textPaint;
  textPaint.setColor(SkColorSetARGB(255, 150, 150, 150));
  SkFont font;
  font.setSize(10.0f);

  const char *laneNames[] = {"Pitch Bend", "Pressure", "Slide", "Expression"};
  const SkColor laneColors[] = {
      SkColorSetRGB(255, 150, 150), // Red for Pitch
      SkColorSetRGB(150, 255, 150), // Green for Pressure
      SkColorSetRGB(150, 150, 255), // Blue for Slide
      SkColorSetRGB(255, 255, 150)  // Yellow for Expression
  };

  int laneIndex = 0;
  for (int i = 0; i < 4; ++i) {
    if (!expressionLaneVisible[i])
      continue;

    SkRect laneRect = SkRect::MakeXYWH(
        area.left(), startY + (laneIndex * expressionLaneHeight), area.width(),
        (float)expressionLaneHeight);

    // Draw background
    canvas->drawRect(laneRect, laneBgPaint);
    canvas->drawRect(laneRect, borderPaint);

    // Draw label
    canvas->drawString(laneNames[i], laneRect.left() + 5, laneRect.top() + 15,
                       font, textPaint);

    // Draw curves for each note
    SkPaint curvePaint;
    curvePaint.setColor(laneColors[i]);
    curvePaint.setStyle(SkPaint::kStroke_Style);
    curvePaint.setStrokeWidth(2.0f);
    curvePaint.setAntiAlias(true);

    // Only draw for visible notes to optimize
    for (const auto &note : noteRects) {
      auto it = noteExpressions.find(note.id);
      if (it == noteExpressions.end())
        continue;

      const std::vector<ExpressionPoint> *points = nullptr;
      switch ((ExpressionType)i) {
      case ExpressionType::PitchBend:
        points = &it->second.pitchBend;
        break;
      case ExpressionType::Pressure:
        points = &it->second.pressure;
        break;
      case ExpressionType::Slide:
        points = &it->second.slide;
        break;
      case ExpressionType::Expression:
        points = &it->second.expression;
        break;
      }

      if (!points || points->empty())
        continue;

      SkPath path;
      bool first = true;

      for (const auto &pt : *points) {
        float x = area.left() + beatsToPixels(note.startBeats + pt.timeOffset);
        float y = laneRect.bottom() - (pt.value * expressionLaneHeight);

        if (first) {
          path.moveTo(x, y);
          first = false;
        } else {
          path.lineTo(x, y);
        }
      }

      // Dim non-selected notes
      if (note.selected) {
        curvePaint.setAlpha(255);
        curvePaint.setStrokeWidth(2.5f);
      } else {
        curvePaint.setAlpha(100);
        curvePaint.setStrokeWidth(1.0f);
      }

      canvas->drawPath(path, curvePaint);

      // distinct points
      SkPaint pointPaint;
      pointPaint.setColor(laneColors[i]);
      pointPaint.setAntiAlias(true);
      if (!note.selected)
        pointPaint.setAlpha(100);

      for (const auto &pt : *points) {
        float x = area.left() + beatsToPixels(note.startBeats + pt.timeOffset);
        float y = laneRect.bottom() - (pt.value * expressionLaneHeight);
        canvas->drawCircle(x, y, 2.5f, pointPaint);
      }
    }

    laneIndex++;
  }

  // Draw MIDI CC lanes
  for (int ccNum : visibleCCLanes) {
    auto it = ccLanes.find(ccNum);
    if (it == ccLanes.end() || !it->second.visible)
      continue;

    const auto &lane = it->second;
    float laneY = startY + (laneIndex * expressionLaneHeight);
    SkRect laneRect = SkRect::MakeXYWH(area.left(), laneY, area.width(),
                                       (float)expressionLaneHeight);

    // Draw background
    canvas->drawRect(laneRect, laneBgPaint);
    canvas->drawRect(laneRect, borderPaint);

    // Draw label
    juce::String label =
        lane.name.isNotEmpty() ? lane.name : ("CC " + juce::String(ccNum));
    canvas->drawString(label.toRawUTF8(), laneRect.left() + 5,
                       laneRect.top() + 15, font, textPaint);

    // Draw CC curve
    if (lane.points.size() > 1) {
      SkPaint ccPaint;
      ccPaint.setColor(SkColorSetRGB(100, 200, 255)); // Light blue for CC
      ccPaint.setStyle(SkPaint::kStroke_Style);
      ccPaint.setStrokeWidth(2.0f);
      ccPaint.setAntiAlias(true);

      SkPath path;
      bool first = true;

      for (const auto &point : lane.points) {
        float x = area.left() + beatsToPixels(point.timeBeats);
        float y =
            laneRect.bottom() - ((point.value / 127.0f) * expressionLaneHeight);

        if (first) {
          path.moveTo(x, y);
          first = false;
        } else {
          path.lineTo(x, y);
        }
      }

      canvas->drawPath(path, ccPaint);

      // Draw points
      SkPaint pointPaint;
      pointPaint.setColor(SkColorSetRGB(100, 200, 255));
      pointPaint.setAntiAlias(true);

      for (const auto &point : lane.points) {
        float x = area.left() + beatsToPixels(point.timeBeats);
        float y =
            laneRect.bottom() - ((point.value / 127.0f) * expressionLaneHeight);
        canvas->drawCircle(x, y, 3.0f, pointPaint);
      }
    }

    laneIndex++;
  }
}

void PianoRollComponent::drawNotes(SkCanvas *canvas, const SkRect &area) {
  SkPaint notePaint;
  notePaint.setAntiAlias(true);
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(SK_ColorBLACK);
  borderPaint.setAntiAlias(true);

  for (const auto &note : noteRects) {
    if (note.bounds.getBottom() < area.top() ||
        note.bounds.getY() > area.bottom() ||
        note.bounds.getRight() < area.left() ||
        note.bounds.getX() > area.right())
      continue;

    SkRect rect =
        SkRect::MakeXYWH(note.bounds.getX(), note.bounds.getY(),
                         note.bounds.getWidth(), note.bounds.getHeight());
    SkRRect rrect = SkRRect::MakeRectXY(rect, 3.0f, 3.0f);

    SkColor baseColor =
        note.selected ? SK_ColorWHITE : getSkiaColorForVelocity(note.velocity);

    // Hover state - brighten the note
    if (note.isHovered && !note.selected) {
      baseColor =
          SkColorSetRGB(juce::jmin(255, (int)SkColorGetR(baseColor) + 30),
                        juce::jmin(255, (int)SkColorGetG(baseColor) + 30),
                        juce::jmin(255, (int)SkColorGetB(baseColor) + 30));
    }

    if (note.muted)
      baseColor = SkColorSetA(baseColor, 100);
    else if (note.probability < 1.0f) {
      // Probability transparency (min 30%) - use std::lround for proper rounding
      float alphaFactor = 0.3f + 0.7f * note.probability;
      int alpha = static_cast<int>(std::lround(255.0f * alphaFactor));
      alpha = juce::jlimit(0, 255, alpha); // Safety clamp
      baseColor = SkColorSetA(baseColor, alpha);
    }

    notePaint.setColor(baseColor);

    // Collision warning - red border
    if (note.hasCollision) {
      borderPaint.setColor(SK_ColorRED);
      borderPaint.setStrokeWidth(2.0f);
    } else {
      borderPaint.setColor(SK_ColorBLACK);
      borderPaint.setStrokeWidth(1.0f);
    }

    // Draw chance indicator if < 1.0
    if (note.probability < 1.0f && !note.muted) {
      SkPaint chancePaint;
      chancePaint.setColor(SK_ColorWHITE);
      chancePaint.setAlpha(150);
      canvas->drawCircle(rect.right() - 4, rect.bottom() - 4, 3, chancePaint);
    }

    // Draw Articulation / Keyswitch ID
    if (note.articulationId > 0 && note.lengthBeats > 0.5) {
      SkFont artFont;
      artFont.setSize(10.0f);
      SkPaint artTextPaint;
      artTextPaint.setColor(SK_ColorLTGRAY);
      juce::String artText = "Art: " + juce::String(note.articulationId);
      canvas->drawString(artText.toRawUTF8(), rect.left() + 4, rect.top() + 10,
                         artFont, artTextPaint);
    }

    // Draw Logic Condition / Recurrence
    if ((note.condition.isNotEmpty() || note.recurrence.isNotEmpty()) &&
        note.lengthBeats > 1.0) {
      SkFont logicFont;
      logicFont.setSize(9.0f);
      SkPaint logicPaint;
      logicPaint.setColor(SkColorSetRGB(255, 200, 100)); // Gold text for logic
      
      // Format text with proper separators
      juce::String logicText;
      if (note.condition.isNotEmpty() && note.recurrence.isNotEmpty()) {
        logicText = note.condition + " | " + note.recurrence;
      } else if (note.condition.isNotEmpty()) {
        logicText = note.condition;
      } else {
        logicText = note.recurrence;
      }
      
      canvas->drawString(logicText.toRawUTF8(), rect.left() + 4,
                         rect.bottom() - 8, logicFont, logicPaint);
    }

    canvas->drawRRect(rrect, notePaint);
    canvas->drawRRect(rrect, borderPaint);
  }
}

void PianoRollComponent::drawVelocityLane(SkCanvas *canvas,
                                          const SkRect &area) {
  SkPaint divPaint;
  divPaint.setColor(SkColorSetRGB(50, 50, 60));
  divPaint.setStrokeWidth(2.0f);
  canvas->drawLine(area.left(), area.top(), area.right(), area.top(), divPaint);

  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(15, 15, 18));
  canvas->drawRect(area, bgPaint);

  SkPaint stalkPaint;
  stalkPaint.setAntiAlias(true);

  for (const auto &note : noteRects) {
    if (note.bounds.getRight() < area.left() ||
        note.bounds.getX() > area.right())
      continue;
    SkRect vRect = SkRect::MakeXYWH(
        note.velocityBounds.getX(), note.velocityBounds.getY(),
        note.velocityBounds.getWidth(), note.velocityBounds.getHeight());
    if (vRect.bottom() > area.bottom())
      vRect.fBottom = area.bottom();
    if (vRect.top() < area.top())
      vRect.fTop = area.top();

    stalkPaint.setColor(note.selected ? SK_ColorWHITE
                                      : getSkiaColorForVelocity(note.velocity));
    float cx = vRect.centerX();
    canvas->drawLine(cx, vRect.top(), cx, area.bottom(), stalkPaint);
    canvas->drawCircle(cx, vRect.top(), 4.0f, stalkPaint);
  }
}

static juce::String getNoteName(int pitchClass) {
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
  return names[std::abs(pitchClass) % 12];
}

static juce::String getScaleTypeName(PianoRollComponent::ScaleType type) {
  using T = PianoRollComponent::ScaleType;
  switch (type) {
  case T::Chromatic:
    return "Chromatic";
  case T::Major:
    return "Major";
  case T::Minor:
    return "Minor";
  case T::HarmonicMinor:
    return "Harm. Min";
  case T::MelodicMinor:
    return "Mel. Min";
  case T::Dorian:
    return "Dorian";
  case T::Phrygian:
    return "Phrygian";
  case T::Lydian:
    return "Lydian";
  case T::Mixolydian:
    return "Mixolydian";
  case T::Aeolian:
    return "Aeolian";
  case T::Locrian:
    return "Locrian";
  case T::MajorPentatonic:
    return "Maj Pent";
  case T::MinorPentatonic:
    return "Min Pent";
  case T::MajorBlues:
    return "Maj Blues";
  case T::MinorBlues:
    return "Min Blues";
  case T::Bhairav:
    return "Bhairav";
  case T::Byzantine:
    return "Byzantine";
  case T::ManGong:
    return "Man Gong";
  default:
    return "Exotic";
  }
}

void PianoRollComponent::drawModernToolbar(SkCanvas *canvas,
                                           const SkRect &fullRect) {
  SkRect toolbarRect =
      SkRect::MakeXYWH(fullRect.right() - 600.0f, 4.0f, 590.0f, 32.0f);
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(200, 30, 30, 35));
  canvas->drawRoundRect(toolbarRect, 16.0f, 16.0f, bgPaint);

  auto drawBtn = [&](float x, float w, const char *label, bool active) {
    SkRect btnRect = SkRect::MakeXYWH(x, toolbarRect.top() + 4.0f, w, 24.0f);
    SkPaint p;
    p.setColor(active ? SkColorSetRGB(0, 255, 100) : SkColorSetRGB(60, 60, 70));
    p.setAntiAlias(true);
    canvas->drawRoundRect(btnRect, 4.0f, 4.0f, p);

    SkFont f;
    f.setSize(10.0f);
    SkPaint tp;
    tp.setColor(SK_ColorWHITE);
    float tw = f.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, x + (w - tw) / 2, btnRect.centerY() + 3.0f, f,
                       tp);
  };

  float x = toolbarRect.left() + 8.0f;
  float gap = 4.0f;
  drawBtn(x, 40.0f, "SPRAY", sprayCanMode);
  x += 40 + gap;
  drawBtn(x, 35.0f, "RIFF", false);
  x += 35 + gap;
  drawBtn(x, 35.0f, "STEP", stepSequencerMode);
  x += 35 + gap;
  drawBtn(x, 35.0f, "LOCK", scaleLockEnabled);
  x += 35 + gap;
  drawBtn(x, 35.0f, "FOLD", foldMode);
  x += 35 + gap;
  x += 8.0f;

  SkFont f;
  f.setSize(12.0f);
  SkPaint tp;
  tp.setColor(SK_ColorLTGRAY);
  juce::String grooveName = "Straight";
  switch (currentGroove) {
  case GrooveTemplate::Straight:
    grooveName = "Straight";
    break;
  case GrooveTemplate::Swing8th:
    grooveName = "Swing 8th";
    break;
  case GrooveTemplate::Swing16th:
    grooveName = "Swing 16th";
    break;
  case GrooveTemplate::Shuffle:
    grooveName = "Shuffle";
    break;
  case GrooveTemplate::MPC:
    grooveName = "MPC";
    break;
  case GrooveTemplate::JDilla:
    grooveName = "J Dilla";
    break;
  case GrooveTemplate::HipHop:
    grooveName = "Hip Hop";
    break;
  case GrooveTemplate::Funk:
    grooveName = "Funk";
    break;
  }
  canvas->drawString(("Groove: " + grooveName).toRawUTF8(), x,
                     toolbarRect.centerY() + 4.0f, f, tp);
  x += 100.0f + gap;

  // Scale Display
  juce::String rootName = getNoteName(scaleHighlight.rootNote);
  juce::String typeName = getScaleTypeName(scaleHighlight.scale);
  juce::String scaleText = "Key: " + rootName + " " + typeName;
  SkPaint pScale = tp;
  if (scaleLockEnabled)
    pScale.setColor(SkColorSetRGB(0, 255, 100));

  canvas->drawString(scaleText.toRawUTF8(), x, toolbarRect.centerY() + 4.0f, f,
                     pScale);
  x += 120.0f + gap;

  drawBtn(x, 35.0f, "ECHO", false);
  x += 35 + gap;
  drawBtn(x, 40.0f, "STRUM", false);
  x += 40 + gap;
  drawBtn(x, 30.0f, "ARP", arpPreviewEnabled);
  x += 30 + gap;
  drawBtn(x, 42.0f, "CHORD", false);
  x += 42 + gap;

  x += 8.0f;
  drawBtn(x, 24.0f, "<", false);
  x += 24 + gap;
  drawBtn(x, 24.0f, "I", false);
  x += 24 + gap;
  drawBtn(x, 24.0f, "2x", false);
  x += 24 + gap;
}

SkColor PianoRollComponent::getSkiaColorForVelocity(int velocity) const {
  float t = velocity / 127.0f;
  if (t < 0.5f)
    return design::interpolateColor(SkColorSetRGB(0, 50, 200),
                                    SkColorSetRGB(0, 255, 255), t * 2.0f);
  return design::interpolateColor(SkColorSetRGB(0, 255, 255),
                                  SkColorSetRGB(255, 255, 255),
                                  (t - 0.5f) * 2.0f);
}

// Missing Logic Implementations

void PianoRollComponent::transformRetrograde() {
  if (getSelectedNoteCount() == 0)
    return;
  projectState.getUndoManager().beginNewTransaction("Retrograde");

  double minTime = std::numeric_limits<double>::max();
  double maxTime = 0.0;
  std::vector<NoteRect *> selected;

  for (auto &note : noteRects) {
    if (note.selected) {
      minTime = std::min(minTime, note.startBeats);
      maxTime = std::max(maxTime, note.startBeats + note.lengthBeats);
      selected.push_back(&note);
    }
  }

  if (selected.empty())
    return;

  for (auto *note : selected) {
    double newStart =
        minTime + (maxTime - (note->startBeats + note->lengthBeats));
    projectState.moveMidiNote(currentClip.clipId, note->id, newStart,
                              note->pitch, "");
  }
  refreshNotesFromProjectState();
}

void PianoRollComponent::transformInversion(int pivotPitch) {
  if (getSelectedNoteCount() == 0)
    return;
  projectState.getUndoManager().beginNewTransaction("Inversion");

  if (pivotPitch < 0) {
    // Auto detect center
    int minP = 127, maxP = 0;
    for (auto &note : noteRects) {
      if (note.selected) {
        minP = std::min(minP, note.pitch);
        maxP = std::max(maxP, note.pitch);
      }
    }
    pivotPitch = (minP + maxP) / 2;
  }

  for (auto &note : noteRects) {
    if (note.selected) {
      int newPitch = pivotPitch - (note.pitch - pivotPitch);
      newPitch = juce::jlimit(0, 127, newPitch);
      if (scaleLockEnabled)
        newPitch = snapPitchToScale(newPitch);
      projectState.moveMidiNote(currentClip.clipId, note.id, note.startBeats,
                                newPitch, "");
    }
  }
  refreshNotesFromProjectState();
}

void PianoRollComponent::transformTimeStretch(double factor) {
  if (getSelectedNoteCount() == 0 || factor <= 0.0)
    return;
  projectState.getUndoManager().beginNewTransaction("Time Stretch");

  double minTime = std::numeric_limits<double>::max();
  for (auto &note : noteRects)
    if (note.selected)
      minTime = std::min(minTime, note.startBeats);

  for (auto &note : noteRects) {
    if (note.selected) {
      double offset = note.startBeats - minTime;
      double newStart = minTime + offset * factor;
      double newLength = note.lengthBeats * factor;
      projectState.moveMidiNote(currentClip.clipId, note.id, newStart,
                                note.pitch, "");
      projectState.setMidiNoteLength(currentClip.clipId, note.id, newLength,
                                     "");
    }
  }
  refreshNotesFromProjectState();
}

// Logic Stubs / Basic Impls for Linker

void PianoRollComponent::resized() {
  // Layout handled by paint or fixed areas
}

void PianoRollComponent::setStepSequencerMode(bool enabled) {
  stepSequencerMode = enabled;
  if (stepSequencerMode && stepSequencerRows.empty()) {
    // Default to GM Drum Kit Range: C1 (36) to D#2 (51)
    // Ordered High-to-Low for visual consistency (Kick at bottom)
    for (int i = 0; i < 16; ++i) {
      stepSequencerRows.push_back(36 + (15 - i)); // 51 down to 36
    }
  }
  repaint();
}

void PianoRollComponent::toggleStep(int row, int step) {
  if (row < 0 || row >= (int)stepSequencerRows.size())
    return;

  int pitch = stepSequencerRows[row];
  double stepTime = step * 0.25; // Assume 16th notes (1/4 beat)

  bool removed = false;
  // Check for existing note to toggle off
  // Iterate backwards to safely remove?
  // We use IDs to remove, so safe.
  for (const auto &note : noteRects) {
    if (note.pitch == pitch && std::abs(note.startBeats - stepTime) < 0.1) {
      projectState.removeMidiNote(currentClip.clipId, note.id, "Toggle Step");
      removed = true;
      break; // Single note per cell assumption
    }
  }

  if (!removed) {
    zenith::ProjectState::MidiNoteSpec newNote;
    newNote.pitch = pitch;
    newNote.startBeats = stepTime;
    newNote.lengthBeats = 0.25;
    newNote.velocity = 100;
    newNote.muted = false;
    projectState.addMidiNote(currentClip.clipId, newNote, "Step Sequencer");
  } else {
    refreshNotesFromProjectState(); // Explicit refresh if removed
  }
  // addMidiNote triggers refresh via listener, but remove might not?
  // Safety refresh
  refreshNotesFromProjectState();
  repaint();
}

void PianoRollComponent::applyStrumming(StrumDirection dir, double amount) {
  if (getSelectedNoteCount() < 2)
    return;

  projectState.getUndoManager().beginNewTransaction("Strum");

  // Randomness for humanization
  std::random_device rd;
  std::mt19937 gen(rd());
  // +/- 0.005 beats (~2-3ms at 120bpm)
  std::uniform_real_distribution<> jitter(-0.005, 0.005);

  // Group selected notes by roughly same start time
  std::map<int, std::vector<NoteRect *>> chords;
  for (auto &note : noteRects) {
    if (note.selected) {
      int quantTime = (int)(note.startBeats * 100); // 0.01 tolerance
      chords[quantTime].push_back(&note);
    }
  }

  for (auto &[time, notes] : chords) {
    if (notes.size() < 2)
      continue;

    // Sort by pitch
    std::sort(notes.begin(), notes.end(),
              [](NoteRect *a, NoteRect *b) { return a->pitch < b->pitch; });

    if (dir == StrumDirection::Up) { // High to Low
      std::reverse(notes.begin(), notes.end());
    }

    for (size_t i = 0; i < notes.size(); ++i) {
      double offset = i * amount + jitter(gen);
      projectState.moveMidiNote(currentClip.clipId, notes[i]->id,
                                notes[i]->startBeats + offset, notes[i]->pitch,
                                "");
      // Optional: Velocity curve
      // Simulate energy loss across strings
      int velChange =
          (dir == StrumDirection::Down) ? (-5 * (int)i) : (-5 * (int)i);
      int newVel = juce::jlimit(1, 127, notes[i]->velocity + velChange);
      projectState.setMidiNoteVelocity(currentClip.clipId, notes[i]->id, newVel,
                                       "");
    }
  }
  refreshNotesFromProjectState();
}

void PianoRollComponent::extendMelody(int bars) {
  if (noteRects.empty())
    return;

  // Better Random
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> mutationDist(1, 10);
  std::uniform_int_distribution<> shiftDist(0, 2); // 0,1,2 -> -1,0,1

  double maxTime = 0;
  for (const auto &n : noteRects)
    maxTime = std::max(maxTime, n.startBeats + n.lengthBeats);

  projectState.getUndoManager().beginNewTransaction("Extend Melody");

  // Calculate Bar Length dynamically
  int num = projectState.getTimeSignatureNumerator();
  int den = projectState.getTimeSignatureDenominator();
  double beatsPerBar = (double)num * (4.0 / den);
  double lookback = (double)bars * beatsPerBar;

  double startRange = std::max(0.0, maxTime - lookback);

  for (const auto &n : noteRects) {
    if (n.startBeats >= startRange) {
      zenith::ProjectState::MidiNoteSpec newNote;
      newNote.pitch = n.pitch;
      // Variation
      if (mutationDist(gen) > 7) {
        int shift = shiftDist(gen) - 1;
        newNote.pitch = snapPitchToScale(newNote.pitch + shift);
      }
      newNote.startBeats = n.startBeats + lookback;
      newNote.lengthBeats = n.lengthBeats;
      newNote.velocity = n.velocity;
      projectState.addMidiNote(currentClip.clipId, newNote, "");
    }
  }
  refreshNotesFromProjectState();
}

void PianoRollComponent::drawStepSequencer(SkCanvas *canvas,
                                           const SkRect &area) {
  if (stepSequencerRows.empty())
    return;

  // Draw Dark Background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(25, 27, 33));
  canvas->drawRect(area, bgPaint);

  float cellW = area.width() / (float)stepSequencerSteps;
  float cellH = area.height() / (float)stepSequencerRows.size();

  SkPaint strokePaint;
  strokePaint.setColor(SkColorSetARGB(30, 255, 255, 255));
  strokePaint.setStyle(SkPaint::kStroke_Style);
  strokePaint.setStrokeWidth(1.0f);

  SkPaint activePaint;
  activePaint.setColor(SkColorSetRGB(0, 255, 180)); // Neon Cyan
  activePaint.setStyle(SkPaint::kFill_Style);

  // OPTIMIZATION: Pre-calculate active steps to avoid O(N) lookup in drawing
  // loop
  std::map<int, std::set<int>> activeSteps;
  for (const auto &note : noteRects) {
    // Quantize to 16th notes (0.25)
    int stepIdx = (int)std::floor(note.startBeats / 0.25 + 0.5);
    if (stepIdx >= 0 && stepIdx < stepSequencerSteps) {
      activeSteps[note.pitch].insert(stepIdx);
    }
  }

  for (int row = 0; row < (int)stepSequencerRows.size(); ++row) {
    int pitch = stepSequencerRows[row];
    // Highlight C notes
    if (pitch % 12 == 0) {
      SkRect rowRect = SkRect::MakeXYWH(area.left(), area.top() + row * cellH,
                                        area.width(), cellH);
      SkPaint cLabelPaint;
      cLabelPaint.setColor(SkColorSetARGB(20, 255, 255, 255));
      canvas->drawRect(rowRect, cLabelPaint);
    }

    bool rowHasNotes = activeSteps.count(pitch);

    for (int step = 0; step < stepSequencerSteps; ++step) {
      SkRect cell = SkRect::MakeXYWH(area.left() + step * cellW,
                                     area.top() + row * cellH, cellW, cellH);

      // Draw active if present in map
      if (rowHasNotes && activeSteps[pitch].count(step)) {
        canvas->drawRect(cell.makeInset(2, 2), activePaint);
      } else {
        canvas->drawRect(cell, strokePaint);
      }

      // beat markers (every 4 steps)
      if (step % 4 == 0) {
        SkPaint beatMarker;
        beatMarker.setColor(SkColorSetARGB(50, 255, 255, 255));
        beatMarker.setStrokeWidth(2.0f);
        canvas->drawLine(cell.left(), cell.top(), cell.left(), cell.bottom(),
                         beatMarker);
      }
    }
  }
}

void PianoRollComponent::drawArpPreview(SkCanvas *canvas, const SkRect &area) {
  if (!arpPreviewEnabled || getSelectedNoteCount() < 1)
    return;

  // Visualize a ghostly UP pattern based on selected chords
  SkPaint previewPaint;
  previewPaint.setColor(SkColorSetARGB(80, 255, 100, 200)); // Pink ghost
  previewPaint.setStyle(SkPaint::kFill_Style);

  for (const auto &note : noteRects) {
    if (!note.selected)
      continue;

    // Draw 3 ghost notes ascending from this note
    for (int i = 1; i <= 3; ++i) {
      // Simple Up Arp Logic simulation
      float x = beatsToPixels(note.startBeats + i * 0.25) + PIANO_WIDTH;
      // Shift pitch up by scale degrees? Just octaves or 3rds for visual
      float y = pitchToPixels(note.pitch + (i * 4)) +
                RULER_HEIGHT; // +Major 3rds stacking

      if (x > area.right())
        continue;

      SkRect ghost =
          SkRect::MakeXYWH(x, y, beatsToPixels(0.25), pixelsPerPitch);
      canvas->drawRect(ghost, previewPaint);

      // Connector line
      SkPaint linePaint;
      linePaint.setColor(SkColorSetARGB(50, 255, 100, 200));
      linePaint.setStrokeWidth(1.0f);

      float prevX =
          beatsToPixels(note.startBeats + (i - 1) * 0.25) + PIANO_WIDTH;
      float prevY = pitchToPixels(note.pitch + ((i - 1) * 4)) + RULER_HEIGHT;
      if (i == 1) { // Connect to original
        prevY = pitchToPixels(note.pitch) + RULER_HEIGHT;
        prevX = beatsToPixels(note.startBeats) + PIANO_WIDTH;
      }
      canvas->drawLine(prevX + 10, prevY + pixelsPerPitch / 2, x + 10,
                       y + pixelsPerPitch / 2, linePaint);
    }
  }
}

// Ghost Notes Implementations

void PianoRollComponent::setGhostNotesEnabled(bool enabled) {
  ghostNotesEnabled = enabled;
  repaint();
}

void PianoRollComponent::setGhostNoteOpacity(float opacity) {
  ghostNoteOpacity = opacity;
  repaint();
}

void PianoRollComponent::addGhostClip(const juce::String &clipId) {
  ghostClipIds.push_back(clipId);
  refreshGhostNotes();
}

void PianoRollComponent::removeGhostClip(const juce::String &clipId) {
  auto it = std::remove(ghostClipIds.begin(), ghostClipIds.end(), clipId);
  ghostClipIds.erase(it, ghostClipIds.end());
  refreshGhostNotes();
}

void PianoRollComponent::clearGhostClips() {
  ghostClipIds.clear();
  refreshGhostNotes();
}

void PianoRollComponent::refreshGhostNotes() {
  ghostNotes.clear();
  if (!ghostNotesEnabled)
    return;

  for (const auto &cid : ghostClipIds) {
    auto notes = projectState.getMidiNotesForClip(cid);
    for (const auto &n : notes) {
      GhostNote gn;
      gn.pitch = n.pitch;
      gn.startBeats = n.startBeats;
      gn.lengthBeats = n.lengthBeats;
      ghostNotes.push_back(gn);
    }
  }
  repaint();
}

void PianoRollComponent::drawGhostNotes(SkCanvas *canvas, const SkRect &area) {
  if (ghostNotes.empty())
    return;

  SkPaint paint;
  paint.setColor(SkColorSetA(SK_ColorLTGRAY, (int)(ghostNoteOpacity * 255)));
  paint.setAntiAlias(true);

  for (const auto &gn : ghostNotes) {
    if (gn.startBeats < pixelsToBeats(area.left() - PIANO_WIDTH))
      continue;
    if (gn.startBeats > pixelsToBeats(area.right() - PIANO_WIDTH))
      continue;

    float x = beatsToPixels(gn.startBeats) + PIANO_WIDTH;
    float y = pitchToPixels(gn.pitch) + RULER_HEIGHT;
    float w = beatsToPixels(gn.lengthBeats);
    float h = pixelsPerPitch;

    canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), paint);
  }
}

void PianoRollComponent::fillEuclideanRow(int row, int pulses, int offset) {
  if (row < 0 || row >= (int)stepSequencerRows.size())
    return;
  int pitch = stepSequencerRows[row];
  int steps = 16; // Standard length for now

  // Clear existing notes in this row
  // (Filter noteRects or logic? Better to iterate and remove)
  // Simple approach: Remove all matching pitch in current bar
  for (const auto &note : noteRects) {
    if (note.pitch == pitch && note.startBeats < 4.0) {
      projectState.removeMidiNote(currentClip.clipId, note.id, "Clear Row");
    }
  }

  // Bjorklund / Euclidean Generation
  // Formula: pulse at step i if (i * pulses) % steps < pulses
  for (int i = 0; i < steps; ++i) {
    if (((i * pulses) % steps) < pulses) {
      // Shift by offset
      int currentStep = (i + offset) % steps;

      zenith::ProjectState::MidiNoteSpec newNote;
      newNote.pitch = pitch;
      newNote.startBeats = currentStep * 0.25;
      newNote.lengthBeats = 0.25;
      newNote.velocity = 100;
      projectState.addMidiNote(currentClip.clipId, newNote, "Euclidean Fill");
    }
  }
  refreshNotesFromProjectState();
  repaint();
}

void PianoRollComponent::autoHarmonize(HarmonyType harmony) {
  if (getSelectedNoteCount() == 0)
    return;

  projectState.getUndoManager().beginNewTransaction("Harmonize");

  // Determine intervals to add
  std::vector<int> scaleIntervals;     // Steps in scale
  std::vector<int> chromaticIntervals; // Semitones

  bool isDrop2 = (harmony == HarmonyType::Drop2);

  switch (harmony) {
  case HarmonyType::Thirds:
    scaleIntervals = {2};     // +2 steps (3rd)
    chromaticIntervals = {4}; // +4 semitones (Maj 3rd default)
    break;
  case HarmonyType::Fifths:
  case HarmonyType::Power:
    scaleIntervals = {4};     // +4 steps (5th)
    chromaticIntervals = {7}; // +7 semitones
    break;
  case HarmonyType::Octaves:
    scaleIntervals = {7};
    chromaticIntervals = {12};
    break;
  case HarmonyType::Triad:
    scaleIntervals = {2, 4};
    chromaticIntervals = {4, 7};
    break;
  case HarmonyType::Drop2:
    // Parallel Tetrad: 1, 3, 5, 7
    // Then we modify logic below to drop the 2nd highest
    scaleIntervals = {2, 4, 6};      // +3rd, +5th, +7th
    chromaticIntervals = {4, 7, 11}; // Maj7 default
    break;
  }

  for (const auto &note : noteRects) {
    if (!note.selected)
      continue;

    const std::vector<int> &intervals =
        scaleLockEnabled ? scaleIntervals : chromaticIntervals;

    // Collect generated notes for this root note
    std::vector<int> chordPitches;
    chordPitches.push_back(note.pitch);

    for (int interval : intervals) {
      int newPitch = note.pitch;

      if (scaleLockEnabled) {
        // Walk up scale degrees
        int steps = 0;
        int p = note.pitch;
        while (steps < interval && p < 127) {
          p++;
          if (isNoteInScale(p))
            steps++;
        }
        newPitch = p;
      } else {
        newPitch = juce::jlimit(0, 127, note.pitch + interval);
      }
      chordPitches.push_back(newPitch);
    }

    if (isDrop2 && chordPitches.size() >= 4) {
      // Drop-2 Logic:
      // chordPitches are sorted Low to High (Root, 3rd, 5th, 7th)
      // 2nd Highest is 5th (Index 2).
      // Drop it an octave.
      int secondHighestIdx = (int)chordPitches.size() - 2;
      if (secondHighestIdx >= 0) {
        chordPitches[secondHighestIdx] -= 12;
      }
    }

    // Apply notes (skip original/root as it exists)
    for (size_t i = 1; i < chordPitches.size(); ++i) {
      int p = chordPitches[i];
      if (p != note.pitch) {
        zenith::ProjectState::MidiNoteSpec newNote;
        newNote.pitch = p;
        newNote.startBeats = note.startBeats;
        newNote.lengthBeats = note.lengthBeats;
        newNote.velocity = (int)(note.velocity * 0.85f);
        projectState.addMidiNote(currentClip.clipId, newNote, "");
      }
    }
  }
  refreshNotesFromProjectState();
}

void PianoRollComponent::updateScaleLockNotes() {}

void PianoRollComponent::drawChordName(SkCanvas *canvas) {
  juce::String chord = getCurrentChordName();
  if (chord.isEmpty())
    return;

  SkPaint paint;
  paint.setColor(SkColorSetARGB(180, 255, 255, 255));
  paint.setAntiAlias(true);
  SkFont font;
  font.setSize(14.0f);

  canvas->drawString(chord.toRawUTF8(), PIANO_WIDTH + 150.0f, 24.0f, font,
                     paint);
}

static std::vector<int> getScaleIntervals(PianoRollComponent::ScaleType type) {
  using T = PianoRollComponent::ScaleType;
  switch (type) {
  case T::Chromatic:
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  case T::Major:
    return {0, 2, 4, 5, 7, 9, 11};
  case T::Minor:
    return {0, 2, 3, 5, 7, 8, 10};
  case T::HarmonicMinor:
    return {0, 2, 3, 5, 7, 8, 11};
  case T::MelodicMinor:
    return {0, 2, 3, 5, 7, 9, 11};
  case T::Dorian:
    return {0, 2, 3, 5, 7, 9, 10};
  case T::Phrygian:
    return {0, 1, 3, 5, 7, 8, 10};
  case T::Lydian:
    return {0, 2, 4, 6, 7, 9, 11};
  case T::Mixolydian:
    return {0, 2, 4, 5, 7, 9, 10};
  case T::Aeolian:
    return {0, 2, 3, 5, 7, 8, 10};
  case T::Locrian:
    return {0, 1, 3, 5, 6, 8, 10};

  // Pentatonic
  case T::MajorPentatonic:
    return {0, 2, 4, 7, 9};
  case T::MinorPentatonic:
    return {0, 3, 5, 7, 10};
  case T::Egyptian:
    return {0, 2, 5, 7, 10};
  case T::ManGong:
    return {0, 1, 5, 7, 10};
  case T::Ritusen:
    return {0, 2, 5, 7, 9};
  case T::Hirajoshi:
    return {0, 2, 3, 7, 8};
  case T::InSen:
    return {0, 1, 5, 7, 10};
  case T::Iwato:
    return {0, 1, 5, 6, 10};
  case T::YoScale:
    return {0, 2, 5, 7, 9};

  // Blues
  case T::MajorBlues:
    return {0, 2, 3, 4, 7, 9};
  case T::MinorBlues:
    return {0, 3, 5, 6, 7, 10};

  // Symmetrical
  case T::WholeTone:
    return {0, 2, 4, 6, 8, 10};
  case T::DiminishedHalfWhole:
    return {0, 1, 3, 4, 6, 7, 9, 10};
  case T::DiminishedWholeHalf:
    return {0, 2, 3, 5, 6, 8, 9, 11};
  case T::Augmented:
    return {0, 3, 4, 7, 8, 11};
  case T::Prometheus:
    return {0, 2, 4, 6, 9, 10};

  // Bebop
  case T::BebopMajor:
    return {0, 2, 4, 5, 7, 8, 9, 11};
  case T::BebopMinor:
    return {0, 2, 3, 4, 5, 7, 9, 10, 11};
  case T::BebopDominant:
    return {0, 2, 4, 5, 7, 9, 10, 11};
  case T::BebopDorian:
    return {0, 2, 3, 4, 5, 7, 9, 10, 11};

  // Exotic
  case T::HungarianMinor:
    return {0, 2, 3, 6, 7, 8, 11};
  case T::HungarianMajor:
    return {0, 3, 4, 6, 7, 9, 10};
  case T::Bhairav:
    return {0, 1, 4, 5, 7, 8, 11};
  case T::Byzantine:
    return {0, 1, 4, 5, 7, 8, 11};
  case T::Persian:
    return {0, 1, 4, 5, 6, 8, 11};
  case T::Arabian:
    return {0, 2, 4, 5, 6, 8, 10};
  case T::Japanese:
    return {0, 1, 5, 7, 8};
  case T::Chinese:
    return {0, 4, 6, 7, 11};
  case T::Balinese:
    return {0, 1, 3, 7, 8};
  case T::NeapolitanMajor:
    return {0, 1, 3, 5, 7, 9, 11};
  case T::NeapolitanMinor:
    return {0, 1, 3, 5, 7, 8, 11};
  case T::Enigmatic:
    return {0, 1, 4, 6, 8, 10, 11};
  case T::DoubleHarmonic:
    return {0, 1, 4, 5, 7, 8, 11};
  case T::SpanishGypsy:
    return {0, 1, 4, 5, 7, 8, 10};
  case T::Algierian:
    return {0, 2, 3, 5, 6, 7, 8, 11};

  default:
    return {0, 2, 4, 5, 7, 9, 11};
  }
}

void PianoRollComponent::setScaleHighlight(int rootNote, ScaleType scale) {
  scaleHighlight.rootNote = rootNote;
  scaleHighlight.scale = scale;
  scaleHighlight.enabled = true;
  updateScaleHighlight();
  repaint();
}

void PianoRollComponent::clearScaleHighlight() {
  scaleHighlight.enabled = false;
  repaint();
}

void PianoRollComponent::updateScaleHighlight() {
  auto intervals = getScaleIntervals(scaleHighlight.scale);
  scaleHighlight.highlightedPitches.assign(128, false);

  if (!scaleHighlight.enabled)
    return;

  for (int i = 0; i < 128; ++i) {
    int pitchClass = (i - scaleHighlight.rootNote) % 12;
    if (pitchClass < 0)
      pitchClass += 12;

    for (int interval : intervals) {
      if (interval == pitchClass) {
        scaleHighlight.highlightedPitches[i] = true;
        break;
      }
    }
  }
}

void PianoRollComponent::setScaleLock(bool enabled) {
  scaleLockEnabled = enabled;
  if (enabled && !scaleHighlight.enabled) {
    setScaleHighlight(0, ScaleType::Major);
  }
}

void PianoRollComponent::setScaleLockKey(int rootNote, ScaleType scale) {
  setScaleHighlight(rootNote, scale);
  setScaleLock(true);
}

int PianoRollComponent::snapPitchToScale(int pitch) const {
  if (!scaleLockEnabled || !scaleHighlight.enabled)
    return pitch;

  if (scaleHighlight.highlightedPitches[pitch])
    return pitch;

  // Search up and down
  for (int i = 1; i < 12; ++i) {
    int up = pitch + i;
    int down = pitch - i;

    if (up <= 127 && scaleHighlight.highlightedPitches[up]) {
      return up;
    }
    if (down >= 0 && scaleHighlight.highlightedPitches[down]) {
      return down;
    }
  }
  return pitch;
}

void PianoRollComponent::quantizeToScale() {
  if (!scaleLockEnabled)
    return;

  projectState.getUndoManager().beginNewTransaction("Quantize to Scale");
  bool changed = false;

  for (const auto &note : noteRects) {
    if (note.selected) {
      int newPitch = snapPitchToScale(note.pitch);
      if (newPitch != note.pitch) {
        projectState.moveMidiNote(currentClip.clipId, note.id, note.startBeats,
                                  newPitch, "");
        changed = true;
      }
    }
  }

  if (changed)
    refreshNotesFromProjectState();
}

bool PianoRollComponent::isNoteInScale(int pitch) const {
  if (!scaleLockEnabled && !scaleHighlight.enabled)
    return true;
  if (scaleHighlight.highlightedPitches.empty())
    return true;
  if (pitch < 0 || pitch > 127)
    return false;
  return scaleHighlight.highlightedPitches[pitch];
}

void PianoRollComponent::updateVisiblePitches() {
  visiblePitches.clear();

  if (!foldMode) {
    // Logic handled dynamically in conversion, but populate for consistency
    // if needed? No, optimize by keeping empty if not folded, or populate all
    // if logic demands. But mapPitchToRow needs to work.
    return;
  }

  std::set<int> pitchSet;

  // 1. Add notes currently in clips
  for (const auto &note : noteRects) {
    pitchSet.insert(note.pitch);
  }

  // 2. Add Scale notes if Scale Lock/Highlighter is active
  if (scaleLockEnabled || scaleHighlight.enabled) {
    updateScaleHighlight(); // Ensure pitches are current
    for (int i = 0; i < 128; ++i) {
      if (scaleHighlight.highlightedPitches[i]) {
        pitchSet.insert(i);
      }
    }
  }

  // Convert to vector
  visiblePitches.assign(pitchSet.begin(), pitchSet.end());
  std::sort(visiblePitches.begin(), visiblePitches.end());
}

int PianoRollComponent::mapPitchToRow(int pitch) const {
  if (!foldMode)
    return pitch;
  
  // Guard against empty visiblePitches
  if (visiblePitches.empty())
    return 0; // Return safe default
    
  auto it = std::find(visiblePitches.begin(), visiblePitches.end(), pitch);
  if (it != visiblePitches.end()) {
    return (int)std::distance(visiblePitches.begin(), it);
  }
  // If pitch not visible, map to nearest
  auto lower =
      std::lower_bound(visiblePitches.begin(), visiblePitches.end(), pitch);
  if (lower == visiblePitches.end())
    return (int)visiblePitches.size() - 1;
  int row = (int)std::distance(visiblePitches.begin(), lower);
  // Ensure row is within bounds
  return juce::jlimit(0, (int)visiblePitches.size() - 1, row);
}

int PianoRollComponent::mapRowToPitch(int row) const {
  if (!foldMode)
    return row;
  if (visiblePitches.empty())
    return 60;
  row = juce::jlimit(0, (int)visiblePitches.size() - 1, row);
  return visiblePitches[row];
}

//==============================================================================
// Timer Callback (for playhead updates)
//==============================================================================

void PianoRollComponent::rebuildSpatialGrid() {
  if (!currentClip.isValid() || currentClip.clipLengthBeats <= 0.0) {
    spatialGrid.clear();
    spatialGridDirty = false;
    return;
  }

  spatialGrid.clear();

  // Only rebuild if we have enough notes to benefit from spatial indexing
  if (noteRects.size() > 10) {
    for (auto &note : noteRects) {
      spatialGrid.addNote(&note, currentClip.clipLengthBeats);
    }
  }

  spatialGridDirty = false;
}

void PianoRollComponent::timerCallback() {
  // Update playhead position if we have a callback to get it
  // Note: This requires Engine* to be passed in or accessed via ProjectState
  // For now, we'll rely on external calls to updatePlayheadPosition()
  // This timer can be used for other periodic updates if needed

  // Periodic refresh of note collisions (only if needed)
  if (getCollisionCount() > 0) {
    detectNoteCollisions();
    repaint();
  }

  // Rebuild spatial grid if dirty
  if (spatialGridDirty) {
    rebuildSpatialGrid();
  }
}

void PianoRollComponent::transformTransposeInScale(int steps) {
  if (!scaleLockEnabled || steps == 0)
    return;

  projectState.getUndoManager().beginNewTransaction("Transpose In Scale");

  // Build scale indices
  std::vector<int> scalePitches;
  for (int i = 0; i < 128; ++i)
    if (scaleHighlight.highlightedPitches[i])
      scalePitches.push_back(i);

  if (scalePitches.empty())
    return;

  for (const auto &note : noteRects) {
    if (note.selected) {
      // Find current index
      int snapped = this->snapPitchToScale(note.pitch);
      auto it =
          std::lower_bound(scalePitches.begin(), scalePitches.end(), snapped);
      if (it != scalePitches.end()) {
        int idx = (int)std::distance(scalePitches.begin(), it);
        int newIdx = idx + steps;
        // Clamp
        if (newIdx >= 0 && newIdx < (int)scalePitches.size()) {
          int newPitch = scalePitches[newIdx];
          projectState.moveMidiNote(currentClip.clipId, note.id,
                                    note.startBeats, newPitch, "");
        }
      }
    }
  }
  refreshNotesFromProjectState();
}

//==============================================================================
// Multi-Clip Editing
//==============================================================================

void PianoRollComponent::addMultiClipContext(const MidiClipContext &clip) {
  // Avoid duplicates
  for (const auto &c : multiClipContexts) {
    if (c.clipId == clip.clipId)
      return;
  }
  multiClipContexts.push_back(clip);
  refreshNotesFromProjectState();
}

void PianoRollComponent::clearMultiClipContexts() {
  multiClipContexts.clear();
  refreshNotesFromProjectState();
}

//==============================================================================
// Riff Machine
//==============================================================================

void PianoRollComponent::generateRiff(RiffSettings settings) {
  if (!currentClip.isValid())
    return;

  std::vector<int> intervals = getScaleIntervals(settings.scale);
  std::vector<int> scaleNotes;

  // Build full scale map 0-127
  for (int i = 0; i < 128; ++i) {
    int note = i % 12;
    int root = settings.rootNote % 12;
    int interval = (note - root + 12) % 12;

    bool inScale = false;
    for (int iv : intervals) {
      if (iv == interval) {
        inScale = true;
        break;
      }
    }
    if (inScale)
      scaleNotes.push_back(i);
  }

  if (scaleNotes.empty())
    return;

  projectState.getUndoManager().beginNewTransaction("Generate Riff");

  double duration = 4.0;
  double step = 0.25; // 16th

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0, 1.0);
  std::uniform_int_distribution<> octaveDis(0, 2);

  for (double t = 0; t < duration; t += step) {
    if (dis(gen) > settings.density)
      continue;

    int baseIndex = (int)(scaleNotes.size() / 2);
    int walk = (int)((dis(gen) - 0.5) * 10 * settings.variation);
    int index = juce::jlimit(0, (int)scaleNotes.size() - 1,
                             baseIndex + walk + (octaveDis(gen) * 7));

    int pitch = scaleNotes[index];
    pitch = juce::jlimit(0, 127, pitch);

    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = pitch;
    note.startBeats = t;
    note.lengthBeats = step * 0.9;
    note.velocity = juce::jlimit(MIN_VELOCITY, MAX_VELOCITY, 
                                 100 + static_cast<int>((dis(gen) - 0.5) * 20));
    // Validate probability: clamp to [0.0, 1.0]
    note.probability = juce::jlimit(0.0f, 1.0f, 1.0f - (dis(gen) * 0.1f));
    note.articulationId = 0; // Default articulation
    note.condition = juce::String(); // Empty by default

    if (dis(gen) > 0.8)
      note.recurrence = "1:2"; // Valid recurrence string

    // All notes added within single transaction (beginNewTransaction called above)
    projectState.addMidiNote(currentClip.clipId, note, "");
  }

  refreshNotesFromProjectState();
}
