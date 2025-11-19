# Phase 5: Wingman v0 - AI Command Console - Implementation Summary

**Date:** 2025-11-14
**Status:** ✅ Complete

---

## Overview

This document summarizes the Wingman v0 AI command console implementation for Zenith DAW Phase 5. Wingman provides an in-DAW JSON command API for AI-driven workflow automation, with a built-in console for testing and manual control.

## Components Implemented

### 1. CommandAPI

**Location:** `Source/commands/CommandAPI.{h,cpp}`

**Responsibilities:**
- Parse JSON command requests
- Execute commands on Engine/ProjectState
- Return structured JSON responses
- Provide comprehensive error handling

**Supported Commands:**

| Command | Parameters | Description |
|---------|-----------|-------------|
| `list_tracks` | None | Returns all tracks with properties |
| `create_track` | `type`, `name` | Creates audio or MIDI track |
| `delete_track` | `trackId` | Removes track from project |
| `rename_track` | `trackId`, `name` | Changes track name |
| `list_clips` | `trackId` | Returns clips for specified track |
| `split_clip` | `trackId`, `clipId`, `splitSamples` | Splits clip at position |
| `move_clip` | `trackId`, `clipId`, `newStartSamples` | Moves clip to new position |
| `set_track_volume` | `trackId`, `volumeDb` | Sets track volume in dB |
| `set_track_pan` | `trackId`, `pan` | Sets track pan (-1.0 to 1.0) |
| `get_session_graph` | None | Returns complete project state |

**Design Principles:**
- Message-thread only (RT-safe, no audio thread access)
- juce::var for JSON parsing/serialization
- Consistent success/error response format
- Simple ID scheme (track_N, clip_N)

### 2. SessionGraph

**Location:** `Source/commands/SessionGraph.{h,cpp}`

**Responsibilities:**
- Serialize complete project state to JSON
- Provide AI context for reasoning about project
- Include transport, tracks, clips, plugins

**Output Structure:**
```json
{
  "transport": {
    "isPlaying": bool,
    "playheadSamples": int,
    "tempo": float,
    "timeSigNumerator": int,
    "timeSigDenominator": int,
    "sampleRate": float
  },
  "tracks": [
    {
      "id": "track_0",
      "name": "Track Name",
      "type": "audio" | "midi",
      "volume": float (linear 0.0-2.0),
      "volumeDb": float (dB),
      "pan": float (-1.0 to 1.0),
      "muted": bool,
      "soloed": bool,
      "armed": bool,
      "enabled": bool,
      "currentLevel": float,
      "peakLevel": float,
      "plugins": [
        {
          "index": int,
          "name": string,
          "format": "VST3",
          "manufacturer": string,
          "category": string,
          "isInstrument": bool,
          "bypassed": bool,
          "latencySamples": int,
          "numParameters": int
        }
      ],
      "clips": [
        {
          "id": "clip_0",
          "name": "Clip Name",
          "type": "audio" | "midi",
          "startSamples": int,
          "lengthSamples": int,
          "offsetSamples": int,
          "isPlaying": bool,
          "isLooping": bool,
          "gain": float,
          "gainDb": float,
          "fadeInSamples": int,
          "fadeOutSamples": int,
          "color": "#RRGGBB",
          "audioFile": "path" (audio clips),
          "audioFileExists": bool (audio clips),
          "numChannels": int (audio clips),
          "numSamples": int (audio clips),
          "midiNoteCount": int (MIDI clips),
          "midiNoteOffCount": int (MIDI clips),
          "midiCCCount": int (MIDI clips),
          "midiOtherEventCount": int (MIDI clips),
          "midiTotalEvents": int (MIDI clips)
        }
      ]
    }
  ]
}
```

**Design Principles:**
- Comprehensive state export (everything AI needs to reason)
- Pretty-print option for readability
- Type-specific properties (audio vs MIDI)
- Level meters included for mixing awareness

### 3. WingmanPanel

**Location:** `Source/ui/WingmanPanel.{h,cpp}`

**Responsibilities:**
- In-DAW command console UI
- Text input for JSON or shorthand commands
- Message history display (input + output)
- Minimal shorthand parser for convenience

**Features:**
- **Command Input:** Single-line text editor with Return key execution
- **History Display:** Multi-line read-only editor with auto-scroll
- **Clear Button:** Clears message history
- **Welcome Message:** Shows available commands on startup
- **Shorthand Syntax:** Human-friendly command shortcuts

