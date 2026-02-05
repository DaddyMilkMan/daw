# ZenithPolySynth Ultimate Roadmap - Be The Best

**Goal:** Make ZenithPolySynth the most advanced, complex synthesizer on the market
**Target:** Surpass Serum, Vital, Pigments, Diva, Kilohearts Phase Plant

---

## Current State Assessment

### What ZenithPolySynth Currently Has ✅

**Oscillators:**
- ✅ 3 oscillators (basic saw, square, triangle, sine)
- ✅ Sub oscillator
- ✅ Noise generator
- ✅ Detune control
- ✅ Wavetable (mentioned in enum, implementation unclear)

**Filter:**
- ✅ Basic filter (lowpass, bandpass, highpass)
- ✅ Cutoff, resonance, drive
- ✅ Key tracking
- ✅ 2 filter models (SVF, ladder)

**Envelopes:**
- ✅ ADSR amplitude envelope
- ✅ ADSR modulation envelope

**LFOs:**
- ✅ 2 LFOs
- ✅ Multiple waveforms (sine, triangle, saw, square, S&H)
- ✅ Rate sync to BPM
- ✅ Retrigger option

**Modulation:**
- ✅ Modulation matrix (64 slots)
- ✅ Sources: LFO1, LFO2, Env1, Env2, velocity, mod wheel, aftertouch, MPE
- ✅ Destinations: pitch, mix, filter cutoff, filter resonance, amp gain

**Polyphony:**
- ✅ 16 voices (expandable)
- ✅ Mono mode
- ✅ Glide
- ✅ Pitch bend range
- ✅ MPE support (juce::MPESynthesiser)

**Effects:**
- ✅ Distortion
- ✅ Chorus
- ✅ Reverb
- ✅ Delay (with sync)

**Other:**
- ✅ Full JUCE UI
- ✅ Preset system (8 banks)
- ✅ Visualizer
- ✅ Parameter manager

---

## Gap Analysis: What We're Missing

### CRITICAL GAPS (Must Have)

| Feature | Serum | Vital | Pigments | Diva | Zenith | Priority |
|---------|--------|-------|----------|------|---------|----------|
| **Wavetable Synthesis** | ✅ | ✅ | ✅ | ❌ | ⚠️ Basic | **P0** |
| **Wavetable Import** | ✅ | ✅ | ✅ | ❌ | ❌ | **P0** |
| **Wavetable Editing** | ✅ | ✅ | ✅ | ❌ | ❌ | **P0** |
| **Vector Synthesis** | ✅ | ✅ | ✅ | ❌ | ❌ | **P0** |
| **FM Synthesis** | ❌ | ✅ | ✅ | ❌ | ⚠️ Basic | **P1** |
| **Ring Mod** | ✅ | ❌ | ✅ | ❌ | ⚠️ Basic | **P1** |
| **Sample Oscillator** | ❌ | ✅ | ✅ | ❌ | ❌ | **P1** |
| **Granular** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Unison** | ✅ 16+ voices | ✅ 16 | ✅ | ❌ | ❌ | **P1** |
| **Spread/Pan** | ✅ | ✅ | ✅ | ✅ | ❌ | **P1** |
| **Filter Slopes** | ✅ 12/24/36/48 dB | ✅ 12/24 | ✅ 12/24/48 | ✅ Variable | ❌ | **P0** |
| **Filter Drift** | ✅ | ✅ | ✅ | ✅ | ❌ | **P1** |
| **Filter Envelopes** | 1 | 1 | 2 | 2 | 1 | **P2** |
| **LFO Shapes** | ✅ Many | ✅ Custom | ✅ Custom | ✅ Many | ⚠️ Basic | **P1** |
| **LFO Unipolar** | ✅ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **LFO Phase** | ✅ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Step LFO** | ✅ | ✅ | ✅ | ❌ | ❌ | **P0** |
| **Random LFO** | ✅ | ✅ | ✅ | ❌ | ❌ | **P1** |
| **Envelope Stages** | 4 (ADSR) | 4 (ADSR) | 4+ | 4 | 4 (ADSR) | **P2** |
| **Envelope Loops** | ❌ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Env Delay/Hold** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Mod Matrix** | ✅ | ✅ | ✅ | ❌ | ✅ | - |
| **Matrix Slots** | 16 | 12 | 24+ | ❌ | 64 | ✅ (More!) |
| **Macro Controls** | ✅ 4 | ✅ 4 | ✅ 4 | ❌ | ✅ | - |
| **Arpeggiator** | ✅ | ✅ | ✅ | ❌ | ❌ | **P0** |
| **Sequencer** | ✅ | ✅ | ✅ | ❌ | ❌ | **P1** |
| **FX Chain** | ✅ 4 slots | ✅ | ✅ | ❌ | ✅ 4 built-in | **P1** |
| **FX Types** | 10+ | 8+ | 20+ | ❌ | 4 basic | **P1** |
| **Polyphonic FX** | ✅ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **MPE Support** | ❌ | ✅ | ✅ | ❌ | ✅ | - |
| **Poly Pressure** | ❌ | ✅ | ✅ | ❌ | ✅ (MPE) | - |
| **Per-Voice Pan** | ❌ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Polyphonic LFO** | ❌ | ✅ | ✅ | ❌ | ❌ | **P2** |

