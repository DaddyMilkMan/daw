# ZenithPolySynth AI Usage Guide

## Overview

ZenithPolySynth is designed to be AI-friendly with two levels of control:
1. **Macro Controls** - High-level semantic controls (recommended)
2. **Modulation Matrix** - Flexible routing system (advanced)

This guide explains how AI agents can effectively use these features to create expressive, musical sounds.

---

## Macro Controls (Recommended)

Macros are the easiest way for AI to control the synthesizer. Each macro affects multiple related parameters with musically meaningful relationships.

### Available Macros

#### 1. Brightness (macro_brightness)
**What it does:** Controls overall tonal brightness
- Increases filter cutoff (80% influence)
- Adds subtle resonance (40% influence)

**When to use:**
- Making sounds brighter or darker
- Creating tonal variation
- Adjusting perceived "openness" of sound

**Examples:**
```javascript
// Bright, sparkly lead
setMacro("macro_brightness", 0.8);

// Dark, muted bass
setMacro("macro_brightness", 0.2);

// Neutral/balanced tone
setMacro("macro_brightness", 0.5);
```

---

#### 2. Thickness (macro_thickness)
**What it does:** Controls sound thickness and richness
- Adds unison voices (70% influence)
- Increases oscillator 2 mix (60% influence)
- Increases unison detune (50% influence)

**When to use:**
- Making sounds fuller and richer
- Adding stereo width
- Creating "supersaw" style sounds

**Examples:**
```javascript
// Thick EDM supersaw
setMacro("macro_thickness", 0.9);

// Thin, focused bass
setMacro("macro_thickness", 0.1);

// Medium richness
setMacro("macro_thickness", 0.5);
```

---

#### 3. Movement (macro_movement)
**What it does:** Controls modulation movement and dynamics
- Increases LFO1 amount (80% influence)
- Increases LFO2 amount (60% influence)

**When to use:**
- Adding wobble, vibrato, or tremolo
- Creating evolving, dynamic sounds
- Making static sounds more interesting

**Examples:**
```javascript
// Heavy wobble bass
setMacro("macro_movement", 0.9);

// Subtle movement
setMacro("macro_movement", 0.3);

// No movement (static)
setMacro("macro_movement", 0.0);
```

---

#### 4. Attack (macro_attack)
**What it does:** Controls how quickly the sound starts
- Decreases amp attack time (90% influence, inverted)
- Decreases mod attack time (70% influence, inverted)

**Note:** Higher macro values = faster attack (inverted relationship)

**When to use:**
- Creating punchy vs. soft sounds
- Adjusting perceived "sharpness" of onset
- Setting pad vs. pluck character

**Examples:**
```javascript
// Instant, punchy attack (pluck/stab)
setMacro("macro_attack", 0.9);

// Slow, gradual fade (pad/string)
setMacro("macro_attack", 0.1);

// Medium attack
setMacro("macro_attack", 0.5);
```

---

#### 5. Release (macro_release)
**What it does:** Controls how long the sound sustains after note release
- Increases amp release time (80% influence)
- Increases mod release time (60% influence)

**When to use:**
- Creating long vs. short tails
- Adjusting sustain character
- Setting staccato vs. legato feel

**Examples:**
```javascript
// Long, sustaining tail
setMacro("macro_release", 0.9);

// Short, tight release
setMacro("macro_release", 0.1);

// Medium release
setMacro("macro_release", 0.5);
```

---

#### 6. Warmth (macro_warmth)
**What it does:** Controls tonal warmth and analog character
- Decreases filter cutoff (60% influence, inverted)
- Increases filter drive/saturation (50% influence)

**When to use:**
- Adding analog-style warmth
- Creating darker, more saturated tones
- Balancing brightness with character

**Examples:**
```javascript
// Warm, saturated analog tone
setMacro("macro_warmth", 0.8);

// Cold, clean digital tone
setMacro("macro_warmth", 0.2);

// Balanced warmth
setMacro("macro_warmth", 0.5);
```

---

#### 7. Detune (macro_detune)
**What it does:** Controls oscillator detuning amount
- Increases osc2 detune positive (70% influence)
- Increases osc3 detune negative (70% influence, inverted)
- Increases unison detune (60% influence)

**When to use:**
- Adding chorus-like width
- Creating detuned character
- Thickening sounds with pitch variation

**Examples:**
```javascript
// Heavy detune/chorus
setMacro("macro_detune", 0.9);

// Tight, in-tune
setMacro("macro_detune", 0.0);

// Subtle detune
setMacro("macro_detune", 0.4);
```

---

#### 8. Depth (macro_depth)
**What it does:** Controls modulation envelope intensity
- Increases mod envelope sustain (70% influence)
- Increases mod envelope decay (60% influence)

**When to use:**
- Adding evolving timbre
- Creating filter sweeps
- Increasing timbral complexity

