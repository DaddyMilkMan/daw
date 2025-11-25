# Command API Reference

This document describes the CommandAPI for Zenith DAW. These commands are designed to be used by AI agents (like Wingman) to control the DAW programmatically.

All commands follow the standard CommandAPI request/response format:

**Request:**
```json
{
  "command": "command_name",
  "params": { ... }
}
```

**Response (Success):**
```json
{
  "status": "ok",
  "data": { ... }
}
```

**Response (Error):**
```json
{
  "status": "error",
  "error": "Error message"
}
```

---

## Instrument Commands

### list_instruments

**Description:** List all available built-in instruments.

**Request:**
```json
{
  "command": "list_instruments",
  "params": {}
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "instruments": [
      {
        "id": "zenith_poly_synth",
        "name": "Zenith Poly Synth",
        "category": "Synth"
      },
      {
        "id": "zenith_sampler",
        "name": "Zenith Sampler",
        "category": "Sampler"
      }
    ]
  }
}
```

---

### list_presets

**Description:** List available presets for a specific instrument, with optional filtering.

**Request:**
```json
{
  "command": "list_presets",
  "params": {
    "instrumentId": "zenith_poly_synth",
    "category": "Factory",  // Optional
    "tag": "pad"           // Optional
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "presets": [
      {
        "id": "init_basic_pad_1234567890",
        "name": "Basic Pad",
        "category": "Factory",
        "tags": ["pad", "warm", "lush"]
      },
      {
        "id": "bright_pluck_1234567891",
        "name": "Bright Pluck",
        "category": "Factory",
        "tags": ["pluck", "bright", "lead"]
      }
    ]
  }
}
```

**Notes:**
- `category` filter matches against the preset author/category field
- `tag` filter checks if the preset has the specified tag

---

### load_preset

**Description:** Load a preset on a track's instrument. Optionally creates the instrument if not present.

