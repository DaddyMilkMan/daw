# Zenith Instrument Preset JSON Schema

This document describes the JSON schema for Zenith instrument presets. This format is designed to be AI-friendly and human-readable, making it easy for external tools to generate and edit preset files.

## File Format

- **Extension**: `.json`
- **Encoding**: UTF-8
- **Format**: Standard JSON with pretty printing

## Schema Overview

```json
{
  "id": "string",
  "name": "string",
  "instrumentId": "string",
  "category": "string",
  "author": "string",
  "description": "string",
  "version": "string",
  "tags": ["string"],
  "params": {
    "param_id": 0.0
  },
  "macros": {
    "macro_id": 0.0
  }
}
```

## Field Descriptions

### Required Fields

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `id` | string | Unique preset identifier | `"deep_bass_preset_001"` |
| `name` | string | Human-readable preset name | `"Deep Bass"` |
| `instrumentId` | string | Target instrument ID | `"zenith_poly_synth"` or `"zenith_sampler"` |
| `params` | object | Parameter ID to value mapping | `{"filter_cutoff": 0.5}` |

### Optional Fields

| Field | Type | Description | Default |
|-------|------|-------------|---------|
| `category` | string | Preset category | `""` |
| `author` | string | Preset author | `"Unknown"` |
| `description` | string | Detailed description | `""` |
| `version` | string | Preset format version | `"1.0.0"` |
| `tags` | array[string] | Search tags | `[]` |
| `macros` | object | Macro ID to value mapping | `{}` |

## Instrument IDs

### Available Instruments

| Instrument ID | Name | Description |
|--------------|------|-------------|
| `zenith_poly_synth` | Zenith PolySynth | Simple polyphonic synthesizer |
| `zenith_sampler` | Zenith Sampler | Sample-based instrument |

## Parameter IDs

### ZenithPolySynth Parameters

| Parameter ID | Name | Range | Description |
|-------------|------|-------|-------------|
| `osc_type` | Oscillator Type | 0.0 - 1.0 | Oscillator waveform (0=Sine, 0.5=Saw, 1.0=Square) |
| `filter_cutoff` | Filter Cutoff | 0.0 - 1.0 | Low-pass filter cutoff frequency |
| `filter_resonance` | Filter Resonance | 0.0 - 1.0 | Filter resonance amount |
| `attack` | Attack | 0.0 - 1.0 | Envelope attack time |
| `decay` | Decay | 0.0 - 1.0 | Envelope decay time |
| `sustain` | Sustain | 0.0 - 1.0 | Envelope sustain level |
| `release` | Release | 0.0 - 1.0 | Envelope release time |

**Macros:**
- `macro_brightness`: Controls filter cutoff (80%) and resonance (50%)
- `macro_envelope_speed`: Controls attack (70%) and release (70%)

### ZenithSampler Parameters

| Parameter ID | Name | Range | Description |
|-------------|------|-------|-------------|
| `attack` | Attack | 0.0 - 1.0 | Envelope attack time |
| `decay` | Decay | 0.0 - 1.0 | Envelope decay time |
| `sustain` | Sustain | 0.0 - 1.0 | Envelope sustain level |
| `release` | Release | 0.0 - 1.0 | Envelope release time |
| `filter_cutoff` | Filter Cutoff | 0.0 - 1.0 | Low-pass filter cutoff frequency |
| `filter_resonance` | Filter Resonance | 0.0 - 1.0 | Filter resonance amount |
| `tune` | Tune | 0.0 - 1.0 | Sample pitch (0.5 = center) |
| `gain` | Gain | 0.0 - 1.0 | Output gain |
| `character` | Character | 0.0 - 1.0 | Tonal character/saturation |

**Macros:**
- `macro_brightness`: Controls filter cutoff (80%) and character (50%)
- `macro_response`: Controls attack (70%) and release (70%)

## Categories

Common preset categories:

- **Bass**: Low-frequency sounds, sub bass, 808s
- **Lead**: Melodic lead sounds, plucks
- **Pad**: Atmospheric, sustained sounds
- **808**: Trap and hip-hop bass sounds
- **Drums**: Percussive sounds
- **Acoustic**: Natural, realistic sounds
- **LoFi**: Vintage, degraded sounds

## Tags

Tags are used for search and filtering. Use lowercase, descriptive keywords:

- Genre: `trap`, `hip-hop`, `ambient`, `techno`
- Character: `warm`, `bright`, `dark`, `clean`, `dirty`
- Function: `bass`, `lead`, `pad`, `pluck`, `percussive`
- Quality: `punchy`, `soft`, `aggressive`, `gentle`, `lush`
- Vibe: `vintage`, `modern`, `nostalgic`, `futuristic`

## Value Ranges

All parameter and macro values MUST be in the range `[0.0, 1.0]` (normalized).