### ADVANCED FEATURES (Competitive Edge)

| Feature | Serum | Vital | Pigments | Diva | Zenith | Priority |
|---------|--------|-------|----------|------|---------|----------|
| **Physical Modeling** | ❌ | ❌ | ❌ | ⚠️ Analog | ❌ | **P1** |
| **Waveshaping** | ✅ | ✅ | ✅ | ❌ | ❌ | **P1** |
| **Wavefolding** | ✅ | ❌ | ✅ | ❌ | ❌ | **P1** |
| **Bitcrush** | ✅ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Sidechain Mod** | ✅ | ✅ | ✅ | ❌ | ❌ | **P1** |
| **Audio Input** | ❌ | ❌ | ❌ | ❌ | ❌ | **P2** |
| **Vocoder** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Ring Mod** | ✅ | ❌ | ✅ | ❌ | ⚠️ Basic | **P1** |
| **Formant Filter** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Comb Filter** | ✅ | ❌ | ✅ | ❌ | ❌ | **P1** |
| **Phaser** | ✅ | ❌ | ✅ | ❌ | ❌ | **P1** |
| **Flanger** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Tremolo** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Auto-pan** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Distortion Types** | 5+ | 4+ | 10+ | ❌ | 1 basic | **P1** |
| **Saturation Models** | 5+ | 4+ | 8+ | ❌ | ❌ | **P1** |
| **Compressor** | ❌ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Limiter** | ❌ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **EQ** | ❌ | ❌ | ✅ | ❌ | ❌ | **P2** |
| **Reverb Types** | 2 | 2 | 5+ | ❌ | 1 basic | **P1** |
| **Delay Types** | 3 | 2 | 5+ | ❌ | 1 basic | **P1** |
| **Randomization** | ✅ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Preset Browser** | ✅ | ✅ | ✅ | ✅ | ✅ | - |
| **Preset Search** | ✅ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Preset Tags** | ✅ | ✅ | ✅ | ❌ | ❌ | **P2** |
| **Wavetable Library** | 100+ | 100+ | 200+ | N/A | 0 | **P0** |

---

## Ultimate Feature Roadmap

### Phase 1: Foundation (Months 1-2) - P0 Features

**1. Wavetable Synthesis Engine**
```
WavetableOscillator class:
- Load single-cycle wavetables (2048 samples)
- Morph between waveforms (interpolation)
- Wave import from audio files
- Wave import from .wt files (Serum/Vital format)
- Waveform editor (draw, generate)
- Waveform synthesis tools (sine add, FM, etc.)
- Wavetable library (200+ factory tables)
```

**2. Advanced Filters**
```
FilterModule class:
- 12dB, 24dB, 36dB, 48dB slopes
- SVF, ladder, diode ladder, Korg MS-20, Moog
- Filter drive + model (5 saturation types)
- Filter drift (randomize cutoff)
- Parabolic shaping (Pigments-style)
- Comb filter (phaser, flanger)
- Per-voice filter (polyphonic FX)
```

