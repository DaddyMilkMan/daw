# Phase 6: Undo/Redo + Advanced Clip Commands - Implementation Summary

**Date:** 2025-11-14
**Status:** ✅ Complete

---

## Overview

Phase 6 adds comprehensive undo/redo support and advanced clip management to Zenith DAW. All Wingman commands are now undoable, with keyboard shortcuts and new clip creation/deletion commands.

---

## Changes Summary

### 1. ProjectState: Undoable Methods

**New methods in ProjectState.h/cpp:**

**Track operations (all undoable):**
- `renameTrack(trackId, newName, actionName)`
- `setTrackVolume(trackId, volumeLinear, actionName)`
- `setTrackPan(trackId, pan, actionName)`
- `setTrackMute(trackId, muted, actionName)`
- `setTrackSolo(trackId, soloed, actionName)`
- `setTrackArmed(trackId, armed, actionName)`

**Clip operations (all undoable):**
- `createClip(trackId, type, start, length, name, actionName)` → returns clipId
- `deleteClip(trackId, clipId, actionName)`
- `moveClip(trackId, clipId, newStart, actionName)`
- `splitClip(trackId, clipId, splitPos, actionName)` → returns pair<leftId, rightId>

**Helper methods:**
- `getTrack(trackId)` → returns ValueTree
- `getClip(trackId, clipId)` → returns ValueTree

**All methods:**
- Use `undoManager.beginNewTransaction(actionName)` for descriptive history
- Update ValueTree with `&undoManager` parameter for undoable changes
- Are RT-safe (message thread only)

---

### 2. CommandAPI: Refactored to Use ProjectState

**Refactored commands:**
- ❌ **Old:** `create_track` directly pushed to Engine vector
- ✅ **New:** Uses `projectState.addTrack()` (already undoable)

- ❌ **Old:** `delete_track` directly erased from Engine vector
- ✅ **New:** Uses `projectState.removeTrack()` (already undoable)

- ❌ **Old:** `rename_track` called `Track::setName()` directly
- ✅ **New:** Uses `projectState.renameTrack()`

- ❌ **Old:** `set_track_volume` called `Track::setVolume()` directly
- ✅ **New:** Uses `projectState.setTrackVolume()`

- ❌ **Old:** `set_track_pan` called `Track::setPan()` directly
- ✅ **New:** Uses `projectState.setTrackPan()`

- ❌ **Old:** `split_clip` directly manipulated Engine clips
- ✅ **New:** Uses `projectState.splitClip()`

- ❌ **Old:** `move_clip` directly called `Clip::setStartPosition()`
- ✅ **New:** Uses `projectState.moveClip()`

**New commands:**
- `create_clip` - Create audio or MIDI clip on track
- `delete_clip` - Delete clip from track
- `undo` - Undo last action
- `redo` - Redo last undone action
- `history` - Query undo/redo availability

**Action naming convention:**
- Wingman commands: `"Wingman: command_name params"`
- Examples:
  - `"Wingman: create_track 'Guitar'"`
  - `"Wingman: set_track_volume track_0 to -6.0 dB"`
  - `"Wingman: split_clip clip_3 at 44100"`

---

### 3. New Wingman Commands

#### `create_clip`

**Request:**
```json
{
  "command": "create_clip",
  "params": {
    "trackId": "track_0",
    "type": "midi",
    "start": 0,
    "length": 44100,
    "name": "New MIDI Clip"
  }
}
```

**Optional for audio clips:**
- `"audioFile": "/path/to/file.wav"` (required for audio clips)

**Response:**
```json
{
  "success": true,
  "result": {
    "clipId": "clip_5",
    "trackId": "track_0",
    "name": "New MIDI Clip",
    "type": "midi",
    "startSamples": 0,
    "lengthSamples": 44100
  }
}
```

#### `delete_clip`

**Request:**
```json
{
  "command": "delete_clip",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_5"
  }
}
```

**Response:**
```json
{
  "success": true,
  "result": {
    "clipId": "clip_5",
    "trackId": "track_0",
    "deleted": true
  }
}
```

#### `undo`

**Request:**
```json
{
  "command": "undo",
  "params": {}
}
```

**Response:**
```json
{
  "success": true,
  "result": {
    "undone": true,
    "message": "Undo successful"
  }
}
```

**Error (nothing to undo):**
```json
{
  "success": false,
  "error": "Nothing to undo"
}
```

#### `redo`

**Request:**
```json
{
  "command": "redo",
  "params": {}
}
```

