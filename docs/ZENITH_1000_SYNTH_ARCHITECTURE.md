# Zenith 1000+ Synth Engine Architecture

## Executive Summary

**Goal:** Create 1000+ unique, high-quality synthesizers that surpass Logic Pro and Ableton Live's stock instruments while maintaining lightweight, optimized performance.

**Solution:** Modular Synth Architecture - Hand-craft 30-50 ultra-optimized DSP modules and combine them into 1000+ unique synth definitions.

**Advantages:**
- ✅ Each synth feels unique (different module combinations)
- ✅ Highly optimized (modules shared across all synths)
- ✅ Maintainable codebase
- ✅ Easy to expand

---

## The 1000 Synth Categories

### Category 1: Classic Subtractive (200 synths)
Virtual analog, modeled after classic hardware
- Minimoog-style (2-3 osc, ladder filter)
- Prophet-5 style (2 osc, curtis filter)
- Juno-style (DCO, HPF, chorus)
- SH-101 style (1 osc, HPF, sub)
- MS-20 style (2 osc, semi-modular)
- ARP 2600 style
- OSCar style
- And 192 more variations...

### Category 2: Wavetable (150 synths)
Complex wavetable synthesis with morphing
- Serum-style spectral morph
- Wave-style wavesequencing
- Massive-style wavetable + modulation
- Blofeld-style
- Microwave-style
- And 144 more...

### Category 3: FM (150 synths)
Frequency modulation synthesis
- DX7-style 6-op FM
- TX81Z-style 4-op
- OPL-style FM (Yamaha FM chips)
- Modern FM (FM8, operators + feedback)
- Phase distortion (Casio CZ)
- And 145 more...

### Category 4: Additive (100 synths)
Additive synthesis with partial control
- Drawbar organ (B3 style)
- Additive resynthesis
- Harmonic sculpting
- And 96 more...

### Category 5: Granular (100 synths)
Granular synthesis engines
- Sample-based granular
- Live input granular
- Spectral granular
- And 97 more...

### Category 6: Physical Modeling (100 synths)
Physical modeling of real instruments
- Piano modeling
- String modeling
- Wind modeling
- Plucked instruments
- And 96 more...

### Category 7: Sample-Based (100 synths)
Sample playback with synthesis
- Rompler-style
- Sample + synthesis hybrid
- Drum synth (kick, snare, hi-hat, etc.)
- And 97 more...

### Category 8: Hybrid/FX (100 synths)
Experimental and effect synths
- Ring modulator
- Vocoder
- Multiband processing
- And 97 more...

---

## Core DSP Modules (30-50 modules)

### Oscillator Modules (12 types)
```cpp
// 1. Classic Analog Oscillator
- Sawtooth (bandlimited, saw./triangle mix)
- Square (PWM, bandlimited)
- Triangle (bandlimited)
- Sine (pure, FM capable)

// 2. Wavetable Oscillator
- 256-frame wavetables
- Morph between tables
- Interpolation (linear/cubic)

// 3. FM Operator
- Sine with feedback
- Multiple waveforms
- Linear vs exponential FM

// 4. Noise Generator
- White noise
- Pink noise
- Filtered noise

// 5. Sub Oscillator
- -1 octave square/sine
- Phase-synced to main osc

// 6. Additive Oscillator
- 64 partials
- Individual partial level/ratio
- Harmonic/inharmonic spectra

// 7. Granular Engine
- Grain size, density, position
- Pitch jitter, randomization

// 8. Sample Player
- One-shot, loop, forward/reverse
- Loop points, crossfade
- Pitch/amp envelopes

// 9. LFO (audio rate)
- 5 waveforms + S&H
- Poly LFO (multiple phases)
- Audio rate modulation

// 10. Unison Detune
- 2-16 voices
- Spread, detune amount
- Stereo width

// 11. Hard Sync Oscillator
- Slave syncs to master
- Sweepable sync point

// 12. Ring Modulator
- Audio rate multiplication
- Built-in carrier oscillator
```

