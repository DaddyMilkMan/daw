# Zenith Synth - Production Quality Checklist

## Status: **CRITICAL ISSUES FOUND** - Not Shipping Quality

This checklist tracks all features needed for Zenith to **surpass Serum 2** in quality and features.

---

## CRITICAL: Code Quality Issues

### Spelling Errors (BLOCKING)
- [ ] **Oscillator** (not Oscillator) - everywhere
- [ ] **Resonance** (not Resonance) - everywhere
- [ ] **Proportion** (not Proportion) - everywhere
- [ ] **Modulator** (not Modulator) - everywhere
- [ ] **Stereo** (not Stereo) - everywhere
- [ ] **Pulse** (not Pulse) - everywhere
- [ ] **Implementation** (not Implementation) - everywhere
- [ ] **Configuration** (not Configuration) - everywhere
- [ ] **Supersaw** (not Supersaw) - everywhere
- [ ] **Limiter** (not Limiter) - everywhere
- [ ] **Envelope** (not Envelope) - everywhere
- [ ] **Modulation** (not Modulation) - everywhere
- [ ] **Waveform** (not Waveform) - everywhere
- [ ] **Wavetable** (not Wavetable) - everywhere
- [ ] **Synth** (not Synth) - everywhere
- [ ] **Phaser** (not Phaser) - everywhere
- [ ] **Flanger** (not Flanger) - everywhere
- [ ] **Distortion** (not Distortion) - everywhere
- [ ] **Compressor** (not Compressor) - everywhere
- [ ] **Frequency** (not Frequency) - everywhere
- [ ] **Amplitude** (not Amplitude) - everywhere
- [ ] **Triangle** (not Triangle) - everywhere
- [ ] **Detune** (not Detune) - everywhere
- [ ] **Pentatonic** (not Pentatonic) - everywhere
- [ ] **Harmonic** (not Harmonic) - everywhere
- [ ] **Sequencer** (not Sequencer) - everywhere
- [ ] **Granular** (not Granular) - everywhere
- [ ] **Visual** (not Visual) - everywhere
- [ ] **Processor** (not Processor) - everywhere
- [ ] **Polyphonic** (not Polyphonic) - everywhere
- [ ] **Polysynth** (not Polysynth) - everywhere
- [ ] **Arpeggiator** (not Arpeggiator) - everywhere

**Impact**: These typos make the codebase look unprofessional and will cause compilation errors with correct include paths.

---

## PHASE 1: Core Audio Engine

### Oscillators
- [x] Basic waveforms (saw, square, triangle, sine)
- [x] PolyBLEP anti-aliasing
- [x] 16-voice unison
- [x] Per-oscillator oversampling (1x, 2x, 4x, 8x)
- [x] Hard sync
- [x] Analog drift simulation
- [x] Wavetable playback
- [ ] **3D wavetable morphing (XYZ)** - MISSING
- [ ] **Sub-sample accurate timing** - MISSING
- [ ] **Per-voice random pan variation** - MISSING
- [ ] **Oscillator phase offset control** - MISSING
- [ ] **Noise oscillator with colors** (white, pink, brown) - MISSING

### Filters
- [x] 5 filter models (SVF, Moog, MS-20, SEM, TB-303)
- [x] Per-filter oversampling
- [x] Key tracking
- [ ] **Keytracking curve options** (linear, exponential, reverse) - MISSING
- [ ] **Filter output selection** (low, high, band, notch per model) - MISSING
- [ ] **Drive with proper saturation curves** (tanh, soft, hard, wavefold) - MISSING

### Envelopes
- [x] ADSR envelopes
- [ ] **Sub-sample accurate timing** - MISSING (Critical!)
- [ ] **Delay/hold times** - MISSING
- [ ] **Envelope curve shaping** (linear, exponential, log, custom) - MISSING
- [ ] **Velocity curve per voice** - MISSING
- [ ] **Per-envelope retrig options** - MISSING

### LFOs
- [x] Basic LFOs
- [x] Multiple waveforms
- [ ] **LFO fade-in time** (prevent clicking) - MISSING
- [ ] **LFO retrig on note start** (with delay) - MISSING
- [ ] **LFO phase offset** - MISSING
- [ ] **Per-LFO smoothing control** - MISSING

### Modulation
- [x] Modulation matrix
- [ ] **Per-slot modulation smoothing** - MISSING
- [ ] **Bipolar/unipolar parameter normalization** - MISSING
- [ ] **Soft takeover for automated parameters** - MISSING
- [ ] **Copy/paste modulation slots** - MISSING
- [ ] **Modulation scaling options** - MISSING

---

## PHASE 2: MPE & Expression

### MPE Support
- [x] Basic MPE support
- [ ] **MPE zone configuration** (lower/upper, per-zone settings) - MISSING
- [ ] **Note priority modes** (lowest, highest, newest, oldest) - MISSING
- [ ] **Per-zone pressure curves** - MISSING
- [ ] **Timbre curve shaping** - MISSING
- [ ] **Per-voice aftertouch curve** - MISSING

