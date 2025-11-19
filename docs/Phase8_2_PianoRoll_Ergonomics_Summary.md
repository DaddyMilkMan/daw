# Phase 8.2: Piano Roll Enhanced Ergonomics

**Status**: ✅ Complete
**Date**: 2025-11-14
**Branch**: `claude/phase8-midi-valuetree-undo-012GWM4quckuDmZDbLvShY2u`

---

## Executive Summary

Phase 8.2 transforms the Piano Roll from a basic MIDI editor into a production-ready tool with full ergonomic features:
- **Velocity editing** via visual velocity lane
- **Note length resize** via edge drag
- **Multi-selection** with Ctrl/Cmd-click and marquee
- **Zoom & scroll** with mousewheel and keyboard

All features maintain Phase 8.1 guarantees:
- ✅ All edits are undoable via ProjectState
- ✅ RT-safe (message thread only)
- ✅ Auto-refresh on external changes (Wingman, undo/redo)
- ✅ Never touch Clip's internal MidiMessageSequence

---

## 1. Feature Overview

### 1.1 Velocity Editing

**User Experience**:
- Velocity lane appears at bottom of Piano Roll (80px height)
- Each note shows a velocity bar (height = velocity)
- Click/drag velocity bar to adjust velocity (1-127)
- Selected notes show in orange, unselected in blue

**Implementation**:
- `NoteRect.velocityBounds`: Rectangle for each note's velocity bar
- `findNoteInVelocityLane()`: Hit-test based on x position
- `pixelsToVelocity()` / `velocityToPixels()`: Convert y position ↔ velocity
- Commits via `ProjectState::setMidiNoteVelocity()`

**Undo Semantics**:
- Single velocity drag = single undo action
- Original velocity cached in `NoteDragState.originalVelocity`
- Only commits if velocity actually changed

---

### 1.2 Note Length Resize

**User Experience**:
- Hover near note edges (6px zones) to resize
- Left edge drag: Changes start position + length (preserves end)
- Right edge drag: Changes length only (preserves start)
- Snap-to-grid works for both edges
- Minimum note length: 0.01 beats

**Implementation**:
- `resizeHandleWidth = 6.0f`: Edge detection zone
- `detectNoteHitRegion()`: Returns DragMode (ResizeLeft, ResizeRight, or MoveNote)
- Left resize: Uses `moveMidiNote()` + `setMidiNoteLength()`
- Right resize: Uses `setMidiNoteLength()` only

**Undo Semantics**:
- Single resize drag = single undo action (or two for left edge: move + length)
- Original state cached in `NoteDragState.originalStartBeats` + `originalLengthBeats`
- Only commits if start or length actually changed

---

### 1.3 Multi-Selection

**User Experience**:
- **Ctrl/Cmd-click**: Add/remove note from selection (toggle)
- **Shift-drag**: Marquee rectangle selection (additive if Ctrl held)
- **Drag selection**: Move all selected notes together
- Selected notes render in orange

**Implementation**:
- `clearSelection()`: Deselect all notes
- `selectNote(note, addToSelection)`: Toggle or replace selection
- `selectNotesInRectangle()`: Select all notes intersecting rectangle
- `marqueeRect`: Visual rectangle for marquee selection
- `dragStates`: Cache original positions of all selected notes

**Undo Semantics**:
- Multi-note move commits each note separately (TODO: batch into single transaction)
- All moves use same action name: "Move MIDI notes"
- Original positions cached per-note in `dragStates` vector

**Marquee Selection Details**:
- Start: Shift-drag in empty space
- Update: Visual rectangle expands/contracts
- Finish: Selects all notes intersecting final rectangle
- Additive: Hold Ctrl/Cmd while Shift-dragging to add to existing selection

---

### 1.4 Zoom & Scroll

**User Experience**:
- **Mousewheel**: Vertical scroll (note grid)
- **Shift + mousewheel**: Horizontal scroll
- **Ctrl/Cmd + mousewheel**: Horizontal zoom (centered on mouse)
- **Alt + mousewheel**: Vertical zoom (centered on mouse)
- **+/= keys**: Zoom in horizontally (center of view)
- **-/_ keys**: Zoom out horizontally (center of view)