**3. Arpeggiator**
```
Arpeggiator class:
- Up, Down, Up-Down, Random, Chord, Order
- Rate sync (1/64 to 16 bars)
- Gate time (length)
- Octave range
- Swing
- Pattern editor (32 steps)
- Velocity patterns
- Hold mode
```

**4. Step LFO**
```
StepLFO class:
- 16, 32, 64 step sequences
- Per-step smoothing (interpolation)
- Pattern editor (draw)
- Randomize, shift, reverse
- Sync to BPM
- Loop, bounce, ping-pong playback
```

**5. Unison & Spread**
```
UnisonVoice class:
- 16 unison voices per note
- Detune (cents)
- Spread (stereo width)
- Random phase offset
- Pan distribution
- Volume compensation
- Blend with dry signal
```

---

### Phase 2: Advanced Synthesis (Months 3-4) - P1 Features

**6. Multi-Engine Architecture**
```
SynthEngine class:
- Oscillator Engine: Wavetable, Virtual Analog, FM, Sample, Granular
- Filter Engine: SVF, Ladder, Comb, Formant, Ring
- User can mix engines (hybrid synthesis)
```

**7. FM Synthesis**
```
FMOperator class:
- 4 operators (carrier, modulator)
- FM routing matrix (any op mod any op)
- Feedback routing
- Operator envelopes (independent)
- FM algorithms (presets: DX7, etc.)
```

**8. Ring Modulation**
```
RingModulator class:
- Ring mod on oscillator pairs
- External audio input ring mod
- Stereo ring mod
- Frequency modulation of carrier
- LFO modulation of modulator
```

**9. Sample Oscillator**
```
SampleOscillator class:
- Load any audio sample
- Loop modes (forward, ping-pong, slice)
- Loop points editable
- Sample start/end
- Reverse playback
- Granular mode (optional)
```

**10. Granular Synthesis**
```
GranularEngine class:
- Granular size (1ms - 1s)
- Density (grain overlap)
- Pitch randomization
- Position randomization
- Grain envelope shape
- Freeze mode (hold buffer)
```

**11. Physical Modeling**
```
PluckedString class:
- Karplus-Strong algorithm
- Decay time
- Brightness
- Inharmonicity
- String damping

BowedString class:
- Physical modeling bow
- Bow pressure, velocity
- Bow position
```

**12. Waveshaping**
```
Waveshaper class:
- 20 distortion algorithms (tanh, soft clip, hard clip, etc.)
- Wavefold (multiple folds)
- Bitcrush (bit depth reduction)
- Sample rate reduction
- Drive + tone controls
```

**13. Advanced LFOs**
```
LFO class:
- Waveform shapes: sine, triangle, saw, square, S&H, noise, random, stepped, smooth
- Unipolar/bipolar mode
- Phase offset (per voice)
- One-shot mode (trigger from envelope)
- Polyphonic LFO (per-voice modulation)
- Random LFO (random value per note)
```

**14. More Envelopes**
```
Envelope class:
- Delay, Attack, Decay, Sustain, Release, Hold
- Loop envelope (AHDSR loop)
- Multi-stage envelope (custom curve)
- Envelope following (audio input)
```

---

### Phase 3: Effects Chain (Months 5-6) - P1 Features

**15. Modular Effects Chain**
```
EffectsChain class:
- 8 FX slots (orderable)
- Per-voice FX (polyphonic)
- Global FX
- Modulate FX parameters (any source)
- FX bypass per slot
- Wet/dry mix per FX
```

**16. Distortion Module**
```
Distortion class:
- Algorithms: Soft clip, hard clip, tanh, sigmoid, bitcrush, decimator, wavefold
- Tone filter (post-distortion)
- Drive amount
- Mix
```

**17. Saturation Models**
```
Saturation class:
- Tape saturation (analog tape model)
- Tube saturation (triode, pentode)
- Console emulation (neve, API)
- Transistor saturation
- Heat amount
```