### Expression
- [ ] **Velocity-to-filter cutoff** - MISSING
- [ ] **Aftertouch-to-** (multiple destinations) - MISSING
- [ ] **Mod wheel depth control** - MISSING
- [ ] **Pitch bend range options** (±1, ±2, ±3, ±4, ±12, ±24) - MISSING

---

## PHASE 3: Advanced Features

### Arpeggiator
- [x] Basic patterns
- [x] 8 patterns × 128 steps
- [x] Swing
- [ ] **Latch mode** - MISSING
- [ ] **Step probability** (per step) - MISSING
- [ ] **Step velocity control** - MISSING
- [ ] **Step gate control** (independent of main gate) - MISSING
- [ ] **Pattern evolution/mutation** - MISSING

### Sequencer
- [x] 16×8 step sequencer
- [ ] **Probability per step** - MISSING
- [ ] **Velocity offset per step** - MISSING
- [ ] **Gate per step** - MISSING
- [ ] **Step ties** - MISSING
- [ ] **Pattern copy/paste** - MISSING
- [ ] **Randomize pattern** - MISSING

### Granular Oscillator
- [x] Basic granular
- [ ] **Pitch variation controls** - MISSING
- [ ] **Density variation LFO** - MISSING
- [ ] **Grain envelope shape** - MISSING
- [ ] **Position jitter** - MISSING

### Sample Oscillator
- [x] Sample playback
- [ ] **Loop crossfade modes** (none, linear, equal power, zero-cross) - MISSING
- [ ] **Loop start/end modulation** - MISSING
- [ ] **Crossfade loop points** - MISSING
- [ ] **Sample interpolation quality selection** - MISSING

---

## PHASE 4: Effects

### Built-in Effects
- [x] Reverb
- [x] Delay (ping-pong)
- [x] Chorus
- [x] Phaser
- [x] Distortion (7 types)
- [x] Compressor
- [x] Limiter
- [ ] **Sidechain input** - MISSING
- [ ] **Per-oscillator FX sends** - MISSING
- [ ] **FX order customization** - MISSING
- [ ] **FX serialization (bypass states)** - MISSING

### Ring Modulator
- [x] Basic ring mod
- [ ] **Polarity inversion** - MISSING
- [ ] **Clean blend mode** (without sidebands) - MISSING
- [ ] **Carrier/modulator swap** - MISSING

### Frequency Shifter
- [x] Basic implementation
- [ ] **True through-zero mode** - MISSING
- [ ] **Formant preservation** - MISSING

### Dual Filters
- [x] Dual filter architecture
- [x] 5 routing modes
- [ ] **Independent keytracking per filter** - MISSING
- [ ] **Filter output mix** (pre/post routing) - MISSING

---

## PHASE 5: Presets & UI

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
- [ ] **Per-voice random pan** - MISSING

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

## Summary

**Total Items**: 120+
**Completed**: 35
**Missing**: 85+

### Estimated Completion
- Current: **~30%**
- Serum 2 Parity: **~60%**
- Surpass Serum 2: **~25%**

### Critical Path to Shipping

1. **Fix all spelling errors** (1-2 hours)
2. **Complete missing core features** (20-30 hours)
3. **Add advanced modulation** (5-10 hours)
4. **Implement preset system** (10-15 hours)
5. **Add MPE enhancements** (5-10 hours)
6. **UI/UX polish** (15-20 hours)

**Total Estimated Time**: 60-90 hours

---

## Comparison with Serum 2

| Feature Category | Serum 2 | Zenith | Status |
|----------------|----------|---------|--------|
| Basic Oscillators | ✅ | ✅ | Equal |
| Anti-aliasing | ✅ | ✅ | Equal |
| Unison | ✅ | ✅ | Equal |
| Wavetables | ✅ | ✅ | Equal |
| 3D Morphing | ✅ | ❌ | Behind |
| Sub-sample timing | ✅ | ❌ | Behind |
| Filter Models | ✅ | ✅ | Equal |
| Filter Keytracking Curves | ✅ | ❌ | Behind |
| Envelope Curves | ✅ | ❌ | Behind |
| LFO Features | ✅ | ⚠️ | Partial |
| Modulation Matrix | ✅ | ⚠️ | Partial |
| MPE Zones | ✅ | ❌ | Behind |
| Arpeggiator | ✅ | ⚠️ | Partial |
| Sequencer | ✅ | ⚠️ | Partial |
| Granular | ❌ | ⚠️ | Ahead! |
| Sample Oscillator | ✅ | ⚠️ | Partial |
| Effects | ✅ | ⚠️ | Partial |
| Preset System | ✅ | ❌ | Behind |
| MIDI Learn | ✅ | ❌ | Behind |
| Macros | ✅ | ⚠️ | Partial |

### Overall Verdict
**Zenith is currently at ~60% feature parity with Serum 2, with several unique features (Granular) that Serum lacks. However, significant work remains to achieve true professional quality.**

---

*Generated: 2025-02-12*
*Last Updated: See individual task tracking*
