# Zenith DAW Core Data Model

**Version:** 1.0.0
**Last Updated:** 2025-11-10

This document defines the canonical data model for Zenith DAW. It serves as the single source of truth for all data structures used throughout the application.

## Table of Contents

1. [Units and Conventions](#units-and-conventions)
2. [Project](#project)
3. [Track](#track)
4. [Clip](#clip)
5. [TempoMap](#tempomap)
6. [AutomationLane](#automationlane)
7. [PluginSlot](#pluginslot)
8. [Marker](#marker)
9. [ID Rules](#id-rules)

---

## Units and Conventions

### Time Units
- **Beats**: Musical time, relative to tempo. Used for clip positions, markers, and automation points.
- **Samples**: Absolute time at the project sample rate. Used internally for audio processing.
- **Seconds**: Real time. Used for display and audio file metadata.

### Audio Units
- **Gain/Volume**: Decibels (dB), range -∞ to +12 dB
- **Pan**: Normalized range -1.0 (left) to +1.0 (right), 0.0 = center
- **Level Meters**: dB, typically -60 dB to 0 dB range

### MIDI Units
- **Note Number**: MIDI standard 0-127 (C-1 to G9)
- **Velocity**: 0-127 (0 = note off)
- **CC Values**: 0-127

### General
- **BPM (Tempo)**: Beats per minute, range 1-999
- **Time Signature**: Numerator (1-64), Denominator (1, 2, 4, 8, 16, 32)

---

## Project

The root container for all DAW data.

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | UUID v4 string | Unique project identifier |
| `name` | string | Project name |
| `version` | string | Schema version (semver) |
| `sampleRate` | number | Audio sample rate (e.g., 44100, 48000, 96000) |
| `bitDepth` | number | Audio bit depth (16, 24, 32) |
| `tempoMap` | TempoMap | Global tempo and time signature changes |
| `tracks` | Track[] | Array of all tracks in project |
| `markers` | Marker[] | Array of timeline markers |
| `createdAt` | ISO 8601 string | Creation timestamp |
| `modifiedAt` | ISO 8601 string | Last modification timestamp |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `description` | string | Project description/notes |
| `author` | string | Creator name |
| `loopStart` | number | Loop region start (beats) |
| `loopEnd` | number | Loop region end (beats) |
| `loopEnabled` | boolean | Whether loop is active |

### Example

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "My Song",
  "version": "1.0.0",
  "sampleRate": 48000,
  "bitDepth": 24,
  "tempoMap": { ... },
  "tracks": [ ... ],
  "markers": [ ... ],
  "createdAt": "2025-11-10T12:00:00Z",
  "modifiedAt": "2025-11-10T14:30:00Z"
}
```

---

## Track

Represents an audio, MIDI, instrument, bus, or master track.

### Track Types

- **audio**: Records and plays back audio clips
- **midi**: Contains MIDI clips, outputs to instruments
- **instrument**: MIDI track with built-in virtual instrument
- **bus**: Routing/mixing track (no direct input)
- **master**: Final output track (one per project)

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | UUID v4 string | Unique track identifier |
| `name` | string | Track name |
| `type` | enum | One of: audio, midi, instrument, bus, master |
| `order` | number | Display order (0-based index) |
| `volumeDb` | number | Track volume in dB (-∞ to +12) |
| `pan` | number | Pan position (-1.0 to +1.0) |
| `muted` | boolean | Track mute state |
| `solo` | boolean | Track solo state |
| `armed` | boolean | Recording arm state |
| `inputSource` | string | Input routing (device ID or "none") |
| `outputTarget` | string | Output routing (track ID or "master") |
| `pluginSlots` | PluginSlot[] | Ordered plugin chain |
| `automationLanes` | AutomationLane[] | Parameter automation |
| `clips` | Clip[] | Audio or MIDI clips on this track |
| `color` | string | Hex color code (e.g., "#3b82f6") |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `height` | number | UI display height in pixels |
| `folded` | boolean | UI collapsed state |
| `frozen` | boolean | Track freeze state (render audio) |
| `sendSlots` | SendSlot[] | Send routing to buses |

### Example

```json
{
  "id": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "name": "Vocals",
  "type": "audio",
  "order": 0,
  "volumeDb": -6.0,
  "pan": 0.0,
  "muted": false,
  "solo": false,
  "armed": true,
  "inputSource": "input-1",
  "outputTarget": "master",
  "pluginSlots": [],
  "automationLanes": [],
  "clips": [],
  "color": "#3b82f6"
}
```

---

## Clip

Represents a region of audio or MIDI data on a track.

### Clip Types

- **audio**: References an audio file with playback parameters
- **midi**: Contains MIDI note/CC data

### Common Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | UUID v4 string | Unique clip identifier |
| `trackId` | UUID v4 string | Parent track ID |
| `type` | enum | "audio" or "midi" |
| `name` | string | Clip name |
| `startBeat` | number | Timeline position in beats |
| `lengthBeats` | number | Clip duration in beats |
| `color` | string | Hex color code |
| `muted` | boolean | Clip mute state |
| `loopEnabled` | boolean | Whether clip content loops |

### Audio Clip Additional Fields

| Field | Type | Description |
|-------|------|-------------|
| `audioFileId` | UUID v4 string | Reference to audio file asset |
| `sourceStart` | number | Start offset in source file (seconds) |
| `sourceEnd` | number | End offset in source file (seconds) |
| `fadeInBeats` | number | Fade in duration (beats) |
| `fadeOutBeats` | number | Fade out duration (beats) |
| `fadeInCurve` | enum | "linear", "exponential", "logarithmic", "sCurve" |
| `fadeOutCurve` | enum | "linear", "exponential", "logarithmic", "sCurve" |
| `gain` | number | Clip gain (0.0 to 2.0, 1.0 = unity) |
| `playbackRate` | number | Time-stretch rate (0.25 to 4.0) |
| `pitchShift` | number | Semitones (-12 to +12) |
| `reversed` | boolean | Play audio backwards |

### MIDI Clip Additional Fields

| Field | Type | Description |
|-------|------|-------------|
| `notes` | MidiNote[] | Array of MIDI notes |

### MidiNote Structure

| Field | Type | Description |
|-------|------|-------------|
| `startBeat` | number | Note start position (relative to clip) |
| `lengthBeats` | number | Note duration |
| `pitch` | number | MIDI note number (0-127) |
| `velocity` | number | Note velocity (1-127) |

### Example Audio Clip

```json
{
  "id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "trackId": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "type": "audio",
  "name": "Vocal Take 1",
  "startBeat": 0,
  "lengthBeats": 32,
  "color": "#3b82f6",
  "muted": false,
  "loopEnabled": false,
  "audioFileId": "file-uuid-here",
  "sourceStart": 0.5,
  "sourceEnd": 15.2,
  "fadeInBeats": 0.5,
  "fadeOutBeats": 1.0,
  "fadeInCurve": "exponential",
  "fadeOutCurve": "exponential",
  "gain": 1.0,
  "playbackRate": 1.0,
  "pitchShift": 0,
  "reversed": false
}
```

---

## TempoMap

Defines tempo and time signature changes over time.

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `defaultTempo` | number | Default BPM (1-999) |
| `defaultTimeSignature` | object | Default time signature |
| `defaultTimeSignature.numerator` | number | Beats per bar (1-64) |
| `defaultTimeSignature.denominator` | number | Note value (1, 2, 4, 8, 16, 32) |
| `changes` | TempoChange[] | Array of tempo/time signature changes |

### TempoChange Structure

| Field | Type | Description |
|-------|------|-------------|
| `beat` | number | Position in beats where change occurs |
| `tempo` | number | New tempo (BPM, 1-999) |
| `timeSignature` | object | Optional time signature change |

### Example

```json
{
  "defaultTempo": 120,
  "defaultTimeSignature": {
    "numerator": 4,
    "denominator": 4
  },
  "changes": [
    {
      "beat": 64,
      "tempo": 140,
      "timeSignature": {
        "numerator": 6,
        "denominator": 8
      }
    }
  ]
}
```

---

## AutomationLane

Parameter automation over time.

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | UUID v4 string | Unique automation lane identifier |
| `trackId` | UUID v4 string | Parent track ID |
| `parameter` | string | Parameter identifier (e.g., "volume", "pan", "plugin.0.param.5") |
| `parameterName` | string | Human-readable parameter name |
| `mode` | enum | "off", "read", "write", "touch", "latch" |
| `points` | AutomationPoint[] | Ordered automation points |
| `curve` | enum | Interpolation: "linear", "bezier", "stepped" |

### AutomationPoint Structure

| Field | Type | Description |
|-------|------|-------------|
| `beat` | number | Time position in beats |
| `value` | number | Normalized value (0.0-1.0) |
| `curve` | number | Optional curve tension (-1.0 to 1.0) |

### Example

```json
{
  "id": "auto-lane-uuid",
  "trackId": "7c9e6679-7425-40de-944b-e07fc1f90ae7",
  "parameter": "volume",
  "parameterName": "Track Volume",
  "mode": "read",
  "points": [
    { "beat": 0, "value": 0.8 },
    { "beat": 16, "value": 0.5, "curve": 0.5 },
    { "beat": 32, "value": 0.8 }
  ],
  "curve": "bezier"
}
```

---

## PluginSlot

A plugin instance in a track's processing chain.

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | UUID v4 string | Unique slot identifier |
| `pluginId` | string | Plugin identifier (VST3/AU/internal ID) |
| `pluginName` | string | Display name |
| `pluginType` | enum | "instrument", "effect", "utility" |
| `order` | number | Position in plugin chain (0-based) |
| `enabled` | boolean | Bypass state (false = bypassed) |
| `parameters` | object | Key-value map of parameter ID to value |
| `state` | string | Plugin state blob (base64 encoded) |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `preset` | string | Current preset name |
| `presetData` | string | Preset data (base64) |

### Example

```json
{
  "id": "plugin-slot-uuid",
  "pluginId": "com.fabfilter.ProQ3",
  "pluginName": "FabFilter Pro-Q 3",
  "pluginType": "effect",
  "order": 0,
  "enabled": true,
  "parameters": {
    "bypass": 0,
    "output_gain": 0.5
  },
  "state": "base64-encoded-plugin-state"
}
```

---

## Marker

Timeline markers for navigation and arrangement.

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | UUID v4 string | Unique marker identifier |
| `name` | string | Marker name |
| `beat` | number | Position in beats |
| `color` | string | Hex color code |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `lengthBeats` | number | For region markers (omit for point markers) |
| `description` | string | Marker notes |

### Example

```json
{
  "id": "marker-uuid",
  "name": "Chorus",
  "beat": 32,
  "color": "#22c55e",
  "lengthBeats": 16,
  "description": "Main chorus section"
}
```

---

## ID Rules

All entities use **UUID v4** (RFC 4122) for unique identification.

### Format
- **Pattern**: `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx`
- **Example**: `550e8400-e29b-41d4-a716-446655440000`

### Generation
- Use `crypto.randomUUID()` in browser/Node.js
- Must be lowercase
- Must include version indicator (4) and variant bits

### Validation Regex
```regex
^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$
```

### References
- Parent-child relationships use UUID references
- Example: `Clip.trackId` references `Track.id`
- Orphaned references should be handled gracefully

---

## Change Log

### Version 1.0.0 (2025-11-10)
- Initial core model definition
- Defined: Project, Track, Clip, TempoMap, AutomationLane, PluginSlot, Marker
- Established units and ID rules