**18. Reverb Module**
```
Reverb class:
- Algorithms: Hall, Room, Plate, Spring, Ambience
- Size, decay, damping, pre-delay
- Modulation (chorusing reverb)
- Diffusion
- High/low damping
- Freeze (infinite decay)
```

**19. Delay Module**
```
Delay class:
- Algorithms: Stereo, Ping-pong, Tape, Multi-tap
- Time (synced to BPM)
- Feedback
- Filter feedback (lowpass, highpass)
- Modulation (LFO modulates delay time)
- Reverse delay
- Ping-pong width
```

**20. Phaser**
```
Phaser class:
- 4-12 stages
- Rate, depth, feedback
- LFO rate (modulate notches)
- Notch frequency
- Stereo spread
```

**21. Flanger**
```
Flanger class:
- Delay time (0-20ms)
- Feedback
- LFO modulation
- Stereo flanger
- Invert phase
```

**22. Compressor**
```
Compressor class:
- Threshold, ratio, attack, release
- Sidechain input (ducking)
- Knee (soft/hard)
- Makeup gain
- Gain reduction meter
```

**23. EQ**
```
EQ class:
- 3-band EQ (low, mid, high)
- Bell, shelf, bandpass
- Frequency, Q, gain
- Linear phase option
```

**24. Vocoder**
```
Vocoder class:
- 20-40 bands
- Carrier input (oscillator, noise)
- Modulator input (external audio)
- Bandpass bands
- Formant shifting
```

---

### Phase 4: Advanced Features (Months 7-8) - P2 Features

**25. Sequencer**
```
Sequencer class:
- 16, 32, 64 steps
- Note, velocity, gate per step
- Probability per step
- Ratchets (repeat notes)
- Randomize, shift, reverse
- Pattern editing (grid)
- Save/load patterns
```

**26. Chord Memory**
```
ChordMemory class:
- Store up to 32 chords
- Recall via MIDI CC
- Trigger full chord with single note
- Strum mode (roll chords)
```

**27. Scale & Key Detection**
```
ScaleDetector class:
- Auto-detect key/scale from MIDI
- Lock to scale (quantize input)
- Highlight scale notes on keyboard UI
- Preset scales (major, minor, dorian, etc.)
```

**28. Randomization**
```
Randomizer class:
- Randomize any parameter
- Lock/unlock parameters
- Randomization depth
- Morph between presets
- Chaos mode (continuous randomization)
```

**29. Macro Controls**
```
MacroControl class:
- 8 macro knobs
- Assign any parameter to macro
- Assign multiple parameters to single macro
- Min/max range per macro assignment
- Smooth transitions
```

**30. Per-Voice Pan**
```
VoicePanner class:
- Pan per voice
- Pan randomization (unison)
- LFO modulation of pan
- Stereo spread control
```

**31. Sidechain Modulation**
```
SidechainModulator class:
- External audio input as modulation source
- Envelope follower on sidechain
- Modulate filter cutoff, amp gain, etc.
- Ducking, pumping effects
```

**32. Audio Input**
```
AudioInput class:
- Route external audio into synth
- Process audio through filters, FX
- Use as carrier for vocoder
- Ring mod with external audio
```

---

### Phase 5: AI Integration (Unique Differentiator) - P0

**33. AI Sound Designer**
```
AISoundDesigner class:
- Generate wavetables from text description
- "Dark brass", "bright pluck", "fat bass"
- AI suggests synthesis techniques
- Auto-create patches from genre/style
- AI-assisted randomization (guided)
```

**34. AI Preset Generation**
```
AIPresetGenerator class:
- Generate presets on-the-fly
- Learn user's patch preferences
- Recommend similar patches
- AI morphing between patches
```

**35. AI Mixing Assistant**
```
AIMixAssistant class:
- Analyze patch frequency spectrum
- Suggest EQ cuts/boosts
- Auto-balance FX levels
- Suggest compression settings
```

**36. AI Melody Generator**
```
AIMelodyGenerator class:
- Generate melodies based on patch
- Scale-aware melody generation
- Arpeggio patterns
- Accompaniment generation
```

