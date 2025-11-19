# Preset Generator Tools

Automated preset generation system for Zenith instruments.

## Overview

This directory contains tools to generate, validate, and manage large preset libraries for ZenithPolySynth and ZenithSampler.

## Files

- **`archetypes.json`** - Archetype definitions for all preset categories
- **`generate_presets.py`** - Main preset generation script
- **`validate_presets.py`** - Preset validation and testing script

## Quick Start

### Generate Presets

```bash
python3 generate_presets.py
```

This will:
1. Load archetype definitions from `archetypes.json`
2. Generate base presets from archetypes
3. Create variations of each base preset
4. Save preset banks to `Content/Presets/`
5. Generate preset index with category counts

**Output:**
- 594 total presets (414 PolySynth, 180 Sampler)
- Category-organized bank files (JSON)
- Master bank files with all presets
- Preset index for quick lookups

### Validate Presets

```bash
python3 validate_presets.py
```

This will:
1. Load all preset bank files
2. Validate parameter ranges
3. Check metadata integrity
4. Verify category consistency
5. Ensure minimum preset count (500+)

## Archetype System

### What are Archetypes?

Archetypes are hand-crafted "master" presets that define the sonic character of each category. The generator creates variations of these archetypes to build a large, diverse library while maintaining sonic coherence.

### Archetype Structure

```json
{
  "name": "Clean Sub 808",
  "description": "Pure sine sub bass for 808-style patterns",
  "tags": ["trap", "808", "sub", "bass"],
  "params": {
    "osc_type": 0.0,
    "filter_cutoff": 0.4,
    "filter_resonance": 0.1,
    "attack": 0.005,
    "decay": 0.4,
    "sustain": 0.0,
    "release": 0.2
  }
}
```

### Current Archetypes

**ZenithPolySynth (69 archetypes):**
- Bass: 15 archetypes (808s, subs, Reese, acid, etc.)
- Lead: 12 archetypes (supersaw, screamer, vocal, etc.)
- Pluck: 10 archetypes (future bass, muted, bell, etc.)
- Pad: 10 archetypes (analog, digital, drone, etc.)
- Arp: 8 archetypes (trance, trap, gate, etc.)
- FX: 8 archetypes (riser, impact, sweep, etc.)
- Keys: 6 archetypes (piano, bell, marimba, etc.)

**ZenithSampler (20 archetypes):**
- 808: 5 archetypes
- Drums: 5 archetypes
- Keys: 5 archetypes
- FX: 5 archetypes

## Variation Algorithm

The generator creates variations by applying constrained random perturbations to base parameters:

### Variation Rules

1. **Variation Amount:** 5-10% of parameter range
2. **Range Clamping:** All values stay within valid min/max
3. **Character Preservation:** Small variations maintain preset identity
4. **Safe Parameters:** Only "safe" parameters are varied (no extreme jumps)

### Example

```python
# Base preset
base_params = {
    "filter_cutoff": 0.8,
    "attack": 0.1,
    "release": 0.3
}

# Variation (8% range)
variation = {
    "filter_cutoff": 0.78,  # ±8% of [0.0, 1.0]
    "attack": 0.11,         # ±8% of [0.0, 1.0]
    "release": 0.29         # ±8% of [0.0, 1.0]
}
```

### Variation Counts

- **PolySynth:** 5 variations per archetype
  - 69 archetypes × 6 (1 base + 5 variations) = 414 presets

- **Sampler:** 8 variations per archetype
  - 20 archetypes × 9 (1 base + 8 variations) = 180 presets

## Adding New Archetypes

### 1. Edit `archetypes.json`

Add a new archetype to the appropriate category:

```json
{
  "zenith_poly_synth": {
    "archetypes": {
      "Bass": [
        {
          "name": "My New Bass",
          "description": "Amazing new bass sound",
          "tags": ["bass", "heavy", "modern"],
          "params": {
            "osc_type": 1.0,
            "filter_cutoff": 0.5,
            "filter_resonance": 0.4,
            "attack": 0.01,
            "decay": 0.3,
            "sustain": 0.7,
            "release": 0.2
          }
        }
        // ... existing archetypes
      ]
    }
  }
}
```

### 2. Regenerate

```bash
python3 generate_presets.py
```

### 3. Validate

```bash
python3 validate_presets.py
```

## Parameter Constraints

### ZenithPolySynth

All parameters normalized to [0.0, 1.0] except `osc_type`:

```python
{
  'osc_type': (0.0, 2.0),        # 0=Sine, 1=Saw, 2=Square
  'filter_cutoff': (0.0, 1.0),
  'filter_resonance': (0.0, 1.0),
  'attack': (0.0, 1.0),          # seconds
  'decay': (0.0, 1.0),           # seconds
  'sustain': (0.0, 1.0),         # level
  'release': (0.0, 1.0)          # seconds
}
```

### ZenithSampler

Extended ranges for envelope and tuning:

```python
{
  'attack': (0.001, 5.0),        # seconds
  'decay': (0.001, 5.0),         # seconds
  'sustain': (0.0, 1.0),         # level
  'release': (0.001, 10.0),      # seconds
  'filter_cutoff': (0.0, 1.0),
  'filter_resonance': (0.0, 1.0),
  'tune': (-12.0, 12.0),         # semitones
  'gain': (0.0, 2.0),            # multiplier
  'character': (0.0, 1.0)        # saturation amount
}
```

## Tagging Best Practices

### Tag Categories

