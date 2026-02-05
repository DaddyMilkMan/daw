# Phase 1 & 2 Testing Guide - Manual Verification

## Overview
This document provides step-by-step instructions to manually verify the Phase 1 (Filters) and Phase 2 (Oscillators) implementations in ZenithPolySynth.

## Prerequisites

1. Build Zenith DAW with latest changes:
```bash
cd /home/micah/Desktop/zenith/daw
cmake --build build -j$(nproc)
```

2. Launch the DAW:
```bash
./build/Zenith\ DAW
```

3. Create a new project with one instance of ZenithPolySynth

## Phase 1: Filter Testing

### Test 1: Moog Ladder Filter

**Setup:**
- Open ZenithPolySynth UI
- Set Filter Model to "Moog Ladder"
- Set Cutoff to 1 kHz
- Set Resonance to 0.0
- Play a middle C note

**Expected Results:**
- [ ] Smooth, warm lowpass sound
- [ ] No digital artifacts or zipper noise
- [ ] Filter sweeps sound musical and creamy

**Test Resonance:**
- Increase Resonance to 0.8
- [ ] Filter should have prominent resonance peak
- [ ] Sound should "sing" at cutoff frequency
- [ ] Bass should be slightly reduced (authentic Moog behavior)

**Test Self-Oscillation:**
- Set Resonance to 0.95-1.0
- [ ] Filter should oscillate on its own (even without input)
- [ ] Pure sine wave output
- [ ] Stable pitch at cutoff frequency

**Test Drive:**
- Set Drive to 3.0-5.0
- [ ] Sound should be warmer and more saturated
- [ ] Subtle grit/edge on loud notes
- [ ] Not distorted in a bad way (just "thicker")

### Test 2: MS-20 Lowpass Filter

**Setup:**
- Set Filter Model to "MS-20"
- Set Cutoff to 500 Hz
- Set Resonance to 0.7

**Expected Results:**
- [ ] More aggressive character than Moog
- [ ] sharper resonance peak
- [ ] "Squawk" when playing hard

**Test Extreme Resonance:**
- Set Resonance to 0.95
- Play C2 (low C)
- [ ] Filter should self-oscillate aggressively
- [ ] Classic acid resonance character
- [ ] Sound should cut through the mix

### Test 3: Prophet-5 Filter

**Setup:**
- Set Filter Model to "Prophet-5"
- Set Cutoff to 800 Hz
- Set Resonance to 0.5

**Expected Results:**
- [ ] Creamy, smooth character
- [ ] Musical filter sweeps
- [ ] Less aggressive than MS-20
- [ ] Perfect for pads and polyphonic sounds

**Test High Notes:**
- Play C6 (high C) with fast filter sweep
- [ ] Filter should maintain character at high frequencies
- [ ] No thinning out or harshness

### Test 4: SEM Filter (Multimode)

**Lowpass Mode:**
- Set Filter Type to Lowpass
- Set Cutoff to 1 kHz, Resonance 0.6
- [ ] Creamy lowpass with distinctive SEM character

**Bandpass Mode:**
- Set Filter Type to Bandpass
- Set Cutoff to 2 kHz, Resonance 0.8
- [ ] Funky bandpass character
- [ ] Great for filter sweeps and expressive leads

**Highpass Mode:**
- Set Filter Type to Highpass
- Set Cutoff to 500 Hz
- [ ] Clean highpass with no artifacts
- [ ] Good for removing mud

### Test 5: TB-303 Filter

**Setup:**
- Set Filter Model to "TB-303"
- Set Cutoff to 400 Hz
- Set Resonance to 0.95

**Expected Results:**
- [ ] Classic acid squelch
- [ ] Aggressive resonance
- [ ] Cuts through the mix
- [ ] Perfect for techno and acid house

**Test with 16th notes:**
- Play C, E, G pattern in 16ths at 130 BPM
- [ ] Authentic TB-303 character
- [ ] Resonant, rubbery bass

### Test 6: Filter Modulation

**Test LFO Modulation:**
- Set LFO1 to modulate Filter Cutoff
- LFO Rate: 3 Hz, Amount: 0.5
- [ ] Smooth filter sweep without zipper noise
- [ ] Modulation sounds even and musical