**Examples:**
```javascript
// Deep modulation evolution
setMacro("macro_depth", 0.8);

// Minimal modulation
setMacro("macro_depth", 0.1);

// Medium depth
setMacro("macro_depth", 0.5);
```

---

## Preset Design Patterns

### Pattern 1: Bright EDM Lead
```javascript
setMacro("macro_brightness", 0.8);  // Bright and sparkly
setMacro("macro_thickness", 0.7);   // Rich and full
setMacro("macro_movement", 0.3);    // Subtle movement
setMacro("macro_attack", 0.7);      // Fairly quick attack
setMacro("macro_release", 0.6);     // Medium sustain
```

### Pattern 2: Dark Wobble Bass
```javascript
setMacro("macro_brightness", 0.2);  // Dark tone
setMacro("macro_thickness", 0.8);   // Thick and powerful
setMacro("macro_movement", 0.9);    // Heavy wobble
setMacro("macro_attack", 0.9);      // Instant attack
setMacro("macro_warmth", 0.7);      // Warm and saturated
```

### Pattern 3: Lush Pad
```javascript
setMacro("macro_brightness", 0.5);  // Balanced brightness
setMacro("macro_thickness", 0.8);   // Very thick
setMacro("macro_movement", 0.4);    // Slow movement
setMacro("macro_attack", 0.1);      // Very slow attack
setMacro("macro_release", 0.9);     // Long release
setMacro("macro_warmth", 0.6);      // Warm character
```

### Pattern 4: Plucky Synth
```javascript
setMacro("macro_brightness", 0.7);  // Bright
setMacro("macro_attack", 0.9);      // Instant attack
setMacro("macro_release", 0.2);     // Short release
setMacro("macro_depth", 0.6);       // Filter pluck
```

---

## Modulation Matrix (Advanced)

For more complex modulation routing, use the modulation matrix. This system has 8 slots for routing sources to destinations.

### Modulation Sources

