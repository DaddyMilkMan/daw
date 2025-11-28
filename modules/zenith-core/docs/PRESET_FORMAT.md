# Zenith Preset Format Documentation

## Overview

The Zenith preset system provides a standardized, human-readable format for instrument presets. This document describes the `.zpreset.json` format for content-pack authors and preset creators.

## Directory Structure

Presets are organized in a hierarchical directory structure:

```
<ContentRoot>/Instruments/Presets/<InstrumentName>/<Category>/*.zpreset.json
```

### Example Structure

```
~/Music/Zenith/Instruments/Presets/
├── zenith.poly_synth/
│   ├── Bass/
│   │   ├── Sub_808.zpreset.json
│   │   ├── Reese_Bass.zpreset.json
│   │   └── Wobble_Bass.zpreset.json
│   ├── Lead/
│   │   ├── Supersaw_Lead.zpreset.json
│   │   ├── Pluck_Lead.zpreset.json
│   │   └── Synth_Lead.zpreset.json
│   ├── Pad/
│   │   ├── Warm_Pad.zpreset.json
│   │   ├── Ambient_Pad.zpreset.json
│   │   └── Strings_Pad.zpreset.json
│   └── Keys/
│       └── Electric_Piano.zpreset.json
└── zenith_sampler/
    └── ... (sampler presets)
```

### Default Categories

Each instrument declares default categories in its metadata. For `zenith.poly_synth`:

- **Bass**: Sub basses, 808s, wobble basses
- **Lead**: Supersaw, plucks, mono leads
- **Pad**: Ambient, warm, atmospheric sounds
- **Keys**: Piano, electric piano, bells
- **Pluck**: Plucked, staccato sounds
- **FX**: Sound effects, risers, impacts
- **808**: Trap/hip-hop 808 bass sounds

## File Format: `.zpreset.json`

Presets are stored as JSON files with the `.zpreset.json` extension.

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique preset identifier (auto-generated recommended) |
| `name` | string | Human-readable preset name |
| `author` | string | Preset creator (e.g., "Factory", "User", "Artist Name") |
| `category` | string | Category for organization (must match directory) |
| `instrumentId` | string | Target instrument ID (e.g., "zenith.poly_synth") |
| `version` | string | Preset format version (currently "1.0.0") |
| `parameters` | object | Parameter ID → normalized value [0-1] mapping |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `description` | string | Detailed preset description |
| `tags` | array[string] | Search/filter tags (e.g., ["warm", "lush", "ambient"]) |

### Complete Example

```json
{
  "id": "warm_pad_001",
  "name": "Warm Pad",
  "author": "Factory",
  "category": "Pad",
  "instrumentId": "zenith.poly_synth",
  "version": "1.0.0",
  "description": "Lush, ambient pad with slow attack and gentle modulation",
  "tags": [
    "pad",
    "ambient",
    "warm",
    "lush",
    "atmospheric"
  ],
  "parameters": {
    "osc1_wave": 0.0,
    "osc1_detune": 0.5,
    "osc1_mix": 0.8,
    "osc2_wave": 0.2,
    "osc2_detune": 0.55,
    "osc2_mix": 0.6,
    "osc3_wave": 0.0,
    "osc3_detune": 0.4,
    "osc3_mix": 0.4,
    "unison_voices": 0.5,
    "unison_detune": 0.3,
    "filter_type": 0.0,
    "filter_cutoff": 0.5,
    "filter_resonance": 0.25,
    "filter_drive": 0.0,
    "amp_attack": 0.6,
    "amp_decay": 0.5,
    "amp_sustain": 0.8,
    "amp_release": 0.7,
    "mod_attack": 0.4,
    "mod_decay": 0.6,
    "mod_sustain": 0.3,
    "mod_release": 0.5,
    "lfo1_rate": 0.1,
    "lfo1_amount": 0.2,
    "lfo1_target": 0.0,
    "lfo2_rate": 0.15,
    "lfo2_amount": 0.15,
    "lfo2_target": 0.0,
    "glide_time": 0.0,
    "mono_mode": 0.0,
    "master_gain": 0.6
  }
}
```

## Parameter Format

### Normalized Values

All parameter values are **normalized to the range [0, 1]**, regardless of the actual parameter range. This ensures compatibility across different instrument versions.

Examples:
- Filter cutoff (20 Hz - 20 kHz) → normalized [0, 1]
- Attack time (1 ms - 10 s) → normalized [0, 1]
- Oscillator mix (0% - 100%) → normalized [0, 1]

