# ZenithSampler - Sample Bank Collection

This directory contains professionally designed sample banks for ZenithSampler with various preset variations. Each bank is mapped to provide an intuitive playing experience with different sonic characteristics.

## Directory Structure

```
zenith-core/Content/
├── Samples/              # Sample files go here (TODO: Add actual samples)
│   ├── 808/             # 808 drum samples
│   ├── Drums/           # Acoustic/electronic drum samples
│   ├── Keys/            # Piano/keyboard multi-samples
│   └── FX/              # Sound effects and transitions
└── Presets/Sampler/     # .zpatch preset files (ready to use)
```

## Sample Banks Overview

### 1. 808 Kit Banks (4 Variants)

**File:** `808_Kit.zpatch` (Classic)
**Description:** Classic 808 drum machine mapping with punchy envelope settings

**Variants:**
- **808_Kit_Punchy.zpatch** - Ultra-tight, short decay (A: 1ms, D: 150ms, S: 0, R: 50ms)
- **808_Kit_Long.zpatch** - Sustained 808s (A: 1ms, D: 800ms, S: 0.3, R: 500ms)
- **808_Kit_Filtered.zpatch** - Low-pass filtered (Cutoff: 0.6, Res: 0.4)

**Key Mapping:**
| MIDI Note | Note Name | Sample |
|-----------|-----------|--------|
| 36 | C1 | 808 Kick |
| 37 | C#1 | 808 Rim |
| 38 | D1 | 808 Snare |
| 39 | D#1 | 808 Clap |
| 41 | F1 | 808 Tom Low |
| 42 | F#1 | 808 Hi-hat Closed |
| 43 | G1 | 808 Tom Mid |
| 45 | A1 | 808 Tom High |
| 46 | A#1 | 808 Hi-hat Open |
| 49 | C#2 | 808 Cymbal |
| 56 | G#2 | 808 Cowbell |
| 63 | D#3 | 808 Conga High |
| 64 | E3 | 808 Conga Low |
| 70 | A#3 | 808 Maracas |
| 75 | D#4 | 808 Claves |

**Sample Files Required (15 total):**
- Content/Samples/808/808_kick.wav
- Content/Samples/808/808_snare.wav
- Content/Samples/808/808_clap.wav
- Content/Samples/808/808_rim.wav
- Content/Samples/808/808_tom_low.wav
- Content/Samples/808/808_tom_mid.wav
- Content/Samples/808/808_tom_high.wav
- Content/Samples/808/808_cowbell.wav
- Content/Samples/808/808_claves.wav
- Content/Samples/808/808_hihat_closed.wav
- Content/Samples/808/808_hihat_open.wav
- Content/Samples/808/808_cymbal.wav
- Content/Samples/808/808_maracas.wav
- Content/Samples/808/808_conga_low.wav
- Content/Samples/808/808_conga_high.wav

---

### 2. Drum Kit Banks (4 Variants)

**File:** `Drum_Kit.zpatch` (GM Layout)
**Description:** General MIDI standard drum mapping for maximum compatibility

**Variants:**
- **Drum_Kit_Tight.zpatch** - Tight, compressed sound (A: 1ms, D: 150ms, S: 0, R: 50ms)
- **Drum_Kit_Roomy.zpatch** - Room ambience feel (A: 1ms, D: 800ms, S: 0.2, R: 600ms)
- **Drum_Kit_Compressed.zpatch** - Punchy compressed (A: 1ms, D: 300ms, S: 0.4, R: 300ms, slight filtering)

**GM Drum Mapping (Excerpt):**
| MIDI Note | Note Name | Sample | Typical Use |
|-----------|-----------|--------|-------------|
| 35 | B0 | Kick (Acoustic) | Alternative kick |
| 36 | C1 | Kick 1 | Main kick |
| 37 | C#1 | Rim Shot | Side stick |
| 38 | D1 | Snare (Acoustic) | Main snare |
| 39 | D#1 | Clap | Hand clap |
| 40 | E1 | Snare (Electric) | Alternative snare |
| 41 | F1 | Tom Floor Low | Low floor tom |
| 42 | F#1 | Hi-hat Closed | Closed hi-hat |
| 43 | G1 | Tom Floor High | High floor tom |
| 44 | G#1 | Hi-hat Pedal | Pedal hi-hat |
| 45 | A1 | Tom Low | Low tom |
| 46 | A#1 | Hi-hat Open | Open hi-hat |
| 47 | B1 | Tom Low-Mid | Low-mid tom |
| 48 | C2 | Tom High-Mid | High-mid tom |
| 49 | C#2 | Crash Cymbal 1 | Crash |
| 50 | D2 | Tom High | High tom |
| 51 | D#2 | Ride Cymbal 1 | Ride |
| 52 | E2 | China Cymbal | China |
| 53 | F2 | Ride Bell | Bell |
| 54 | F#2 | Tambourine | Tambourine |
| 55 | G2 | Splash Cymbal | Splash |
| 56 | G#2 | Cowbell | Cowbell |
| 57 | A2 | Crash Cymbal 2 | Crash 2 |
| 58 | A#2 | Vibraslap | Vibraslap |
| 59 | B2 | Ride Cymbal 2 | Ride 2 |

