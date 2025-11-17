# CommandAPI Reference

**Zenith DAW - JSON Command API**

Version: Extended Coverage (Phase 14)

## Overview

The CommandAPI provides a JSON-based interface for external tools (like Wingman AI) to control and query the DAW. All commands follow a consistent request/response format.

### Request Format

```json
{
  "command": "command_name",
  "params": {
    "param1": "value1",
    "param2": "value2"
  }
}
```

### Response Format

**Success:**
```json
{
  "status": "ok",
  "data": {
    "result_field": "value"
  }
}
```

**Error:**
```json
{
  "status": "error",
  "error": "Error message"
}
```

---

## Command Reference

### Project Commands

#### `get_project_info`

Get current project information.

**Parameters:** None

**Response:**
```json
{
  "name": "My Project",
  "tempo": 120.0,
  "timeSignatureNumerator": 4,
  "timeSignatureDenominator": 4,
  "numTracks": 3,
  "isPlaying": false
}
```

#### `set_tempo`

Set project tempo.

**Parameters:**
- `tempo` (number): BPM (e.g., 120.0)

**Response:**
```json
{
  "success": true
}
```

**Example:**
```json
{
  "command": "set_tempo",
  "params": { "tempo": 140.0 }
}
```

---

### Track Commands

#### `add_track`

Create a new track.

**Parameters:**
- `name` (string): Track name
- `type` (string): "audio" or "midi"

**Response:**
```json
{
  "trackId": "track_123"
}
```

**Example:**
```json
{
  "command": "add_track",
  "params": {
    "name": "MIDI 1",
    "type": "midi"
  }
}
```

#### `delete_track`

Delete a track.

**Parameters:**
- `trackId` (string): Track ID

**Response:**
```json
{
  "success": true
}
```

#### `rename_track`

Rename a track.

**Parameters:**
- `trackId` (string): Track ID
- `name` (string): New name

**Response:**
```json
{
  "success": true
}
```

#### `get_tracks`

List all tracks.

**Parameters:** None

**Response:**
```json
{
  "tracks": [
    {
      "id": "track_0",
      "name": "Audio 1",
      "type": "audio",
      "volume": 0.8,
      "pan": 0.0,
      "mute": false,
      "solo": false
    },
    {
      "id": "track_1",
      "name": "MIDI 1",
      "type": "midi",
      "volume": 0.8,
      "pan": 0.0,
      "mute": false,
      "solo": false
    }
  ]
}
```

#### `set_track_property`

Set track property (volume, pan, mute, solo).

**Parameters:**
- `trackId` (string): Track ID
- `property` (string): "volume", "pan", "mute", or "solo"
- `value` (number/boolean): Property value
  - volume: 0.0 - 1.0
  - pan: -1.0 (left) to 1.0 (right)
  - mute/solo: true/false

**Response:**
```json
{
  "success": true
}
```

**Example:**
```json
{
  "command": "set_track_property",
  "params": {
    "trackId": "track_0",
    "property": "volume",
    "value": 0.5
  }
}
```

---

### Clip Commands

#### `create_clip`

Create a clip on a track.

**Parameters:**
- `trackId` (string): Track ID
- `startBeats` (number): Start position in beats
- `lengthBeats` (number): Clip length in beats
- `type` (string): "audio" or "midi"

**Response:**
```json
{
  "clipId": "clip_456"
}
```

**Example:**
```json
{
  "command": "create_clip",
  "params": {
    "trackId": "track_1",
    "startBeats": 0.0,
    "lengthBeats": 4.0,
    "type": "midi"
  }
}
```

#### `delete_clip`

Delete a clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID

**Response:**
```json
{
  "success": true
}
```

#### `move_clip`

Move a clip to a new position.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `startBeats` (number): New start position

**Response:**
```json
{
  "success": true
}
```

#### `resize_clip`

Resize a clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `lengthBeats` (number): New length

**Response:**
```json
{
  "success": true
}
```

#### `get_clips`

Get all clips for a track.

**Parameters:**
- `trackId` (string): Track ID

**Response:**
```json
{
  "clips": [
    {
      "id": "clip_0",
      "type": "midi",
      "start": 0.0,
      "length": 4.0
    },
    {
      "id": "clip_1",
      "type": "audio",
      "start": 8.0,
      "length": 2.0
    }
  ]
}
```

---

### MIDI Note Commands

#### `create_note`

Add a MIDI note to a clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `startBeats` (number): Start position (relative to clip)
- `lengthBeats` (number): Note duration
- `pitch` (integer): MIDI note number (0-127)
- `velocity` (integer): Note velocity (0-127)

**Response:**
```json
{
  "noteId": "note_789"
}
```

**Example:**
```json
{
  "command": "create_note",
  "params": {
    "trackId": "track_1",
    "clipId": "clip_0",
    "startBeats": 0.0,
    "lengthBeats": 1.0,
    "pitch": 60,
    "velocity": 100
  }
}
```

#### `delete_note`

Delete a MIDI note.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `noteId` (string): Note ID

**Response:**
```json
{
  "success": true
}
```

#### `move_note`

Move a note (change time and/or pitch).

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `noteId` (string): Note ID
- `startBeats` (number): New start position
- `pitch` (integer): New MIDI note number (0-127)

**Response:**
```json
{
  "success": true
}
```

#### `resize_note`