### Parameter IDs

Parameter IDs are **stable strings** defined by each instrument. For `zenith.poly_synth`:

#### Oscillator Parameters (9 params × 3 oscillators)
- `osc1_wave`, `osc2_wave`, `osc3_wave` - Waveform (0=Sine, 0.2=Saw, 0.4=Square, 0.6=Triangle, 0.8=Noise, 1.0=Supersaw)
- `osc1_detune`, `osc2_detune`, `osc3_detune` - Detune amount in cents
- `osc1_mix`, `osc2_mix`, `osc3_mix` - Oscillator level/mix

#### Unison Parameters (2 params)
- `unison_voices` - Number of unison voices
- `unison_detune` - Unison detune amount

#### Filter Parameters (4 params)
- `filter_type` - Filter type (0=Lowpass, 0.5=Bandpass, 1.0=Highpass)
- `filter_cutoff` - Cutoff frequency
- `filter_resonance` - Resonance/Q
- `filter_drive` - Filter drive/saturation

#### Envelope Parameters (8 params)
- Amp Envelope: `amp_attack`, `amp_decay`, `amp_sustain`, `amp_release`
- Mod Envelope: `mod_attack`, `mod_decay`, `mod_sustain`, `mod_release`

#### LFO Parameters (6 params)
- LFO 1: `lfo1_rate`, `lfo1_amount`, `lfo1_target`
- LFO 2: `lfo2_rate`, `lfo2_amount`, `lfo2_target`

#### Global Parameters (3 params)
- `glide_time` - Portamento/glide time
- `mono_mode` - Mono/poly mode (0=poly, 1=mono)
- `master_gain` - Master output level

**Total: 33 parameters**

### Querying Available Parameters

To get a complete list of parameters for an instrument, use the CommandAPI:

```bash
curl -X POST http://localhost:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{
    "command": "get_instrument_metadata",
    "parameters": {
      "instrument_id": "zenith.poly_synth"
    }
  }'
```

This returns all parameter metadata including:
- Parameter IDs
- Display names
- Value ranges
- Default values
- Recommended step sizes

## Creating Presets

### Manual Creation

1. **Capture from Instrument**: Use the API to capture current instrument state:
   ```
   POST /api/command
   {
     "command": "capture_preset",
     "parameters": {
       "instrument_id": "zenith.poly_synth",
       "name": "My Preset",
       "category": "Lead",
       "author": "Your Name"
     }
   }
   ```

2. **Manual JSON**: Create a `.zpreset.json` file following the format above.

3. **ID Generation**: Use lowercase name with underscores + timestamp:
   ```
   "id": "my_preset_name_1732000000000"
   ```

### Best Practices

1. **Naming**: Use descriptive, searchable names
   - Good: "Warm Analog Pad", "Deep Sub Bass", "Bright Pluck Lead"
   - Bad: "Preset 1", "Untitled", "Test"

2. **Tags**: Include relevant search terms
   - Genre tags: `"edm"`, `"trap"`, `"ambient"`
   - Character tags: `"warm"`, `"bright"`, `"dark"`, `"aggressive"`
   - Usage tags: `"bass"`, `"lead"`, `"pad"`, `"keys"`

3. **Categories**: Use standard categories when possible
   - Matches instrument's `defaultPresetCategories`
   - Enables better organization and browsing

4. **Descriptions**: Provide context and usage tips
   - Good: "Lush pad with slow attack, perfect for ambient and cinematic tracks"
   - Bad: "A pad sound"

5. **Version**: Always include `"version": "1.0.0"` for forward compatibility

## Loading Presets

### Programmatic Loading

```cpp
#include "ZenithPresetManager.h"

auto& manager = ZenithPresetManager::getInstance();

// Load all presets for an instrument
auto presets = manager.loadAllPresetsForInstrument("zenith.poly_synth");

// Get presets in a specific category
auto bassPresets = manager.getPresetList("zenith.poly_synth", "Bass");

// Load a specific preset
auto preset = manager.loadPreset("zenith.poly_synth", "warm_pad_001");

// Apply to instrument
bool success = manager.applyPresetToInstrument(preset, instrument);
```

### Thread Safety

**IMPORTANT**: All ZenithPresetManager methods perform file I/O and **MUST** be called from the message thread only. Audio thread access is prohibited.

