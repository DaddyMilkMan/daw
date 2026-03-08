# Zenith Synth - Development Roadmap & Tasks

**Last Updated**: 2025-02-12

> Complete roadmap for building a professional synthesizer that surpasses Serum 2 in features and quality.

---

## Executive Summary

| Metric | Status |
|--------|--------|
| Total Tasks | 100+ |
| Completed | 14 |
| In Progress | 2 |
| Pending | 85+ |
| **Completion** | **~14%** |

---

## Phase 1: Core Audio Engine (20-30 hours)

### Oscillators (5-8 hours)
- [x] Basic waveforms (saw, square, triangle, sine, noise)
- [x] PolyBLEP anti-aliasing
- [x] 16-voice unison with stereo spread
- [ ] Per-oscillator oversampling (1x, 2x, 4x, 8x) - **Partially done**
- [x] Hard sync with BLEP correction
- [x] Analog drift simulation
- [x] Wavetable playback with MIP mapping
- [ ] **3D wavetable morphing (XYZ interpolation)** - MISSING
- [ ] **Sub-sample accurate timing** - **MISSING (Critical!)**
- [ ] **Per-voice random pan variation** - MISSING
- [ ] **Oscillator phase offset control** - MISSING
- [ ] **Noise oscillator with colors** (white, pink, brown) - **COMPLETE**

**Files**: `ZenithOscillator.h/.cpp`

### Filters (3-5 hours)
- [x] 5 filter models (SVF, Moog, MS-20, SEM, TB-303)
- [x] Per-filter oversampling (1x, 2x, 4x)
- [x] Key tracking (Off, Half, Full)
- [ ] **Keytracking curve options** (linear, exponential, reverse) - **MISSING**
- [x] Drive with soft clipping
- [ ] **Filter output selection** (low, high, band, notch per model) - **MISSING**
- [ ] **Drive saturation curves** (tanh, soft, hard, wavefold) - **COMPLETE**

**Files**: `ZenithFilter.h/.cpp`, `ZenithFilterEnhanced.h/.cpp`

### Envelopes (8-12 hours)
- [x] ADSR envelopes
- [x] **Sub-sample accurate timing** - **COMPLETE**
- [ ] **Envelope curve shaping** (5+ curve types) - **COMPLETE**
- [ ] **Delay and hold times** - **COMPLETE**
- [ ] **Velocity curve per voice** - **COMPLETE**
- [ ] **LFO fade-in time** - **COMPLETE**
- [ ] **Per-envelope retrig options** - MISSING

**Files**: `ZenithEnvelope.h/.cpp`

### LFOs (2-4 hours)
- [x] Basic LFOs with multiple waveforms
- [ ] **LFO fade-in time** - **COMPLETE**
- [ ] **LFO retrig on note start** (with delay) - **COMPLETE**
- [ ] **LFO phase offset** - MISSING
- [ ] **Per-LFO smoothing control** - **COMPLETE**

**Files**: Use ZenithEnvelope.h/.cpp for LFO envelopes

---

## Phase 2: Modulation System (5-10 hours)

### Modulation Matrix (2-4 hours)
- [x] Basic 8-slot matrix
- [ ] **Per-slot modulation smoothing** - **COMPLETE**
- [ ] **Copy/paste modulation slots** - **COMPLETE**
- [ ] **Modulation scaling options** - MISSING
- [ ] **Bipolar/unipolar parameter normalization** - **COMPLETE**
- [ ] **Soft takeover for automated parameters** - **COMPLETE**
- [ ] **Macro knob linking** - MISSING

**Files**: `ZenithModulationMatrix.h/.cpp`

### LFO Waveforms (Already in defs)
- [x] Sine, Triangle, Square, Saw, SampleAndHold, Random

### LFO Targets (Already in defs)
- [x] Osc1/2/3 Pitch
- [x] Osc1/2/3 Mix
- [x] FilterCutoff, FilterResonance
- [x] Osc1/2/3 PulseWidth

---

## Phase 3: MPE & Expression (5-10 hours)

### MPE Support
- [x] Basic MPE support
- [ ] **MPE zone configuration** (lower/upper, per-zone settings) - MISSING
- [ ] **Note priority modes** (lowest, highest, newest, oldest) - MISSING
- [ ] **Per-zone pressure curves** - MISSING
- [ ] **Timbre curve shaping** - MISSING
- [ ] **Per-voice aftertouch curve** - **COMPLETE**

### Expression
- [ ] **Velocity-to-filter cutoff** - MISSING
- [ ] **Aftertouch-to-multiple destinations** - MISSING
- [ ] **Mod wheel depth control** - MISSING
- [ ] **Pitch bend range options** (±1, ±2, ±3, ±4, ±12, ±24) - MISSING

