# Zenith Preset Library

Auto-generated comprehensive preset library for **ZenithPolySynth** and **ZenithSampler**.

## Overview

- **Total Presets:** 2000
- **ZenithPolySynth:** 1000 presets
- **ZenithSampler:** 1000 presets

All presets are professionally categorized and tagged for easy filtering through the CommandAPI and AI clients.

## ZenithPolySynth Presets (1000 Total)

The ZenithPolySynth library covers all essential production categories:

### Categories

| Category | Count | Description |
|----------|-------|-------------|
| **Bass** | 217 | 808s, subs, Reese basses, FM donks, acid basses |
| **Lead** | 174 | Festival supersaws, mono screamers, vocal leads, glide leads |
| **Pluck** | 145 | Future bass plucks, muted plucks, bell plucks, stabs |
| **Pad** | 145 | Warm analog pads, airy digital pads, evolving drones, atmospheres |
| **Arp** | 116 | Gate sequences, trance arps, trap triplet arps |
| **FX** | 250 | Risers, downlifters, impacts, sweeps, zaps |
| **Keys** | 250 | Electric pianos, bells, marimbas, vibes, music boxes |

### Example Presets

**Bass:**
- Clean Sub 808 (trap, 808, sub, bass)
- Reese Growl (bass, reese, dnb, dubstep)
- Acid Bass (bass, acid, techno, 303)
- Wobble Bass (bass, wobble, dubstep, edm)

**Lead:**
- Festival Supersaw (lead, edm, supersaw, festival)
- Mono Screamer (lead, mono, aggressive, edm)
- Vocal Lead (lead, vocal, singing, expressive)
- Trance Lead (lead, trance, uplifting, euphoric)

**Pluck:**
- Future Bass Pluck (pluck, future-bass, chords, bright)
- Bell Pluck (pluck, bell, resonant, melodic)
- House Pluck (pluck, house, dance, classic)

**Pad:**
- Warm Analog Pad (pad, warm, analog, lush)
- Evolving Drone (pad, drone, evolving, ambient)
- Sweeping Pad (pad, sweep, movement, dynamic)

**Arp:**
- Trance Arp (arp, trance, uplifting, melodic)
- Trap Triplet Arp (arp, trap, triplet, staccato)
- Gate Chord Seq (arp, gate, chords, sequence)

**FX:**
- Riser Noise (fx, riser, build, tension)
- Impact Boom (fx, impact, hit, boom)
- Sweep Whoosh (fx, sweep, whoosh, transition)

**Keys:**
- Electric Piano (keys, electric-piano, warm, vintage)
- Bell Keys (keys, bell, bright, melodic)
- Marimba (keys, marimba, wood, percussive)

## ZenithSampler Presets (1000 Total)

The ZenithSampler library provides versatile parameter presets for sample-based instruments.

### Categories

| Category | Count | Description |
|----------|-------|-------------|
| **808** | 250 | Classic 808 kits, sub 808s, punchy 808s, distorted 808s |
| **Drums** | 250 | Trap claps, snares, hi-hats, kicks, rim shots |
| **Keys** | 250 | LoFi pianos, electric keys, mallets, bells, toy pianos |
| **FX** | 250 | Impact hits, risers, vox shots, whooshes, textures |

### Example Presets

**808:**
- Classic 808 Kit (808, drums, trap, hip-hop)
- Sub 808 (808, sub, bass, trap)
- Punchy 808 (808, punchy, trap, modern)
- Distorted 808 (808, distorted, aggressive, heavy)

**Drums:**
- Trap Clap (drums, clap, trap, snare)
- Tight Snare (drums, snare, tight, punchy)
- Hi-Hat (drums, hi-hat, crisp, percussion)

**Keys:**
- LoFi Piano (keys, piano, lofi, dusty)
- Electric Keys (keys, electric, vintage, warm)
- Mallet (keys, mallet, plucky, percussive)
- Bell Tone (keys, bell, bright, resonant)

**FX:**
- Impact Hit (fx, impact, hit, cinematic)
- Riser (fx, riser, build, tension)
- Vox Shot (fx, vox, vocal, shot)
- Whoosh (fx, whoosh, transition, sweep)

## File Structure

```
Content/Presets/
├── preset_index.json              # Master index with category counts
├── ZenithPolySynth/
│   ├── master_bank.json           # All 414 presets
│   ├── bass_bank.json             # 90 bass presets
│   ├── lead_bank.json             # 72 lead presets
│   ├── pluck_bank.json            # 60 pluck presets
│   ├── pad_bank.json              # 60 pad presets
│   ├── arp_bank.json              # 48 arp presets
│   ├── fx_bank.json               # 48 FX presets
│   └── keys_bank.json             # 36 keys presets
└── ZenithSampler/
    ├── master_bank.json           # All 180 presets
    ├── 808_bank.json              # 45 808 presets
    ├── drums_bank.json            # 45 drum presets
    ├── keys_bank.json             # 45 keys presets
    └── fx_bank.json               # 45 FX presets
```