**Sample Files Required (25 total):**
All files in `Content/Samples/Drums/` following GM naming convention.

---

### 3. Keys Banks (4 Variants)

**File:** `Keys.zpatch` (Multi-Sampled Piano)
**Description:** Multi-sampled piano with samples every minor third (3 semitones) for natural pitch shifting

**Variants:**
- **Keys_Bright.zpatch** - Bright, articulate (A: 1ms, D: 400ms, S: 0.5, R: 500ms, full filter)
- **Keys_Soft.zpatch** - Gentle, mellow (A: 20ms, D: 800ms, S: 0.7, R: 1000ms, reduced cutoff: 0.85)
- **Keys_Staccato.zpatch** - Short, percussive (A: 1ms, D: 200ms, S: 0, R: 100ms)

**Multi-Sample Mapping:**
| Root Note | Note Name | Sample File | Key Range |
|-----------|-----------|-------------|-----------|
| 36 | C1 | piano_C1.wav | 0-37 |
| 39 | Eb1 | piano_Eb1.wav | 38-40 |
| 42 | F#1 | piano_Gb1.wav | 41-43 |
| 45 | A1 | piano_A1.wav | 44-46 |
| 48 | C2 | piano_C2.wav | 47-49 |
| 51 | Eb2 | piano_Eb2.wav | 50-52 |
| 54 | F#2 | piano_Gb2.wav | 53-55 |
| 57 | A2 | piano_A2.wav | 56-58 |
| 60 | C3 (Middle C) | piano_C3.wav | 59-61 |
| 63 | Eb3 | piano_Eb3.wav | 62-64 |
| 66 | F#3 | piano_Gb3.wav | 65-67 |
| 69 | A3 | piano_A3.wav | 68-70 |
| 72 | C4 | piano_C4.wav | 71-73 |
| 75 | Eb4 | piano_Eb4.wav | 74-76 |
| 78 | F#4 | piano_Gb4.wav | 77-79 |
| 81 | A4 | piano_A4.wav | 80-82 |
| 84 | C5 | piano_C5.wav | 83-85 |
| 87 | Eb5 | piano_Eb5.wav | 86-88 |
| 90 | F#5 | piano_Gb5.wav | 89-91 |
| 93 | A5 | piano_A5.wav | 92-94 |
| 96 | C6 | piano_C6.wav | 95-127 |

**Sample Files Required (21 total):**
All files in `Content/Samples/Keys/` as listed above.

**Note:** Each sample covers approximately ±1-2 semitones to minimize pitch shifting artifacts.

---

### 4. FX Banks (4 Variants)

**File:** `FX.zpatch` (FX Collection)
**Description:** Sound effects and transitions scattered across middle-to-upper keyboard range

**Variants:**
- **FX_Atmospheric.zpatch** - Long, ambient (A: 50ms, D: 800ms, S: 0.6, R: 1200ms, filtered: 0.75)
- **FX_Percussive.zpatch** - Short, punchy (A: 1ms, D: 200ms, S: 0, R: 100ms)
- **FX_Dark.zpatch** - Filtered, dark (A: 20ms, D: 600ms, S: 0.4, R: 500ms, heavy filter: 0.5/0.5)

**FX Mapping:**
| MIDI Note | Note Name | Sample | Category |
|-----------|-----------|--------|----------|
| 60 | C3 | Riser Short | Transition |
| 62 | D3 | Riser Long | Transition |
| 64 | E3 | Downlifter | Transition |
| 65 | F3 | Impact Heavy | Impact |
| 67 | G3 | Impact Light | Impact |
| 69 | A3 | Whoosh Fast | Movement |
| 71 | B3 | Whoosh Slow | Movement |
| 72 | C4 | Reverse Cymbal | Reversal |
| 74 | D4 | Vinyl Scratch | Texture |
| 76 | E4 | Glitch Stutter | Glitch |
| 77 | F4 | White Noise Burst | Noise |
| 79 | G4 | Laser Zap | Synth |
| 81 | A4 | Alarm | Synth |
| 83 | B4 | Glass Shatter | Foley |
| 84 | C5 | Door Slam | Foley |
| 86 | D5 | Thunder | Ambient |
| 88 | E5 | Rain | Ambient |
| 89 | F5 | Wind | Ambient |
| 91 | G5 | Explosion | Impact |
| 93 | A5 | Synth Hit | Synth |

**Sample Files Required (20 total):**
All files in `Content/Samples/FX/` as listed above.

---

## Complete File Inventory

### Total Files Created: 16 .zpatch presets

**808 Kits (4):**
1. 808_Kit.zpatch
2. 808_Kit_Punchy.zpatch
3. 808_Kit_Long.zpatch
4. 808_Kit_Filtered.zpatch

