# Instrument CommandAPI Reference

This document describes the CommandAPI extensions for controlling built-in instruments in Zenith DAW. These commands are designed to be used by AI agents (like Wingman) to discover, configure, and control instruments without needing to know internal implementation details.

## Overview

The Instrument CommandAPI provides:
- **Discovery**: List available instruments and their capabilities
- **Configuration**: Attach instruments to tracks and load presets
- **Control**: Modify parameters and macros in real-time

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
  "success": true,
  "data": { ... }
}
```

**Response (Error):**
```json
{
  "success": false,
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable error message"
  }
}
```

## Error Codes

| Code | Description |
|------|-------------|
| `UNKNOWN_TRACK` | Track ID not found |
| `TRACK_HAS_NO_INSTRUMENT` | Track exists but has no instrument attached |
| `UNKNOWN_INSTRUMENT` | Instrument ID not recognized |
| `UNKNOWN_PARAMETER` | Parameter ID not found on instrument |
| `UNKNOWN_MACRO` | Macro ID not found on instrument |
| `UNKNOWN_PRESET` | Preset ID not found for instrument |
| `INVALID_VALUE` | Missing or invalid parameter value |
| `INTERNAL_ERROR` | Unexpected internal error |

---

## Commands

### 1. list_instruments

**Description:** Get a list of all available built-in instruments.

**Request:**
```json
{
  "command": "list_instruments"
}
```

**Response:**
```json
{
  "success": true,
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
```

---

### 2. describe_instrument

**Description:** Get full metadata for an instrument, including all parameters and macros.

**Request:**
```json
{
  "command": "describe_instrument",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
```

**Response:**
```json
{
  "success": true,
  "instrument": {
    "instrumentId": "zenith_poly_synth",
    "name": "Zenith Poly Synth",
    "category": "Synth",
    "description": "Polyphonic synthesizer with oscillator, filter, and envelope",
    "parameters": [
      {
        "id": "osc_type",
        "name": "Oscillator Type",
        "category": "Oscillator",
        "type": "choice",
        "defaultValue": 0.0,
        "minValue": 0.0,
        "maxValue": 2.0,
        "units": "",
        "choices": ["Sine", "Saw", "Square"]
      },
      {
        "id": "filter_cutoff",
        "name": "Filter Cutoff",
        "category": "Filter",
        "type": "float",
        "defaultValue": 0.8,
        "minValue": 0.0,
        "maxValue": 1.0,
        "units": "%"
      },
      ...
    ],
    "macros": [
      {
        "id": "macro_brightness",
        "name": "Brightness",
        "description": "Controls filter cutoff and resonance",
        "targets": [
          {
            "parameterId": "filter_cutoff",
            "amount": 0.8
          },
          {
            "parameterId": "filter_resonance",
            "amount": 0.5
          }
        ]
      },
      ...
    ]
  }
}
```

**Parameter Types:**
- `float`: Continuous value (0.0 - 1.0)
- `bool`: On/Off (0.0 or 1.0)
- `choice`: Discrete selection (see `choices` array)

---

### 3. list_instrument_presets

**Description:** Get available presets for a specific instrument.

**Request:**
```json
{
  "command": "list_instrument_presets",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
```

**Response:**
```json
{
  "success": true,
  "presets": [
    "init_basic_pad",
    "bright_pluck",
    "lofi_keys_01"
  ]
}
```

---

### 4. set_track_instrument

**Description:** Attach an instrument to a track, optionally loading a preset.

**Request:**
```json
{
  "command": "set_track_instrument",
  "params": {
    "trackId": "track_1",
    "instrumentId": "zenith_poly_synth",
    "presetId": "init_basic_pad"  // Optional
  }
}
```

**Response:**
```json
{
  "success": true,
  "instrument": {
    "instrumentId": "zenith_poly_synth",
    "name": "Zenith Poly Synth",
    ...
  },
  "presetId": "init_basic_pad"  // If preset was loaded
}
```

**Notes:**
- The track will be converted to an Instrument track if needed
- If no preset is specified, default parameter values are used
- Previous instrument (if any) is replaced

---

### 5. set_instrument_param

**Description:** Set a specific parameter on a track's instrument.

**Request:**
```json
{
  "command": "set_instrument_param",
  "params": {
    "trackId": "track_1",
    "parameterId": "filter_cutoff",
    "value": 0.65
  }
}
```

**Response:**
```json
{
  "success": true,
  "parameterId": "filter_cutoff",
  "value": 0.65
}
```

**Notes:**
- Value is automatically clamped to valid range [0.0, 1.0]
- Parameter must exist on the instrument (use `describe_instrument` to check)
- Track must have an instrument attached

**Error Examples:**
```json
{
  "success": false,
  "error": {
    "code": "TRACK_HAS_NO_INSTRUMENT",
    "message": "Track 'track_1' has no instrument attached"
  }
}
```

```json
{
  "success": false,
  "error": {
    "code": "UNKNOWN_PARAMETER",
    "message": "Parameter 'osc3_detune' not found on instrument 'zenith_poly_synth'"
  }
}
```

---

### 6. set_instrument_macro

**Description:** Set a macro value (affects multiple parameters simultaneously).

**Request:**
```json
{
  "command": "set_instrument_macro",
  "params": {
    "trackId": "track_1",
    "macroId": "macro_brightness",
    "value": 0.9
  }
}
```

**Response:**
```json
{
  "success": true,
  "macroId": "macro_brightness",
  "value": 0.9
}
```

**Notes:**
- Macros provide high-level control over multiple related parameters
- Each macro has targets that define which parameters it affects and by how much
- Use `describe_instrument` to see available macros and their targets

---

### 7. load_instrument_preset

**Description:** Load a preset for the instrument on a track.

**Request:**
```json
{
  "command": "load_instrument_preset",
  "params": {
    "trackId": "track_1",
    "presetId": "lofi_keys_01"
  }
}
```

**Response:**
```json
{
  "success": true,
  "presetId": "lofi_keys_01"
}
```

**Notes:**
- Preset must exist for the instrument (use `list_instrument_presets` to check)
- All parameter values are set according to the preset
- Track must have an instrument attached

---

## Example Workflows

### Workflow 1: Create a Synth Track

Create a new instrument track with a synthesizer and customize it:

```json
// 1. Add a new track
{
  "command": "add_track",
  "params": {
    "name": "Lead Synth",
    "type": "midi"
  }
}
// Response: { "success": true, "trackId": "track_0" }

// 2. Attach the poly synth
{
  "command": "set_track_instrument",
  "params": {
    "trackId": "track_0",
    "instrumentId": "zenith_poly_synth",
    "presetId": "init_basic_pad"
  }
}

// 3. Adjust filter for brightness
{
  "command": "set_instrument_param",
  "params": {
    "trackId": "track_0",
    "parameterId": "filter_cutoff",
    "value": 0.9
  }
}

// 4. Speed up envelope
{
  "command": "set_instrument_macro",
  "params": {
    "trackId": "track_0",
    "macroId": "macro_envelope_speed",
    "value": 0.3
  }
}
```

### Workflow 2: Discover and Explore Instruments

Systematically explore available instruments:

```json
// 1. List all instruments
{
  "command": "list_instruments"
}
// Response: Lists zenith_poly_synth and zenith_sampler

// 2. Describe the poly synth
{
  "command": "describe_instrument",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
// Response: Full metadata with parameters and macros

// 3. Check available presets
{
  "command": "list_instrument_presets",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
// Response: ["init_basic_pad", "bright_pluck", "lofi_keys_01"]
```

### Workflow 3: Create Warm Sampler Track

```json
// 1. Create track
{
  "command": "add_track",
  "params": {
    "name": "Sampler",
    "type": "midi"
  }
}

// 2. Set sampler instrument with warm preset
{
  "command": "set_track_instrument",
  "params": {
    "trackId": "track_1",
    "instrumentId": "zenith_sampler",
    "presetId": "warm_sample"
  }
}

// 3. Further warm it up with brightness macro
{
  "command": "set_instrument_macro",
  "params": {
    "trackId": "track_1",
    "macroId": "macro_brightness",
    "value": 0.4
  }
}

// 4. Adjust attack for slower onset
{
  "command": "set_instrument_param",
  "params": {
    "trackId": "track_1",
    "parameterId": "attack",
    "value": 0.2
  }
}
```

---

## Built-in Instruments

### zenith_poly_synth

**Parameters:**
- `osc_type`: Oscillator waveform (Sine/Saw/Square)
- `filter_cutoff`: Low-pass filter cutoff frequency
- `filter_resonance`: Filter resonance/Q
- `attack`: Envelope attack time
- `decay`: Envelope decay time
- `sustain`: Envelope sustain level
- `release`: Envelope release time

**Macros:**
- `macro_brightness`: Controls filter cutoff and resonance
- `macro_envelope_speed`: Controls attack and release times

**Presets:**
- `init_basic_pad`: Warm pad with slow attack
- `bright_pluck`: Fast, bright plucked sound
- `lofi_keys_01`: Lo-fi, mellow keyboard sound

### zenith_sampler

**Parameters:**
- `attack`: Envelope attack time
- `decay`: Envelope decay time
- `sustain`: Envelope sustain level
- `release`: Envelope release time
- `filter_cutoff`: Low-pass filter cutoff
- `filter_resonance`: Filter resonance/Q

**Macros:**
- `macro_brightness`: Controls filter cutoff
- `macro_envelope_speed`: Controls attack and release times

**Presets:**
- `fast_attack`: Instant attack, short release
- `slow_fade`: Slow attack and release
- `warm_sample`: Moderate envelope with warm filter

---

## AI Agent Guidelines

### Best Practices

1. **Discovery First**: Always use `describe_instrument` before controlling an instrument to understand available parameters and macros.

2. **Validate IDs**: Use the metadata from `describe_instrument` to validate parameter/macro IDs before setting them.

3. **Handle Errors**: Check the `success` field and handle errors gracefully. Common errors include:
   - Track not found
   - No instrument attached to track
   - Invalid parameter/macro IDs

4. **Use Macros**: For high-level adjustments (brightness, warmth, speed), prefer macros over individual parameters.

5. **Start with Presets**: Begin with a preset close to the desired sound, then fine-tune with parameters or macros.

6. **Normalized Values**: All parameter and macro values are in range [0.0, 1.0]. The system handles conversion to actual units internally.

### Common Patterns

**Pattern: Safe Parameter Change**
```javascript
// First, describe the instrument to validate the parameter exists
const describeResponse = await sendCommand({
  command: "describe_instrument",
  params: { instrumentId: "zenith_poly_synth" }
});

if (describeResponse.success) {
  const params = describeResponse.instrument.parameters;
  const hasParam = params.some(p => p.id === "filter_cutoff");

  if (hasParam) {
    await sendCommand({
      command: "set_instrument_param",
      params: {
        trackId: "track_0",
        parameterId: "filter_cutoff",
        value: 0.7
      }
    });
  }
}
```

**Pattern: Preset-based Workflow**
```javascript
// 1. List available presets
const presetsResponse = await sendCommand({
  command: "list_instrument_presets",
  params: { instrumentId: "zenith_poly_synth" }
});

// 2. Pick a preset (e.g., based on user description or AI decision)
const presetId = presetsResponse.presets[0];

// 3. Set instrument with preset
await sendCommand({
  command: "set_track_instrument",
  params: {
    trackId: "track_0",
    instrumentId: "zenith_poly_synth",
    presetId: presetId
  }
});

// 4. Fine-tune as needed
await sendCommand({
  command: "set_instrument_macro",
  params: {
    trackId: "track_0",
    macroId: "macro_brightness",
    value: 0.85
  }
});
```

---

## Thread Safety

All instrument commands run on the **MESSAGE THREAD** and are thread-safe:
- Parameter changes use JUCE's standard parameter API
- No direct audio thread access is required
- Changes are applied atomically and safely

---

## Future Extensions

Planned extensions (not yet implemented):
- Custom instrument loading (VST3/AU plugins)
- MIDI routing configuration
- Sample loading for sampler
- Automation of instrument parameters
- User preset saving/loading