**Response:**
```json
{
  "success": true,
  "result": {
    "redone": true,
    "message": "Redo successful"
  }
}
```

#### `history`

**Request:**
```json
{
  "command": "history",
  "params": {}
}
```

**Response:**
```json
{
  "success": true,
  "result": {
    "canUndo": true,
    "canRedo": false,
    "message": "Full history tracking not yet implemented"
  }
}
```

**Note:** JUCE UndoManager doesn't expose action names easily. Future versions may add full history tracking with action names.

---

### 4. Wingman Shorthand Updates

**New shortcuts:**
- `undo` or `u` → undo last action
- `redo` or `r` → redo last action
- `history` or `h` → show undo/redo status

**Updated help:**
```
Available shorthand commands:
  tracks                     → list all tracks
  create <audio|midi> <name> → create track
  delete <trackId>           → delete track
  rename <trackId> <name>    → rename track
  clips <trackId>            → list clips
  volume <trackId> <db>      → set volume
  pan <trackId> <value>      → set pan
  graph                      → session graph
  undo (or u)                → undo last action
  redo (or r)                → redo last action
  history (or h)             → show undo/redo status
```

---

### 5. Keyboard Shortcuts

**MainComponent now implements `juce::KeyListener`:**

**Shortcuts:**
- **Ctrl+Z** (Windows/Linux) or **Cmd+Z** (macOS) → **Undo**
- **Ctrl+Shift+Z** (Windows/Linux) or **Cmd+Shift+Z** (macOS) → **Redo**
- **Ctrl+Y** (Windows/Linux) or **Cmd+Y** (macOS) → **Redo** (alternative)

**Implementation:**
- `MainComponent::keyPressed()` handles key events
- Calls `projectState.undo()` / `projectState.redo()`
- Returns `true` if handled, `false` if not
- Logs actions to debug console

**Changes:**
- MainComponent constructor now takes `ProjectState&` parameter
- MainComponent registers as key listener: `addKeyListener(this)`
- MainComponent sets keyboard focus: `setWantsKeyboardFocus(true)`

---

## Architecture

### Single UndoManager

**Location:** `ProjectState` owns the single `juce::UndoManager`

**Access:** All components get UndoManager via `ProjectState::getUndoManager()`

**Usage:**
- All track/clip mutations route through ProjectState undoable methods
- CommandAPI calls ProjectState methods (not Engine directly)
- UI components (future Piano Roll) will also use ProjectState methods

**Transaction naming:**
- Every undoable operation starts with `undoManager.beginNewTransaction(actionName)`
- Action names are descriptive: "Create track 'Guitar'", "Set track_0 volume to -6 dB"
- Enables readable undo history (when/if JUCE exposes it)

---

## RT-Safety

✅ **All undo operations are RT-safe:**
- All CommandAPI operations execute on **message thread only**
- No audio thread access in ProjectState methods
- No locks or heavy operations in undo transactions
- ValueTree modifications are non-blocking

---

## Manual Test Plan

### Test 1: Track Creation + Undo

```
> create audio Guitar
> tracks
> u
> tracks
```

**Expected:**
- Track created successfully
- Track listed after creation
- Undo removes track
- Track not listed after undo

---

### Test 2: Track Rename + Undo/Redo

```
> create midi Drums
> rename track_0 Kick Drums
> tracks
> u
> tracks
> r
> tracks
```

**Expected:**
- Track renamed to "Kick Drums"
- Undo reverts to "Drums"
- Redo reapplies "Kick Drums"

---

### Test 3: Volume/Pan + Keyboard Shortcut

```
> create audio Bass
> volume track_0 -6
> pan track_0 0.5
> tracks
(Press Ctrl+Z twice)
> tracks
(Press Ctrl+Shift+Z twice)
> tracks
```

**Expected:**
- Volume and pan applied
- First Ctrl+Z undoes pan
- Second Ctrl+Z undoes volume
- Ctrl+Shift+Z (×2) redoes both

---

### Test 4: Clip Creation + Deletion

```
> create midi Drums
> {"command":"create_clip","params":{"trackId":"track_0","type":"midi","start":0,"length":44100,"name":"Beat 1"}}
> clips track_0
> {"command":"delete_clip","params":{"trackId":"track_0","clipId":"clip_0"}}
> clips track_0
> u
> clips track_0
```

**Expected:**
- MIDI clip created
- Clip listed
- Clip deleted
- Undo restores clip

---

### Test 5: Clip Split + Undo