**Shorthand Commands:**

| Shorthand | Equivalent JSON |
|-----------|----------------|
| `tracks` | `{"command":"list_tracks","params":{}}` |
| `graph` | `{"command":"get_session_graph","params":{}}` |
| `create audio Guitar` | `{"command":"create_track","params":{"type":"audio","name":"Guitar"}}` |
| `create midi Drums` | `{"command":"create_track","params":{"type":"midi","name":"Drums"}}` |
| `delete track_0` | `{"command":"delete_track","params":{"trackId":"track_0"}}` |
| `rename track_0 Lead Vocal` | `{"command":"rename_track","params":{"trackId":"track_0","name":"Lead Vocal"}}` |
| `clips track_0` | `{"command":"list_clips","params":{"trackId":"track_0"}}` |
| `volume track_0 -6` | `{"command":"set_track_volume","params":{"trackId":"track_0","volumeDb":-6}}` |
| `pan track_1 0.5` | `{"command":"set_track_pan","params":{"trackId":"track_1","pan":0.5}}` |
| `help` | Shows shorthand syntax guide |

**UI Layout:**
- Title bar (30px): "Wingman Console"
- Clear button (top right, 80x25)
- History display (main area, monospace font)
- Command input (bottom, 30px, monospace font)
- Vertical panel (400px wide, right side of MainWindow)

**Design Principles:**
- Simple text-based interface (no fancy UI)
- Monospace fonts for readability
- Auto-scroll to latest message
- Immediate feedback (success ✓ or error ✗)
- Pretty-printed JSON results

### 4. MainWindow Integration

**Modified Files:** `include/MainWindow.h`, `src/MainWindow.cpp`

**Changes:**
- Added `CommandAPI` member to `MainWindow` (requires Engine + ProjectState)
- Updated `MainComponent` constructor to accept `CommandAPI&` reference
- Added `WingmanPanel` member to `MainComponent`
- Updated layout: WingmanPanel on right side (400px), Arranger fills remaining space
- Updated status label: "Phase 5: Wingman v0"

**Initialization Order:**
1. Engine created
2. ProjectState created
3. **CommandAPI created** (requires Engine + ProjectState)
4. MainComponent created (receives CommandAPI reference)
5. WingmanPanel created inside MainComponent (receives CommandAPI reference)

### 5. Build Configuration

**Modified Files:** `CMakeLists.txt`

**Changes:**
- Added `Source/commands/CommandAPI.{h,cpp}`
- Added `Source/commands/SessionGraph.{h,cpp}`
- Added `Source/ui/WingmanPanel.{h,cpp}`
- Added `Source/commands` to include directories

**No additional dependencies** - all based on existing JUCE modules.

---

## Command JSON Format

### Request Format

All commands follow this structure:

```json
{
  "command": "command_name",
  "params": {
    "param1": value1,
    "param2": value2
  }
}
```

### Response Format (Success)

```json
{
  "success": true,
  "result": {
    // command-specific result data
  }
}
```

### Response Format (Error)

```json
{
  "success": false,
  "error": "error message describing what went wrong"
}
```

### Examples

#### Create Track

**Request:**
```json
{
  "command": "create_track",
  "params": {
    "type": "audio",
    "name": "Guitar"
  }
}
```

**Success Response:**
```json
{
  "success": true,
  "result": {
    "trackId": "track_0",
    "name": "Guitar",
    "type": "audio"
  }
}
```

**Error Response:**
```json
{
  "success": false,
  "error": "Invalid type: must be 'audio' or 'midi'"
}
```

#### List Tracks

**Request:**
```json
{
  "command": "list_tracks",
  "params": {}
}
```

**Success Response:**
```json
{
  "success": true,
  "result": {
    "tracks": [
      {
        "id": "track_0",
        "name": "Guitar",
        "type": "audio",
        "volume": 0.8,
        "pan": 0.0,
        "muted": false,
        "soloed": false,
        "numClips": 2,
        "numPlugins": 1
      },
      {
        "id": "track_1",
        "name": "Drums",
        "type": "midi",
        "volume": 1.0,
        "pan": 0.0,
        "muted": false,
        "soloed": false,
        "numClips": 1,
        "numPlugins": 0
      }
    ],
    "count": 2
  }
}
```

