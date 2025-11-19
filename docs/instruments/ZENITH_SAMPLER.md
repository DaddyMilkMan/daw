# Zenith Sampler

## Overview

The **Zenith Sampler** is a built-in sample-based instrument for Zenith DAW. It provides professional multi-sample playback with key ranges, velocity layers, envelope control, and filtering.

## Features

- **Multi-sample playback** with key range and velocity layer support
- **Amp envelope** (ADSR) for shaping the sound
- **Low-pass filter** with cutoff and resonance controls
- **Global controls**: Tune, Gain, and Character
- **Async patch loading** (all file I/O happens off the audio thread)
- **Cross-platform** sample/preset management

---

## Directory Structure

The Zenith Sampler uses a standardized directory structure for organizing samples and patches:

```
<ContentRoot>/
  Instruments/
    ZenithSampler/
      <PatchName>/
        Samples/
          sample1.wav
          sample2.wav
          ...
        <PatchName>.zpatch
```

### Platform-Specific Default Locations

| Platform | Default Content Root |
|----------|---------------------|
| **macOS** | `~/Music/Zenith/` |
| **Windows** | `%USERPROFILE%\Music\Zenith\` |
| **Linux** | `~/Music/Zenith/` |

You can customize the content root directory programmatically using:

```cpp
zenith::instruments::ContentPaths::getInstance()
    .setContentRoot(juce::File("/path/to/custom/location"));
```

---

## .zpatch Format

Patches are defined using JSON-based `.zpatch` files. Here's the complete format:

### Example: Piano.zpatch

```json
{
  "name": "Piano",
  "version": "1.0",
  "description": "A simple piano patch",
  "parameters": {
    "attack": 0.01,
    "decay": 0.2,
    "sustain": 0.7,
    "release": 0.5,
    "filterCutoff": 1.0,
    "filterResonance": 0.0,
    "tune": 0.0,
    "gain": 0.8
  },
  "samples": [
    {
      "file": "piano_c4.wav",
      "rootNote": 60,
      "lowNote": 57,
      "highNote": 62,
      "lowVelocity": 0,
      "highVelocity": 127,
      "gain": 1.0
    },
    {
      "file": "piano_c5.wav",
      "rootNote": 72,
      "lowNote": 69,
      "highNote": 74,
      "lowVelocity": 0,
      "highVelocity": 127,
      "gain": 1.0
    }
  ]
}
```

### Field Reference

#### Root Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Display name of the patch |
| `version` | string | No | Version string (e.g., "1.0") |
| `description` | string | No | Brief description |
| `parameters` | object | No | Default parameter values |
| `samples` | array | Yes | Array of sample definitions |

#### Parameters Object

All parameters are normalized (0.0 - 1.0, except where noted):

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `attack` | float | 0.001 - 5.0s | 0.01 | Attack time in seconds |
| `decay` | float | 0.001 - 5.0s | 0.1 | Decay time in seconds |
| `sustain` | float | 0.0 - 1.0 | 0.7 | Sustain level |
| `release` | float | 0.001 - 10.0s | 0.3 | Release time in seconds |
| `filterCutoff` | float | 0.0 - 1.0 | 1.0 | Filter cutoff (0=20Hz, 1=20kHz) |
| `filterResonance` | float | 0.0 - 1.0 | 0.0 | Filter resonance |
| `tune` | float | -12.0 - +12.0 | 0.0 | Global tuning in semitones |
| `gain` | float | 0.0 - 2.0 | 0.8 | Output gain |

#### Sample Object

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `file` | string | Yes | - | Filename relative to Samples/ directory |
| `rootNote` | int | Yes | - | MIDI note number for original pitch (0-127) |
| `lowNote` | int | No | 0 | Lowest MIDI note for this sample |
| `highNote` | int | No | 127 | Highest MIDI note for this sample |
| `lowVelocity` | int | No | 0 | Minimum velocity (0-127) |
| `highVelocity` | int | No | 127 | Maximum velocity (0-127) |
| `gain` | float | No | 1.0 | Sample-specific gain multiplier |

### MIDI Note Numbers

Common note reference:

```
C-2 = 0
C-1 = 12
C0  = 24
C1  = 36
C2  = 48
C3  = 60  (Middle C)
C4  = 72
C5  = 84
C6  = 96
C7  = 108
G8  = 127
```

---

## Creating a Patch

### Step 1: Create Directory Structure

```bash
# Navigate to Zenith content directory
cd ~/Music/Zenith

