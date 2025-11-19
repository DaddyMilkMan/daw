# AI Preset Design Guide

## Overview

This guide explains how external AI systems can intelligently design instrument presets using the Zenith DAW instrument parameter schema API.

## CommandAPI: describe_instrument

### Request Format

```json
{
  "command": "describe_instrument",
  "params": {
    "instrumentId": "zenith_poly_synth"
  }
}
```

### Response Format

The response provides complete metadata about the instrument, including:

- **Basic Info**: `instrumentId`, `name`, `category`, `description`
- **Parameters**: Array of parameter metadata with extended AI-friendly fields
- **Macros**: High-level controls that affect multiple parameters

See `instrument-parameter-schema-example.json` for a complete example response.

## Parameter Schema

Each parameter includes the following fields:

### Core Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Stable parameter identifier (e.g., "filter_cutoff") |
| `name` | string | Human-readable name (e.g., "Filter Cutoff") |
| `category` | string | Parameter category (e.g., "Filter", "Envelope") |
| `type` | string | Data type: "float", "bool", or "choice" |
| `defaultValue` | float | Default normalized value (0.0-1.0) |
| `minValue` | float | Minimum value |
| `maxValue` | float | Maximum value |
| `units` | string | Display units (e.g., "Hz", "dB", "%", "s") |

### AI-Specific Fields (New)

| Field | Type | Description |
|-------|------|-------------|
| `group` | string | Functional group: "Oscillator", "Filter", "Envelope", "LFO", "FX" |
| `role` | string | Semantic role: "tone", "mod", "time", "level", "stereo", "distortion" |
| `recommendedStep` | float | Recommended step size for automation (default: 0.01) |
| `safeRangeForRandomisation` | object | Optional safe range with `min` and `max` values |

## How to Interpret Parameters

### By Group

- **Oscillator**: Controls the basic tone generation (waveform, pitch, phase)
- **Filter**: Shapes the frequency spectrum of the sound
- **Envelope**: Controls how sound evolves over time (ADSR)
- **LFO**: Low-frequency oscillators for modulation
- **FX**: Effects processing (reverb, delay, distortion, etc.)

### By Role

- **tone**: Affects the timbre/character of the sound (e.g., filter cutoff, oscillator type)
- **mod**: Modulation parameters (e.g., LFO rate, depth)
- **time**: Temporal parameters (e.g., attack, decay, release)
- **level**: Amplitude/volume controls (e.g., sustain, gain)
- **stereo**: Spatial positioning (e.g., pan, width)
- **distortion**: Saturation/distortion amount

## Designing Intelligent Presets

### 1. Understanding Parameter Relationships

**Filter Parameters** (`filter_cutoff`, `filter_resonance`):
- **Low cutoff (0.2-0.4)**: Dark, muffled tones; good for bass, pads
- **Mid cutoff (0.5-0.7)**: Balanced, warm sounds; good for keys, leads
- **High cutoff (0.8-1.0)**: Bright, aggressive tones; good for plucks, leads
- **Resonance**: Adds emphasis at cutoff frequency; use sparingly (0.1-0.6 typically safe)

**Envelope Parameters** (ADSR):
- **Attack**: How quickly sound reaches full volume
  - Fast (0.0-0.1): Plucks, percussion, stabs
  - Slow (0.3-0.7): Pads, swells, atmospheric sounds
- **Decay**: How quickly sound drops to sustain level
  - Fast (0.05-0.2): Percussive, punchy sounds
  - Slow (0.3-0.8): Smoother transitions
- **Sustain**: Held note level (0.0-1.0)
  - Low (0.0-0.4): Percussive, decaying sounds
  - High (0.6-1.0): Sustained notes, pads
- **Release**: How quickly sound fades after note off
  - Fast (0.05-0.2): Staccato, tight sounds
  - Slow (0.4-0.9): Legato, smooth transitions

### 2. Preset Archetypes

#### Pad Sound
```
filter_cutoff: 0.5-0.7 (warm, smooth)
filter_resonance: 0.2-0.4 (gentle emphasis)
attack: 0.4-0.6 (slow swell)
decay: 0.3-0.5
sustain: 0.7-0.9 (high sustained level)
release: 0.5-0.8 (smooth fade)
```

#### Pluck Sound
```
filter_cutoff: 0.7-0.95 (bright)
filter_resonance: 0.3-0.6 (pronounced)
attack: 0.0-0.02 (instant)
decay: 0.1-0.3 (quick)
sustain: 0.2-0.4 (short tail)
release: 0.05-0.2 (fast)
```