#### Set Track Volume

**Request:**
```json
{
  "command": "set_track_volume",
  "params": {
    "trackId": "track_0",
    "volumeDb": -6.0
  }
}
```

**Success Response:**
```json
{
  "success": true,
  "result": {
    "trackId": "track_0",
    "volumeDb": -6.0,
    "volumeLinear": 0.501187
  }
}
```

#### Get Session Graph

**Request:**
```json
{
  "command": "get_session_graph",
  "params": {}
}
```

**Success Response:**
```json
{
  "success": true,
  "result": {
    "transport": {
      "isPlaying": false,
      "playheadSamples": 0,
      "tempo": 120.0,
      "timeSigNumerator": 4,
      "timeSigDenominator": 4,
      "sampleRate": 44100.0
    },
    "tracks": [
      // ... full track data as shown in SessionGraph section
    ]
  }
}
```

---

## Manual Test Plan

### Test 1: Launch Wingman Console

**Steps:**
1. Build and run Zenith DAW
2. Verify Wingman panel appears on right side of window (400px width)
3. Check welcome message displays in history

**Expected:**
- Wingman panel visible with title "Wingman Console"
- Welcome message shows available commands
- Command input box ready (placeholder text visible)
- Clear button visible in top right

---

### Test 2: Shorthand - List Tracks (Empty)

**Steps:**
1. Type `tracks` in command input
2. Press Return

**Expected:**
- History shows `> tracks`
- Response shows `✓ Success:`
- Result shows `{ "tracks": [], "count": 0 }`

---

### Test 3: Shorthand - Create Audio Track

**Steps:**
1. Type `create audio Guitar` in command input
2. Press Return

**Expected:**
- History shows `> create audio Guitar`
- Response shows `✓ Success:`
- Result shows: `{ "trackId": "track_0", "name": "Guitar", "type": "audio" }`

---

### Test 4: Shorthand - List Tracks (With Content)

**Steps:**
1. Type `tracks` in command input
2. Press Return

**Expected:**
- History shows `> tracks`
- Response shows `✓ Success:`
- Result shows track_0 with name "Guitar", type "audio", volume 0.8, etc.
- Count shows 1

---

### Test 5: Shorthand - Create MIDI Track

**Steps:**
1. Type `create midi Drums` in command input
2. Press Return
3. Type `tracks` to verify

**Expected:**
- First response: track_1 created
- Second response: 2 tracks listed (Guitar + Drums)

---

### Test 6: Shorthand - Rename Track

**Steps:**
1. Type `rename track_0 Lead Guitar` in command input
2. Press Return
3. Type `tracks` to verify

**Expected:**
- Rename succeeds
- Track listing shows updated name "Lead Guitar"

---

### Test 7: Shorthand - Set Volume

**Steps:**
1. Type `volume track_0 -6` in command input
2. Press Return

**Expected:**
- Response shows volumeDb: -6.0 and volumeLinear: ~0.501

---

### Test 8: Shorthand - Set Pan

**Steps:**
1. Type `pan track_1 0.5` in command input
2. Press Return

**Expected:**
- Response shows pan: 0.5

---

### Test 9: Shorthand - Delete Track

**Steps:**
1. Type `delete track_0` in command input
2. Press Return
3. Type `tracks` to verify

**Expected:**
- Delete succeeds
- Track listing shows only 1 track remaining
- Track IDs may have changed (tracks renumbered)

---

### Test 10: JSON Format - Create Track

**Steps:**
1. Type exact JSON: `{"command":"create_track","params":{"type":"audio","name":"Bass"}}`
2. Press Return

**Expected:**
- Command executes successfully
- Response shows new track created

---

### Test 11: Session Graph Export

**Steps:**
1. Create 2 tracks (audio + MIDI)
2. Type `graph` in command input
3. Press Return

**Expected:**
- Large JSON output with:
  - Transport section (isPlaying, tempo, timeSig, sampleRate)
  - Tracks array with full details
  - Plugins array (empty if no plugins)
  - Clips array (empty if no clips)

---

### Test 12: Error Handling - Invalid Command

**Steps:**
1. Type `{"command":"invalid_cmd","params":{}}` in command input
2. Press Return

**Expected:**
- Response shows `✗ Error: Unknown command: invalid_cmd`

---

### Test 13: Error Handling - Missing Parameters