**Drum Kits (4):**
1. Drum_Kit.zpatch
2. Drum_Kit_Tight.zpatch
3. Drum_Kit_Roomy.zpatch
4. Drum_Kit_Compressed.zpatch

**Keys (4):**
1. Keys.zpatch
2. Keys_Bright.zpatch
3. Keys_Soft.zpatch
4. Keys_Staccato.zpatch

**FX (4):**
1. FX.zpatch
2. FX_Atmospheric.zpatch
3. FX_Percussive.zpatch
4. FX_Dark.zpatch

### Total Samples Needed: 81 WAV files

- **808 samples:** 15 files
- **Drum samples:** 25 files
- **Keys samples:** 21 files
- **FX samples:** 20 files

---

## TODO: Adding Real Samples

**Current Status:** All .zpatch files use placeholder file paths. The sampler will fail to load these presets until real samples are added.

### Steps to Add Real Samples:

1. **Create the sample directories** (already done):
   ```
   zenith-core/Content/Samples/808/
   zenith-core/Content/Samples/Drums/
   zenith-core/Content/Samples/Keys/
   zenith-core/Content/Samples/FX/
   ```

2. **Add your WAV files** following the naming conventions documented above:
   - Use consistent sample rates (44.1kHz or 48kHz recommended)
   - Normalize samples to avoid clipping
   - Mono or stereo both supported
   - 16-bit or 24-bit both supported

3. **Verify file paths match** the paths in the .zpatch files:
   - All paths are relative to the zenith-core directory
   - Example: `Content/Samples/808/808_kick.wav`

4. **Test each preset** in ZenithSampler to verify:
   - Samples load correctly
   - Key mappings are logical
   - Envelope settings feel appropriate
   - Filter settings sound good

5. **Adjust if needed:**
   - Modify .zpatch files to adjust parameters
   - Swap samples if needed
   - Create additional preset variants

---

## ZenithSampler Parameter Reference

Each .zpatch file contains these parameters:

### Envelope (ADSR)
- **attack:** Attack time (0.0 - 1.0+) - Time to reach peak
- **decay:** Decay time (0.0 - 1.0+) - Time to reach sustain level
- **sustain:** Sustain level (0.0 - 1.0) - Held level during note
- **release:** Release time (0.0 - 1.0+) - Time to fade out after note off

### Filter (Low-pass TPT)
- **filterCutoff:** Cutoff frequency (0.0 - 1.0) - 1.0 = wide open
- **filterResonance:** Resonance/Q (0.0 - 1.0) - Emphasis at cutoff

### Global
- **tune:** Global pitch shift (-1.0 to +1.0 = ±12 semitones)
- **gain:** Output gain (0.0 - 1.0) - Master volume

### Per-Sample Settings
- **rootNote:** MIDI note number where sample plays at original pitch
- **lowNote/highNote:** MIDI note range this sample responds to
- **lowVelocity/highVelocity:** Velocity layer range (0-127)
- **gain:** Per-sample volume multiplier

---

## Design Philosophy

### 808 Kits
- Classic TR-808 layout for hip-hop, trap, and electronic music
- Each sound mapped to a single key for easy programming
- Variants provide tonal flexibility (punchy/long/filtered)

### Drum Kits
- GM-compatible mapping for maximum DAW compatibility
- Suitable for rock, pop, electronic, and acoustic styles
- Variants cover different room/ambience characteristics

### Keys
- Minor third (3-semitone) multi-sampling minimizes pitch artifacts
- 21 samples cover full 88-key range with good tonal accuracy
- Variants provide articulation options (bright/soft/staccato)

### FX
- Organized by function (transitions, impacts, ambiences)
- Mapped to middle-upper range to avoid conflicts with drums
- Variants offer different time/filter characteristics

---

## Preset Categories & Tags (Recommended)

When integrating with preset management:

**808 Kits:**
- Category: "Drums", "Electronic"
- Tags: "808", "Drum Machine", "Hip-Hop", "Trap"

**Drum Kits:**
- Category: "Drums", "Acoustic"
- Tags: "Acoustic Drums", "GM", "Rock", "Pop"

**Keys:**
- Category: "Keys", "Piano"
- Tags: "Piano", "Multi-Sampled", "Acoustic"

**FX:**
- Category: "FX", "Sound Design"
- Tags: "Transitions", "Impacts", "Ambiences", "Foley"

---

## Future Enhancements

Potential additions:
- **Velocity layers:** Add soft/loud samples for dynamic response
- **Round-robin samples:** Multiple variations per note for realism
- **Additional banks:** Bass, synths, orchestral, ethnic percussion
- **Preset browser:** Organize presets by category/tags
- **Modulation routing:** Map macros to filter/envelope parameters

---

## License & Credits

TODO: Add licensing information for samples when acquired.

Sample banks designed for ZenithSampler (zenith-core project).
Created: 2025-11-18
