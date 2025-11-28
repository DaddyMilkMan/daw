# ZenithSampler Content Guide

## Overview

ZenithSampler is a multi-sample instrument designed for drums, 808s, pianos, and one-shot FX. It uses RT-safe sample loading via AudioFilePool and supports:

- Multi-sample mappings with MIDI note/velocity ranges
- Per-sample root note, tuning, gain, and loop modes
- ADSR envelope, lowpass filter, and global controls
- Async background loading (no audio thread blocking)

## Sample Bank Format

Sample banks are defined in JSON format with the `.zpatch` extension:

```json
{
  "name": "My Sample Bank",
  "category": "drums",
  "parameters": {
    "attack": 0.01,
    "decay": 0.1,
    "sustain": 0.7,
    "release": 0.3,
    "filterCutoff": 1.0,
    "filterResonance": 0.0,
    "sampleStartOffset": 0.0,
    "pitchFine": 0.0,
    "pitchSemitones": 0.0,
    "globalPan": 0.5,
    "globalGain": 0.8
  },
  "regions": [
    {
      "filePath": "kick.wav",
      "rootNote": 36,
      "lowNote": 36,
      "highNote": 36,
      "lowVel": 0,
      "highVel": 127,
      "loopMode": "none",
      "gain": 1.0,
      "tune": 0.0
    },
    {
      "filePath": "snare.wav",
      "rootNote": 38,
      "lowNote": 38,
      "highNote": 38,
      "lowVel": 0,
      "highVel": 127,
      "loopMode": "none",
      "gain": 1.0,
      "tune": 0.0
    }
  ]
}
```

### Field Descriptions

**Bank Metadata:**
- `name`: Display name for the bank
- `category`: Category tag ("drums", "808", "keys", "fx")

**Parameters:** Default parameter values when the bank is loaded

**Regions:** Sample mapping definitions
- `filePath`: Path to WAV file (relative to bank's Samples/ directory)
- `rootNote`: MIDI note for original pitch (0-127)
- `lowNote/highNote`: MIDI note range this sample covers
- `lowVel/highVel`: Velocity range (0-127)
- `loopMode`: "none", "forward", or "pingpong"
- `gain`: Per-sample volume multiplier (0.0-2.0)
- `tune`: Per-sample pitch offset in semitones

## Directory Structure

The default content directory is:
- **macOS/Linux**: `~/Music/Zenith/`
- **Windows**: `%USERPROFILE%\Music\Zenith\`

Structure:
```
~/Music/Zenith/
  Instruments/
    ZenithSampler/
      808Essentials/
        808Essentials.zpatch
        Samples/
          808-kick.wav
          808-snare.wav
          808-hihat-closed.wav
          ...
      LoFiKeys/
        LoFiKeys.zpatch
        Samples/
          lofi-piano-C3.wav
          lofi-piano-C4.wav
          ...
```

## Loading Sample Banks

### From Disk (File-based)

```cpp
// Get the processor
auto* processor = dynamic_cast<ZenithSamplerProcessor*>(getAudioProcessor());

// Load by name (searches in content directory)
processor->loadSampleBankByName("808Essentials");

// Load from explicit path
auto bankFile = juce::File("~/Music/Zenith/Instruments/ZenithSampler/MyBank/MyBank.zpatch");
processor->loadSampleBank(bankFile);
```

### From JSON String (Built-in Banks)

```cpp
const char* bankJson = R"({
  "name": "Custom Bank",
  "category": "drums",
  "parameters": { ... },
  "regions": [ ... ]
})";

processor->loadSampleBankFromJson(bankJson, "Custom Bank");
```

### Using AudioFilePool Integration

When a ZenithSamplerProcessor is created within the Engine context:

```cpp
// In your Engine initialization or instrument creation:
auto sampler = std::make_unique<ZenithSamplerProcessor>();

// Connect to the AudioFilePool
sampler->setAudioFilePool(&engine.getAudioFilePool());

// Now all sample loading will use the pool for RT-safe access
sampler->loadSampleBankByName("808Essentials");
```

## Mounting a Custom Content Folder

To use a different content directory:

```cpp
#include "instruments/ContentPaths.h"

// Set custom content root
auto customPath = juce::File("/path/to/my/content");
ContentPaths::getInstance().setContentRoot(customPath);

// Now all banks will be loaded from:
// /path/to/my/content/Instruments/ZenithSampler/
```

## Creating New Sample Banks

1. **Create directory structure:**
   ```
   ~/Music/Zenith/Instruments/ZenithSampler/MyNewBank/
   ~/Music/Zenith/Instruments/ZenithSampler/MyNewBank/Samples/
   ```

2. **Add your WAV samples** to the `Samples/` directory

3. **Create `MyNewBank.zpatch`** with JSON definition

4. **Load the bank:**
   ```cpp
   processor->loadSampleBankByName("MyNewBank");
   ```

## Built-in Sample Banks

The following stub banks are defined in code (require actual WAV files):

1. **808 Essentials** (`category: "808"`)
   - Kick, snare, hi-hats, 808 bass
   - Optimized for trap/hip-hop

2. **LoFi Keys** (`category: "keys"`)
   - Multi-sampled piano with velocity layers
   - Looped sustain, vintage character

3. **FX Hits** (`category: "fx"`)
   - Risers, impacts, reverse FX, whooshes
   - One-shot effects for transitions

## Parameters for AI Control

The sampler exposes these parameters for AI automation and UI:

**Envelope:**
- `attack`, `decay`, `sustain`, `release`

**Filter:**
- `filter_cutoff`, `filter_resonance`

**Sample:**
- `sample_start_offset`

**Pitch:**
- `pitch_fine` (cents), `pitch_semitones`

**Global:**
- `global_pan`, `global_gain`, `character`

**Macros** (high-level controls):
- `macro_brightness`: Controls filter and character
- `macro_response`: Controls attack and release

## RT-Safety Guarantees

- Sample loading happens on background thread
- Audio thread never blocks on file I/O
- AudioFilePool uses shared_ptr for atomic ref counting
- Samples are pre-loaded into memory before playback
- Synth sound swapping is atomic (JUCE Synthesiser guarantee)

## Future: Large Sample Libraries

For large libraries (e.g., 10GB+ orchestral), extend this architecture by:

1. Using streaming from AudioFilePool (Phase 2+)
2. Implementing LRU cache for sample eviction
3. Adding multi-threaded background loading
4. Supporting compressed sample formats

Current implementation (Phase 1.2) loads entire files into RAM, suitable for drum kits, short samples, and typical electronic music instruments.