### Filter Modules (10 types)
```cpp
// 1. Ladder Filter (Moog-style)
- 24dB/oct lowpass
- Resonance, drive
- Self-oscillation

// 2. State Variable Filter
- LP, HP, BP, notch
- 12/24dB slopes
- Resonance

// 3. Curtis Filter (Prophet-style)
- 2-pole lowpass
- Distortion circuit

// 4. SEM Filter (Oberheim-style)
- 12dB multimode
- HP, BP, LP, notch

// 5. Comb Filter
- Tunable feedback
- Flanging, phasing effects

// 6. Formant Filter
- 2-4 formants
- Vocal vowel sounds

// 7. MS-20 Filter
- 2 filters in series
- LP and HP separately

// 8. Diode Ladder
- Korg-style lowpass
- Character distortion

// 9. Slew Limiter
- Portamento effect
- Glide for all parameters

// 10. Multiband Filter
- 2-4 bands
- Individual band control
```

### Envelope Modules (6 types)
```cpp
// 1. ADSR
- Attack, decay, sustain, release
- Linear/exponential curves

// 2. AHDSR
- Added hold stage

// 3. Multi-Stage (8 stages)
- Custom envelopes
- Loop, sustain point

// 4. Flex Envelope
- Drawable envelope
- Time, level per node

// 5. Envelope Follower
- Input level → envelope
- FFT-based

// 6. Gate
- Triggered envelope
- Adjustable length
```

### LFO/Modulation Modules (8 types)
```cpp
// 1. Multi-LFO
- 5 simultaneous LFOs
- Different waveforms

// 2. Envelope Generator
- 4-stage ADSR
- Loopable

// 3. Random Generator
- Sample & hold
- Random walk
- Smoothed random

// 4. Step Sequencer
- 16-32 steps
- Per-step probability

// 5. Mod Matrix
- 8 slots, many sources
- Destinations: all params

// 6. Keytracking
- MIDI note → modulation
- Scaleable amount

// 7. Velocity
- Note velocity → modulation
- Curve, invert

// 8. Macro Controls
- 8 macro knobs
- Each controls multiple params
```

### Effect Modules (10 types)
```cpp
// 1. Distortion
- 7 algorithms: soft clip, hard clip, bitcrush, etc.
- Drive, tone, mix

// 2. Chorus
- Multi-voice (2-8)
- Rate, depth, feedback

// 3. Delay
- Tempo-synced
- Ping-pong, filter
- 3 taps max

// 4. Reverb
- Algorithmic plate
- Size, decay, predelay

// 5. Phaser
- 4-12 stages
- Rate, depth, feedback

// 6. Flanger
- Through-zero flanging
- Feedback, mix

// 7. EQ
- 3-band parametric
- Low/high cut

// 8. Compressor
- Threshold, ratio, attack, release
- Sidechain available

// 9. Limiter
- Ceiling, lookahead

// 10. Bitcrusher
- Bit depth, sample rate
- Aliasing control
```

### Utility Modules (4 types)
```cpp
// 1. Mixer
- Mix multiple sources
- Per-source level, pan

// 2. VCA
- Voltage controlled amp
- Linear/expponential response

// 3. Signal Shaper
- Waveshaping transfer function
- 5 curves

// 4. Noise Gate
- Threshold, hold, release
```

---

## Synth Definition Format

Each synth is defined by a JSON file specifying:
- Which modules to use
- How to connect them
- Default parameter values
- Modulation routing