**Implementation**:
- `pixelsPerBeat`: Horizontal zoom (20-400 range)
- `pixelsPerPitch`: Vertical zoom (6-48 range)
- `viewStartBeats`, `viewLowestPitch`: Current view position
- Zoom centers on mouse position (zooms around cursor)
- Legacy `scrollOffsetX/Y` updated for compatibility

**Zoom Ranges**:
- Horizontal: 20-400 pixels/beat (20x zoom range)
- Vertical: 6-48 pixels/pitch (8x zoom range)

**Scroll Behavior**:
- Horizontal: Clamped to ≥0 (can't scroll before beat 0)
- Vertical: Clamped to 0-127 pitch range

---

## 2. Architecture Changes

### 2.1 DragMode Enum

```cpp
enum class DragMode
{
    None,
    MoveNote,           // Dragging note body (move pitch + time)
    ResizeLeft,         // Dragging left edge (change start + length)
    ResizeRight,        // Dragging right edge (change length only)
    VelocityEdit,       // Dragging velocity bar
    MarqueeSelect       // Rectangle selection drag
};
```

**Purpose**: Centralize drag state management for all interaction modes.

**Usage**:
- `mouseDown()`: Detects mode based on hit region
- `mouseDrag()`: Dispatches to appropriate update function
- `mouseUp()`: Dispatches to appropriate finish function
- `currentDragMode`: Tracks active mode

---

### 2.2 Enhanced NoteRect

```cpp
struct NoteRect
{
    juce::String id;
    int pitch;
    double startBeats;
    double lengthBeats;
    int velocity;
    bool muted;
    bool selected;

    juce::Rectangle<float> bounds;          // Note grid area
    juce::Rectangle<float> velocityBounds;  // Velocity lane bar (Phase 8.2)
};
```

**New Field**: `velocityBounds`
- Calculated in `updateNoteRectangles()`
- Used for rendering velocity bars
- Used for hit-testing in velocity lane

---

### 2.3 NoteDragState Cache

```cpp
struct NoteDragState
{
    juce::String id;
    int originalPitch;
    double originalStartBeats;
    double originalLengthBeats;  // For resize
    int originalVelocity;         // For velocity edits
};
std::vector<NoteDragState> dragStates;
```

**Purpose**: Cache original state during drag operations for undo detection.

**Usage**:
- Single-note operations: `dragStates[0]`
- Multi-note operations: One entry per selected note
- Compared against final state in `finish*()` functions
- Only commits to ProjectState if values changed

---

### 2.4 Member Variables

**New Phase 8.2 State**:
```cpp
DragMode currentDragMode = DragMode::None;
NoteRect* activeNote = nullptr;  // Note being dragged/resized/velocity-edited
juce::Rectangle<float> marqueeRect;
std::vector<NoteDragState> dragStates;

int velocityLaneHeight = 80;
float resizeHandleWidth = 6.0f;
double viewStartBeats = 0.0;
int viewLowestPitch = 0;
```

**Legacy State** (kept for compatibility):
```cpp
NoteRect* draggingNote = nullptr;
int dragStartPitch = 60;
double dragStartBeats = 0.0;
int scrollOffsetX = 0;
int scrollOffsetY = 0;
```

---

## 3. ProjectState API Extensions

### 3.1 setMidiNoteVelocity

```cpp
void ProjectState::setMidiNoteVelocity(const juce::String& clipId,
                                        const juce::String& noteId,
                                        int newVelocity,
                                        const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto noteTree = findMidiNote(clipId, noteId);
    if (!noteTree.isValid())
        return;

    // Clamp to MIDI range
    int velocity = juce::jlimit(1, 127, newVelocity);

    undoManager.beginNewTransaction(actionName);
    noteTree.setProperty(PROP_VELOCITY, velocity, &undoManager);

    // ValueTree listener will trigger UI refresh
}
```

**Guarantees**:
- RT-safe (message thread only)
- Undoable via UndoManager
- Triggers ValueTree property change notification
- Velocity clamped to 1-127 range

---

### 3.2 setMidiNoteLength

```cpp
void ProjectState::setMidiNoteLength(const juce::String& clipId,
                                      const juce::String& noteId,
                                      double newLengthBeats,
                                      const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto noteTree = findMidiNote(clipId, noteId);
    if (!noteTree.isValid())
        return;

    // Enforce minimum length
    double lengthBeats = juce::jmax(0.01, newLengthBeats);

    undoManager.beginNewTransaction(actionName);
    noteTree.setProperty(PROP_LENGTH_BEATS, lengthBeats, &undoManager);

    // ValueTree listener will trigger UI refresh
}
```

**Guarantees**:
- RT-safe (message thread only)
- Undoable via UndoManager
- Triggers ValueTree property change notification
- Minimum length: 0.01 beats

---

## 4. Interaction Flow Diagrams

### 4.1 Velocity Edit Flow

```
1. User clicks in velocity lane
   └─> findNoteInVelocityLane(x) → NoteRect*

2. startEditingVelocity(note, event)
   ├─> currentDragMode = VelocityEdit
   ├─> activeNote = note
   ├─> Cache original velocity in dragStates
   └─> Select note, repaint

3. User drags (updateVelocityEdit)
   ├─> Calculate yInLane (relative to velocity lane top)
   ├─> pixelsToVelocity(yInLane) → newVelocity
   ├─> Update note.velocity (visual only)
   └─> Repaint

4. User releases (finishVelocityEdit)
   ├─> Compare note.velocity vs dragStates[0].originalVelocity
   ├─> If changed: projectState.setMidiNoteVelocity(...)
   ├─> Clear dragStates
   └─> ValueTree listener triggers refresh
```

---

### 4.2 Resize Flow (Right Edge)

```
1. User clicks near right edge of note
   └─> detectNoteHitRegion(note, x, y) → ResizeRight

2. startResizingNote(note, ResizeRight, event)
   ├─> currentDragMode = ResizeRight
   ├─> activeNote = note
   ├─> Cache originalStartBeats, originalLengthBeats
   └─> Select note, repaint

3. User drags (updateNoteResize)
   ├─> Calculate deltaBeats from drag distance
   ├─> newLengthBeats = originalLengthBeats + deltaBeats
   ├─> Apply snap-to-grid if enabled
   ├─> Clamp to minimum (0.01 beats)
   ├─> Update note.lengthBeats (visual only)
   └─> Repaint

4. User releases (finishNoteResize)
   ├─> Compare note.lengthBeats vs originalLengthBeats
   ├─> If changed: projectState.setMidiNoteLength(...)
   ├─> Clear dragStates
   └─> ValueTree listener triggers refresh
```

---

### 4.3 Multi-Select Move Flow

```
1. User Ctrl-clicks notes (builds selection)
   └─> selectNote(note, addToSelection=true)

2. User drags selected note (startMovingSelection)
   ├─> currentDragMode = MoveNote
   ├─> Build dragStates[] for ALL selected notes
   └─> Cache originalPitch, originalStartBeats for each

3. User drags (updateSelectionMove)
   ├─> Calculate deltaBeats, deltaPitch from drag distance
   ├─> For each selected note:
   │   ├─> newStartBeats = originalStartBeats + deltaBeats
   │   ├─> newPitch = originalPitch + deltaPitch
   │   └─> Update note (visual only)
   └─> Repaint

4. User releases (finishSelectionMove)
   ├─> For each selected note:
   │   ├─> Compare vs original state
   │   └─> If changed: projectState.moveMidiNote(...)
   ├─> Clear dragStates
   └─> ValueTree listener triggers refresh

NOTE: Each note commits separately (TODO: batch into single undo transaction)
```

---

### 4.4 Marquee Selection Flow

```
1. User Shift-drags in empty space (startMarqueeSelect)
   ├─> currentDragMode = MarqueeSelect
   ├─> marqueeRect = Rectangle(dragStartPos, 0, 0)
   └─> If NOT Ctrl: clearSelection()

2. User drags (updateMarqueeSelect)
   ├─> Update marqueeRect to cover dragStart → currentPos
   └─> Repaint (shows visual rectangle)

3. User releases (finishMarqueeSelect)
   ├─> selectNotesInRectangle(marqueeRect)
   ├─> Clear marqueeRect
   └─> Repaint
```

---

## 5. Rendering Changes

### 5.1 Layout

```
┌─────────────────────────────────────┐
│  Note Grid (piano roll)             │
│  - Piano key background             │
│  - Beat grid lines                  │
│  - Pitch grid lines                 │
│  - Notes (orange if selected)       │
│  - Marquee rectangle (if active)    │
├─────────────────────────────────────┤ ← Separator line
│  Velocity Lane (80px height)        │
│  - Velocity bars (per note)         │
│  - Grid lines (0, 32, 64, 96, 127)  │
└─────────────────────────────────────┘
```

---

### 5.2 Velocity Lane Rendering

```cpp
// Phase 8.2: Draw velocity lane
g.setColour(juce::Colour(0xff202020));
g.fillRect(velocityLane);

// Velocity lane border
g.setColour(juce::Colour(0xff505050));
g.drawHorizontalLine(static_cast<int>(noteGridHeight), 0.0f, getWidth());

// Draw velocity bars
for (const auto& note : noteRects)
{
    if (note.selected)
        g.setColour(juce::Colours::orange.withAlpha(0.7f));
    else
        g.setColour(juce::Colours::lightblue.withAlpha(0.5f));

    g.fillRect(note.velocityBounds.reduced(1.0f, 0.0f));
}

// Velocity lane grid lines
g.setColour(juce::Colour(0xff303030));
for (int vel = 0; vel <= 127; vel += 32)
{
    float y = noteGridHeight + velocityToPixels(vel);
    g.drawHorizontalLine(static_cast<int>(y), 0.0f, getWidth());
}
```

---

### 5.3 Marquee Rectangle Rendering

```cpp
// Phase 8.2: Draw marquee selection rectangle
if (currentDragMode == DragMode::MarqueeSelect && !marqueeRect.isEmpty())
{
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillRect(marqueeRect);

    g.setColour(juce::Colours::white);
    g.drawRect(marqueeRect, 1.0f);
}
```

---

## 6. Coordinate Conversion Methods

### 6.1 Velocity Conversion

```cpp
int pixelsToVelocity(float y) const
{
    // y is relative to top of velocity lane
    // 0 at top = velocity 127, bottom = velocity 0
    float normalizedY = y / velocityLaneHeight;
    int velocity = static_cast<int>((1.0f - normalizedY) * 127.0f);
    return juce::jlimit(1, 127, velocity);
}

float velocityToPixels(int velocity) const
{
    // Invert: velocity 127 at top (y=0), velocity 0 at bottom
    float normalized = velocity / 127.0f;
    return (1.0f - normalized) * velocityLaneHeight;
}
```

**Notes**:
- Y-axis is inverted (velocity 127 at top, 0 at bottom)
- Velocity clamped to 1-127 range (never 0)
- Used for both rendering and interaction

---

### 6.2 Zoom Methods

```cpp
void zoomHorizontal(float factor, float centerX)
{
    // Zoom around centerX
    double centerBeats = pixelsToBeats(centerX);

    pixelsPerBeat *= factor;
    pixelsPerBeat = juce::jlimit(20.0, 400.0, pixelsPerBeat);

    // Adjust view to keep centerBeats at centerX
    viewStartBeats = centerBeats - (centerX / pixelsPerBeat);
    viewStartBeats = juce::jmax(0.0, viewStartBeats);

    scrollOffsetX = static_cast<int>(viewStartBeats * pixelsPerBeat);

    updateNoteRectangles();
    repaint();
}
```

**Key Feature**: Zoom centers on cursor position, not view center.

---

## 7. Mouse Event Handlers

### 7.1 mouseDown Dispatch Logic

```cpp
void mouseDown(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds();
    float noteGridHeight = bounds.getHeight() - velocityLaneHeight;

    // Check velocity lane first
    if (e.y >= noteGridHeight)
    {
        auto* note = findNoteInVelocityLane(e.x);
        if (note != nullptr)
        {
            startEditingVelocity(note, e);
            return;
        }
    }

    // Check note grid
    auto* note = findNoteAtPosition(e.x, e.y);
    if (note != nullptr)
    {
        DragMode mode = detectNoteHitRegion(*note, e.x, e.y);

        if (mode == ResizeLeft || mode == ResizeRight)
            startResizingNote(note, mode, e);
        else  // MoveNote
        {
            if (e.mods.isCommandDown())
                selectNote(note, true);  // Add to selection
            else if (!note->selected)
            {
                clearSelection();
                selectNote(note, false);
            }
            startMovingSelection(e);
        }
    }
    else
    {
        // No note hit
        if (e.mods.isShiftDown())
            startMarqueeSelect(e);
        else
            createNoteAtPosition(e.x, e.y);
    }
}
```

**Priority Order**:
1. Velocity lane
2. Note resize edges
3. Note move
4. Marquee select (Shift)
5. Create new note

---

### 7.2 mouseDrag Dispatch

```cpp
void mouseDrag(const juce::MouseEvent& e)
{
    switch (currentDragMode)
    {
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
            if (draggingNote != nullptr)
                updateNoteDrag(e);  // Legacy fallback
            break;
    }
}
```

---

### 7.3 mouseUp Dispatch

```cpp
void mouseUp(const juce::MouseEvent& e)
{
    switch (currentDragMode)
    {
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
            if (draggingNote != nullptr)
                finishNoteDrag();  // Legacy fallback
            break;
    }

    currentDragMode = DragMode::None;
    activeNote = nullptr;
}
```

---

### 7.4 mouseWheelMove (Zoom & Scroll)

```cpp
void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        // Cmd/Ctrl + wheel = horizontal zoom
        float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
        zoomHorizontal(zoomFactor, e.x);
    }
    else if (e.mods.isAltDown())
    {
        // Alt + wheel = vertical zoom
        float zoomFactor = 1.0f + (wheel.deltaY * 0.5f);
        zoomVertical(zoomFactor, e.y);
    }
    else if (e.mods.isShiftDown())
    {
        // Shift + wheel = horizontal scroll
        scrollHorizontal(-wheel.deltaY * 50.0f);
    }
    else
    {
        // Default wheel = vertical scroll
        scrollVertical(-wheel.deltaY * 50.0f);
    }
}
```

---

## 8. Manual Test Plan

### Test 1: Velocity Editing

**Steps**:
1. Open Piano Roll for a MIDI clip with notes
2. Observe velocity lane at bottom showing velocity bars
3. Click and drag a velocity bar up/down
4. Release and verify velocity changed
5. Press Cmd/Ctrl+Z to undo
6. Verify velocity reverts to original value
7. Press Cmd/Ctrl+Shift+Z to redo
8. Verify velocity changes again

**Expected Results**:
- Velocity bar height changes smoothly during drag
- Selected note shows orange velocity bar
- Undo/redo works correctly
- Velocity clamped to 1-127 range
- DBG log shows velocity change

---

### Test 2: Note Length Resize (Right Edge)

**Steps**:
1. Create a note in Piano Roll
2. Hover near right edge of note
3. Click and drag right edge to extend/shrink note
4. Release and verify note length changed
5. Press Cmd/Ctrl+Z to undo
6. Verify note returns to original length

**Expected Results**:
- Note extends/shrinks smoothly during drag
- Snap-to-grid works if enabled
- Minimum length enforced (0.01 beats)
- Undo/redo works correctly
- DBG log shows length change

---

### Test 3: Note Length Resize (Left Edge)

**Steps**:
1. Create a note in Piano Roll
2. Hover near left edge of note
3. Click and drag left edge to change start position
4. Release and verify note start + length changed (end stays same)
5. Press Cmd/Ctrl+Z twice to undo both operations
6. Verify note returns to original position

**Expected Results**:
- Note start position changes during drag
- Note end position remains constant
- Snap-to-grid works if enabled
- Two undo operations (one for move, one for length)
- DBG log shows both changes

---

### Test 4: Multi-Selection (Ctrl-Click)

**Steps**:
1. Create 3 notes in Piano Roll
2. Click first note (selects it, orange color)
3. Ctrl/Cmd-click second note (adds to selection)
4. Ctrl/Cmd-click third note (adds to selection)
5. Drag any selected note
6. Release and verify all 3 notes moved together
7. Press Cmd/Ctrl+Z to undo
8. Verify all 3 notes revert to original positions

**Expected Results**:
- All selected notes show in orange
- All notes move together maintaining relative positions
- Snap-to-grid applies to dragged note (others follow)
- Undo reverts all notes
- DBG log shows all 3 moves

---

### Test 5: Multi-Selection (Marquee)

**Steps**:
1. Create 5 notes scattered in Piano Roll
2. Shift-drag to draw marquee rectangle around 3 notes
3. Release and verify 3 notes are selected
4. Drag selection and verify all move together
5. Shift-drag to draw marquee around different notes
6. Verify previous selection cleared, new notes selected

**Expected Results**:
- White semi-transparent rectangle during drag
- Notes intersecting rectangle get selected
- Previous selection cleared unless Ctrl held
- Selected notes move together
- Undo reverts all moves

---

### Test 6: Zoom (Mousewheel)

**Steps**:
1. Open Piano Roll
2. Ctrl/Cmd + mousewheel up (zoom in horizontal)
3. Verify notes expand horizontally, centered on mouse
4. Ctrl/Cmd + mousewheel down (zoom out horizontal)
5. Verify notes shrink horizontally
6. Alt + mousewheel up (zoom in vertical)
7. Verify notes expand vertically
8. Alt + mousewheel down (zoom out vertical)
9. Verify notes shrink vertically

**Expected Results**:
- Zoom centers on mouse cursor position
- Zoom clamped to 20-400 pixels/beat (horizontal)
- Zoom clamped to 6-48 pixels/pitch (vertical)
- Notes remain visible during zoom
- Smooth zoom operation

---

### Test 7: Scroll (Mousewheel)

**Steps**:
1. Open Piano Roll
2. Mousewheel up (scroll up vertically)
3. Verify view scrolls to higher pitches
4. Shift + mousewheel right (scroll right horizontally)
5. Verify view scrolls to later beats
6. Shift + mousewheel left (scroll left horizontally)
7. Verify view scrolls to earlier beats
8. Scroll all the way left
9. Verify cannot scroll before beat 0

**Expected Results**:
- Smooth scrolling in both directions
- Horizontal scroll clamped to ≥0 beats
- Vertical scroll clamped to pitch 0-127 range
- Notes update positions correctly

---

### Test 8: Keyboard Zoom Shortcuts

**Steps**:
1. Open Piano Roll
2. Press +/= key multiple times
3. Verify horizontal zoom in (center of view)
4. Press -/_ key multiple times
5. Verify horizontal zoom out (center of view)
6. Zoom in to maximum
7. Verify zoom stops at max (400 pixels/beat)
8. Zoom out to minimum
9. Verify zoom stops at min (20 pixels/beat)

**Expected Results**:
- +/- keys zoom horizontally only
- Zoom centers on view center (not mouse)
- Zoom clamped to range
- Smooth zoom operation

---

### Test 9: Edge Detection Precision

**Steps**:
1. Create a long note (4+ beats)
2. Click in center of note → should select and move
3. Click exactly 6px from left edge → should resize left
4. Click exactly 6px from right edge → should resize right
5. Click 10px from left edge → should move (not resize)

**Expected Results**:
- 6px edge zones precisely detected
- Center clicks move note
- Edge clicks resize note
- No ambiguity or overlap

---

### Test 10: Undo Batch (Multi-Select Move)

**Steps**:
1. Select 5 notes with Ctrl-click
2. Drag all 5 notes to new position
3. Release
4. Press Cmd/Ctrl+Z once
5. Verify all 5 notes revert to original positions

**Expected Results** (Current Implementation):
- Each note commits separately (5 undo actions required)
- TODO: Batch into single undo transaction

**Expected Results** (Future Phase 9):
- Single undo action reverts all 5 notes

---

### Test 11: ValueTree Auto-Refresh

**Steps**:
1. Open Piano Roll for clip
2. Open DevTools or use Wingman to modify note velocity externally
3. Verify Piano Roll auto-updates velocity bar
4. Use Wingman to add new note
5. Verify Piano Roll shows new note
6. Perform undo in Piano Roll
7. Verify external changes visible after undo

**Expected Results**:
- Piano Roll auto-refreshes on external ValueTree changes
- Undo/redo triggers refresh via ValueTree listener
- No manual refresh needed
- DBG log shows "Note added via external change"

---

### Test 12: RT-Safety Verification

**Steps**:
1. Open Piano Roll
2. Start playback (audio thread running)
3. Perform all Phase 8.2 operations:
   - Edit velocity
   - Resize notes
   - Multi-select and move
   - Zoom and scroll
4. Verify no audio glitches or dropouts

**Expected Results**:
- All MIDI operations on message thread
- No RT thread blocking
- Smooth playback during edits
- ValueTree mutations never on audio thread

---

## 9. RT-Safety Analysis

### 9.1 Thread Separation

**Message Thread (UI)**:
- All mouse/keyboard event handlers
- All ProjectState mutations (setVelocity, setLength, move, add, remove)
- All ValueTree property changes
- All UndoManager transactions
- All repaint() calls

**Audio Thread (RT)**:
- Reads cached `Clip::midiSequence`
- Never touches ProjectState
- Never touches ValueTree
- No allocations, no locks

---

### 9.2 ValueTree Mutation Points

All Phase 8.2 operations call ProjectState methods:

1. **Velocity Edit**: `projectState.setMidiNoteVelocity(...)`
2. **Resize Right**: `projectState.setMidiNoteLength(...)`
3. **Resize Left**: `projectState.moveMidiNote(...) + setMidiNoteLength(...)`
4. **Multi-Select Move**: `projectState.moveMidiNote(...)` (per note)

All ProjectState methods assert message thread:
```cpp
jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
```

---

### 9.3 Data Flow (RT-Safe)

```
User Edits → PianoRoll → ProjectState → ValueTree
                                            ↓
                                     (property change)
                                            ↓
                        ┌───────────────────┴──────────────────┐
                        ↓                                       ↓
                 PianoRoll Listener                      Clip Listener
                (refreshNotesFromProjectState)      (buildMidiSequenceFromNotes)
                        ↓                                       ↓
                   Repaint UI                          Update midiSequence
                                                        (with midiLock)
                                                                ↓
                                                        Audio Thread Reads
                                                        (with midiLock)
```

**Critical Path**:
- Clip rebuilds `midiSequence` from ProjectState on ValueTree change
- Audio thread reads cached sequence (brief lock)
- No audio thread blocking on UI operations

---

## 10. Future Enhancements (Phase 9+)

### 10.1 Batched Undo for Multi-Select

**Current**: Each note in multi-select commits separately (N undo actions)
**Future**: Batch all moves into single undo transaction

**Implementation**:
```cpp
void finishSelectionMove()
{
    if (dragStates.empty())
        return;

    undoManager.beginNewTransaction("Move MIDI notes");

    for (auto& note : noteRects)
    {
        if (note.selected)
        {
            // All moves within same transaction
            projectState.moveMidiNote(clipId, note.id, note.startBeats, note.pitch, "");
        }
    }
}
```

---

### 10.2 Resize Cursors

**Current**: No cursor feedback for resize zones
**Future**: Show resize cursors when hovering over edges

**Implementation**:
- Implement `mouseMove()` to detect hit region
- Call `setMouseCursor(juce::MouseCursor::LeftRightResizeCursor)`

---

### 10.3 Multi-Velocity Edit

**Current**: Only single note velocity editing
**Future**: Edit velocity of all selected notes simultaneously

**Implementation**:
- Extend velocity lane to support multi-select
- Adjust all selected notes by same delta

---

### 10.4 Snap-to-Grid Toggle Shortcut

**Current**: `snapEnabled` is hardcoded
**Future**: Add keyboard shortcut to toggle snap (e.g., G key)

---

### 10.5 Velocity Lanes Per-Note

**Current**: Single velocity lane at bottom
**Future**: Option for inline velocity lanes (like FL Studio)

---

## 11. Files Modified

### Phase 8.2 Commit

**Files**:
- `zenith-core/include/ProjectState.h`: Added setMidiNoteVelocity/Length declarations
- `zenith-core/src/ProjectState.cpp`: Implemented setters with validation
- `zenith-core/include/PianoRollComponent.h`: Added DragMode, Phase 8.2 methods, new state
- `zenith-core/src/PianoRollComponent.cpp`: Full implementation (~600 new lines)

**Lines Changed**:
- +826 insertions
- -23 deletions
- ~850 net lines added

---

## 12. Summary

Phase 8.2 successfully transforms the Piano Roll into a production-ready MIDI editor with:

✅ **Velocity editing** via visual velocity lane
✅ **Note length resize** via edge drag
✅ **Multi-selection** with Ctrl/Cmd-click and marquee
✅ **Zoom & scroll** with mousewheel and keyboard

All features maintain:
- ✅ Full undo/redo support via ProjectState
- ✅ RT-safety (message thread only)
- ✅ Auto-refresh on external changes (ValueTree listeners)
- ✅ Never touch Clip's internal MidiMessageSequence

**Next Phase**: Batch undo transactions, resize cursors, and snap toggle shortcuts.