- Values are automatically clamped to this range when loaded
- Use `0.0` for minimum, `0.5` for center, `1.0` for maximum
- Higher precision is supported (e.g., `0.75`, `0.42`)

## Example Presets

### Deep Bass (PolySynth)

```json
{
  "id": "deep_bass_preset_001",
  "name": "Deep Bass",
  "instrumentId": "zenith_poly_synth",
  "category": "Bass",
  "author": "Factory",
  "description": "Deep, warm sub bass with smooth filter sweep. Perfect for 808-style bass lines.",
  "version": "1.0.0",
  "tags": ["bass", "808", "sub", "deep", "warm"],
  "params": {
    "osc_type": 0.0,
    "filter_cutoff": 0.25,
    "filter_resonance": 0.15,
    "attack": 0.05,
    "decay": 0.3,
    "sustain": 0.7,
    "release": 0.4
  },
  "macros": {
    "macro_brightness": 0.3,
    "macro_envelope_speed": 0.4
  }
}
```

### Natural Acoustic (Sampler)

```json
{
  "id": "natural_acoustic_preset_001",
  "name": "Natural Acoustic",
  "instrumentId": "zenith_sampler",
  "category": "Acoustic",
  "author": "Factory",
  "description": "Clean, natural sound with moderate attack and smooth decay.",
  "version": "1.0.0",
  "tags": ["natural", "acoustic", "clean", "realistic"],
  "params": {
    "attack": 0.1,
    "decay": 0.4,
    "sustain": 0.7,
    "release": 0.3,
    "filter_cutoff": 0.8,
    "filter_resonance": 0.1,
    "tune": 0.5,
    "gain": 0.7,
    "character": 0.2
  },
  "macros": {
    "macro_brightness": 0.7,
    "macro_response": 0.5
  }
}
```

## Usage in External Tools

### Loading a Preset

```cpp
// C++ (JUCE)
auto file = juce::File("path/to/preset.json");
auto preset = ZenithInstrumentPreset::loadFromJsonFile(file);

// Apply to instrument
instrument->applyPreset(preset);
```

### Creating a Preset

```cpp
// C++ (JUCE)
ZenithInstrumentPreset preset;
preset.name = "My Preset";
preset.instrumentId = "zenith_poly_synth";
preset.category = "Bass";
preset.tags = {"bass", "custom", "experimental"};
preset.parameters["filter_cutoff"] = 0.42;
preset.parameters["attack"] = 0.1;

auto file = juce::File("path/to/preset.json");
preset.saveToJsonFile(file);
```

### Filtering Presets

```cpp
// Get all presets for an instrument
auto presets = presetManager.getPresetsForInstrument("zenith_poly_synth");

// Find by tag
auto bassPresets = presetManager.findPresetsByTag("zenith_poly_synth", "bass");

// Find by category
auto padPresets = presetManager.findPresetsByCategory("zenith_poly_synth", "Pad");
```

## File Locations

### Factory Presets

```
~/Library/Application Support/Zenith/Instruments/Factory/
  ├── zenith_poly_synth/
  │   ├── deep_bass.json
  │   ├── bright_lead.json
  │   └── warm_pad.json
  └── zenith_sampler/
      ├── natural_acoustic.json
      ├── punchy_drums.json
      └── lofi_vinyl.json
```

### User Presets

```
~/Library/Application Support/Zenith/Instruments/User/
  ├── zenith_poly_synth/
  └── zenith_sampler/
```

### Development/Distribution

```
zenith-core/Content/Presets/
  ├── PolySynth/*.json
  └── Sampler/*.json
```

## Best Practices

1. **Unique IDs**: Generate unique IDs using a naming convention like `{name}_{type}_{version}`
2. **Descriptive Names**: Use clear, concise names that describe the sound
3. **Rich Metadata**: Include category, description, and tags for better discoverability
4. **Normalized Values**: Always use values in `[0.0, 1.0]` range
5. **Unknown Parameters**: Unknown parameter IDs are safely ignored during loading
6. **Version String**: Use semantic versioning (e.g., "1.0.0") for preset format version

## Error Handling

The preset system is designed to be robust:

- **Unknown parameters**: Ignored (allows forward compatibility)
- **Out-of-range values**: Automatically clamped to `[0.0, 1.0]`
- **Missing required fields**: Use sensible defaults
- **Invalid instrument ID**: Preset loading fails gracefully
- **Malformed JSON**: Returns empty preset

## AI Tool Guidelines

When generating presets programmatically:

1. Ensure `instrumentId` matches the target instrument exactly
2. Use only valid parameter IDs for the instrument (see tables above)
3. Generate musically meaningful value combinations
4. Include descriptive tags for searchability
5. Set appropriate categories based on the sound character
6. Provide helpful descriptions that explain the preset's use case
7. Use the examples above as templates

## Version History

- **1.0.0** (2025-11-18): Initial JSON schema with categories, tags, and AI-friendly metadata
