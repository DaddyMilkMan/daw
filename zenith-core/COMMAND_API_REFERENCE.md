# Command API Reference

## Overview

The Command API provides a JSON-based interface for external tools (AI assistants, automation scripts) to control Zenith DAW. All commands follow a request/response pattern using JSON.

**Request Format:**
```json
{
  "command": "command_name",
  "params": { ... }
}
```

**Response Format:**
```json
{
  "status": "ok" | "error",
  "data": { ... },      // if status == "ok"
  "error": "message"    // if status == "error"
}
```

## Command Categories

- [Project Management](#project-management)
- [Transport Control](#transport-control)
- [Track Management](#track-management)
- [Clip Management](#clip-management)
- [MIDI Note Management](#midi-note-management)
- [Audio File Management](#audio-file-management)
- [Automation](#automation)
- [Plugin Management](#plugin-management-stubbed)
- [Export](#export-stubbed)
- [Undo/Redo](#undoredo)

---

## Project Management

### `get_project_info`

Get project metadata and state.

**Parameters:** `{}`

**Response:**
```json
{
  "status": "ok",
  "data": {
    "name": "My Project",
    "tempo": 120.0,
    "timeSignatureNumerator": 4,
    "timeSignatureDenominator": 4,
    "numTracks": 3,
    "isPlaying": false
  }
}
```

**Example:**
```json
{
  "command": "get_project_info",
  "params": {}
}
```

---

### `set_tempo`

Set project tempo.

**Parameters:**
- `tempo` (number): Tempo in BPM (e.g., 120.0)

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Example:**
```json
{
  "command": "set_tempo",
  "params": {
    "tempo": 140.0
  }
}
```

---

## Transport Control

### `play`

Start playback.

**Parameters:** `{}`

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Example:**
```json
{
  "command": "play",
  "params": {}
}
```

---

### `stop`

Stop playback.

**Parameters:** `{}`

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Example:**
```json
{
  "command": "stop",
  "params": {}
}
```

---

## Track Management

### `add_track`

Create a new track.

**Parameters:**
- `name` (string): Track name
- `type` (string): "audio" or "midi"

**Response:**
```json
{
  "status": "ok",
  "data": {
    "trackId": "track_0"
  }
}
```

**Example:**
```json
{
  "command": "add_track",
  "params": {
    "name": "Drums",
    "type": "midi"
  }
}
```

---

## Clip Management

### `create_clip`

Create a clip on a track.

**Parameters:**
- `trackId` (string): Track ID
- `startBeats` (number): Start position in beats
- `lengthBeats` (number): Clip length in beats
- `type` (string): "audio" or "midi"

**Response:**
```json
{
  "status": "ok",
  "data": {
    "clipId": "clip_0"
  }
}
```

**Example:**
```json
{
  "command": "create_clip",
  "params": {
    "trackId": "track_0",
    "startBeats": 0.0,
    "lengthBeats": 4.0,
    "type": "midi"
  }
}
```

---

### `delete_clip`

Delete a clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Example:**
```json
{
  "command": "delete_clip",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0"
  }
}
```

---

### `get_track_clips`

Get all clips on a track.

**Parameters:**
- `trackId` (string): Track ID

**Response:**
```json
{
  "status": "ok",
  "data": {
    "clips": [
      {
        "id": "clip_0",
        "type": "midi",
        "startBeats": 0.0,
        "lengthBeats": 4.0
      },
      {
        "id": "clip_1",
        "type": "audio",
        "startBeats": 4.0,
        "lengthBeats": 8.0,
        "audioFile": "/path/to/audio.wav"
      }
    ]
  }
}
```

**Example:**
```json
{
  "command": "get_track_clips",
  "params": {
    "trackId": "track_0"
  }
}
```

---

## MIDI Note Management

### `create_note`

Create a MIDI note in a clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `note` (number): MIDI note number (0-127, e.g., 60 = middle C)
- `velocity` (number): MIDI velocity (0-127, e.g., 100)
- `startBeats` (number): Start position in beats (relative to clip start)
- `lengthBeats` (number): Note length in beats

**Response:**
```json
{
  "status": "ok",
  "data": {
    "noteId": "note_0"
  }
}
```

**Example:**
```json
{
  "command": "create_note",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0",
    "note": 60,
    "velocity": 100,
    "startBeats": 0.0,
    "lengthBeats": 1.0
  }
}
```

---

### `delete_note`

Delete a MIDI note.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `noteId` (string): Note ID

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Example:**
```json
{
  "command": "delete_note",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0",
    "noteId": "note_0"
  }
}
```

---

### `get_clip_notes`

Get all MIDI notes in a clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID

**Response:**
```json
{
  "status": "ok",
  "data": {
    "notes": [
      {
        "id": "note_0",
        "note": 60,
        "velocity": 100,
        "startBeats": 0.0,
        "lengthBeats": 1.0
      },
      {
        "id": "note_1",
        "note": 64,
        "velocity": 90,
        "startBeats": 1.0,
        "lengthBeats": 1.0
      }
    ]
  }
}
```

**Example:**
```json
{
  "command": "get_clip_notes",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0"
  }
}
```

---

## Audio File Management

### `set_clip_audio_file`

Attach an audio file to an existing clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `path` (string): Absolute path to audio file

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Example:**
```json
{
  "command": "set_clip_audio_file",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0",
    "path": "/home/user/audio/kick.wav"
  }
}
```

---

### `create_audio_clip`

Create a new audio clip with a file in one step.

**Parameters:**
- `trackId` (string): Track ID
- `startBeats` (number): Start position in beats
- `lengthBeats` (number): Clip length in beats
- `path` (string): Absolute path to audio file

**Response:**
```json
{
  "status": "ok",
  "data": {
    "clipId": "clip_0"
  }
}
```

**Example:**
```json
{
  "command": "create_audio_clip",
  "params": {
    "trackId": "track_1",
    "startBeats": 0.0,
    "lengthBeats": 8.0,
    "path": "/home/user/audio/drums.wav"
  }
}
```

---

## Automation

### `add_automation_point`

Add an automation point for a track parameter.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): Parameter name ("volume", "pan", or "mute")
- `timeBeats` (number): Time position in beats
- `value` (number): Value (0-1 for volume, -1 to 1 for pan, 0/1 for mute)

**Response:**
```json
{
  "status": "ok",
  "data": {
    "pointId": "point_0"
  }
}
```

**Example:**
```json
{
  "command": "add_automation_point",
  "params": {
    "trackId": "track_0",
    "param": "volume",
    "timeBeats": 8.0,
    "value": 0.5
  }
}
```

---

### `get_automation`

Get automation points for a track parameter.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): Parameter name ("volume", "pan", or "mute")

**Response:**
```json
{
  "status": "ok",
  "data": {
    "points": [
      {
        "id": "point_0",
        "timeBeats": 0.0,
        "value": 0.8
      },
      {
        "id": "point_1",
        "timeBeats": 8.0,
        "value": 0.5
      }
    ]
  }
}
```

**Example:**
```json
{
  "command": "get_automation",
  "params": {
    "trackId": "track_0",
    "param": "volume"
  }
}
```

---

### `clear_automation`

Clear all automation for a track parameter.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): Parameter name ("volume", "pan", or "mute")

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Example:**
```json
{
  "command": "clear_automation",
  "params": {
    "trackId": "track_0",
    "param": "volume"
  }
}
```

---

## Plugin Management (Stubbed)

**Note:** Plugin functionality is not yet implemented in the engine. These commands return placeholder responses.

### `scan_plugins`

Scan for available plugins.

**Parameters:**
- `force` (boolean, optional): Force rescan from disk

**Response:**
```json
{
  "status": "ok",
  "data": {
    "count": 0,
    "message": "Plugin system not yet implemented"
  }
}
```

---

### `get_plugins`

Get list of available plugins.

**Parameters:**
- `type` (string, optional): Filter by "instrument" or "effect"
- `searchTerm` (string, optional): Search filter

**Response:**
```json
{
  "status": "ok",
  "data": {
    "plugins": [],
    "message": "Plugin system not yet implemented"
  }
}
```

---

### `add_track_plugin`

Add a plugin to a track's plugin chain.

**Parameters:**
- `trackId` (string): Track ID
- `pluginId` (string): Plugin ID from `get_plugins`

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": false,
    "message": "Plugin system not yet implemented"
  }
}
```

---

### `remove_track_plugin`

Remove a plugin from a track.

**Parameters:**
- `trackId` (string): Track ID
- `pluginIndex` (number): Plugin index in chain (0-based)

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": false,
    "message": "Plugin system not yet implemented"
  }
}
```

---

### `set_track_plugin_bypassed`

Bypass/unbypass a plugin.

**Parameters:**
- `trackId` (string): Track ID
- `pluginIndex` (number): Plugin index in chain (0-based)
- `bypassed` (boolean): true to bypass, false to unbypass

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": false,
    "message": "Plugin system not yet implemented"
  }
}
```

---

### `get_track_plugins`

Get a track's plugin chain.

**Parameters:**
- `trackId` (string): Track ID

**Response:**
```json
{
  "status": "ok",
  "data": {
    "plugins": [],
    "message": "Plugin system not yet implemented"
  }
}
```

---

## Export (Stubbed)

**Note:** Export functionality is not yet implemented in the engine.

### `export_project`

Export project to WAV file.

**Parameters:**
- `path` (string): Output file path
- `startBeats` (number, optional): Start position in beats
- `endBeats` (number, optional): End position in beats

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": false,
    "message": "Export functionality not yet implemented"
  }
}
```

---

## Undo/Redo

### `undo`

Undo the last action.

**Parameters:** `{}`

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "canUndo": false
  }
}
```

**Example:**
```json
{
  "command": "undo",
  "params": {}
}
```

---

### `redo`

Redo the last undone action.

**Parameters:** `{}`

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "canRedo": false
  }
}
```

**Example:**
```json
{
  "command": "redo",
  "params": {}
}
```

---

## Complete Workflow Examples

### Example 1: AI Builds a MIDI Track

```json
// Step 1: Add a MIDI track
{
  "command": "add_track",
  "params": {
    "name": "Lead Synth",
    "type": "midi"
  }
}
// Response: { "status": "ok", "data": { "trackId": "track_0" } }

// Step 2: Create a MIDI clip
{
  "command": "create_clip",
  "params": {
    "trackId": "track_0",
    "startBeats": 0.0,
    "lengthBeats": 16.0,
    "type": "midi"
  }
}
// Response: { "status": "ok", "data": { "clipId": "clip_0" } }

// Step 3: Add MIDI notes (C major chord)
{
  "command": "create_note",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0",
    "note": 60,
    "velocity": 100,
    "startBeats": 0.0,
    "lengthBeats": 4.0
  }
}

{
  "command": "create_note",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0",
    "note": 64,
    "velocity": 95,
    "startBeats": 0.0,
    "lengthBeats": 4.0
  }
}

{
  "command": "create_note",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0",
    "note": 67,
    "velocity": 90,
    "startBeats": 0.0,
    "lengthBeats": 4.0
  }
}

// Step 4: Add volume automation
{
  "command": "add_automation_point",
  "params": {
    "trackId": "track_0",
    "param": "volume",
    "timeBeats": 0.0,
    "value": 0.0
  }
}

{
  "command": "add_automation_point",
  "params": {
    "trackId": "track_0",
    "param": "volume",
    "timeBeats": 4.0,
    "value": 1.0
  }
}

// Step 5: Start playback
{
  "command": "play",
  "params": {}
}
```

### Example 2: AI Imports and Arranges Audio

```json
// Step 1: Add audio track
{
  "command": "add_track",
  "params": {
    "name": "Drums",
    "type": "audio"
  }
}
// Response: { "status": "ok", "data": { "trackId": "track_1" } }

// Step 2: Import drum loop
{
  "command": "create_audio_clip",
  "params": {
    "trackId": "track_1",
    "startBeats": 0.0,
    "lengthBeats": 8.0,
    "path": "/home/user/samples/drum_loop.wav"
  }
}
// Response: { "status": "ok", "data": { "clipId": "clip_1" } }

// Step 3: Add bass track
{
  "command": "add_track",
  "params": {
    "name": "Bass",
    "type": "audio"
  }
}

// Step 4: Import bass line
{
  "command": "create_audio_clip",
  "params": {
    "trackId": "track_2",
    "startBeats": 0.0,
    "lengthBeats": 16.0,
    "path": "/home/user/samples/bass_line.wav"
  }
}

// Step 5: Set project tempo
{
  "command": "set_tempo",
  "params": {
    "tempo": 128.0
  }
}

// Step 6: Start playback
{
  "command": "play",
  "params": {}
}
```

### Example 3: Query and Modify Existing Project

```json
// Step 1: Get project info
{
  "command": "get_project_info",
  "params": {}
}
// Response shows 3 tracks at 120 BPM

// Step 2: Get clips on first track
{
  "command": "get_track_clips",
  "params": {
    "trackId": "track_0"
  }
}
// Response shows clip_0 is a MIDI clip

// Step 3: Get MIDI notes in the clip
{
  "command": "get_clip_notes",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0"
  }
}
// Response shows existing notes

// Step 4: Delete a note
{
  "command": "delete_note",
  "params": {
    "trackId": "track_0",
    "clipId": "clip_0",
    "noteId": "note_5"
  }
}

// Step 5: Undo if needed
{
  "command": "undo",
  "params": {}
}
```

---

## Error Handling

All commands return errors in a consistent format:

```json
{
  "status": "error",
  "error": "Missing required parameter: trackId"
}
```

Common error scenarios:
- Missing required parameters
- Invalid parameter values (e.g., note > 127)
- Invalid trackId/clipId references
- Invalid command names
- Malformed JSON

---

## Implementation Status

| Feature | Status |
|---------|--------|
| Project management | ✅ Implemented |
| Transport control | ✅ Implemented |
| Track management | ✅ Implemented |
| Clip management | ✅ Implemented |
| MIDI notes | ✅ Implemented |
| Audio file import | ✅ Implemented (state only) |
| Automation | ✅ Implemented |
| Undo/Redo | ✅ Implemented |
| Plugin scanning | ⚠️  Stubbed (not yet implemented) |
| Plugin management | ⚠️  Stubbed (not yet implemented) |
| Export to WAV | ⚠️  Stubbed (not yet implemented) |

**Note:** Stubbed features return placeholder responses and will be fully implemented in future updates when the underlying engine systems are available.