#### Bass Sound
```
filter_cutoff: 0.2-0.5 (dark, focused)
filter_resonance: 0.2-0.5 (moderate)
attack: 0.0-0.05 (punchy)
decay: 0.1-0.3
sustain: 0.6-0.9 (sustained)
release: 0.1-0.3
```

#### Lead Sound
```
filter_cutoff: 0.7-0.9 (bright, cutting)
filter_resonance: 0.4-0.7 (pronounced character)
attack: 0.01-0.1 (quick but smooth)
decay: 0.2-0.4
sustain: 0.5-0.8
release: 0.2-0.5
```

### 3. Safe Range for Randomization

Use the `safeRangeForRandomisation` field to avoid extreme values that might sound bad:

```json
{
  "id": "filter_cutoff",
  "minValue": 0.0,
  "maxValue": 1.0,
  "safeRangeForRandomisation": {
    "min": 0.2,  // Avoid completely muffled sound
    "max": 0.95  // Avoid harsh high frequencies
  }
}
```

**When to use safe ranges:**
- Randomization/mutation algorithms
- Generative preset creation
- Initial preset suggestions
- User-requested "surprise me" features

### 4. Using Macros

Macros provide high-level semantic controls that affect multiple parameters:

**macro_brightness**:
- Controls `filter_cutoff` (amount: 0.8) and `filter_resonance` (amount: 0.5)
- Use this to quickly adjust overall tone from dark to bright
- Value range: 0.0 (dark) to 1.0 (bright)

**macro_envelope_speed**:
- Controls `attack` (amount: 0.7) and `release` (amount: 0.7)
- Use this to adjust overall envelope speed
- Value range: 0.0 (slow/pad-like) to 1.0 (fast/percussive)

## Implementation Notes

### Thread Safety

The `describe_instrument` command:
- ✅ Is **cheap** - only queries metadata, no audio processing
- ✅ Runs on the **message thread** - no real-time audio thread interaction
- ✅ Does **not** create audio device instances
- ✅ Uses InstrumentRegistry which stores pre-computed metadata

### Error Handling

If an instrument is not found, you'll receive:

```json
{
  "success": false,
  "error": "Instrument not found: invalid_instrument_id"
}
```

### Available Instruments

To get a list of all available instruments, you can query the InstrumentRegistry (future API endpoint).

Current built-in instruments:
- `zenith_poly_synth` - Polyphonic synthesizer
- `zenith_sampler` - Sample playback instrument

## Example AI Workflow

```python
# 1. Query instrument schema
response = api.execute_command({
    "command": "describe_instrument",
    "params": {"instrumentId": "zenith_poly_synth"}
})

instrument = response["result"]

# 2. Analyze parameters by role
tone_params = [p for p in instrument["parameters"] if p["role"] == "tone"]
time_params = [p for p in instrument["parameters"] if p["role"] == "time"]
level_params = [p for p in instrument["parameters"] if p["role"] == "level"]

# 3. Design preset based on desired character
# Example: Create a "Bright Pluck" preset
preset = {
    "filter_cutoff": 0.85,      # Bright (role: tone)
    "filter_resonance": 0.45,   # Moderate (role: tone)
    "attack": 0.01,             # Fast (role: time)
    "decay": 0.15,              # Quick (role: time)
    "sustain": 0.3,             # Short (role: level)
    "release": 0.1              # Fast (role: time)
}

# 4. Validate against safe ranges
for param_id, value in preset.items():
    param = next(p for p in instrument["parameters"] if p["id"] == param_id)
    if "safeRangeForRandomisation" in param:
        safe = param["safeRangeForRandomisation"]
        value = max(safe["min"], min(safe["max"], value))

# 5. Apply preset to instrument
# (via future set_instrument_parameter API)
```

## Best Practices

1. **Respect safe ranges** when generating random/mutated presets
2. **Use semantic roles** to understand parameter relationships
3. **Consider group coherence** - parameters in the same group often work together
4. **Test envelope consistency** - fast attack usually pairs with fast release for percussive sounds
5. **Balance filter parameters** - high cutoff + high resonance can be harsh
6. **Use recommended steps** for smooth automation curves
7. **Leverage macros** for high-level adjustments before fine-tuning individual parameters

## Future Extensions

Planned enhancements to the parameter schema:

- [ ] Parameter dependencies (e.g., "filter_type" affects "filter_cutoff" behavior)
- [ ] Value mapping functions (e.g., exponential scaling for frequency parameters)
- [ ] Semantic tags (e.g., "aggressive", "smooth", "vintage")
- [ ] Parameter grouping hints for UI layout
- [ ] Modulation source/destination information
- [ ] Parameter correlation hints (parameters that often change together)

## Support

For questions or issues with the instrument parameter schema API, please refer to the Zenith DAW documentation or create an issue in the project repository.
