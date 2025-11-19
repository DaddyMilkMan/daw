# ZenithPolySynth Preset Generator

This directory contains a procedural preset generator for ZenithPolySynth that creates 500+ high-quality presets across popular sound categories.

## Overview

The preset generator creates professionally-designed presets for the following categories:

### Bass (90 presets)
- **Sub/808 Bass** (30 presets): Classic sub bass and 808-style bass sounds
- **Reese Bass** (25 presets): Detuned saw bass with thick, growling character
- **Wobble Bass** (20 presets): Dubstep-style wobble bass with LFO modulation
- **FM Bass** (15 presets): FM-style bass with multiple oscillators

### Leads (90 presets)
- **Supersaw Lead** (40 presets): Epic supersaw leads for festival/EDM
- **Pluck Lead** (20 presets): Sharp, plucky lead sounds
- **Sync Lead** (15 presets): Aggressive sync-style leads
- **Brass Lead** (15 presets): Brass-like synth leads

### Plucks (65 presets)
- **EDM Pluck** (30 presets): Bright EDM pluck sounds
- **Bell Pluck** (20 presets): Bell-like pluck sounds
- **Marimba** (15 presets): Marimba-like mallet sounds

### Pads (85 presets)
- **Warm Analog Pad** (25 presets): Warm analog-style pads
- **Glass/Digital Pad** (20 presets): Bright, glassy digital pads
- **Ambient Drone** (20 presets): Dark ambient drone pads
- **String Pad** (20 presets): String ensemble pads

### Keys (50 presets)
- **Electric Piano** (20 presets): Electric piano sounds
- **Organ** (15 presets): Organ-like key sounds
- **Clavinet** (15 presets): Clavinet-style funky keys

### Bells (35 presets)
- **Glass Bell** (20 presets): Glassy, crystalline bell sounds
- **Metal Bell** (15 presets): Metallic bell sounds

### FX (30 presets)
- **Riser** (15 presets): Riser effects for builds and transitions
- **Impact** (15 presets): Impact and hit sounds

### Arpeggios (30 presets)
- **Pulse Arp** (15 presets): Pulsing arpeggio sounds
- **Sequence Arp** (15 presets): Bright sequencer-style arpeggios

**Total: 475 presets across 24 categories**

## Features

- **Deterministic Generation**: Uses seeded random number generation for reproducible results
- **Category-Specific Parameter Ranges**: Each category has carefully designed parameter ranges based on sound design principles
- **Automatic Gain Compensation**: Presets automatically adjust master gain based on complexity (oscillator count, unison voices) to prevent clipping
- **CPU-Aware**: Limits unison voices and other CPU-intensive parameters to maintain reasonable performance
- **Professional Naming**: Each preset gets a descriptive name combining adjectives with category descriptors

## Building and Running

### Method 1: Using CMake (JUCE version)

```bash
# From the repository root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target GeneratePolySynthPresets

# Run the generator
./GeneratePolySynthPresets
```

### Method 2: Standalone Compilation (No Dependencies)

```bash
# From the repository root
g++ -std=c++17 -o GeneratePolySynthPresetsStandalone \
    tools/GeneratePolySynthPresetsStandalone.cpp

# Run the generator
./GeneratePolySynthPresetsStandalone
```

### Method 3: Using the build script

```bash
# From tools/ directory
cd tools
g++ -std=c++17 -o GeneratePolySynthPresetsStandalone \
    GeneratePolySynthPresetsStandalone.cpp
./GeneratePolySynthPresetsStandalone
```

## Output

By default, presets are saved to:
```
zenith-core/Content/Presets/PolySynth_Generated/
```

You can specify a custom output directory as the first argument:
```bash
./GeneratePolySynthPresetsStandalone /path/to/output/directory
```

The generator creates a directory structure like:
```
PolySynth_Generated/
├── Bass_Sub808/
│   ├── Bass_Sub808_1.json
│   ├── Bass_Sub808_2.json
│   └── ...
├── Lead_Supersaw/
│   ├── Lead_Supersaw_1.json
│   ├── Lead_Supersaw_2.json
│   └── ...
└── [other categories]/
```

