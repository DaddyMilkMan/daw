# Zenith Sampler - Sample Maps & Content Guide

This directory contains example `.zsamplemap.json` files that demonstrate how to create sample-based instruments for Zenith DAW.

## Directory Structure

```
Content/Examples/
├── README.md                           # This file
├── SampleMaps/                        # Sample map definitions (.zsamplemap.json)
│   ├── 808-essentials.zsamplemap.json
│   ├── lofi-keys.zsamplemap.json
│   ├── trap-pluck.zsamplemap.json
│   ├── orchestral-strings.zsamplemap.json
│   └── fx-impacts.zsamplemap.json
└── Samples/                           # Place your .wav files here
    └── (your sample files go here)
```

## What is a .zsamplemap.json file?

A `.zsamplemap.json` file defines a complete sampler instrument, including:
- **Metadata**: Name, category, description, version
- **Parameters**: Default ADSR envelope, filter, pitch, and global settings
- **Regions**: Sample file mappings with key ranges, velocity layers, loop modes, and tuning

## Sample Map Schema

### Basic Structure

```json
{
  "name": "Instrument Name",
  "category": "drums | keys | synth | orchestral | fx",
  "description": "Brief description of the instrument",
  "author": "Your Name",
  "version": "1.0.0",

  "parameters": {
    "attack": 0.01,         // Envelope attack (0.001-5.0 seconds)
    "decay": 0.1,           // Envelope decay (0.001-5.0 seconds)
    "sustain": 0.7,         // Envelope sustain level (0.0-1.0)
    "release": 0.3,         // Envelope release (0.001-10.0 seconds)
    "filterCutoff": 1.0,    // Filter cutoff (0.0-1.0, mapped to 20Hz-20kHz)
    "filterResonance": 0.0, // Filter resonance (0.0-1.0)
    "sampleStartOffset": 0.0, // Sample start position (0.0-1.0)
    "pitchFine": 0.0,       // Fine tuning in cents (-100 to +100)
    "pitchSemitones": 0.0,  // Coarse tuning in semitones (-24 to +24)
    "globalPan": 0.5,       // Stereo pan (0.0=left, 0.5=center, 1.0=right)
    "globalGain": 0.8       // Global gain (0.0-2.0)
  },

  "regions": [
    {
      "filePath": "sample-file.wav",  // Relative to Samples/ directory
      "rootNote": 60,                  // MIDI note for original pitch (C4=60)
      "lowNote": 54,                   // Lowest MIDI note for this region
      "highNote": 65,                  // Highest MIDI note for this region
      "lowVel": 0,                     // Lowest velocity (0-127)
      "highVel": 127,                  // Highest velocity (0-127)
      "loopMode": "none",              // none | forward | ping-pong
      "gain": 1.0,                     // Per-sample gain multiplier
      "tune": 0.0,                     // Per-sample tuning in semitones
      "comment": "Optional description"
    }
  ],

  "notes": {
    "usage": "Additional information about using this instrument",
    "tips": "Production tips or techniques"
  }
}
```

### Field Descriptions

#### Metadata
- **name**: Display name of the instrument
- **category**: Helps organize instruments in the browser
- **description**: Brief description (shown in preset browser)
- **author**: Creator name
- **version**: Semantic version (e.g., "1.0.0")

#### Parameters
Default parameter values that load with the instrument. All values are normalized (0.0-1.0) except:
- **pitchFine**: Cents (-100 to +100)
- **pitchSemitones**: Semitones (-24 to +24)
- **globalGain**: Amplitude (0.0-2.0)

#### Regions
Each region defines a sample file and its mapping:

- **filePath**: Path to WAV file, relative to `Samples/` directory
- **rootNote**: MIDI note number where the sample plays at original pitch (Middle C = 60)
- **lowNote/highNote**: Key range this sample covers
- **lowVel/highVel**: Velocity range (0-127) for velocity layering
- **loopMode**:
  - `none`: One-shot playback (drums, FX, plucks)
  - `forward`: Loop from start to end (sustained instruments)
  - `ping-pong`: Loop forward then backward (special effects)