Change note duration.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID
- `noteId` (string): Note ID
- `lengthBeats` (number): New duration

**Response:**
```json
{
  "success": true
}
```

#### `get_notes`

Get all notes in a clip.

**Parameters:**
- `trackId` (string): Track ID
- `clipId` (string): Clip ID

**Response:**
```json
{
  "notes": [
    {
      "id": "note_0",
      "start": 0.0,
      "length": 1.0,
      "pitch": 60,
      "velocity": 100
    },
    {
      "id": "note_1",
      "start": 1.0,
      "length": 0.5,
      "pitch": 64,
      "velocity": 90
    }
  ]
}
```

---

### Automation Commands

#### `add_automation_point`

Add an automation point.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): "volume", "pan", or "mute"
- `timeBeats` (number): Time position in beats
- `value` (number): Parameter value
  - volume: 0.0 - 1.0
  - pan: -1.0 to 1.0
  - mute: 0.0 (off) or 1.0 (on)

**Response:**
```json
{
  "pointId": "point_123"
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

#### `delete_automation_point`

Delete a specific automation point.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): "volume", "pan", or "mute"
- `pointId` (string): Point ID

**Response:**
```json
{
  "success": true
}
```

#### `move_automation_point`

Move an automation point.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): "volume", "pan", or "mute"
- `pointId` (string): Point ID
- `timeBeats` (number): New time position
- `value` (number): New value

**Response:**
```json
{
  "success": true
}
```

#### `clear_automation`

Remove all automation for a parameter.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): "volume", "pan", or "mute"

**Response:**
```json
{
  "success": true
}
```

#### `get_automation`

Get all automation points for a parameter.

**Parameters:**
- `trackId` (string): Track ID
- `param` (string): "volume", "pan", or "mute"

**Response:**
```json
{
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
```

---

### Transport Commands

#### `play`

Start playback.

**Parameters:** None

**Response:**
```json
{
  "success": true
}
```

#### `stop`

Stop playback.

**Parameters:** None

**Response:**
```json
{
  "success": true
}
```

---

### Undo/Redo Commands

#### `undo`

Undo last action.

**Parameters:** None

**Response:**
```json
{
  "success": true,
  "canUndo": false,
  "canRedo": true
}
```

#### `redo`

Redo last undone action.

**Parameters:** None

**Response:**
```json
{
  "success": true,
  "canUndo": true,
  "canRedo": false
}
```

---

## Summary Table

| Category | Command | Description |
|----------|---------|-------------|
| **Project** | `get_project_info` | Get project metadata |
| | `set_tempo` | Set BPM |
| **Tracks** | `add_track` | Create new track |
| | `delete_track` | Remove track |
| | `rename_track` | Change track name |
| | `get_tracks` | List all tracks |
| | `set_track_property` | Set volume/pan/mute/solo |
| **Clips** | `create_clip` | Add clip to track |
| | `delete_clip` | Remove clip |
| | `move_clip` | Change clip position |
| | `resize_clip` | Change clip length |
| | `get_clips` | List clips on track |
| **MIDI Notes** | `create_note` | Add note to clip |
| | `delete_note` | Remove note |
| | `move_note` | Change note time/pitch |
| | `resize_note` | Change note duration |
| | `get_notes` | List notes in clip |
| **Automation** | `add_automation_point` | Add point to envelope |
| | `delete_automation_point` | Remove point |
| | `move_automation_point` | Change point time/value |
| | `clear_automation` | Remove all points |
| | `get_automation` | List all points |
| **Transport** | `play` | Start playback |
| | `stop` | Stop playback |
| **Undo/Redo** | `undo` | Undo last action |
| | `redo` | Redo undone action |

**Total: 28 commands**

---

## Features Not Yet Implemented

The following features are planned for future phases:

- **Export**: Export project to WAV/MP3
- **Metronome**: Enable/disable metronome
- **Transport Position**: Get/set playback position
- **Plugins**: Load/configure VST/AU plugins
- **Audio Files**: Load audio files into clips
- **Markers**: Add/edit timeline markers
- **Time Signature**: Change time signature mid-project
- **Recording**: Start/stop recording on armed tracks

---

## Notes

- All operations are **undoable** (except queries like `get_*`)
- Commands run on the **message thread** (thread-safe)
- IDs are auto-generated in format `{type}_{number}`
- Times are specified in **beats** (not samples or seconds)
- MIDI note numbers follow standard MIDI spec (C4 = 60)

---

## Example Workflow

```json
// 1. Create a MIDI track
{ "command": "add_track", "params": { "name": "Bass", "type": "midi" } }
// → { "status": "ok", "data": { "trackId": "track_0" } }

// 2. Create a clip
{ "command": "create_clip", "params": {
    "trackId": "track_0", "startBeats": 0.0, "lengthBeats": 4.0, "type": "midi"
} }
// → { "status": "ok", "data": { "clipId": "clip_0" } }

// 3. Add notes
{ "command": "create_note", "params": {
    "trackId": "track_0", "clipId": "clip_0",
    "startBeats": 0.0, "lengthBeats": 1.0, "pitch": 36, "velocity": 100
} }

// 4. Add automation
{ "command": "add_automation_point", "params": {
    "trackId": "track_0", "param": "volume", "timeBeats": 0.0, "value": 0.8
} }

// 5. Play
{ "command": "play", "params": {} }
```