**Test Envelope Modulation:**
- Set Filter Envelope to modulate Cutoff
- Fast attack, medium decay
- [ ] Filter "opens" with each note
- [ ] Smooth, musical response

**Test Audio-Rate Modulation:**
- Set Osc1 to modulate Filter Cutoff
- Use high LFO rate (50-100 Hz)
- [ ] Filter FM effects should be audible
- [ ] No harshness or instability

## Phase 2: Oscillator Testing

### Test 1: Basic Waveforms (Baseline)

**Saw Wave:**
- Set Osc1 to Saw
- [ ] Classic saw tooth character
- [ ] Rich harmonics
- [ ] Good for bass and leads

**Square Wave:**
- Set Osc1 to Square
- [ ] Hollow, nasal character
- [ ] Good for chiptune and retro sounds

**Sine Wave:**
- Set Osc1 to Sine
- [ ] Pure, clean tone
- [ ] No harmonics
- [ ] Good for sub-bass

### Test 2: Buchla Wavefolder

**Setup:**
- Set Osc1 to Wavefolder
- Set Base Wave to Sine
- Set Folds to 2.0
- Set Symmetry to 0.0

**Expected Results:**
- [ ] Metallic, clangorous character
- [ ] Rich inharmonic harmonics
- [ ] West-coast synthesis sound

**Test Symmetry:**
- Adjust Symmetry from -1.0 to +1.0
- [ ] Waveform character changes noticeably
- [ ] Asymmetric waveforms have different tone

**Test with Filter:**
- Combine Wavefolder with Moog Ladder
- [ ] Classic west-coast + east-coast combination
- [ ] Evolving, complex timbres

### Test 3: Phase Distortion

**Setup:**
- Set Osc1 to Phase Dist
- Set Distortion to 0.5
- Set Waveform to Sine→Saw

**Expected Results:**
- [ ] Digital character with edge
- [ ] Not subtle like analog
- [ ] Classic 80s Casio tone

**Test All Waveforms:**
- Try Sine→Saw, Sine→Square, Sine→Triangle
- [ ] Each has distinct character
- [ ] All useful for different sounds

**Test Extreme Distortion:**
- Set Distortion to 1.0
- [ ] Aggressive digital character
- [ ] Good for brass and metallic sounds

### Test 4: Additive Synthesis

**Setup:**
- Set Osc1 to Additive
- Set to Harmonic Series
- [ ] Clean, organ-like tone

**Test Custom Partials:**
- Adjust partial levels manually
- Set even partials higher than odd
- [ ] Strings-like character
- [ ] Smooth, evolving timbre

**Test Inharmonic:**
- Set to Inharmonic Series
- [ ] Bell and metallic tones
- [ ] Complex, evolving spectra

**Test with Modulation:**
- Modulate partial levels with LFO
- [ ] Animated, evolving timbres
- [ ] Unique sounds not possible with subtractive

### Test 5: Granular Synthesis

**Setup:**
- Set Osc1 to Granular
- Load a sample (sine wave or simple wave)
- Set Grain Size to 0.2s
- Set Density to 10 grains/sec

**Expected Results:**
- [ ] Ambient, textural character
- [ ] Not pitch-perfect (by design)
- [ ] Evolving, organic feel

**Test Short Grains:**
- Set Grain Size to 0.01s
- [ ] Metallic, shimmering texture
- [ ] Good for atmospheric pads

**Test Long Grains:**
- Set Grain Size to 0.5s
- [ ] Frozen time effect
- [ ] Slowly evolving texture

**Test Randomness:**
- Increase Randomness to 0.7
- [ ] More organic, less repetitive
- [ ] Natural, evolving soundscapes

### Test 6: Oscillator Combinations

**Test Dual Oscillator:**
- Osc1: Saw, Osc2: Wavefolder
- Mix both 50%
- [ ] Complex, rich timbre
- [ ] More depth than single oscillator

**Test Detuning:**
- Osc1 and Osc2 slightly detuned
- [ ] Thick, wide stereo field
- [ ] Classic supersaw character

**Test Hard Sync:**
- Enable Osc2 Sync
- [ ] Aggressive, piercing character
- [ ] Good for leads and bass

## Performance Testing

### CPU Load Test

1. Open ZenithPolySynth with 16 voices
2. Play a full chord (all voices active)
3. Monitor CPU usage in DAW

