# Zenith Auto-Tune: Complete Feature Checklist

**Goal:** Beat Auto-Tune Pro, Waves Tune Real-Time, and Melodyne in every category.

---

## 🔍 COMPETITIVE ANALYSIS

### Auto-Tune Pro ($399)
- Auto Mode (real-time)
- Graph Mode (manual editing)
- Classic vs Auto-Tune 5 algorithm
- Formant correction
- Throat modeling
- Humanize
- Natural vibrato
- Note transition controls
- MIDI input
- Low latency mode

### Waves Tune Real-Time ($79)
- Ultra-low latency (0-4ms)
- Programmable correction amount
- Vibrato shape editing
- Studio-quality sound
- Live performance optimized

### Melodyne ($99-699)
- Polyphonic pitch correction
- Note-based editing
- Formant editing per note
- Amplitude/timing editing
- Direct Note Access
- ARA integration

### Logic Pro Flex Pitch ($199 with Logic)
- Pitch drift editing
- Formant shift
- Gain editing per note
- Vibrato depth/rate
- Note splitting/merging

---

## ✅ ZENITH AUTO-TUNE MASTER CHECKLIST

### PHASE 1: CORE DSP (FOUNDATION)

#### Pitch Detection
- [x] YIN algorithm implementation
- [ ] McLeod Pitch Method (MPM) - alternative, better for certain voices
- [ ] Multi-detection voting (YIN + MPM + Comb filter)
- [ ] Confidence scoring per algorithm
- [ ] Noise-robust detection
- [ ] Polyphonic detection (for background vocals)

#### Pitch Correction
- [x] Basic semitone snapping
- [x] Scale-aware correction
- [ ] Microtonal correction (cents precision)
- [ ] Pitch drift correction (smooth out wavering)
- [ ] Adaptive correction strength (more correction on off notes)

#### Pitch Shifting
- [ ] **Replace with Rubber Band or SoundStretch library**
- [ ] Phase vocoder with phase locking
- [ ] Formant-aware shifting
- [ ] Multi-resolution processing
- [ ] Anti-aliasing filters

### PHASE 2: FORMANT & TIMBRE

#### Formant Preservation
- [ ] LPC (Linear Predictive Coding) analysis
- [ ] Formant extraction (F1, F2, F3 frequencies)
- [ ] Formant shifting independent of pitch
- [ ] Formant envelope following
- [ ] Gender shifting (formant shift without pitch)

#### Throat Modeling
- [ ] Physical modeling of vocal tract
- [ ] Throat length parameter
- [ ] Throat width parameter
- [ ] Breathiness control

### PHASE 3: VIBRATO & EXPRESSION

#### Vibrato Detection
- [ ] Automatic vibrato detection
- [ ] Vibrato rate measurement (Hz)
- [ ] Vibrato depth measurement (cents)
- [ ] Vibrato shape analysis (sinusoidal vs irregular)

#### Vibrato Control
- [ ] Preserve natural vibrato
- [ ] Flatten vibrato (remove entirely)
- [ ] Enhance vibrato (increase depth)
- [ ] Synthesize vibrato (add where none exists)
- [ ] Vibrato rate adjustment
- [ ] Vibrato depth adjustment
- [ ] Attack time (delay before vibrato starts)

### PHASE 4: TRANSITIONS & TIMING

#### Note Transitions
- [ ] Note transition speed (global)
- [ ] Different speeds for up/down transitions
- [ ] Legato mode (smooth between notes)
- [ ] Staccato mode (hard cuts)
- [ ] Portamento/glide between notes

#### Attack/Release
- [ ] Note attack time
- [ ] Note release time
- [ ] Correction onset delay
- [ ] Smooth correction curves

### PHASE 5: SCALE & MUSICALITY

#### Scales & Keys
- [x] 12 built-in scales
- [ ] Custom scale editor (user-defined)
- [ ] Scale detection from audio
- [ ] Chord-aware correction
- [ ] Modulation detection

#### Note Handling
- [ ] Note exclusion (bypass certain notes)
- [ ] Note emphasis (more correction on certain notes)
- [ ] Octave range limiting
- [ ] Frequency range limiting

### PHASE 6: MANUAL EDITING (GRAPH MODE)

#### Note Display
- [ ] Pitch curve visualization
- [ ] Detected notes as blocks
- [ ] Confidence coloring
- [ ] Formant display
- [ ] Vibrato visualization

#### Note Editing
- [ ] Drag notes to new pitch
- [ ] Drag note boundaries (timing)
- [ ] Split notes
- [ ] Merge notes
- [ ] Delete notes
- [ ] Create notes manually