## Preset Format

Each preset is saved as a JSON file with the following structure:

```json
{
  "id": "Bass_Sub808_1",
  "name": "Deep Sub808 1",
  "instrumentId": "zenith.poly_synth",
  "category": "Bass_Sub808",
  "author": "Factory",
  "description": "Classic sub bass and 808-style bass sounds",
  "version": "1.0.0",
  "tags": ["bass", "sub", "808", "hip-hop", "trap"],
  "params": {
    "osc1_wave": 0.0000,
    "osc1_detune": 0.5000,
    "osc1_mix": 1.0000,
    ...
  }
}
```

All parameter values are normalized to the range [0.0, 1.0] for storage. The synth engine converts these back to actual ranges when loading.

## Design Principles

### Bass Sounds
- Primarily use sine oscillators for sub content
- Low filter cutoffs (0.15-0.45)
- Sharp attacks, medium-long decays
- Moderate resonance for character

### Lead Sounds
- Multiple detuned saw oscillators
- High unison voice counts (4-7 voices)
- Bright filter settings (0.6-0.95)
- Medium attack/sustain for playable leads

### Pluck Sounds
- Very fast attacks (<0.01)
- Fast decay (0.05-0.3)
- Low sustain
- High resonance for articulation

### Pad Sounds
- Long attacks (0.2-0.8)
- High sustain/release
- Multiple oscillators for thickness
- Often include LFO modulation for movement

### Gain Compensation

The generator automatically calculates a complexity factor based on:
- Number of active oscillators
- Unison voice count
- Filter drive amount

Master gain is then adjusted to prevent clipping:
```cpp
complexity = 1.0 + sum(oscillator_mixes) + sqrt(unison_voices)
master_gain = min(0.7 / sqrt(complexity), 0.85)
```

This ensures that complex sounds with many oscillators or high unison don't clip, while keeping simple sounds at an appropriate level.

## Customization

To modify the generator:

1. **Add new categories**: Add a new `CategoryDefinition` in `initializeCategoryDefinitions()`
2. **Adjust parameter ranges**: Modify the `paramRanges` map for existing categories
3. **Change number of variations**: Set `numVariations` for each category
4. **Modify naming**: Edit the `adjectives` array in `generatePresetName()`

Example adding a new category:

```cpp
{
    CategoryDefinition cat;
    cat.name = "Bass_NewType";
    cat.description = "Your new bass type description";
    cat.numVariations = 20;
    cat.tags = {"bass", "new", "custom"};

    // Set parameter ranges
    cat.paramRanges["osc1_wave"] = {0.0f, 0.2f};  // Sine to Saw
    cat.paramRanges["osc1_mix"] = {1.0f, 1.0f};
    cat.paramRanges["filter_cutoff"] = {0.2f, 0.4f};
    // ... more parameters

    categories_.push_back(cat);
}
```

## Implementation Files

- **GeneratePolySynthPresets.cpp**: JUCE-based version (integrates with CMake build)
- **GeneratePolySynthPresetsStandalone.cpp**: Standalone version with no dependencies (easier to compile)

Both versions produce identical output.

## Notes

- The generator uses a deterministic seed based on preset index, so regenerating will produce the same presets
- All presets are designed to be CPU-efficient (no excessive unison or extreme parameter values)
- Filter cutoffs and time-based parameters use the same normalization curves as the synth (skew factor 0.3)
- LFO targets are limited to implemented targets (FilterCutoff is the most commonly used)

## Integration with ZenithPolySynth

To use these presets in ZenithPolySynth:

1. Copy the generated preset directories to the appropriate location:
   - Factory presets: `<UserAppData>/Zenith/Instruments/Factory/`
   - User presets: `<UserAppData>/Zenith/Instruments/User/`

2. Or move the `PolySynth_Generated` directory into:
   - `zenith-core/Content/Presets/PolySynth/`

3. The preset browser will automatically detect and load all JSON presets from these directories.

4. Presets are searchable by:
   - Category
   - Tags
   - Name

## License

These presets are generated as part of the Zenith DAW project and are intended for distribution with the software as factory content.