Example: "Minimoog Clone"
```json
{
  "name": "Mini-65",
  "category": "Classic Subtractive",
  "architecture": {
    "oscillators": [
      {"type": "ClassicAnalog", "wave": "saw", "detune": 0, "mix": 1.0},
      {"type": "ClassicAnalog", "wave": "square", "detune": 3, "mix": 0.8}
    ],
    "filter": {
      "type": "Ladder",
      "cutoff": 2000,
      "resonance": 0.7,
      "drive": 0.3
    },
    "envelopes": [
      {"type": "ADSR", "target": "amp", "attack": 0.01, "decay": 0.3, "sustain": 0.5, "release": 0.2},
      {"type": "ADSR", "target": "filter", "attack": 0.05, "decay": 0.2, "sustain": 0.0, "release": 0.5}
    ],
    "lfo": {"type": "MultiLFO", "rate": 5.0, "targets": ["filter_cutoff", "osc_pitch"]},
    "effects": [
      {"type": "Distortion", "drive": 0.2, "mix": 0.5},
      {"type": "Delay", "time": 0.3, "feedback": 0.4}
    ]
  },
  "polyphony": 8,
  "voice_mode": "poly"
}
```

---

## Implementation Plan

### Phase 1: Core Module Development (Weeks 1-8)
**Deliverable:** 30-50 ultra-optimized DSP modules

Week 1-2: Oscillator Modules
- [ ] ClassicAnalogOsc (bandlimited, saw/square/triangle/sine)
- [ ] WavetableOsc (256 frames, morph)
- [ ] FMOperator (sine + feedback)
- [ ] NoiseGenerator (white, pink)
- [ ] SubOsc (-1 octave)
- [ ] AdditiveOsc (64 partials)
- [ ] GranularEngine
- [ ] SamplePlayer
- [ ] UnisonDetune
- [ ] HardSync
- [ ] RingMod
- [ ] LFO (audio rate)

Week 3-4: Filter Modules
- [ ] LadderFilter (Moog, 24dB)
- [ ] StateVariableFilter
- [ ] CurtisFilter
- [ ] SEMFilter
- [ ] CombFilter
- [ ] FormantFilter
- [ ] MS20Filter
- [ ] DiodeLadder
- [ ] SlewLimiter
- [ ] MultibandFilter

Week 5-6: Envelope & Modulation
- [ ] ADSREnvelope
- [ ] AHDSREnvelope
- [ ] MultiStageEnvelope
- [ ] FlexEnvelope
- [ ] EnvelopeFollower
- [ ] GateEnvelope
- [ ] MultiLFO
- [ ] RandomGenerator
- [ ] StepSequencer
- [ ] ModMatrix

Week 7-8: Effects & Utilities
- [ ] Distortion (7 algorithms)
- [ ] Chorus
- [ ] Delay
- [ ] Reverb
- [ ] Phaser
- [ ] Flanger
- [ ] EQ
- [ ] Compressor
- [ ] Limiter
- [ ] Bitcrusher
- [ ] Mixer
- [ ] VCA
- [ ] SignalShaper
- [ ] NoiseGate

### Phase 2: Synth Engine Framework (Weeks 9-12)
**Deliverable:** Synth definition parser and engine

Week 9:
- [ ] SynthDefinitionParser (JSON → module config)
- [ ] ModularSynthEngine (combines modules)
- [ ] Voice management
- [ ] Polyphony handling

Week 10:
- [ ] Modulation routing system
- [ ] Parameter management
- [ ] Preset save/load
- [ ] State serialization

Week 11:
- [ ] UI generation system
- [ ] Per-synth custom UIs
- [ ] Parameter automation
- [ ] MIDI learn

Week 12:
- [ ] Optimization (SIMD, vectorization)
- [ ] CPU profiling
- [ ] Memory optimization
- [ ] RT-safety verification

### Phase 3: Synth Definitions (Weeks 13-30)
**Deliverable:** 1000+ hand-crafted synth definitions

**Strategy:** Design 50-100 synths per week, organized by category

Week 13-15: Classic Subtractive (200 synths)
Week 16-18: Wavetable (150 synths)
Week 19-21: FM (150 synths)
Week 22-23: Additive (100 synths)
Week 24-25: Granular (100 synths)
Week 26-27: Physical Modeling (100 synths)
Week 28-29: Sample-Based (100 synths)
Week 30: Hybrid/FX (100 synths)