#### Pitch Curve Editing
- [ ] Draw pitch curves
- [ ] Smooth pitch transitions
- [ ] Flatten pitch segments
- [ ] Quantize pitch to scale

#### Per-Note Parameters
- [ ] Correction amount per note
- [ ] Formant shift per note
- [ ] Gain/volume per note
- [ ] Pan per note
- [ ] Note start/end timing

### PHASE 7: MIDI CONTROL

#### MIDI Input
- [ ] MIDI note sets target pitch
- [ ] MIDI pitch bend support
- [ ] MIDI expression/CC control
- [ ] Sidechain MIDI input

#### Output
- [ ] MIDI output of detected pitch
- [ ] MIDI trigger on note detection
- [ ] MIDI CC output for parameters

### PHASE 8: LIVE PERFORMANCE

#### Low Latency
- [ ] <10ms total latency mode
- [ ] Predictive processing
- [ ] Reduced-quality fast mode
- [ ] Buffer size auto-adjustment

#### Live Features
- [ ] Live pitch display
- [ ] Note name display
- [ ] Confidence meter
- [ ] Tuning meter (like guitar tuner)
- [ ] Visual feedback for correction

### PHASE 9: ADVANCED FEATURES

#### Algorithm Modes
- [ ] Classic mode (Auto-Tune 5 sound)
- [ ] Modern mode (transparent)
- [ ] Creative mode (extreme effects)
- [ ] Instrument mode (for guitars, etc.)

#### Processing Modes
- [ ] Mono (standard)
- [ ] Stereo (process both channels)
- [ ] Mid/Side processing
- [ ] Multi-band processing

#### Special Effects
- [ ] Robot/vocoder mode
- [ ] Megaphone effect
- [ ] Gender bender
- [ ] Chipmunk/slow-mo (formant preserve)
- [ ] Harmony generation (2-4 voices)

### PHASE 10: INTEGRATION

#### DAW Integration
- [x] VST3 plugin format
- [ ] ARA2 integration (for manual editing)
- [ ] Sidechain input
- [ ] Preset management
- [ ] Undo/redo support

#### Zenith Specific
- [ ] Built-in mixer insert
- [ ] Per-track pitch correction
- [ ] Global key/scale from project
- [ ] Batch processing for clips
- [ ] Pitch correction as clip effect

---

## 🎯 IMPLEMENTATION PRIORITY

### MUST HAVE (Ship v1.0)
1. ✅ YIN pitch detection
2. ✅ Basic scale correction
3. ✅ Retune speed control
4. ✅ Humanize
5. ✅ Formant preservation (basic)
6. ✅ Presets
7. **Replace pitch shifter with pro library**

### SHOULD HAVE (v1.1)
8. Graph mode (manual editing)
9. MIDI input
10. Per-note parameters
11. Vibrato control
12. Throat modeling

### NICE TO HAVE (v1.2)
13. ARA2 integration
14. Harmony generation
15. Polyphonic correction
16. Scale detection

---

## 📊 COMPETITIVE ADVANTAGES TO IMPLEMENT

### What Auto-Tune Pro Has That We DON'T
1. Graph mode (manual editing) - **PRIORITY 1**
2. Classic/5 algorithm modes
3. Throat modeling
4. Advanced formant control
5. ARA integration

### What We'll Do BETTER
1. **Included FREE** (not $399)
2. **Lower latency** (target <5ms)
3. **Better presets** (genre-specific)
4. **AI-powered scale detection**
5. **Integrated with DAW** (no plugin needed)
6. **Real-time visual feedback**

---

## 🔧 TECHNICAL DECISIONS

### Pitch Shifting Library Options
1. **Rubber Band Library** (Recommended)
   - Open source (GPL)
   - Pro quality
   - Formant preservation
   - Time stretching included

2. **SoundTouch**
   - Open source (LGPL)
   - Good quality
   - Lower CPU
   - Less features

3. **Write custom phase vocoder**
   - Most control
   - Most work
   - Risk of bugs

**Decision:** Use Rubber Band for pro quality.

### Formant Preservation
- Implement LPC analysis
- 10-20 pole filter typical for speech
- Separate formant envelope from excitation

### Latency Optimization
- Overlap-add processing
- 50% overlap typical
- Smaller FFT windows for lower latency

---

## 📅 TIMELINE

### Week 1: Core Replacement
- Integrate Rubber Band library
- Implement proper formant preservation
- Optimize latency

### Week 2: Graph Mode
- Pitch visualization
- Note detection and display
- Basic drag-to-edit

### Week 3: Advanced Features
- MIDI input
- Per-note parameters
- Vibrato control

### Week 4: Polish
- Presets (genre-specific)
- UI/UX refinement
- Testing with real vocals

**Total: 4 weeks to beat Auto-Tune Pro**