**Genre Tags:**
```
trap, hip-hop, edm, house, techno, dnb, dubstep, trance,
future-bass, lofi, ambient, minimal, acid
```

**Sound Type Tags:**
```
bass, sub, lead, pluck, pad, arp, fx, keys, drums, 808
```

**Character Tags:**
```
warm, bright, dark, soft, aggressive, clean, dirty, punchy,
smooth, gritty, resonant, hollow, ethereal
```

**Style Tags:**
```
analog, digital, vintage, modern, retro, classic
```

**Technical Tags:**
```
sine, saw, square, supersaw, reese, wobble, glide, vocal,
percussive, melodic, atmospheric, evolving
```

### Tagging Rules

1. **3-5 tags per preset** - Enough for filtering, not too many
2. **Most specific first** - `["bass", "sub", "808", "trap"]`
3. **Include category** - Always tag with sound type (bass, lead, etc.)
4. **Genre relevant** - Add genre tags for style-specific sounds
5. **Character descriptors** - Add 1-2 sonic character tags

## Output Format

### Bank File Structure

```json
{
  "instrument": "zenith_poly_synth",
  "bank_name": "Bass Bank",
  "category": "Bass",
  "version": "1.0",
  "preset_count": 90,
  "presets": [
    {
      "id": "clean_sub_808",
      "name": "Clean Sub 808",
      "category": "Bass",
      "tags": ["trap", "808", "sub", "bass"],
      "description": "Pure sine sub bass",
      "parameters": {
        "osc_type": 0.0,
        "filter_cutoff": 0.4,
        // ... etc
      }
    }
    // ... more presets
  ]
}
```

### Preset Index Structure

```json
{
  "version": "1.0",
  "generated": "2025-11-18",
  "total_presets": 594,
  "instruments": {
    "zenith_poly_synth": {
      "name": "Zenith Poly Synth",
      "total": 414,
      "categories": {
        "Bass": 90,
        "Lead": 72,
        // ... etc
      }
    },
    "zenith_sampler": {
      "name": "Zenith Sampler",
      "total": 180,
      "categories": {
        "808": 45,
        // ... etc
      }
    }
  }
}
```

## Validation Rules

The validator checks:

1. **File Integrity**
   - All bank files are valid JSON
   - All required fields present

2. **Parameter Validation**
   - All parameter IDs are recognized
   - All values within valid ranges
   - No NaN or infinite values

3. **Metadata Validation**
   - Categories are valid
   - Tags are recognized (warnings only)
   - IDs are unique within bank

4. **Count Validation**
   - Declared preset_count matches actual
   - Total presets >= 500 (requirement)

5. **Structure Validation**
   - Preset index matches actual counts
   - Bank organization is correct

## Customization

### Change Variation Amount

Edit `generate_presets.py`:

```python
# More aggressive variations (15%)
var_params = self.create_variation(
    archetype['params'],
    param_ranges,
    variation_amount=0.15  # default: 0.08 for PolySynth
)
```

### Change Variation Count

Modify the `run()` call in `main()`:

```python
# Generate more variations per preset
index = generator.run(
    polysynth_variations=10,  # default: 5
    sampler_variations=15     # default: 8
)
```

### Add New Categories

1. Add category to `archetypes.json`
2. Update validator's category lists in `validate_presets.py`:

```python
self.polysynth_categories = {
    'Bass', 'Lead', 'Pluck', 'Pad', 'Arp', 'FX', 'Keys',
    'YourNewCategory'  # Add here
}
```

3. Regenerate presets

## Performance

- **Generation time:** ~1-2 seconds
- **Validation time:** ~1-2 seconds
- **Total file size:** ~2-3 MB (all banks)
- **Memory usage:** Minimal (~10-20 MB)

## Integration

### Loading Presets in C++

```cpp
#include <juce_data_structures/juce_data_structures.h>

// Load bass bank
File bankFile("Content/Presets/ZenithPolySynth/bass_bank.json");
var bankData = JSON::parse(bankFile.loadFileAsString());

// Iterate presets
var presets = bankData["presets"];
for (int i = 0; i < presets.size(); i++)
{
    var preset = presets[i];
    String id = preset["id"];
    String name = preset["name"];

    var params = preset["parameters"];
    float cutoff = params["filter_cutoff"];
    // ... apply parameters
}
```

### Quick Category Lookup

```cpp
// Load index for fast category counts
File indexFile("Content/Presets/preset_index.json");
var index = JSON::parse(indexFile.loadFileAsString());

int bassCount = index["instruments"]["zenith_poly_synth"]["categories"]["Bass"];
// Result: 90
```

## Troubleshooting

### "Unknown parameter" error

Check that parameter ID matches instrument parameter list exactly:
- `filter_cutoff` ✓
- `filterCutoff` ✗ (wrong case)
- `cutoff` ✗ (wrong name)

### "Value out of range" error

Verify parameter is within valid range for that instrument.

### Low preset count

Increase variations per archetype in `main()` function.

### Missing tags warning

Add tag to `valid_tags` set in `validate_presets.py` (optional).

## Future Enhancements

Potential improvements:

1. **Macro variations** - Vary macro values in addition to parameters
2. **Smart variations** - Use AI to generate musically coherent variations
3. **Category cross-pollination** - Blend archetypes from different categories
4. **User preset learning** - Analyze user presets to generate similar ones
5. **MIDI mapping** - Auto-generate MIDI CC mappings per preset
6. **Audio preview** - Generate audio previews for each preset

## License

Part of the Zenith DAW project.