- **gain**: Per-sample volume adjustment (1.0 = unity)
- **tune**: Per-sample pitch adjustment in semitones (use to fix out-of-tune samples)

## How to Hook Real WAV Files

### Step 1: Organize Your Samples

Create a directory structure for your instrument:

```
Content/
└── Instruments/
    └── ZenithSampler/
        └── MyInstrument/          # Your instrument name
            ├── MyInstrument.zpatch  # Copy and rename a .zsamplemap.json file
            └── Samples/
                ├── sample1.wav
                ├── sample2.wav
                └── sample3.wav
```

### Step 2: Prepare Your Samples

**Audio Requirements:**
- **Format**: WAV, AIFF, FLAC, OGG, or MP3
- **Bit Depth**: 16-bit or 24-bit recommended
- **Sample Rate**: 44.1kHz or 48kHz (will be resampled automatically)
- **Channels**: Mono or stereo

**Best Practices:**
- Trim silence from the start (or use `sampleStartOffset` to adjust)
- Normalize to avoid clipping (leave ~3dB headroom)
- For sustained instruments: ensure clean loop points
- For drums/FX: short, tight samples work best

### Step 3: Create Your Sample Map

1. Copy one of the example `.zsamplemap.json` files from `Content/Examples/SampleMaps/`
2. Rename it to match your instrument (e.g., `MyInstrument.zpatch`)
3. Edit the JSON to match your samples:

```json
{
  "name": "My Custom Instrument",
  "category": "keys",
  "description": "My awesome instrument",

  "parameters": { /* adjust to taste */ },

  "regions": [
    {
      "filePath": "sample1.wav",    // Must exist in Samples/ directory
      "rootNote": 60,                // C4
      "lowNote": 54,                 // F#3
      "highNote": 65,                // F4
      "lowVel": 0,
      "highVel": 127,
      "loopMode": "forward",
      "gain": 1.0,
      "tune": 0.0
    }
    // Add more regions for other samples
  ]
}
```

### Step 4: Load Your Instrument

In Zenith DAW, your instrument will appear in:
- Instrument browser under the specified category
- Can be loaded via CommandAPI: `load_sample_bank("MyInstrument")`

## Common Mapping Strategies

### 1. Drum Kit Mapping (GM Standard)
Map one sample per MIDI note:
```
36 (C1)  = Bass Drum
38 (D1)  = Snare
42 (F#1) = Closed Hi-Hat
46 (A#1) = Open Hi-Hat
```

### 2. Chromatic Mapping (Multi-sampled Instruments)
Sample every 3-12 semitones, let engine pitch-shift in between:
```
Region 1: C2 (36) covering 30-41
Region 2: C3 (48) covering 42-53
Region 3: C4 (60) covering 54-65
Region 4: C5 (72) covering 66-77
```

### 3. Velocity Layering
Map same notes to different samples based on velocity:
```
Region 1: C3 soft  (48, vel 0-63)
Region 2: C3 hard  (48, vel 64-127)
Region 3: C4 soft  (60, vel 0-63)
Region 4: C4 hard  (60, vel 64-127)
```

### 4. Round-Robin Sampling (Future)
Not yet supported, but planned for Phase 2

## Loop Mode Guide

### `"loopMode": "none"`
**Use for:**
- Drums
- Percussion
- One-shot FX
- Plucks and stabs

The sample plays once from start to end.

### `"loopMode": "forward"`
**Use for:**
- Sustained instruments (piano, strings, pads)
- Bass loops
- Synth waves

The sample loops from start to end continuously while the note is held.

**Loop Point Tips:**
- Set loop points in your sample editor (e.g., Audacity, Ableton)
- Look for zero-crossings to avoid clicks
- For natural sustain, loop the steady-state portion after the attack

### `"loopMode": "ping-pong"`
**Use for:**
- Special effects
- Experimental textures

The sample plays forward, then backward, then forward again.

## Tuning Guide

### Root Note
The `rootNote` is the MIDI note where the sample plays at its recorded pitch.