---

## Architecture Plan

### Modular Voice Architecture

```
Voice (Per-Note):
├── Oscillators
│   ├── Osc1 (WavetableEngine)
│   ├── Osc2 (WavetableEngine)
│   ├── Osc3 (SampleEngine)
│   ├── SubOsc
│   └── Noise
├── FM Operators (4)
├── Ring Mod (Osc1×Osc2)
├── Mixer
│   ├── Osc1-3 levels
│   ├── Pan
│   └── Unison (16 voices)
├── Filter
│   ├── Type (8 filter models)
│   ├── Slope (12/24/36/48 dB)
│   ├── Drive + Saturation
│   └── Envelope
├── Envelopes
│   ├── Amp (ADSR)
│   ├── Filter (ADSR)
│   ├── FM (4 envelopes)
│   └── LFO envelope
├── LFOs
│   ├── LFO1 (StepLFO or WaveLFO)
│   ├── LFO2 (StepLFO or WaveLFO)
│   └── PolyLFO (per-voice)
├── Per-Voice FX
│   ├── Distortion
│   ├── Phaser
│   └── Delay (short)
└── Pan (per-voice)

Global:
├── Master FX Chain (8 slots)
├── Arpeggiator
├── Sequencer
├── Chord Memory
├── Scale Detector
├── AI Engine
└── Modulation Matrix (128 slots)
```

### Modulation Matrix (Expanded)

**Sources (32):**
- LFO1-8 (wave, step, random, poly)
- Envelopes (6: amp, filter, FM1-4)
- MIDI: Velocity, ModWheel, Aftertouch, PitchBend, Breath, CC1-8
- MPE: Y-axis (timbre), Z-axis (pressure), Slide
- Sidechain
- Audio Input (envelope follower)
- Random (continuous, triggered)
- Macro controls (8)
- Note number
- Voice index (per-voice modulation)
- AI: AI modulation (AI-generated LFO)

**Destinations (64):**
- Osc1-3: Pitch, Shape, Mix, Phase, FM amount
- FM1-4: Frequency, Depth, Feedback, Env amount
- Ring Mod: Carrier freq, Modulator freq, Mix
- Filter: Cutoff, Resonance, Drive, Slope, Model, Mix
- Env1-6: Delay, Hold, Amount, Loop point
- LFO1-8: Rate, Phase, Shape, Amount
- FX1-8: All FX parameters
- Amp: Gain, Pan, Spread
- Arp: Rate, Gate, Octave, Swing, Pattern
- Seq: Step velocity, step note
- Global: Master tune, pitch bend range

---

## Implementation Priority Order

**Immediate (Week 1-4):**
1. Wavetable oscillator (load, morph, import)
2. Filter slopes (12/24/36/48 dB)
3. Arpeggiator (basic)
4. Step LFO
5. Unison (8 voices)

**Short-term (Months 2-3):**
6. Wavetable editor (draw, generate)
7. Wavetable library (200+ tables)
8. Advanced filter models (diode, MS-20, Moog)
9. Filter drive + saturation
10. Comb filter
11. More LFO shapes
12. Random LFO

**Medium-term (Months 4-6):**
13. FM synthesis (4 operators)
14. Ring mod (stereo)
15. Sample oscillator
16. Granular synthesis
17. Physical modeling (plucked, bowed)
18. Waveshaper (10 algorithms)
19. FX chain (8 slots)
20. Distortion types
21. Reverb types (5)
22. Delay types (4)
23. Phaser, Flanger
24. Compressor, EQ

**Long-term (Months 7-12):**
25. Sequencer
26. Chord memory
27. Scale detection
28. Randomization
29. Macro controls (8)
30. Per-voice pan
31. Sidechain modulation
32. Audio input
33. Vocoder
34. Formant filter
35. AI sound designer
36. AI preset generator

---

## Performance Targets