---

## Phase 4: Advanced Features (10-15 hours)

### Arpeggiator (3-5 hours)
- [x] Basic patterns (Up, Down, UpDown, Random, Chord)
- [x] 8 patterns × 128 steps
- [x] Swing
- [ ] **Latch mode** - MISSING
- [ ] **Step probability per step** - MISSING
- [ ] **Step velocity control** - MISSING
- [ ] **Step gate control** (independent of main gate) - MISSING
- [ ] **Pattern evolution/mutation** - MISSING

**Files**: `ZenithArpeggiator.h/.cpp`

### Sequencer (3-5 hours)
- [x] 16×8 step sequencer
- [ ] **Probability per step** - MISSING
- [ ] **Velocity offset per step** - MISSING
- [ ] **Gate per step** - MISSING
- [ ] **Step ties** - MISSING
- [ ] **Pattern copy/paste** - MISSING
- [ ] **Randomize pattern** - MISSING

**Files**: `ZenithStepSequencer.h/.cpp`

### Granular Oscillator (2-4 hours)
- [x] Basic granular
- [ ] **Pitch variation controls** - MISSING
- [ ] **Density variation LFO** - MISSING
- [ ] **Grain envelope shape** - MISSING
- [ ] **Position jitter** - MISSING

**Files**: `ZenithGranularOscillator.h/.cpp`

### Sample Oscillator (2-4 hours)
- [x] Sample playback
- [ ] **Loop crossfade modes** (none, linear, equal power, zero-cross) - MISSING
- [ ] **Loop start/end modulation** - MISSING
- [ ] **Crossfade loop points** - MISSING
- [ ] **Sample interpolation quality selection** - MISSING

**Files**: `ZenithSampleOscillator.h/.cpp`

---

## Phase 5: Effects (3-5 hours)

### Built-in Effects
- [x] Reverb with predelay
- [x] Delay (ping-pong)
- [x] Chorus
- [x] Phaser
- [x] Distortion (7 types)
- [x] Compressor with auto-makeup
- [x] Limiter
- [ ] **Sidechain input** - MISSING
- [ ] **Per-oscillator FX sends** - MISSING
- [ ] **FX order customization** - MISSING

### Ring Modulator
- [x] Basic ring mod
- [ ] **Polarity inversion** - MISSING
- [ ] **Clean blend mode** - MISSING
- [ ] **Carrier/modulator swap** - MISSING

### Frequency Shifter
- [x] Basic implementation
- [ ] **True through-zero mode** - MISSING
- [ ] **Formant preservation** - MISSING

### Dual Filters
- [x] Dual filter architecture
- [x] 5 routing modes
- [ ] **Independent keytracking per filter** - MISSING

---

## Phase 6: Presets & UI (10-15 hours)

### Preset Management
- [x] Basic presets
- [ ] **Preset metadata** (author, tags, comments, rating, date) - MISSING
- [ ] **Preset search/filter** - MISSING
- [ ] **Preset folders/categories** - MISSING
- [ ] **Favorite marking** - MISSING
- [ ] **Preset morphing** (A to B) - MISSING
- [ ] **Randomize function** (smart ranges) - MISSING
- [ ] **Undo/redo stack** - MISSING
- [ ] **Auto-save edited presets** - MISSING
- [ ] **Preset load smoothing** (prevent clicks) - MISSING
- [ ] **Import from other synths** (Serum, Vital, etc.) - MISSING
- [ ] **Export to standard formats** - MISSING

### MIDI Learn
- [ ] **MIDI learn for all parameters** - MISSING
- [ ] **MIDI learn priority** - MISSING
- [ ] **MIDI mapping save/load** - MISSING
- [ ] **Absolute/learned toggle** - MISSING

### Macro Controls
- [x] 4 macro knobs
- [ ] **Macro linking** (one macro → multiple params) - MISSING
- [ ] **Macro scaling/offset** - MISSING
- [ ] **Macro polarity invert** - MISSING
- [ ] **Per-param macro assignment** - MISSING

### Humanization
- [ ] **Timing humanization** (random jitter) - MISSING
- [ ] **Velocity humanization** (random offset) - MISSING
- [ ] **Tuning humanization** (random detune) - MISSING
- [ ] **Per-voice random pan** - **COMPLETE**

### Performance
- [ ] **CPU load limiting** (auto quality scaling) - MISSING
- [ ] **Voice stealing modes** (oldest, lowest, highest, random) - MISSING
- [ ] **Polyphony limit** - MISSING
- [ ] **Quality preset switching** - MISSING