# Create directories for your patch
mkdir -p Instruments/ZenithSampler/MyPatch/Samples
```

### Step 2: Add Samples

Copy your WAV files to the `Samples/` directory:

```bash
cp /path/to/your/samples/*.wav Instruments/ZenithSampler/MyPatch/Samples/
```

**Supported formats:**
- WAV (16/24/32-bit PCM, float)
- Sample rates: Any (will be resampled automatically)
- Channels: Mono or stereo

### Step 3: Create .zpatch File

Create `MyPatch.zpatch` in the patch directory:

```json
{
  "name": "MyPatch",
  "parameters": {
    "attack": 0.01,
    "decay": 0.1,
    "sustain": 0.7,
    "release": 0.3,
    "filterCutoff": 1.0,
    "filterResonance": 0.0,
    "tune": 0.0,
    "gain": 0.8
  },
  "samples": [
    {
      "file": "sample1.wav",
      "rootNote": 60,
      "lowNote": 0,
      "highNote": 127
    }
  ]
}
```

### Step 4: Load in Zenith DAW

1. Launch Zenith DAW
2. Create a MIDI/Instrument track
3. Load ZenithSampler instrument
4. Select "MyPatch" from the preset dropdown
5. Play MIDI notes!

---

## Factory Content

### Minimal Example Patch

A minimal example patch structure is provided below. You can use this as a template:

**Location:** `~/Music/Zenith/Instruments/ZenithSampler/Example/`

**Files:**
- `Example.zpatch` - Patch definition
- `Samples/example.wav` - A simple sine wave or test tone

**Example.zpatch:**

```json
{
  "name": "Example",
  "description": "Simple example patch for testing",
  "parameters": {
    "attack": 0.01,
    "decay": 0.1,
    "sustain": 0.7,
    "release": 0.3,
    "filterCutoff": 1.0,
    "filterResonance": 0.0,
    "tune": 0.0,
    "gain": 0.8
  },
  "samples": [
    {
      "file": "example.wav",
      "rootNote": 60,
      "lowNote": 0,
      "highNote": 127,
      "lowVelocity": 0,
      "highVelocity": 127,
      "gain": 1.0
    }
  ]
}
```

### Generating Test Samples

If you don't have sample files, you can generate a simple test tone using tools like:

**Sox (macOS/Linux):**
```bash
sox -n -r 44100 -c 1 example.wav synth 1 sine 440
```

**Audacity:**
1. Generate > Tone
2. Frequency: 440 Hz
3. Duration: 1 second
4. Export as WAV

---

## UI Controls

The Zenith Sampler UI is divided into four sections:

### 1. Preset Section (Top)
- **Patch dropdown**: Select from available patches
- **Status label**: Shows loading status and current patch

### 2. Envelope Section (Middle-Left)
- **Attack**: Time for sound to reach full volume
- **Decay**: Time to decay from peak to sustain level
- **Sustain**: Level held while note is pressed
- **Release**: Time to fade out after note release

### 3. Filter Section (Middle-Right)
- **Cutoff**: Low-pass filter frequency (0 = dark, 1 = bright)
- **Resonance**: Filter resonance/emphasis

### 4. Global Section (Bottom)
- **Tune**: Global pitch in semitones (-12 to +12)
- **Gain**: Output level (0 to 2.0)
- **Character**: Adds subtle saturation (0.5 = neutral, 1.0 = warm)

---

## Manual Testing

### Prerequisites

1. Zenith DAW built and running
2. Sample content directory created: `~/Music/Zenith/Instruments/ZenithSampler/`
3. At least one valid `.zpatch` file with samples

### Test Procedure

#### Test 1: Patch Loading

1. Launch Zenith DAW
2. Create an Instrument track (or MIDI track)
3. Load ZenithSampler instrument
4. Open the instrument editor
5. **Expected**: Preset dropdown shows available patches

**Pass criteria:**
- All patches in `~/Music/Zenith/Instruments/ZenithSampler/` appear in dropdown
- Status label shows "No patch loaded" initially
- No crashes or errors

#### Test 2: Sample Playback

1. Select a patch from dropdown
2. Wait for "Patch loaded" status
3. Play MIDI notes (either via controller or virtual keyboard)
4. **Expected**: Audio output matches sample content

**Pass criteria:**
- MIDI notes trigger sample playback
- Pitch follows MIDI note number correctly
- No audio glitches or dropouts
- Polyphony works (multiple notes can play simultaneously)

#### Test 3: Envelope Controls

1. Load a patch
2. Adjust Attack to maximum (5 seconds)
3. Play a MIDI note
4. **Expected**: Slow fade-in over 5 seconds

5. Adjust Release to maximum (10 seconds)
6. Play and release a MIDI note
7. **Expected**: Slow fade-out over 10 seconds

**Pass criteria:**
- Envelope parameters respond immediately
- Audio behavior matches control settings
- No clicks or pops during parameter changes

#### Test 4: Filter Controls

1. Load a patch with harmonic content (not a sine wave)
2. Set Cutoff to 0.0 (minimum)
3. Play MIDI note
4. **Expected**: Very dark/muffled sound

5. Slowly increase Cutoff to 1.0
6. **Expected**: Sound becomes brighter

7. Set Resonance to maximum while Cutoff is at 0.5
8. **Expected**: Emphasis at cutoff frequency

**Pass criteria:**
- Filter audibly affects the sound
- Cutoff sweeps smoothly
- Resonance creates noticeable peak

#### Test 5: Global Controls

1. Load a patch
2. Set Tune to +12 semitones
3. Play C4 (MIDI 60)
4. **Expected**: Sounds like C5 (one octave higher)

5. Set Gain to 0.0
6. **Expected**: No audio output

7. Set Gain to 2.0
8. **Expected**: Louder output (may distort if source is loud)

9. Set Character to 1.0 (maximum)
10. Play loud notes
11. **Expected**: Subtle warmth/saturation

**Pass criteria:**
- Tuning changes pitch correctly
- Gain controls volume
- Character adds subtle coloration without harsh distortion

#### Test 6: Async Loading

1. Load a large patch (many samples or large WAV files)
2. **Expected**: UI remains responsive during loading
3. Status shows "Loading patch..."
4. Audio thread continues processing without dropouts

**Pass criteria:**
- No UI freezes during patch loading
- No audio dropouts on active notes
- Status updates correctly

#### Test 7: State Save/Restore

1. Load a patch and adjust parameters
2. Save project
3. Close and reopen project
4. **Expected**: Same patch loads with same parameter values

**Pass criteria:**
- Patch name is restored
- All parameter values are restored
- Audio output is identical

---

## Troubleshooting

### No Patches Found

**Symptom:** Preset dropdown shows "No patches found"

**Solutions:**
1. Check content directory exists: `~/Music/Zenith/Instruments/ZenithSampler/`
2. Verify patch structure:
   ```
   ZenithSampler/
     MyPatch/
       MyPatch.zpatch  ← Must match directory name!
       Samples/
         sample.wav
   ```
3. Check `.zpatch` file is valid JSON

### Patch Won't Load

**Symptom:** Status stays on "Loading patch..." or shows error

**Solutions:**
1. Check `.zpatch` JSON syntax (use a JSON validator)
2. Verify all sample files exist in `Samples/` directory
3. Check file paths in `.zpatch` (case-sensitive on Linux/macOS)
4. Ensure WAV files are in a supported format (PCM, not compressed)

### No Sound

**Symptom:** MIDI notes trigger but no audio output

**Solutions:**
1. Check Gain parameter is not at 0
2. Verify Track volume/mute/solo settings
3. Check sample file contains audio data (not silence)
4. Try increasing Attack if set too low
5. Verify audio output device is configured

### Audio Glitches

**Symptom:** Clicks, pops, or dropouts during playback

**Solutions:**
1. Increase audio buffer size in DAW settings
2. Reduce polyphony (fewer simultaneous notes)
3. Use smaller sample files (< 10 seconds)
4. Ensure sample files are 44.1kHz or 48kHz (avoids resampling)

---

## Advanced Usage

### Velocity Layers

Create multiple samples for the same key range with different velocity ranges:

```json
{
  "samples": [
    {
      "file": "piano_c4_soft.wav",
      "rootNote": 60,
      "lowNote": 60,
      "highNote": 60,
      "lowVelocity": 0,
      "highVelocity": 63
    },
    {
      "file": "piano_c4_loud.wav",
      "rootNote": 60,
      "lowNote": 60,
      "highNote": 60,
      "lowVelocity": 64,
      "highVelocity": 127
    }
  ]
}
```

### Key Zones

Map different samples to different note ranges:

```json
{
  "samples": [
    {
      "file": "bass.wav",
      "rootNote": 36,
      "lowNote": 0,
      "highNote": 48
    },
    {
      "file": "mid.wav",
      "rootNote": 60,
      "lowNote": 49,
      "highNote": 72
    },
    {
      "file": "treble.wav",
      "rootNote": 84,
      "lowNote": 73,
      "highNote": 127
    }
  ]
}
```

---

## Future Enhancements

Planned features for future versions:

- [ ] SFZ format import
- [ ] Loop point support
- [ ] Filter envelope (separate from amp)
- [ ] LFO modulation
- [ ] Round-robin sample switching
- [ ] Drag-and-drop sample loading
- [ ] Built-in sample editor
- [ ] Preset browser with search
- [ ] Factory content library

---

## Developer Notes

### Architecture

- **Thread safety**: All file I/O happens on a background thread
- **Lock-free**: Parameter updates use atomic operations
- **Modular**: Easily extensible for future features
- **JUCE-native**: Uses JUCE's built-in Synthesiser framework

### Key Classes

- `ZenithSampler` - Main AudioProcessor
- `ZenithSamplerSound` - Represents a single sample with metadata
- `ZenithSamplerVoice` - Handles per-voice playback, envelope, and filter
- `ZenithSamplerEditor` - Custom UI component
- `ContentPaths` - Cross-platform path management

### Integration with InstrumentRegistry

Instruments can be registered using the factory pattern:

```cpp
#include "instruments/InstrumentRegistry.h"
#include "instruments/ZenithSampler.h"

// Register at startup
zenith::instruments::InstrumentRegistry::InstrumentInfo info;
info.id = "zenith_sampler";
info.name = "Zenith Sampler";
info.category = "Sampler";
info.description = "Sample-based instrument";
info.version = "1.0.0";
info.factory = []() {
    return std::make_unique<zenith::instruments::ZenithSampler>();
};

zenith::instruments::InstrumentRegistry::getInstance()
    .registerInstrument(info);
```

---

## License

Part of Zenith DAW - see main project LICENSE file.

---

## Support

For bug reports and feature requests, please see the main Zenith DAW documentation.