```
> create audio Guitar
> {"command":"create_clip","params":{"trackId":"track_0","type":"midi","start":0,"length":88200,"name":"Riff"}}
> {"command":"split_clip","params":{"trackId":"track_0","clipId":"clip_0","splitSamples":44100}}
> clips track_0
> u
> clips track_0
```

**Expected:**
- Clip split into 2 clips
- Undo restores original clip
- Split clips removed

---

### Test 6: Complex Workflow

```
> create audio Vocals
> create audio Guitar
> create midi Drums
> volume track_0 -3
> volume track_1 -6
> pan track_0 -0.3
> pan track_1 0.3
> tracks
(Press Ctrl+Z 6 times)
> tracks
> h
```

**Expected:**
- All operations applied
- All operations undone step-by-step
- History shows canUndo=false, canRedo=true
- Tracks list shows empty or default state

---

### Test 7: Undo/Redo via Wingman

```
> create audio Test
> undo
> redo
> history
```

**Expected:**
- Track created
- `undo` removes track
- `redo` restores track
- `history` shows current state

---

### Test 8: Error Handling

```
> undo
> redo
> {"command":"delete_clip","params":{"trackId":"invalid","clipId":"invalid"}}
```

**Expected:**
- First `undo`: "✗ Error: Nothing to undo"
- `redo`: "✗ Error: Nothing to redo"
- Invalid clip delete: "✗ Error: Clip not found"

---

## Known Limitations

### 1. Piano Roll MIDI Edits Not Undoable (Yet)

**Status:** PianoRollComponent still directly modifies MidiMessageSequence

**Impact:** MIDI note edits (create/move/delete) in Piano Roll are NOT undoable

**Future:** Phase 7 will add undoable MIDI note methods to ProjectState or Clip

### 2. Full History Not Exposed

**Status:** JUCE UndoManager doesn't easily expose action names

**Impact:** `history` command only shows canUndo/canRedo, not action list

**Workaround:** Implement custom action tracking ring buffer if needed

### 3. Engine/ProjectState Sync

**Status:** Engine still owns Track objects, ProjectState owns ValueTree

**Impact:** Changes to ProjectState don't automatically update Engine (yet)

**Future:** Add ValueTree listeners to sync Engine with ProjectState changes

### 4. Clip Audio/MIDI Data Not in ValueTree

**Status:** Clip ValueTree stores metadata only, not audio buffers or MIDI sequences

**Impact:** Undo/redo recreates clip metadata but doesn't restore actual audio/MIDI content

**Future:** Store MIDI sequences in ValueTree or implement separate content undo system

---

## Files Modified

**ProjectState:**
- `include/ProjectState.h` - Added undoable methods + clip properties
- `src/ProjectState.cpp` - Implemented undoable methods

**CommandAPI:**
- `Source/commands/CommandAPI.h` - Added new command handlers
- `Source/commands/CommandAPI.cpp` - Refactored all commands to use ProjectState

**WingmanPanel:**
- `Source/ui/WingmanPanel.cpp` - Added undo/redo/history shortcuts

**MainWindow/MainComponent:**
- `include/MainWindow.h` - MainComponent implements KeyListener, added ProjectState reference
- `src/MainWindow.cpp` - Added keyPressed() handler + keyboard shortcuts

---

## Next Steps (Phase 7+)

### Piano Roll Undo

1. Add undoable MIDI note methods to ProjectState or Clip
2. Refactor PianoRollComponent to use undoable methods
3. Transaction naming: "Create note C4", "Move note C4 to D4", etc.

### External AI Integration

1. Implement WebSocket server (AIBridgeServer)
2. Add LLM integration for natural language commands
3. MagentaService for AI music generation

### Advanced Features

1. Macro recording (record command sequences)
2. Template systems (save/load command presets)
3. Batch command processing with transactions
4. Full undo history with action names

---

## Summary

✅ **Phase 6 Complete:**

**What's New:**
- 13 total Wingman commands (was 10, now +3: create_clip, delete_clip, undo/redo/history)
- All track operations undoable
- All clip operations undoable
- Keyboard shortcuts (Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z, Ctrl/Cmd+Y)
- Wingman shortcuts (u, r, h)
- ProjectState as single source of truth for undo

**Architecture:**
- Single UndoManager in ProjectState
- All mutations via ProjectState undoable methods
- CommandAPI refactored to use ProjectState
- RT-safe (message thread only)
- Descriptive action names for history

**Not Done (Future):**
- Piano Roll MIDI edits still not undoable
- Full history tracking not implemented
- Engine/ProjectState sync still manual
- Clip content (audio/MIDI) not fully integrated

---

**Status:** Phase 6 implementation complete and ready for testing!