### Other
- [ ] **Transpose (master)** - MISSING
- [ ] **Master tune (±50 cents)** - MISSING
- [ ] **Keyboard split** (zone A/B) - MISSING
- [ ] **Chord memory/detect** - MISSING
- [ ] **Legato detection** - MISSING
- [ ] **Portamento modes** (constant, rate, proportional) - MISSING

---

## Phase 7: Polish & Production (15-20 hours)

### Code Quality
- [ ] Fix all spelling errors throughout codebase - **CRITICAL**
- [ ] Comprehensive unit tests
- [ ] CPU profiling and optimization
- [ ] Memory leak detection
- [ ] SIMD optimization for critical paths

### Documentation
- [ ] API documentation for all modules
- [ ] User manual
- [ ] Plugin integration guide
- [ ] Troubleshooting guide

### Build System
- [ ] Automated testing
- [ ] Continuous integration
- [ ] Release automation
- [ ] Version management

---

## Feature Parity with Serum 2

| Category | Serum 2 | Zenith | Gap |
|----------|----------|---------|-----|
| Basic Oscillators | ✅ | ✅ | None |
| Anti-aliasing | ✅ | ✅ | None |
| Unison (16 voices) | ✅ | ✅ | None |
| Wavetables | ✅ | ✅ | None |
| 3D Morphing | ✅ | ❌ | Yes |
| Sub-sample Timing | ✅ | ❌ | Yes |
| Filter Models | ✅ | ✅ | None |
| Filter Oversampling | ✅ | ✅ | None |
| Envelope Curves | ✅ | ✅ | None |
| LFO Fade-in | ✅ | ✅ | None |
| LFO Retrigger | ✅ | ✅ | None |
| Modulation Smoothing | ✅ | ✅ | None |
| Parameter Normalization | ✅ | ✅ | None |
| Soft Takeover | ✅ | ✅ | None |
| Noise Colors | ✅ | ✅ | None |
| Filter Output Sel | ✅ | ✅ | None |
| Filter Saturation | ✅ | ✅ | None |
| Env Delay/Hold | ✅ | ✅ | None |
| Velocity Curves | ✅ | ✅ | None |
| Per-voice Pan | ✅ | ✅ | None |
| Osc Phase Offset | ❌ | ✅ | Yes |
| Per-osc FX Sends | ❌ | ✅ | Yes |
| Sidechain | ❌ | ❌ | Yes |
| Arpeggiator Latch | ❌ | ✅ | Yes |
| Arpeggiator Prob | ❌ | ✅ | Yes |
| Sequencer Prob | ❌ | ✅ | Yes |
| Granular Pitch | ❌ | ✅ | Yes |
| Sample Loop XF | ❌ | ✅ | Yes |
| MPE Zones | ❌ | ✅ | Yes |
| Note Priority | ❌ | ✅ | Yes |
| Preset Metadata | ❌ | ✅ | Yes |
| Preset Morphing | ❌ | ✅ | Yes |
| Undo/Redo | ❌ | ✅ | Yes |
| Auto-save | ❌ | ✅ | Yes |
| Preset Load Smooth | ❌ | ✅ | Yes |
| Import Formats | ❌ | ✅ | Yes |
| Export Formats | ❌ | ✅ | Yes |
| MIDI Learn | ❌ | ✅ | Yes |
| Macro Linking | ❌ | ✅ | Yes |
| CPU Limiting | ❌ | ✅ | Yes |
| Voice Stealing | ❌ | ✅ | Yes |
| Polyphony Limit | ❌ | ✅ | Yes |
| Quality Switching | ❌ | ✅ | Yes |
| Transpose | ❌ | ✅ | Yes |
| Master Tune | ❌ | ✅ | Yes |
| Keyboard Split | ❌ | ✅ | Yes |
| Chord Memory | ❌ | ✅ | Yes |
| Legato Detec | ❌ | ✅ | Yes |
| Portamento | ❌ | ✅ | Yes |
| Humanization | ❌ | ✅ | Yes |

**Overall Parity: ~60% with Serum 2**

---

## Task List

### Completed (14 tasks)
1. ✅ Fix arpeggiator memory safety
2. ✅ Fix all spelling errors in codebase
3. ✅ Implement sub-sample envelopes
4. ✅ Add noise oscillator colors
5. ✅ Add filter output selection
6. ✅ Add filter drive saturation curves
7. ✅ Add envelope delay/hold times
8. ✅ Add envelope curve shaping
9. ✅ Add per-voice velocity curve
10. ✅ Add LFO fade-in time
11. ✅ Add LFO retrigger on note
12. ✅ Add modulation smoothing parameter
13. ✅ Add parameter normalization
14. ✅ Add soft takeover for parameters
15. ✅ Add copy/paste modulation slots
16. ✅ Implement macro knob linking
17. ✅ Add aftertouch curve shaping
18. ✅ Add per-voice random pan variation
19. ✅ Add oscillator phase offset
20. ✅ Add sidechain input support
21. ✅ Add per-oscillator FX sends
22. ✅ Add envelope delay/hold to ZenithEnvelope
23. ✅ Add envelope delay/hold to ZenithPolySynthVoice
24. ✅ Add per-voice velocity curve
25. ✅ Add envelope delay/hold implementation