**Steps:**
1. Type `{"command":"create_track","params":{}}` (missing type)
2. Press Return

**Expected:**
- Response shows `✗ Error: Missing 'type' parameter (must be 'audio' or 'midi')`

---

### Test 14: Error Handling - Invalid Track ID

**Steps:**
1. Type `delete track_999` in command input
2. Press Return

**Expected:**
- Response shows `✗ Error: Track not found: track_999`

---

### Test 15: Clear History

**Steps:**
1. Execute several commands
2. Click "Clear" button

**Expected:**
- History clears
- Shows "History cleared." message

---

### Test 16: Shorthand Help

**Steps:**
1. Type `help` in command input
2. Press Return

**Expected:**
- Help message shows available shorthand commands
- No actual command executed (help is local to UI)

---

### Test 17: Multi-Track Workflow

**Steps:**
1. `create audio Vocals`
2. `create audio Guitar`
3. `create midi Drums`
4. `volume track_0 -3`
5. `volume track_1 -6`
6. `pan track_0 -0.3`
7. `pan track_1 0.3`
8. `tracks` (verify all changes)
9. `graph` (see complete state)

**Expected:**
- All commands execute successfully
- Final track listing shows all 3 tracks with correct properties
- Session graph shows complete project state

---

### Test 18: History Auto-Scroll

**Steps:**
1. Execute many commands (>20) to fill history beyond visible area
2. Observe scroll behavior

**Expected:**
- History automatically scrolls to show latest messages
- Older messages remain accessible by scrolling up

---

### Test 19: Input Field Focus

**Steps:**
1. Click in history display
2. Start typing

**Expected:**
- Input goes to command input field (not history)
- History remains read-only

---

### Test 20: Window Resize

**Steps:**
1. Resize main window to various sizes
2. Verify Wingman panel layout

**Expected:**
- Wingman panel maintains 400px width
- History display adjusts height
- Command input stays at bottom
- All components remain visible and functional

---

## Architecture Decisions

### 1. Message-Thread Only Operations

**Decision:** All CommandAPI operations execute on the message thread, never on audio thread.

**Reason:**
- RT-safety: Audio thread must never wait or block
- JUCE best practices: UI/state changes on message thread
- Simplicity: No lock-free programming complexity for MVP

**Tradeoff:**
- Commands execute slightly slower (not real-time)
- Acceptable for AI workflow automation (not performance-critical)

### 2. Simple ID Scheme (track_N, clip_N)

**Decision:** Use index-based IDs instead of UUIDs.

**Reason:**
- MVP simplicity: Easy to implement and debug
- Human-readable: Users can understand `track_0` vs `uuid-1234-5678`
- Sufficient for Phase 5 scope

**Tradeoff:**
- IDs change when tracks/clips are deleted/reordered
- Future: May need persistent UUIDs for undo/redo

### 3. Direct Engine Modification

**Decision:** CommandAPI directly modifies Engine tracks vector, bypassing ProjectState for some operations.

**Reason:**
- MVP expedience: Fastest path to working implementation
- Engine already owns tracks (Phase 4 architecture)

**Tradeoff:**
- ProjectState not always in sync with Engine
- Future: Should route all changes through ProjectState for undo/redo

### 4. Shorthand Parser in UI

**Decision:** WingmanPanel parses shorthand locally, not in CommandAPI.

**Reason:**
- Separation of concerns: CommandAPI is pure JSON processor
- UI convenience layer doesn't pollute API
- Future: External AI agents can use JSON directly

**Tradeoff:**
- Shorthand syntax not available to external clients
- Acceptable: Shorthand is UI convenience, not core feature

### 5. No Undo/Redo (Yet)

**Decision:** Commands are immediate and irreversible.

**Reason:**
- MVP scope: Undo/redo is complex (Phase 6+)
- Focus on core command execution first

**Tradeoff:**
- No undo if user makes mistake
- Acceptable for testing/development
- **Future:** Integrate juce::UndoManager with ProjectState

---

## Known Limitations / Future Work

### Current Limitations

1. **No Undo/Redo:**
   - Commands are permanent
   - **Future:** Integrate juce::UndoManager

2. **No Clip Creation:**
   - Can only list/move/split existing clips
   - **Future:** Add `create_audio_clip`, `create_midi_clip` commands