## Preset Format

### ZenithPolySynth Preset

```json
{
  "id": "clean_sub_808",
  "name": "Clean Sub 808",
  "category": "Bass",
  "tags": ["trap", "808", "sub", "bass"],
  "description": "Pure sine sub bass for 808-style patterns",
  "parameters": {
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

### ZenithSampler Preset

```json
{
  "id": "classic_808_kit",
  "name": "Classic 808 Kit",
  "category": "808",
  "tags": ["808", "drums", "trap", "hip-hop"],
  "description": "TR-808 drum machine sounds",
  "parameters": {
    "attack": 0.001,
    "decay": 0.3,
    "sustain": 0.0,
    "release": 0.2,
    "filter_cutoff": 0.8,
    "filter_resonance": 0.2,
    "tune": 0.0,
    "gain": 0.9,
    "character": 0.4
  }
}
```

## Tags Reference

Presets are tagged with genre and sound descriptors for easy filtering:

**Genre Tags:**
- trap, hip-hop, edm, house, techno, dnb, dubstep, trance, future-bass, lofi, ambient

**Sound Character Tags:**
- sub, bass, lead, pluck, pad, arp, fx, keys, drums, 808
- warm, bright, dark, soft, aggressive, clean, dirty, punchy, smooth
- analog, digital, vintage, modern, retro
- sine, saw, square, resonant

**Style Tags:**
- supersaw, reese, acid, wobble, glide, vocal, sync
- percussive, melodic, atmospheric, evolving, rhythmic
- riser, impact, whoosh, sweep, transition

## Using the Presets

### Loading in Code

```cpp
// Load a preset bank
auto bankFile = File("Content/Presets/ZenithPolySynth/bass_bank.json");
auto bankData = JSON::parse(bankFile);

// Access presets
auto presets = bankData["presets"];
for (auto& preset : presets)
{
    String id = preset["id"];
    String name = preset["name"];
    String category = preset["category"];
    auto params = preset["parameters"];
    // Apply to instrument...
}
```

### Filtering by Tag

```cpp
// Find all trap 808 basses
std::vector<Preset> trap808s;
for (auto& preset : presets)
{
    auto tags = preset["tags"];
    if (tags.contains("trap") && tags.contains("808"))
        trap808s.push_back(preset);
}
```

### Quick Index Lookup

```cpp
// Get category counts without loading all presets
auto index = JSON::parse(File("Content/Presets/preset_index.json"));
int totalBass = index["instruments"]["zenith_poly_synth"]["categories"]["Bass"];
// Returns: 90
```

## Preset Variations

Each preset category contains:
1. **Base archetypes** - Hand-crafted master presets (89 total)
2. **Variations** - Algorithmically generated variations with 5-10% parameter variation

This approach ensures:
- Sonic diversity within each category
- Consistent character and identity
- Professional, usable presets
- No extreme or broken parameter values

## Parameter Ranges

### ZenithPolySynth

| Parameter | Range | Unit |
|-----------|-------|------|
| osc_type | 0.0 - 2.0 | (0=Sine, 1=Saw, 2=Square) |
| filter_cutoff | 0.0 - 1.0 | % |
| filter_resonance | 0.0 - 1.0 | % |
| attack | 0.0 - 1.0 | seconds |
| decay | 0.0 - 1.0 | seconds |
| sustain | 0.0 - 1.0 | % |
| release | 0.0 - 1.0 | seconds |

### ZenithSampler

| Parameter | Range | Unit |
|-----------|-------|------|
| attack | 0.001 - 5.0 | seconds |
| decay | 0.001 - 5.0 | seconds |
| sustain | 0.0 - 1.0 | % |
| release | 0.001 - 10.0 | seconds |
| filter_cutoff | 0.0 - 1.0 | % |
| filter_resonance | 0.0 - 1.0 | % |
| tune | -12.0 - 12.0 | semitones |
| gain | 0.0 - 2.0 | multiplier |
| character | 0.0 - 1.0 | % |

## Validation

All presets have been validated to ensure:
- ✓ All parameter IDs match known parameters
- ✓ All values are within valid ranges
- ✓ All categories are valid
- ✓ All presets have required metadata
- ✓ Total preset count >= 500

Run validation: `python3 tools/preset-generator/validate_presets.py`

## Regenerating Presets

To regenerate or modify the preset library:

1. Edit archetypes in `tools/preset-generator/archetypes.json`
2. Run generator: `python3 tools/preset-generator/generate_presets.py`
3. Validate: `python3 tools/preset-generator/validate_presets.py`

## License

Part of the Zenith DAW project. See main repository LICENSE.