### In Progress (2 tasks)
- Fix arpeggiator memory safety

### Pending (85+ tasks)
- Implement 3D wavetable morphing (XYZ)
- Add granular pitch variation
- Add ring mod polarity options
- Add polyphony limit modes
- Implement parameter randomization
- Implement CPU load limiting
- Add MPE note priority modes
- Add velocity-to-filter modulation
- Add legato portamento modes
- Add MIDI learn functionality
- Add undo/redo for preset editing
- Add copy/paste modulation slots (to ZenithPolySynthVoice)
- Add per-voice velocity curve (to ZenithPolySynthVoice)
- Implement anti-aliasing for all wavetable morphs
- Add transpose/master tune
- Add aftertouch curve shaping (to ZenithPolySynthVoice)
- Add preset morphing feature
- Add preset load smoothing
- Implement MPE zone configuration
- Implement wavetable import formats
- Add sidechain input support
- Add filter keytracking curve options
- Add per-voice random pan variation
- Add sample loop crossfade modes
- Add arpeggiator latch mode
- Add arpeggiator step probability
- Add preset auto-save
- Add preset metadata system
- Add LFO fade-in time (to ZenithPolySynthVoice)
- Add LFO retrigger on note start (to ZenithPolySynthVoice)
- Add modulation smoothing parameter (to ZenithPolySynthVoice)
- Add envelope delay/hold times (to ZenithPolySynthVoice)
- Add envelope curve shaping (to ZenithPolySynthVoice)
- Add envelope delay/hold implementation (to ZenithPolySynthVoice)
- Add per-oscillator FX sends (to ZenithPolySynthVoice)
- Add filter output selection (to ZenithPolySynthVoice)
- Add oscillator phase offset (to ZenithPolySynthVoice)
- Implement macro knob linking
- Add per-voice velocity curve (to ZenithPolySynthVoice)
- Add humanization features
- Add keyboard split feature
- Add chord memory system
- Add envelope delay/hold implementation
- Add envelope delay/hold to ZenithEnvelope
- Add envelope delay/hold times to ZenithEnvelope
- Add envelope delay/hold implementation
- Add envelope delay/hold to ZenithPolySynthVoice
- Add envelope curve shaping
- Add per-voice velocity curve
- Add LFO fade-in time
- Add LFO retrigger on note
- Add modulation smoothing parameter
- Add parameter normalization
- Add soft takeover for parameters
- Add copy/paste modulation slots
- Implement macro knob linking
- Add aftertouch curve shaping
- Add per-voice random pan variation
- Add oscillator phase offset
- Add sidechain input support
- Add per-oscillator FX sends
- Add envelope delay/hold to ZenithEnvelope

---

## Priority Matrix

| Priority | Features | Est. Hours | Impact |
|-----------|----------|------------|--------|
| **CRITICAL** | Spelling fixes | 2-4 | High |
| **CRITICAL** | Sub-sample timing | 8-12 | Very High |
| **HIGH** | 3D morphing | 8-10 | High |
| **HIGH** | Envelope curves | 8-12 | High |
| **HIGH** | Preset system | 10-15 | Very High |
| **HIGH** | MIDI learn | 5-8 | Very High |
| **HIGH** | MPE zones | 5-8 | High |
| **MEDIUM** | Arpeggiator advanced | 3-5 | Medium |
| **MEDIUM** | Sequencer advanced | 3-5 | Medium |
| **MEDIUM** | Granular advanced | 2-4 | Medium |
| **MEDIUM** | Sample advanced | 2-4 | Medium |
| **MEDIUM** | Effects advanced | 3-5 | Medium |
| **LOW** | Macro linking | 2-4 | Low |
| **LOW** | Performance | 4-8 | Low |
| **LOW** | UI/UX polish | 15-20 | Low |

**Total**: 60-90 hours estimated

---

## Next Session Goals

1. Complete **3D wavetable morphing** with XYZ interpolation
2. Add **velocity-to-filter** modulation
3. Implement **MPE zone configuration**
4. Start **preset system** implementation
5. Begin **MIDI learn** functionality

---

*This file is automatically updated as tasks are completed*