**Request:**
```json
{
  "command": "load_preset",
  "params": {
    "trackId": "track_0",
    "presetId": "init_basic_pad_1234567890",
    "instrumentId": "zenith_poly_synth"  // Optional - only needed if track has no instrument
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Notes:**
- If track has no instrument and `instrumentId` is provided, the instrument will be created automatically
- Creates an undo transaction with name "Load preset {presetId}"
- All parameter values are set according to the preset

---

### save_preset

**Description:** Save the current instrument state as a new user preset.

**Request:**
```json
{
  "command": "save_preset",
  "params": {
    "trackId": "track_0",
    "name": "My Custom Pad",
    "category": "User",                    // Optional, defaults to "User"
    "tags": ["custom", "pad", "experimental"]  // Optional
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "presetId": "my_custom_pad_1234567892"
  }
}
```

**Notes:**
- Captures all current parameter and macro values
- Preset is saved to the user preset directory
- Returns the generated preset ID for future use

---

### get_instrument_parameters

**Description:** Get all parameters for a track's instrument, including current values.

**Request:**
```json
{
  "command": "get_instrument_parameters",
  "params": {
    "trackId": "track_0"
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "parameters": [
      {
        "id": "filter_cutoff",
        "name": "Filter Cutoff",
        "min": 0.0,
        "max": 1.0,
        "default": 0.8,
        "value": 0.65,
        "tags": []
      },
      {
        "id": "filter_resonance",
        "name": "Filter Resonance",
        "min": 0.0,
        "max": 1.0,
        "default": 0.5,
        "value": 0.3,
        "tags": []
      }
    ]
  }
}
```

**Notes:**
- Returns all parameters defined in the instrument's metadata
- Values are normalized (0-1 range)
- `value` reflects the current state of the parameter

---

### set_instrument_parameters

**Description:** Set multiple instrument parameters at once. This is the batch version for efficient parameter updates.

**Request:**
```json
{
  "command": "set_instrument_parameters",
  "params": {
    "trackId": "track_0",
    "params": {
      "filter_cutoff": 0.8,
      "filter_resonance": 0.3,
      "attack": 0.1
    }
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "updated": ["filter_cutoff", "filter_resonance", "attack"]
  }
}
```

**Notes:**
- Creates an undo transaction with name "AI tweak parameters"
- Parameters are validated against the instrument's metadata
- Invalid parameter IDs are skipped (logged but don't cause errors)
- Values are automatically clamped to the valid range [min, max]
- Returns list of successfully updated parameters

---

### set_instrument_on_track

**Description:** Assign an instrument to a track. Creates the instrument and attaches it to the specified track.

**Request:**
```json
{
  "command": "set_instrument_on_track",
  "params": {
    "trackId": "track_0",
    "instrumentId": "zenith_poly_synth"
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "instrumentId": "zenith_poly_synth"
  }
}
```

**Notes:**
- Creates an undo transaction with name "Set instrument {instrumentId} on {trackId}"
- If track already has an instrument, it will be replaced
- The instrument is created using the InstrumentRegistry
- Track type does not need to be "instrument" - MIDI tracks can have instruments

---

### set_instrument_param

**Description:** Set a single instrument parameter. Use this for individual parameter tweaks.

**Request:**
```json
{
  "command": "set_instrument_param",
  "params": {
    "trackId": "track_0",
    "paramId": "filter_cutoff",
    "value": 0.8
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "paramId": "filter_cutoff",
    "value": 0.8
  }
}
```

**Notes:**
- Creates an undo transaction with name "Set {paramId} to {value}"
- Parameter ID is validated against the instrument's metadata
- Value is automatically clamped to the valid range [min, max]
- Returns the actual value set (after clamping)

---

### get_instrument_param

**Description:** Get a single instrument parameter's current value and metadata.

**Request:**
```json
{
  "command": "get_instrument_param",
  "params": {
    "trackId": "track_0",
    "paramId": "filter_cutoff"
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "paramId": "filter_cutoff",
    "name": "Filter Cutoff",
    "value": 0.65,
    "min": 0.0,
    "max": 1.0,
    "default": 0.8
  }
}
```

**Notes:**
- Returns full parameter metadata along with current value
- Useful for understanding parameter ranges before setting
- No undo transaction created (read-only operation)

---

### randomize_instrument_params

**Description:** Randomize all instrument parameters with safe ranges. Useful for sound exploration and variation.

**Request:**
```json
{
  "command": "randomize_instrument_params",
  "params": {
    "trackId": "track_0",
    "intensity": 0.5
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "randomized": ["filter_cutoff", "filter_resonance", "attack", "decay", "sustain", "release"],
    "intensity": 0.5
  }
}
```

**Notes:**
- Creates an undo transaction with name "Randomize instrument parameters"
- `intensity` parameter is optional (default: 0.7, range: 0.0-1.0)
- Intensity controls how far from current values parameters can deviate
- At intensity=0.5, parameters can change by up to ±25% of their total range
- All values are clamped to valid parameter ranges
- Returns list of successfully randomized parameters

---

## Track Management Commands

### add_track

**Description:** Add a new track to the project.

**Request:**
```json
{
  "command": "add_track",
  "params": {
    "name": "Lead Synth",
    "type": "midi"  // "audio" or "midi"
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "trackId": "track_3"
  }
}
```

---

## Automation Commands

### add_automation_point

**Description:** Add an automation point for a track parameter.

**Request:**
```json
{
  "command": "add_automation_point",
  "params": {
    "trackId": "track_0",
    "param": "volume",      // "volume", "pan", or "mute"
    "timeBeats": 8.0,
    "value": 0.5
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "pointId": "point_123"
  }
}
```

---

### clear_automation

**Description:** Clear all automation points for a track parameter.

**Request:**
```json
{
  "command": "clear_automation",
  "params": {
    "trackId": "track_0",
    "param": "volume"
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

---

### get_automation

**Description:** Get all automation points for a track parameter.

**Request:**
```json
{
  "command": "get_automation",
  "params": {
    "trackId": "track_0",
    "param": "volume"
  }
}
```

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

---

## Transport Commands

### play

**Description:** Start playback.

**Request:**
```json
{
  "command": "play",
  "params": {}
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

---

### stop

**Description:** Stop playback.

**Request:**
```json
{
  "command": "stop",
  "params": {}
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

---

## Project Commands

### get_project_info

**Description:** Get project information.

**Request:**
```json
{
  "command": "get_project_info",
  "params": {}
}
```

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

---

### set_tempo

**Description:** Set project tempo.

**Request:**
```json
{
  "command": "set_tempo",
  "params": {
    "tempo": 140.0
  }
}
```

**Response:**
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

---

## Undo/Redo Commands

### undo

**Description:** Undo the last action.

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
  "status": "ok",
  "data": {
    "success": true,
    "canUndo": false,
    "canRedo": true
  }
}
```

---

### redo

**Description:** Redo the last undone action.

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
  "status": "ok",
  "data": {
    "success": true,
    "canUndo": true,
    "canRedo": false
  }
}
```

---

## Thread Safety

All commands run on the **MESSAGE THREAD** and are thread-safe:
- Parameter changes use JUCE's standard parameter API
- No direct audio thread access is required
- Changes are applied atomically and safely
- Undo/redo integration is automatic via ProjectState

---

## AI Agent Usage Examples

### Example 1: Turn a "basic pad" into a Future Bass pluck

This workflow demonstrates how to transform a warm pad sound into a bright, punchy Future Bass pluck by adjusting filter and envelope parameters.

```json
[
  {
    "command": "add_track",
    "params": {
      "name": "Future Bass Lead",
      "type": "midi"
    }
  },
  {
    "command": "load_preset",
    "params": {
      "trackId": "track_0",
      "instrumentId": "zenith_poly_synth",
      "presetId": "init_basic_pad_1234567890"
    }
  },
  {
    "command": "set_instrument_parameters",
    "params": {
      "trackId": "track_0",
      "params": {
        "filter_cutoff": 0.95,
        "filter_resonance": 0.7,
        "attack": 0.0,
        "decay": 0.15,
        "sustain": 0.3,
        "release": 0.2
      }
    }
  },
  {
    "command": "save_preset",
    "params": {
      "trackId": "track_0",
      "name": "Future Bass Pluck",
      "category": "User",
      "tags": ["pluck", "future bass", "bright", "punchy"]
    }
  }
]
```

**Explanation:**
1. Create a new MIDI track for the lead sound
2. Load the "Basic Pad" preset as a starting point
3. Transform it into a pluck by:
   - Increasing filter cutoff to 0.95 for brightness
   - Increasing resonance to 0.7 for character
   - Setting attack to 0.0 for instant onset
   - Short decay (0.15) and low sustain (0.3) for pluck characteristic
   - Medium release (0.2) to avoid clicks
4. Save the result as a new preset for future use

---

### Example 2: Swap a track's preset to a random 808

This workflow demonstrates discovering available 808 presets and loading one randomly.

```json
[
  {
    "command": "list_presets",
    "params": {
      "instrumentId": "zenith_sampler",
      "tag": "808"
    }
  }
]
```

Response:
```json
{
  "status": "ok",
  "data": {
    "presets": [
      {
        "id": "808_kick_classic_1234567893",
        "name": "808 Kick Classic",
        "category": "Factory",
        "tags": ["808", "kick", "bass"]
      },
      {
        "id": "808_snare_short_1234567894",
        "name": "808 Snare Short",
        "category": "Factory",
        "tags": ["808", "snare", "percussion"]
      },
      {
        "id": "808_hihat_closed_1234567895",
        "name": "808 Hi-Hat Closed",
        "category": "Factory",
        "tags": ["808", "hihat", "percussion"]
      }
    ]
  }
}
```

**AI Agent Logic:**
```javascript
// Parse response and pick a random preset
const presets = response.data.presets;
const randomPreset = presets[Math.floor(Math.random() * presets.length)];

// Load the random preset
{
  "command": "load_preset",
  "params": {
    "trackId": "track_2",
    "presetId": randomPreset.id
  }
}
```

Final command:
```json
{
  "command": "load_preset",
  "params": {
    "trackId": "track_2",
    "presetId": "808_kick_classic_1234567893"
  }
}
```

**Explanation:**
1. Query all presets for the sampler with the "808" tag
2. AI randomly selects one from the returned list
3. Load the selected preset on track 2

---

### Example 3: Complete Workflow - Create Track, Assign Instrument, Load Preset, Tweak Parameters

This workflow demonstrates the full instrument setup process from track creation to parameter tweaking.

**Step 1: Create a MIDI track for the instrument**
```json
{
  "command": "add_track",
  "params": {
    "name": "Synth Lead",
    "type": "midi"
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "trackId": "track_0"
  }
}
```

**Step 2: Assign ZenithPolySynth to the track**
```json
{
  "command": "set_instrument_on_track",
  "params": {
    "trackId": "track_0",
    "instrumentId": "zenith_poly_synth"
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "instrumentId": "zenith_poly_synth"
  }
}
```

**Step 3: List available presets**
```json
{
  "command": "list_presets",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "presets": [
      {
        "id": "init_basic_pad_1234567890",
        "name": "Basic Pad",
        "category": "Factory",
        "tags": ["pad", "warm", "lush"]
      },
      {
        "id": "bright_pluck_1234567891",
        "name": "Bright Pluck",
        "category": "Factory",
        "tags": ["pluck", "bright", "lead"]
      }
    ]
  }
}
```

**Step 4: Load a preset**
```json
{
  "command": "load_preset",
  "params": {
    "trackId": "track_0",
    "presetId": "init_basic_pad_1234567890"
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "success": true
  }
}
```

**Step 5: Tweak filter cutoff**
```json
{
  "command": "set_instrument_param",
  "params": {
    "trackId": "track_0",
    "paramId": "filter_cutoff",
    "value": 0.85
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "paramId": "filter_cutoff",
    "value": 0.85
  }
}
```

**Step 6: Adjust envelope parameters**
```json
{
  "command": "set_instrument_parameters",
  "params": {
    "trackId": "track_0",
    "params": {
      "attack": 0.05,
      "decay": 0.3,
      "sustain": 0.6,
      "release": 0.4
    }
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "updated": ["attack", "decay", "sustain", "release"]
  }
}
```

**Complete Command Sequence:**
```json
[
  {
    "command": "add_track",
    "params": { "name": "Synth Lead", "type": "midi" }
  },
  {
    "command": "set_instrument_on_track",
    "params": { "trackId": "track_0", "instrumentId": "zenith_poly_synth" }
  },
  {
    "command": "load_preset",
    "params": { "trackId": "track_0", "presetId": "init_basic_pad_1234567890" }
  },
  {
    "command": "set_instrument_param",
    "params": { "trackId": "track_0", "paramId": "filter_cutoff", "value": 0.85 }
  },
  {
    "command": "set_instrument_parameters",
    "params": {
      "trackId": "track_0",
      "params": {
        "attack": 0.05,
        "decay": 0.3,
        "sustain": 0.6,
        "release": 0.4
      }
    }
  }
]
```

---

### Example 4: Sound Exploration with Randomization

This workflow demonstrates using randomization to explore sound variations quickly.

**Step 1: Randomize with low intensity for subtle variations**
```json
{
  "command": "randomize_instrument_params",
  "params": {
    "trackId": "track_0",
    "intensity": 0.3
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "randomized": ["filter_cutoff", "filter_resonance", "attack", "decay", "sustain", "release", "osc1_level", "osc2_level"],
    "intensity": 0.3
  }
}
```

**Step 2: Check a specific parameter after randomization**
```json
{
  "command": "get_instrument_param",
  "params": {
    "trackId": "track_0",
    "paramId": "filter_cutoff"
  }
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "paramId": "filter_cutoff",
    "name": "Filter Cutoff",
    "value": 0.73,
    "min": 0.0,
    "max": 1.0,
    "default": 0.8
  }
}
```

**Step 3: If you don't like the result, undo and try again**
```json
{
  "command": "undo",
  "params": {}
}
```

Response:
```json
{
  "status": "ok",
  "data": {
    "success": true,
    "canUndo": true,
    "canRedo": true
  }
}
```

---

## Audio/MIDI Raw Access Helpers

These commands expose offline audio renders and full MIDI note dumps so AI agents can analyze or regenerate content without touching the audio thread.

### export_audio

**Description:** Offline render the current project mix to a WAV file for analysis or downstream processing.

**Request:**
```json
{
  "command": "export_audio",
  "params": {
    "outputPath": "C:/temp/zenith_render.wav",
    "sampleRate": 48000,
    "bitDepth": 24,
    "durationSeconds": 0
  }
}
```

**Response:**
```json
{
  "success": true,
  "result": {
    "outputPath": "C:/temp/zenith_render.wav",
    "sampleRate": 48000,
    "bitDepth": 24,
    "durationSeconds": 0
  }
}
```

**Notes:**
- Runs on the message thread using the offline export path; never touches the real-time audio callback.
- `durationSeconds` of 0 auto-detects from the project length fallback in the engine.

### get_midi_data

**Description:** Retrieve MIDI note data for all MIDI clips (or a specific track/clip) to enable AI-side analysis or transformation.

**Request (single track):**
```json
{
  "command": "get_midi_data",
  "params": {
    "trackId": "track_1"
  }
}
```

**Response:**
```json
{
  "success": true,
  "result": {
    "tracks": [
      {
        "id": "track_1",
        "type": "midi",
        "clipCount": 1,
        "clips": [
          {
            "id": "clip_5",
            "trackId": "track_1",
            "startBeats": 0.0,
            "lengthBeats": 8.0,
            "laneIndex": 0,
            "noteCount": 2,
            "notes": [
              { "id": "note_1", "pitch": 60, "startBeats": 0.0, "lengthBeats": 1.0, "velocity": 100, "muted": false },
              { "id": "note_2", "pitch": 64, "startBeats": 1.0, "lengthBeats": 1.0, "velocity": 95, "muted": false }
            ]
          }
        ]
      }
    ],
    "trackCount": 1
  }
}
```

**Notes:**
- Optional `trackId` and `clipId` filters narrow the dump; otherwise all MIDI clips are returned.
- Clips are sourced from ProjectState, preserving beat positions, lane indices, velocities, and mute flags.

### set_clip_notes

**Description:** Replace all MIDI notes in a clip with a provided list so AI can regenerate parts after analysis.

**Request:**
```json
{
  "command": "set_clip_notes",
  "params": {
    "trackId": "track_1",
    "clipId": "clip_5",
    "notes": [
      { "pitch": 36, "startBeats": 0.0, "lengthBeats": 0.5, "velocity": 110 },
      { "pitch": 42, "startBeats": 0.5, "lengthBeats": 0.5, "velocity": 105 }
    ]
  }
}
```

**Response:**
```json
{
  "success": true,
  "result": {
    "trackId": "track_1",
    "clipId": "clip_5",
    "cleared": 4,
    "added": 2
  }
}
```

**Notes:**
- Only valid for MIDI clips; returns an error for audio clips.
- Existing notes are removed before the new set is inserted, each under its own undo transaction.

---

## Best Practices for AI Agents

1. **Discovery First**: Always use `list_instruments` and `list_presets` before making assumptions about available resources

2. **Validate Before Set**: Use `get_instrument_parameters` or `get_instrument_param` to understand available parameters before setting

3. **Batch vs. Single Parameter Updates**:
   - Use `set_instrument_parameters` for multiple parameters at once (more efficient)
   - Use `set_instrument_param` for single parameter tweaks (creates cleaner undo history)

4. **Handle Errors Gracefully**: Check response status and handle errors appropriately

5. **Use Descriptive Preset Names**: When saving presets, use clear, descriptive names and tags for future discoverability

6. **Leverage Undo**: All instrument commands create undo transactions automatically - no need to manage this manually

7. **Normalized Values**: All parameter values are normalized to [0, 1] range. The system handles conversion to actual units internally

8. **Randomization for Exploration**: Use `randomize_instrument_params` with varying intensities to discover interesting sounds quickly

9. **Track Setup Sequence**: Follow the pattern: create track → set instrument → load preset → tweak parameters