3. **No Plugin Control:**
   - Can list plugins but not add/remove/configure
   - **Future:** Add `add_plugin`, `remove_plugin`, `set_plugin_parameter` commands

4. **No Automation:**
   - Cannot create/edit automation lanes
   - **Future:** Add automation graph commands

5. **No Audio File Import:**
   - Cannot load audio files via command
   - **Future:** Add `import_audio` command

6. **No MIDI Event Editing:**
   - Cannot create/modify MIDI notes via command
   - **Future:** Add `add_midi_note`, `delete_midi_note` commands

7. **No Tempo/Time Signature Control:**
   - Hardcoded to 120 BPM, 4/4
   - **Future:** Add `set_tempo`, `set_time_signature` commands

8. **No WebSocket Server:**
   - No external AI agent connectivity yet
   - **Future:** Add AIBridgeServer for external LLM integration

9. **No Command History Persistence:**
   - History clears on app restart
   - **Future:** Save command history to file

10. **No Command Batching:**
    - Each command is separate transaction
    - **Future:** Add batch command support for efficiency

---

## Files Created

**New Command Processing:**
- `Source/commands/CommandAPI.h`
- `Source/commands/CommandAPI.cpp`
- `Source/commands/SessionGraph.h`
- `Source/commands/SessionGraph.cpp`

**New UI Components:**
- `Source/ui/WingmanPanel.h`
- `Source/ui/WingmanPanel.cpp`

**Documentation:**
- `docs/Phase5_Wingman_Summary.md`

**Modified Files:**
- `include/MainWindow.h` (added WingmanPanel, CommandAPI members)
- `src/MainWindow.cpp` (integrated Wingman components)
- `CMakeLists.txt` (added new source files)

---

## Next Steps (Phase 6+)

### Phase 6: Advanced Commands + Undo/Redo

1. **Undo/Redo System:**
   - Integrate juce::UndoManager with ProjectState
   - Make all commands undoable
   - Add `undo`, `redo` commands to API

2. **Clip Creation:**
   - `create_audio_clip`: Import audio file and create clip
   - `create_midi_clip`: Create empty MIDI clip with optional notes
   - `duplicate_clip`: Copy existing clip

3. **Plugin Control:**
   - `add_plugin`: Insert plugin into track chain
   - `remove_plugin`: Remove plugin from chain
   - `set_plugin_parameter`: Adjust plugin parameter value
   - `get_plugin_parameters`: List all plugin parameters

4. **MIDI Editing:**
   - `add_midi_note`: Create MIDI note in clip
   - `delete_midi_note`: Remove MIDI note from clip
   - `move_midi_note`: Change note pitch/time
   - `set_midi_note_velocity`: Adjust note velocity

### Phase 7: External AI Integration

1. **WebSocket Server:**
   - AIBridgeServer for external LLM connectivity
   - Implement message protocol from planning docs
   - Support concurrent AI agent connections

2. **LLM Integration:**
   - Claude/GPT API integration
   - Natural language → command translation
   - Session graph reasoning for smart suggestions

3. **MagentaService:**
   - Local AI music generation
   - Melody/chord/drum pattern generation
   - Integration with MIDI clip commands

### Phase 8: Advanced Automation

1. **Automation Commands:**
   - `create_automation_lane`: Add automation for parameter
   - `add_automation_point`: Create breakpoint
   - `delete_automation_point`: Remove breakpoint
   - `get_automation_curve`: Export automation data

2. **Batch Processing:**
   - Command sequences with transaction support
   - Macro recording (record command sequence)
   - Template systems (save/load command presets)

---

## Conclusion

✅ **Phase 5 Complete:** Zenith DAW now has a functional AI command console (Wingman v0):

- **CommandAPI:** 10 commands for track/clip/session control
- **SessionGraph:** Complete project state serialization
- **WingmanPanel:** In-DAW console with JSON + shorthand support
- **RT-Safe:** All operations on message thread
- **Well-Documented:** Comprehensive command reference and test plan

The implementation provides a solid foundation for:
- AI-driven workflow automation
- External LLM agent integration (Phase 6+)
- Natural language DAW control
- Advanced command scripting

**Developer Experience:**
- Simple JSON API for programmatic control
- Human-friendly shorthand for manual testing
- Comprehensive error messages
- Structured response format

---

**Next:** Phase 6 - Undo/Redo + Advanced Commands + External AI Integration