**Example**: If you recorded a piano sample by playing C4 (MIDI note 60), set:
```json
"rootNote": 60
```

### Per-Sample Tuning
If a sample is slightly out of tune, use the `tune` parameter:
```json
"tune": -0.15  // Tune down by 15 cents
```

### Global Tuning
Users can adjust tuning in real-time with:
- `pitchFine`: Fine tuning (-100 to +100 cents)
- `pitchSemitones`: Coarse tuning (-24 to +24 semitones)

## Performance Tips

### CPU Optimization
- **Fewer samples = better performance**: Use wider key ranges
- **Smaller files = faster loading**: Use compressed formats (OGG, FLAC)
- **Shorter samples = less memory**: Trim unused tails

### Memory Usage
The ZenithSampler uses AudioFilePool for RT-safe streaming:
- Samples are loaded into RAM on the message thread
- No disk I/O happens on the audio thread
- Total memory = sum of all sample file sizes

**Memory estimates:**
- 10 seconds stereo @ 44.1kHz 16-bit ≈ 1.7 MB
- 60 seconds stereo @ 44.1kHz 16-bit ≈ 10.3 MB

### Recommended Sample Sizes
- **Drums/FX**: < 5 seconds per sample
- **Loops**: 1-4 bars (4-16 seconds)
- **Sustained instruments**: 5-10 seconds with loop points

## Example Instruments

This directory includes 5 example instruments:

1. **808-essentials.zsamplemap.json**
   - Classic 808 drum machine
   - One-shot samples, no looping
   - Standard GM drum mapping

2. **lofi-keys.zsamplemap.json**
   - Vintage piano with character
   - 2 velocity layers (soft/hard)
   - Forward looping for sustain

3. **trap-pluck.zsamplemap.json**
   - Modern trap pluck synth
   - 5 chromatic zones
   - No looping, punchy envelope

4. **orchestral-strings.zsamplemap.json**
   - Lush string ensemble
   - 3 ranges, 2 velocity layers
   - Forward looping with long release

5. **fx-impacts.zsamplemap.json**
   - Cinematic sound effects
   - One sample per note
   - No looping

## Technical Details

### AudioFilePool Integration
ZenithSampler uses AudioFilePool for thread-safe sample loading:
- All file I/O happens on the message thread
- Audio thread accesses pre-loaded buffers via shared_ptr
- No allocations on the audio thread
- Reference counting ensures samples stay loaded while in use

### Supported Audio Formats
- WAV (PCM, float)
- AIFF
- FLAC (lossless compression)
- OGG Vorbis (lossy compression)
- MP3 (via JUCE's built-in decoder)

### Playback Engine
- Linear interpolation for pitch-shifting
- ADSR envelope per voice
- State Variable TPT low-pass filter
- Polyphonic (16 voices default)
- Velocity-sensitive amplitude

## Troubleshooting

### Sample Not Loading
- Check file path is correct (relative to `Samples/` directory)
- Verify file format is supported
- Check console logs for error messages

### Sample Playing at Wrong Pitch
- Verify `rootNote` matches the recorded pitch
- Check `tune` parameter isn't offsetting it
- Ensure sample rate matches (44.1kHz or 48kHz recommended)

### Clicks or Pops
- For looped samples: check loop points are at zero-crossings
- Adjust `attack` parameter to add fade-in
- Check `sampleStartOffset` isn't cutting into transient

### Sample Too Quiet/Loud
- Adjust per-sample `gain` parameter
- Normalize samples in audio editor before importing
- Use `globalGain` parameter for overall level

## Next Steps

1. Create your sample library
2. Organize samples in the correct directory structure
3. Create a `.zsamplemap.json` file (use examples as templates)
4. Place in `Content/Instruments/ZenithSampler/YourInstrument/`
5. Restart Zenith or reload instruments
6. Test in the DAW!

For more information, see:
- `zenith-core/Source/instruments/ZenithSampler.h`
- `zenith-core/Source/instruments/ZenithSampler.cpp`
- JUCE AudioFormatReader documentation

---

**Happy sampling! 🎵**