**Expected CPU per voice:**
- [ ] SVF Filter: < 0.5%
- [ ] Moog Ladder: < 1.5%
- [ ] MS-20: < 1.5%
- [ ] Prophet-5: < 1.5%
- [ ] Additive (64 partials): < 2%
- [ ] Granular: < 3%

**Total CPU for 16 voices should be < 50%** on modern CPU

### Polyphony Test

1. Set max voices to 16
2. Play fast arpeggios covering full range
3. Listen for voice stealing

**Expected:**
- [ ] No voice stealing with 16 voices
- [ ] All notes sound consistent
- [ ] No dropouts or glitches

### Real-Time Modulation Test

1. Assign multiple modulators to Filter Cutoff
2. Modulate heavily (LFO + Envelope + Velocity)
3. Listen for zipper noise

**Expected:**
- [ ] Smooth parameter changes
- [ ] No zipper noise or stepping
- [ ] All modulation is musical

## Sound Quality Verification

### Anti-Aliasing Test

**High Frequency Test:**
- Play C7 (high C, 2093 Hz)
- Listen for aliasing artifacts (metallic ringing)

**Expected:**
- [ ] Clean sound without harsh aliasing
- [ ] Wavetable oscillator uses MIP-mapping
- [ ] Wavefolder doesn't create excessive highs

### Dynamic Range Test

**Quiet Test:**
- Play very softly (velocity 10-20)
- [ ] No noise floor or hum
- [ ] Smooth response

**Loud Test:**
- Play fortissimo (velocity 120-127)
- [ ] No digital clipping
- [ ] Smooth saturation with drive

### Stereo Test

**Panning Test:**
- Pan Osc1 hard left, Osc2 hard right
- [ ] Proper stereo separation
- [ ] No crosstalk or phase issues

## Bug Checklist

- [ ] No crashes when changing filter models
- [ ] No crashes when changing oscillator types
- [ ] No audio dropouts
- [ ] No zipper noise on parameter changes
- [ ] No stuck notes (voice stealing works)
- [ ] No memory leaks (test by opening/closing synth multiple times)
- [ ] No UI lag when modulating parameters

## Comparison to Reference Synths

### Filter Comparison

**Moog Ladder vs. u-he Diva:**
- [ ] Similar warmth and character
- [ ] Similar resonance behavior
- [ ] Our version is more CPU efficient

**MS-20 vs. Korg MS-20:**
- [ ] Similar aggressive resonance
- [ ] Similar squelch character
- [ ] Our version has better drive control

### Oscillator Comparison

**Wavefolder vs. Buchla 259:**
- [ ] Similar metallic character
- [ ] Similar folding behavior
- [ ] Our version has better symmetry control

**Additive vs. Pigments:**
- [ ] Similar 64-partial capacity
- [ ] Similar harmonic control
- [ ] Our version has simpler workflow

## Success Criteria

**Phase 1 (Filters):**
- [✓] 5 circuit-modeled filters implemented
- [ ] All filters produce expected character
- [ ] Filters are stable at all settings
- [ ] Self-oscillation works on resonant filters
- [ ] CPU usage is acceptable (< 2% per voice)

**Phase 2 (Oscillators):**
- [✓] 5 advanced oscillator types implemented
- [ ] All oscillators produce unique timbres
- [ ] No excessive aliasing
- [ ] Granular engine works with samples
- [ ] Additive synthesis is editable
- [ ] CPU usage is acceptable (< 3% per voice)

**Overall:**
- [ ] Synth is stable and doesn't crash
- [ ] Sound quality rivals commercial plugins
- [ ] Performance is acceptable for real-time use
- [ ] Ready for Phase 3 (Effects)

## Next Steps After Testing

1. **Document any bugs found** in GitHub Issues
2. **Fix critical issues** before Phase 3
3. **Create presets** showing off new features
4. **Record audio samples** for comparison
5. **Begin Phase 3** (Effects expansion) if tests pass

## Contact

For questions or issues during testing, create a GitHub issue or contact the development team.

---

**Test Execution Date:** ___________
**Tester:** ___________
**Build Version:** ___________
**Overall Result:** PASS / FAIL

**Notes:**
_______________________________________________________________________
_______________________________________________________________________
_______________________________________________________________________