```cpp
// Good ✓
juce::MessageManager::callAsync([&]() {
    auto presets = manager.loadAllPresetsForInstrument("zenith.poly_synth");
});

// Bad ✗ - Never call from audio thread!
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    auto presets = manager.loadAllPresetsForInstrument("..."); // WRONG!
}
```

## Factory vs User Presets

### Factory Presets (Read-Only)

Located in the application's content directory:
```
<ContentRoot>/Instruments/Presets/<InstrumentName>/
```

Typically bundled with the application or installed as content packs.

### User Presets (Read-Write)

Located in the user's application data directory:
```
~/.local/share/Zenith/Presets/<InstrumentName>/    (Linux)
~/Library/Application Support/Zenith/Presets/...   (macOS)
%APPDATA%/Zenith/Presets/...                        (Windows)
```

Created by users or imported content packs.

## Content Pack Creation

### Structure

A content pack is a collection of presets organized by instrument and category:

```
MyContentPack/
├── README.md
├── LICENSE.txt
└── Instruments/
    └── Presets/
        └── zenith.poly_synth/
            ├── Bass/
            │   ├── Heavy_Sub.zpreset.json
            │   └── Wobble_Bass.zpreset.json
            ├── Lead/
            │   └── Supersaw_Lead.zpreset.json
            └── Pad/
                └── Ethereal_Pad.zpreset.json
```

### Installation

Users install content packs by:
1. Extracting to `<ContentRoot>/` (factory) or `~/.local/share/Zenith/` (user)
2. Restarting the application or refreshing the preset list

### Metadata Requirements

Each preset must include:
- Valid `instrumentId` matching the target instrument
- Unique `id` (avoid collisions with other packs)
- Proper `category` matching the directory structure
- Descriptive `name`, `author`, and `tags`

## API Integration

### ZenithPresetManager Methods

| Method | Description |
|--------|-------------|
| `loadAllPresetsForInstrument(id)` | Load all presets (factory + user) for instrument |
| `getPresetList(id, category)` | Get metadata-only list (lightweight) |
| `loadPreset(id, presetId)` | Load specific preset by ID |
| `getCategories(id)` | Get all category names for instrument |
| `applyPresetToInstrument(preset, instrument)` | Apply preset to instrument instance |
| `capturePresetFromInstrument(...)` | Capture current instrument state |
| `savePreset(preset, userPreset)` | Save to disk (factory or user directory) |
| `deletePreset(id, presetId, userPreset)` | Delete preset from disk |

### InstrumentRegistry Integration

Each instrument declares:
- `instrumentId` - Stable identifier
- `defaultPresetCategories` - Suggested categories
- Serialization hooks (optional) - Custom parameter format

## Version Compatibility

The preset format is designed for forward/backward compatibility:

- **Version 1.0.0** (current): Basic JSON format with normalized parameters
- Future versions may add optional fields while maintaining compatibility

### Handling Unknown Parameters

When a preset contains unknown parameter IDs:
- The preset manager silently ignores them
- Known parameters are still applied
- No error is raised

This allows presets to be used across different instrument versions.

## Troubleshooting

### Preset Not Loading

1. **Check file extension**: Must be `.zpreset.json`
2. **Verify JSON syntax**: Use a JSON validator
3. **Confirm instrumentId**: Must exactly match target instrument
4. **Check directory structure**: Must be in `<ContentRoot>/Instruments/Presets/<InstrumentName>/<Category>/`

### Parameter Values Out of Range

All values should be normalized [0, 1]. Out-of-range values are automatically clamped:
```cpp
value = juce::jlimit(0.0f, 1.0f, value);
```

### Thread Safety Violations

If you see crashes during preset loading:
- Ensure all preset operations are on the message thread
- Never call preset manager methods from audio thread
- Use `juce::MessageManager::callAsync()` for async operations

## Examples

See the factory presets in:
```
zenith-core/Content/Instruments/Presets/zenith.poly_synth/
```

Example presets:
- `Bass/Sub_808.zpreset.json` - Deep 808 bass
- `Lead/Supersaw_Lead.zpreset.json` - EDM supersaw lead
- `Pad/Warm_Pad.zpreset.json` - Ambient pad sound

## License

Factory presets are licensed under the same terms as Zenith DAW. User-created presets belong to their respective authors.

---

**For questions or contributions**, please refer to the main Zenith documentation or open an issue on GitHub.
