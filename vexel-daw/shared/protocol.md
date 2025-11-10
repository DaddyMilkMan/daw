# Zenith DAW Protocol Specification

**Version:** 1.0.0
**Last Updated:** 2025-11-10

This document defines the command and event protocol for Zenith DAW. It establishes the communication contract between the DAW engine, UI, and external integrations.

## Table of Contents

1. [Protocol Overview](#protocol-overview)
2. [Commands](#commands)
3. [Events](#events)
4. [Message Format](#message-format)
5. [Error Handling](#error-handling)

---

## Protocol Overview

### Architecture
The protocol follows a **command-event** pattern:
- **Commands**: Requests sent to the DAW engine to perform actions
- **Events**: Notifications broadcast by the DAW engine when state changes

### Message Flow
```
┌──────────┐                    ┌──────────────┐
│    UI    │───── Command ─────>│ DAW Engine   │
│          │                    │              │
│          │<──── Event ────────│              │
└──────────┘                    └──────────────┘
```

### Transport
- **Internal**: Direct function calls (TypeScript/JavaScript)
- **IPC**: Electron main ↔ renderer process
- **Future**: WebSocket for remote control, OSC for hardware

### Guarantees
- **Commands**: Request-response (may return errors)
- **Events**: Fire-and-forget broadcast (no acknowledgment)
- **Ordering**: Events are emitted in the order state changes occur

---

## Commands

Commands are actions that modify DAW state. All commands return a result indicating success or failure.

### Transport Control

#### `transport_play`
Start playback from the current position.

**Parameters:** None

**Returns:** `{ success: boolean }`

---

#### `transport_stop`
Stop playback and optionally return to start position.

**Parameters:**
- `returnToStart` (boolean, optional): If true, reset playhead to 0. Default: false.

**Returns:** `{ success: boolean }`

---

#### `transport_set_position`
Move the playhead to a specific position.

**Parameters:**
- `beat` (number, required): Position in beats. Range: 0 to project end.

**Returns:** `{ success: boolean, beat: number }`

---

#### `transport_record`
Start recording on armed tracks.

**Parameters:** None

**Returns:** `{ success: boolean }`

---

### Tempo and Time Signature

#### `set_tempo`
Change the global tempo.

**Parameters:**
- `tempo` (number, required): BPM value. Range: 1-999.
- `beat` (number, optional): Position for tempo change. If omitted, changes immediately.

**Returns:** `{ success: boolean, tempo: number }`

---

#### `set_time_signature`
Change the time signature.

**Parameters:**
- `numerator` (number, required): Beats per bar. Range: 1-64.
- `denominator` (number, required): Note value. Values: 1, 2, 4, 8, 16, 32.
- `beat` (number, optional): Position for change. If omitted, changes immediately.

**Returns:** `{ success: boolean, timeSignature: { numerator, denominator } }`

---

### Track Management

#### `create_track`
Create a new track.

**Parameters:**
- `type` (enum, required): "audio", "midi", "instrument", "bus", or "master"
- `name` (string, required): Track name
- `position` (number, optional): Insert position. Default: end of track list.

**Returns:** `{ success: boolean, trackId: string }`

---

#### `delete_track`
Remove a track and all its clips.

**Parameters:**
- `trackId` (string, required): UUID of track to delete

**Returns:** `{ success: boolean }`

---

#### `set_track_properties`
Update track properties.

**Parameters:**
- `trackId` (string, required): Track UUID
- `properties` (object, required): Properties to update
  - `name` (string, optional): Track name
  - `volumeDb` (number, optional): Volume in dB. Range: -∞ to +12
  - `pan` (number, optional): Pan position. Range: -1.0 to +1.0
  - `muted` (boolean, optional): Mute state
  - `solo` (boolean, optional): Solo state
  - `armed` (boolean, optional): Recording arm state
  - `color` (string, optional): Hex color code

**Returns:** `{ success: boolean, trackId: string }`

---

#### `reorder_tracks`
Change track order.

**Parameters:**
- `trackId` (string, required): Track to move
- `newPosition` (number, required): New order index (0-based)

**Returns:** `{ success: boolean }`

---

### Clip Management

#### `create_midi_clip`
Create a new MIDI clip on a track.

**Parameters:**
- `trackId` (string, required): Parent track UUID
- `startBeat` (number, required): Clip start position
- `lengthBeats` (number, required): Clip duration. Range: 0.25 to ∞
- `name` (string, optional): Clip name. Default: "MIDI Clip"

**Returns:** `{ success: boolean, clipId: string }`

---

#### `set_clip_notes`
Set or update notes in a MIDI clip.

**Parameters:**
- `clipId` (string, required): MIDI clip UUID
- `notes` (array, required): Array of note objects
  - `startBeat` (number): Note start (relative to clip)
  - `lengthBeats` (number): Note duration
  - `pitch` (number): MIDI note number (0-127)
  - `velocity` (number): Velocity (1-127)

**Returns:** `{ success: boolean, clipId: string, noteCount: number }`

---

#### `load_audio_clip`
Create an audio clip from a file.

**Parameters:**
- `trackId` (string, required): Parent track UUID
- `audioFileId` (string, required): Audio file asset UUID
- `startBeat` (number, required): Clip start position
- `name` (string, optional): Clip name. Default: filename

**Returns:** `{ success: boolean, clipId: string }`

---

#### `set_clip_properties`
Update clip properties.

**Parameters:**
- `clipId` (string, required): Clip UUID
- `properties` (object, required): Properties to update
  - `name` (string, optional)
  - `startBeat` (number, optional)
  - `lengthBeats` (number, optional)
  - `muted` (boolean, optional)
  - `color` (string, optional)
  - Audio-specific:
    - `fadeInBeats` (number, optional): Range: 0 to clip length
    - `fadeOutBeats` (number, optional): Range: 0 to clip length
    - `gain` (number, optional): Range: 0.0 to 2.0
    - `playbackRate` (number, optional): Range: 0.25 to 4.0
    - `pitchShift` (number, optional): Range: -12 to +12

**Returns:** `{ success: boolean, clipId: string }`

---

#### `delete_clip`
Remove a clip from its track.

**Parameters:**
- `clipId` (string, required): Clip UUID

**Returns:** `{ success: boolean }`

---

### Plugin Management

#### `insert_plugin`
Add a plugin to a track's processing chain.

**Parameters:**
- `trackId` (string, required): Parent track UUID
- `pluginId` (string, required): Plugin identifier (VST3/AU/internal)
- `position` (number, optional): Slot position. Default: end of chain

**Returns:** `{ success: boolean, pluginSlotId: string }`

---

#### `remove_plugin`
Remove a plugin from a track.

**Parameters:**
- `trackId` (string, required): Parent track UUID
- `pluginSlotId` (string, required): Plugin slot UUID

**Returns:** `{ success: boolean }`

---

#### `set_plugin_param`
Change a plugin parameter value.

**Parameters:**
- `pluginSlotId` (string, required): Plugin slot UUID
- `parameterId` (string, required): Plugin-specific parameter ID
- `value` (number, required): Normalized value (0.0-1.0)

**Returns:** `{ success: boolean }`

---

### Automation

#### `create_automation_lane`
Add an automation lane to a track.

**Parameters:**
- `trackId` (string, required): Parent track UUID
- `parameter` (string, required): Parameter to automate (e.g., "volume", "plugin.0.param.5")
- `parameterName` (string, required): Display name

**Returns:** `{ success: boolean, automationLaneId: string }`

---

#### `add_automation_point`
Add a point to an automation lane.

**Parameters:**
- `automationLaneId` (string, required): Automation lane UUID
- `beat` (number, required): Time position
- `value` (number, required): Normalized value (0.0-1.0)

**Returns:** `{ success: boolean }`

---

#### `set_automation_mode`
Change automation recording mode.

**Parameters:**
- `automationLaneId` (string, required): Automation lane UUID
- `mode` (enum, required): "off", "read", "write", "touch", or "latch"

**Returns:** `{ success: boolean }`

---

### Project Management

#### `save_project`
Save the current project to disk.

**Parameters:**
- `path` (string, required): File path for project
- `format` (enum, optional): "json" or "binary". Default: "json"

**Returns:** `{ success: boolean, path: string, size: number }`

---

#### `load_project`
Load a project from disk.

**Parameters:**
- `path` (string, required): File path to project

**Returns:** `{ success: boolean, projectId: string }`

---

#### `new_project`
Create a new empty project.

**Parameters:**
- `name` (string, required): Project name
- `sampleRate` (number, optional): Default: 48000
- `bitDepth` (number, optional): Default: 24

**Returns:** `{ success: boolean, projectId: string }`

---

## Events

Events are emitted when DAW state changes. Clients should listen and update their state accordingly.

### Project Events

#### `project_state`
Full project state snapshot. Emitted on load or major changes.

**Payload:**
```json
{
  "type": "project_state",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "project": { /* Full Project object */ }
  }
}
```

---

### Transport Events

#### `transport_state`
Playback state changed.

**Payload:**
```json
{
  "type": "transport_state",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "isPlaying": true,
    "isRecording": false,
    "position": 64.5,
    "tempo": 120,
    "timeSignature": { "numerator": 4, "denominator": 4 }
  }
}
```

---

#### `playhead_position`
Playhead moved (emitted frequently during playback).

**Payload:**
```json
{
  "type": "playhead_position",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "beat": 32.75,
    "bar": 8,
    "beat_in_bar": 3,
    "sample": 3456789
  }
}
```

---

### Track Events

#### `track_added`
New track created.

**Payload:**
```json
{
  "type": "track_added",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "track": { /* Full Track object */ }
  }
}
```

---

#### `track_updated`
Track properties changed.

**Payload:**
```json
{
  "type": "track_updated",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "trackId": "uuid-here",
    "changes": {
      "volumeDb": -6.0,
      "name": "New Name"
    }
  }
}
```

---

#### `track_removed`
Track deleted.

**Payload:**
```json
{
  "type": "track_removed",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "trackId": "uuid-here"
  }
}
```

---

### Clip Events

#### `clip_added`
Clip created on a track.

**Payload:**
```json
{
  "type": "clip_added",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "clip": { /* Full Clip object */ }
  }
}
```

---

#### `clip_updated`
Clip properties changed.

**Payload:**
```json
{
  "type": "clip_updated",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "clipId": "uuid-here",
    "changes": {
      "startBeat": 16,
      "lengthBeats": 8
    }
  }
}
```

---

#### `clip_removed`
Clip deleted from track.

**Payload:**
```json
{
  "type": "clip_removed",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "clipId": "uuid-here",
    "trackId": "track-uuid"
  }
}
```

---

### Plugin Events

#### `plugin_inserted`
Plugin added to track.

**Payload:**
```json
{
  "type": "plugin_inserted",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "pluginSlot": { /* Full PluginSlot object */ }
  }
}
```

---

#### `plugin_param_changed`
Plugin parameter value changed.

**Payload:**
```json
{
  "type": "plugin_param_changed",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "pluginSlotId": "uuid-here",
    "parameterId": "cutoff",
    "value": 0.75,
    "displayValue": "750 Hz"
  }
}
```

---

#### `plugin_removed`
Plugin removed from track.

**Payload:**
```json
{
  "type": "plugin_removed",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "pluginSlotId": "uuid-here",
    "trackId": "track-uuid"
  }
}
```

---

### Metering Events

#### `meters`
Audio level meters update (high frequency).

**Payload:**
```json
{
  "type": "meters",
  "timestamp": "2025-11-10T12:00:00Z",
  "data": {
    "tracks": {
      "track-uuid-1": {
        "peak": [-3.5, -2.8],
        "rms": [-12.3, -11.5]
      },
      "track-uuid-2": {
        "peak": [-6.2, -6.5],
        "rms": [-18.0, -17.8]
      }
    },
    "master": {
      "peak": [-1.2, -1.0],
      "rms": [-8.5, -8.2]
    }
  }
}
```

---

## Message Format

### Command Request
```json
{
  "id": "request-uuid",
  "command": "set_tempo",
  "params": {
    "tempo": 140
  }
}
```

### Command Response (Success)
```json
{
  "id": "request-uuid",
  "success": true,
  "result": {
    "tempo": 140
  }
}
```

### Command Response (Error)
```json
{
  "id": "request-uuid",
  "success": false,
  "error": {
    "code": "INVALID_PARAM",
    "message": "Tempo must be between 1 and 999 BPM",
    "details": {
      "param": "tempo",
      "value": 1500
    }
  }
}
```

### Event Format
```json
{
  "type": "event_name",
  "timestamp": "2025-11-10T12:00:00.123Z",
  "data": { /* event-specific payload */ }
}
```

---

## Error Handling

### Error Codes

| Code | Description |
|------|-------------|
| `INVALID_PARAM` | Parameter validation failed |
| `NOT_FOUND` | Entity (track/clip/plugin) not found |
| `PERMISSION_DENIED` | Operation not allowed in current state |
| `RESOURCE_BUSY` | Resource locked by another operation |
| `AUDIO_ERROR` | Audio engine failure |
| `IO_ERROR` | File system operation failed |
| `PLUGIN_ERROR` | Plugin loading or execution error |

### Error Response Structure
```typescript
{
  code: string;        // Machine-readable error code
  message: string;     // Human-readable description
  details?: object;    // Additional context
}
```

---

## Version History

### Version 1.0.0 (2025-11-10)
- Initial protocol specification
- Defined transport, track, clip, plugin, and automation commands
- Defined state change events and metering
- Established error handling conventions