| Source | Range | Description |
|--------|-------|-------------|
| LFO1 | -1 to +1 | Low-frequency oscillator 1 (sine wave) |
| LFO2 | -1 to +1 | Low-frequency oscillator 2 (sine wave) |
| Env1 | 0 to 1 | Amplitude envelope (ADSR) |
| Env2 | 0 to 1 | Modulation envelope (ADSR) |
| Velocity | 0 to 1 | Note-on velocity |
| ModWheel | 0 to 1 | MIDI mod wheel (CC#1) |
| Aftertouch | 0 to 1 | MIDI channel pressure |

### Modulation Destinations

| Destination | Effect | Range |
|-------------|--------|-------|
| FilterCutoff | Filter frequency | +/- 10kHz |
| FilterResonance | Filter resonance/Q | 0 to 1 |
| Osc1Pitch | Oscillator 1 pitch | +/- 12 semitones |
| Osc2Pitch | Oscillator 2 pitch | +/- 12 semitones |
| Osc3Pitch | Oscillator 3 pitch | +/- 12 semitones |
| WavetablePos | Wavetable position | 0 to 1 |
| Pan | Stereo panning | -1 (left) to +1 (right) |
| Volume | Output volume | 0 to 2x |
| Osc1Mix | Oscillator 1 mix | 0 to 1 |
| Osc2Mix | Oscillator 2 mix | 0 to 1 |
| Osc3Mix | Oscillator 3 mix | 0 to 1 |

### Common Modulation Routing Examples

#### Wobble Bass (LFO to Filter)
```javascript
// Set LFO1 rate to 1/4 note
setParameter("lfo1_rate", 0.25); // Normalized value for ~2Hz

// Route LFO1 to filter cutoff with heavy amount
setModulationSlot(0, ModulationSource.LFO1,
                  ModulationDestination.FilterCutoff, 0.8);
```

#### Velocity-Sensitive Brightness
```javascript
// Route velocity to filter cutoff
setModulationSlot(1, ModulationSource.Velocity,
                  ModulationDestination.FilterCutoff, 0.7);

// Route velocity to filter resonance for extra expression
setModulationSlot(2, ModulationSource.Velocity,
                  ModulationDestination.FilterResonance, 0.5);
```

#### Vibrato via Mod Wheel
```javascript
// Set LFO2 to vibrato rate (~5Hz)
setParameter("lfo2_rate", 0.5); // Normalized

// Route LFO2 to all oscillator pitches
setModulationSlot(3, ModulationSource.LFO2,
                  ModulationDestination.Osc1Pitch, 0.1); // Subtle
setModulationSlot(4, ModulationSource.LFO2,
                  ModulationDestination.Osc2Pitch, 0.1);
setModulationSlot(5, ModulationSource.LFO2,
                  ModulationDestination.Osc3Pitch, 0.1);

// Control vibrato depth with mod wheel
// (This would require parameter-level modulation routing)
```

#### Envelope-Based Filter Sweep
```javascript
// Route Env2 to filter cutoff for classic filter sweep
setModulationSlot(0, ModulationSource.Env2,
                  ModulationDestination.FilterCutoff, 0.6);

// Set Env2 parameters for desired sweep
setParameter("mod_attack", 0.01);  // Fast attack
setParameter("mod_decay", 0.5);    // Medium decay
setParameter("mod_sustain", 0.0);  // Drop to zero
setParameter("mod_release", 0.2);  // Short release
```

#### Auto-Pan
```javascript
// Route LFO1 to pan for auto-pan effect
setModulationSlot(6, ModulationSource.LFO1,
                  ModulationDestination.Pan, 0.7);

// Set LFO rate for desired pan speed
setParameter("lfo1_rate", 0.3); // Slow pan
```

---

## Best Practices for AI

1. **Start with Macros**
   - Use macros for initial sound design
   - They're easier to reason about musically
   - Cover 80% of common use cases

2. **Combine Macros + Matrix**
   - Use macros for basic tone shaping
   - Add modulation matrix for special effects
   - Example: Macro for brightness + LFO routing for movement

3. **Understand Value Ranges**
   - Macros: 0.0 (min) to 1.0 (max), centered at 0.5
   - Modulation amounts: -1.0 to +1.0 (bipolar)
   - Parameters: 0.0 to 1.0 (normalized)

4. **Preset Categories**
   - **Bass**: Low brightness, high thickness, high attack
   - **Lead**: High brightness, medium thickness, medium attack
   - **Pad**: Medium brightness, high thickness, low attack, high release
   - **Pluck**: Medium-high brightness, high attack, low release, medium depth

5. **Modulation Etiquette**
   - Don't over-modulate (keep amounts under 0.8 usually)
   - Use fewer slots with higher amounts vs. many with low amounts
   - Match modulation speed to musical context

6. **RT-Safety**
   - All modulation is computed once per audio buffer
   - No allocations in audio thread
   - Safe for real-time performance

---

## Quick Reference: Macro Cheat Sheet

| Macro | Low (0.0-0.3) | Medium (0.4-0.6) | High (0.7-1.0) |
|-------|---------------|------------------|----------------|
| Brightness | Dark, muted | Balanced | Bright, sparkly |
| Thickness | Thin, focused | Moderate | Thick, rich |
| Movement | Static | Subtle motion | Heavy modulation |
| Attack | Slow fade | Medium | Instant punch |
| Release | Short tail | Medium | Long sustain |
| Warmth | Cold, clean | Balanced | Warm, saturated |
| Detune | Tight, in-tune | Slight chorus | Heavy detune |
| Depth | Minimal evolution | Medium | Deep modulation |

---

## Advanced Topics

### Combining Multiple Modulation Sources

You can route multiple sources to the same destination for complex effects:

```javascript
// Combine velocity and envelope for dynamic filter
setModulationSlot(0, ModulationSource.Velocity,
                  ModulationDestination.FilterCutoff, 0.5);
setModulationSlot(1, ModulationSource.Env2,
                  ModulationDestination.FilterCutoff, 0.6);
// Result: Filter responds to both velocity AND envelope
```

### Inverting Modulation

Use negative amounts to invert modulation:

```javascript
// LFO creates alternating pitch bend (vibrato)
setModulationSlot(0, ModulationSource.LFO1,
                  ModulationDestination.Osc1Pitch, 0.2);

// Opposite on Osc2 for wider stereo effect
setModulationSlot(1, ModulationSource.LFO1,
                  ModulationDestination.Osc2Pitch, -0.2);
```

### Crossfading Oscillators

```javascript
// Use envelope to crossfade from Osc1 to Osc2
setModulationSlot(0, ModulationSource.Env2,
                  ModulationDestination.Osc1Mix, -0.5); // Fade out Osc1
setModulationSlot(1, ModulationSource.Env2,
                  ModulationDestination.Osc2Mix, 0.5);  // Fade in Osc2
```

---

## Troubleshooting

**Sound is too static:**
- Increase `macro_movement`
- Add LFO routing to filter or pitch
- Increase `macro_depth` for evolving timbre

**Sound is too harsh:**
- Decrease `macro_brightness`
- Increase `macro_warmth`
- Reduce filter resonance

**Sound is too thin:**
- Increase `macro_thickness`
- Add more oscillators (increase osc2_mix, osc3_mix)
- Increase unison voices

**Modulation not working:**
- Check modulation slot is set correctly
- Verify source and destination are valid
- Check amount is non-zero
- Ensure LFO rate/envelope parameters are set

---

## Summary

ZenithPolySynth provides powerful yet intuitive sound design for AI:
- **8 semantic macros** for quick, musical results
- **Flexible modulation matrix** for advanced routing
- **RT-safe implementation** for real-time performance
- **Comprehensive documentation** for AI reasoning

Start with macros, expand to modulation matrix as needed. Happy synthesizing!