### Phase 4: Preset Library (Weeks 31-34)
**Deliverable:** 10,000+ presets across all synths

- Average 10 presets per synth
- Hand-crafted by sound designers
- Categorized by genre, type, mood
- Searchable, taggable

### Phase 5: Polish & Launch (Weeks 35-40)
**Deliverable:** Production-ready synth ecosystem

- Documentation
- Tutorials
- Demo videos
- Beta testing
- Performance optimization
- Bug fixes

---

## Optimization Strategy

### 1. SIMD Vectorization
- All oscillators use SSE/AVX
- Process 4-8 samples simultaneously
- 4-8x speedup on modern CPUs

### 2. Lookup Tables
- Sine waves (4096 entries)
- Wavetables (cached)
- Filter coefficients (pre-computed)

### 3. Object Pooling
- Voice objects reused
- No runtime allocations
- RT-safe guaranteed

### 4. Branchless DSP
- Where possible
- Uses conditional moves (CMOV)
- Reduces pipeline stalls

### 5. Multi-Threading
- Background voice stealing
- Parameter smoothing on UI thread
- Audio thread: pure DSP

### 6. Compiler Optimizations
- `-O3 -march=native -ffast-math`
- Profile-guided optimization (PGO)
- Link-time optimization (LTO)

### 7. Memory Layout
- Structure-of-arrays (SoA)
- Cache-friendly access
- Aligned allocations (64-byte)

**Expected CPU:**
- Poly synth (8 voices): < 3% CPU
- Wavetable (16 voices): < 5% CPU
- FM (6 operators): < 4% CPU
- Granular (8 grains): < 6% CPU

---

## Comparison to Competitors

| Metric | Zenith | Logic Pro | Ableton Live |
|--------|--------|-----------|--------------|
| **Number of synths** | 1000+ | ~30 | ~20 |
| **Synth types** | 8 categories | 5 types | 4 types |
| **Wavetables** | 5000+ | 100+ | 200+ |
| **CPU efficiency** | SIMD + pooling | Moderate | Moderate |
| **Modulation** | Unlimited | Limited | Limited |
| **Preset count** | 10,000+ | ~3000 | ~2000 |
| **AI integration** | ✅ Native | ❌ | ❌ |

---

## File Structure

```
apps/desktop/Source/
  synth_engine/
    core/
      SynthEngine.h/cpp
      Voice.h/cpp
      VoiceManager.h/cpp
    modules/
      oscillators/
        ClassicAnalogOsc.h/cpp
        WavetableOsc.h/cpp
        FMOperator.h/cpp
        ...
      filters/
        LadderFilter.h/cpp
        StateVariableFilter.h/cpp
        ...
      envelopes/
        ADSREnvelope.h/cpp
        ...
      modulation/
        MultiLFO.h/cpp
        ModMatrix.h/cpp
        ...
      effects/
        Distortion.h/cpp
        Chorus.h/cpp
        ...
    definitions/
      ClassicSubtractive/
        Mini65.json
        Prophet5.json
        ...
      Wavetable/
        SerumStyle.json
        ...
    presets/
      Mini65/
        Bass001.preset
        Lead002.preset
        ...
Content/
  Synths/
    Definitions/ (1000+ .json files)
    Presets/ (10,000+ .preset files)
    Wavetables/ (5000+ .wav files)
    Samples/ (ROM content)
```

---

## Next Steps

1. **Review this architecture** - Approve the modular approach
2. **Begin Phase 1** - Start implementing core DSP modules
3. **Hire sound designers** - For synth definitions and presets
4. **Set up CI/CD** - Automated testing of all 1000+ synths
5. **Create development roadmap** - Detailed task breakdown

---

**Total Development Time:** ~40 weeks for 1000+ production-ready synths

**Cost:** Similar to developing 5-10 high-end VST plugins, but with 100x the value.

**ROI:** Zenith becomes the most comprehensive synth ecosystem in existence, dwarfing Logic Pro and Ableton Live.