- **Max Voices:** 32 (expandable to 128)
- **CPU:** <5% per voice at 48kHz (high quality)
- **Latency:** <10ms at 128 samples buffer
- **Memory:** <200MB total
- **Sample Rate:** 44.1kHz, 48kHz, 96kHz support
- **Quality:** 3 quality modes (Low/Med/High)

---

## File Structure Plan

```
apps/desktop/Source/instruments/ZenithPolySynth/
├── ZenithPolySynth.h (main)
├── ZenithPolySynth.cpp
├── ZenithPolySynthDefs.h (enums, structs)
├── ZenithPolySynthParameterManager.h
├── ZenithPolySynthParameterManager.cpp
├── voices/
│   ├── Voice.h
│   ├── Voice.cpp
│   └── VoiceState.h
├── oscillators/
│   ├── OscillatorBase.h
│   ├── WavetableOscillator.h
│   ├── WavetableOscillator.cpp
│   ├── VirtualAnalogOscillator.h
│   ├── FMOscillator.h
│   ├── SampleOscillator.h
│   ├── GranularOscillator.h
│   └── WavetableLibrary.h
├── filters/
│   ├── FilterBase.h
│   ├── SVFFilter.h
│   ├── LadderFilter.h
│   ├── DiodeLadderFilter.h
│   ├── MS20Filter.h
│   ├── MoogFilter.h
│   ├── CombFilter.h
│   └── FormantFilter.h
├── envelopes/
│   ├── Envelope.h
│   ├── ADSREnvelope.h
│   ├── MultiStageEnvelope.h
│   └── LoopEnvelope.h
├── lfos/
│   ├── LFOBase.h
│   ├── WaveLFO.h
│   ├── StepLFO.h
│   ├── RandomLFO.h
│   └── PolyLFO.h
├── fx/
│   ├── EffectsChain.h
│   ├── Distortion.h
│   ├── Reverb.h
│   ├── Delay.h
│   ├── Phaser.h
│   ├── Flanger.h
│   ├── Compressor.h
│   ├── EQ.h
│   └── Vocoder.h
├── modulation/
│   ├── ModulationMatrix.h
│   ├── ModulationSource.h
│   ├── ModulationDestination.h
│   ├── MacroControls.h
│   └── SidechainModulator.h
├── sequencer/
│   ├── Arpeggiator.h
│   ├── Sequencer.h
│   ├── ChordMemory.h
│   └── ScaleDetector.h
├── ai/
│   ├── AISoundDesigner.h
│   ├── AIPresetGenerator.h
│   └── AIMixAssistant.h
└── wavetables/
    ├── Wavetable.h
    ├── WavetableEditor.h
    ├── WavetableLibrary.h
    └── WavetableImporter.h
```

---

## Competitive Advantage Summary

### What Will Make ZenithPolySynth #1:

1. **AI Integration** 🏆
   - Only synth with AI sound design
   - AI preset generation
   - AI mixing assistance
   - **Unique differentiator**

2. **Most Comprehensive** 🏆
   - All synthesis methods in one plugin
   - Wavetable + FM + Sample + Granular + Physical Modeling
   - More FX types than any competitor
   - Most modulation destinations

3. **Best Sound Quality** 🏆
   - Oversampling (2x, 4x)
   - Zero-delay filters
   - High-quality resampling
   - Anti-aliasing

4. **Most Flexible** 🏆
   - 128-slot modulation matrix (most is 24)
   - Modular FX chain (8 slots)
   - Per-voice FX and pan
   - Multi-engine architecture

5. **Future-Proof** 🏆
   - MPE support
   - 96kHz support
   - Modern GPU-accelerated UI (Skia)
   - AI integration ready

---

## Final Thoughts

**This is a 12-18 month roadmap** to build the ultimate synthesizer. Start with P0 features (wavetable, advanced filters, arpeggiator, step LFO, unison), then iterate.

**Key Differentiator:** AI integration. No other synth has this. Lean heavily into AI-assisted sound design, preset generation, and mixing assistance.

**Execution:** Implement incrementally, test each module thoroughly, optimize for performance, and build a library of high-quality factory presets (500+ patches).

**Result:** The most advanced synthesizer on the market, period.
