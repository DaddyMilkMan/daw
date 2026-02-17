# Zenith Ultra Synth - User Guide

## Table of Contents

1. [Quick Start](#quick-start)
2. [Interface Overview](#interface-overview)
3. [Sound Design Basics](#sound-design-basics)
4. [Oscillators](#oscillators)
5. [Filters](#filters)
6. [Envelopes](#envelopes)
7. [LFOs & Modulation](#lfos--modulation)
8. [Effects](#effects)
9. [Arpeggiator](#arpeggiator)
10. [Presets](#presets)
11. [Advanced Features](#advanced-features)
12. [Tips & Tricks](#tips--tricks)

---

## Quick Start

### Getting Your First Sound

1. **Load a Preset**: Click the preset browser button (top left) and select "Init" to start fresh
2. **Play**: Use your MIDI keyboard or the virtual keyboard to trigger notes
3. **Adjust Sound**: Use the main knobs to shape your sound:
   - **Cutoff**: Filter brightness (darker → brighter)
   - **Resonance**: Filter resonance (adds "squelch")
   - **Env Amt**: How much the envelope affects the filter
4. **Save**: Click "Save" in the preset browser to store your creation

### Basic Navigation

- **Left Click**: Adjust parameters
- **Right Click**: Open context menu (reset, MIDI learn, automation)
- **Ctrl+Click**: Fine adjustment (hold for precision)
- **Double Click**: Reset parameter to default
- **Drag**: Draw automation in the sequencer/arpeggiator

---

## Interface Overview

### Main Sections

```
┌─────────────────────────────────────────────────────────────┐
│ [PRESETS] │ 📊 Visualizer │ [GLOBAL] │ [A/B] │ [RANDOMIZE] │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─OSC 1──┐ ┌─OSC 2──┐ ┌─SUB───┐    ┌─FILTER──┐            │
│  │ WAVE │ │ │ WAVE │ │ │ LEVEL │    │ TYPE    │            │
│  │ DETUNE│ │ │ DETUNE│ │ │ OCTAVE│    │ CUTOFF  │            │
│  │ MIX   │ │ │ MIX  │ │ │       │    │ RES     │            │
│  └────────┘ └───────┘ └───────┘    │ ENV     │            │
│                                   │ DRIVE   │            │
│  ┌─ENV 1 (AMP)───────┐           └─────────┘            │
│  │  A   D   S   R    │                                    │
│  └───────────────────┘    ┌─LFO 1──┐ ┌─LFO 2──┐        │
│                            │ RATE   │ │ RATE   │        │
│  ┌─ENV 2 (FLT)───────┐   │ DEPTH  │ │ DEPTH  │        │
│  │  A   D   S   R    │   │ TARGET │ │ TARGET │        │
│  └───────────────────┘   └────────┘ └────────┘        │
│                                                           │
│  ┌─FX────────────────────────────────────────┐          │
│  │ [DIST] [CHORUS] [REVERB] [DELAY] [LIMIT] │          │
│  └───────────────────────────────────────────┘          │
│                                                           │
│  ┌─MACROS──────────────────────────────────┐            │
│  │ [1] [2] [3] [4] [5] [6] [7] [8]          │            │
│  └───────────────────────────────────────────┘            │
└───────────────────────────────────────────────────────────┘
```

---

## Sound Design Basics

### Creating Different Types of Sounds

#### Bass Sounds

1. **Sub Bass**:
   - OSC 1: Sine wave, full level
   - Filter: Lowpass, low cutoff (~200Hz), no resonance
   - ENV 1: Fast attack, medium decay, high sustain
   - Add SUB oscillator at -1 octave for weight

2. **Reese Bass**:
   - OSC 1: Saw wave, slight detune (+5 cents)
   - OSC 2: Saw wave, detune (-10 cents)
   - Filter: Moog type, medium resonance, slight drive
   - ENV 2: Medium attack for filter sweep

3. **Acid Bass**:
   - OSC 1: Square wave, pulse width ~40%
   - Filter: TB-303 type, high resonance (70%+)
   - ENV 2: Fast decay for filter "pluck"
   - ENV 2 Amount: High for dramatic sweeps

#### Lead Sounds

1. **Saw Lead**:
   - OSC 1: Saw wave, full level
   - OSC 2: Saw wave, detune (-7 cents), 50% mix
   - Filter: Lowpass, bright (3-5kHz), low resonance
   - ENV 1: Fast attack, medium decay, medium sustain

2. **Supersaw Lead**:
   - OSC 1: Supersaw type, full level
   - Filter: Lowpass, bright (4-6kHz)
   - FX: Add chorus for width
   - ENV 1: Fast attack, high sustain

#### Pad Sounds

1. **Warm Pad**:
   - OSC 1: Triangle wave, 60% mix
   - OSC 2: Sine wave, detune (+5 cents), 40% mix
   - Filter: Lowpass, medium brightness (2-3kHz)
   - ENV 1: Slow attack (300-500ms), high sustain, long release
   - FX: Add reverb (40-60%)

2. **Evolving Pad**:
   - Use above settings plus:
   - LFO 1: Slow rate (0.5-1Hz), modulating filter
   - LFO 2: Different rate, modulating panning

---

## Oscillators

### Oscillator Types

| Type | Character | Best For |
|------|-----------|----------|
| Sine | Pure, clean | Sub bass, FM, adding clarity |
| Saw | Bright, harmonically rich | Leads, basses, pads |
| Square | Hollow, mid-focused | Retro leads, basses |
| Triangle | Mellow, flute-like | Pads, keys |
| Supersaw | Huge, detuned | Trance leads, massive pads |
| Wavefolder | Crunched, metallic | FX, experimental |
| Phase Distortion | Metallic, bell-like | Keys, plucks |
| Additive | Pure, customizable | Bells, metallic sounds |
| Granular | Textural, noise-like | FX, atmospheres |

### Oscillator Controls

- **Wave Type**: Select the waveform
- **Detune**: Detune in cents (+/- 100 cents = +/- 1 semitone)
- **Mix**: Balance between oscillators
- **Shape**: Pulse width for square, additional shaping for others
- **Phase**: Starting phase position
- **Pan**: Stereo position
- **Sync**: Hard sync OSC 2 to OSC 1 (creates rich harmonics)
- **FM**: Frequency modulation amount from OSC 2 to OSC 1

### Sub Oscillator

- **Level**: Mix amount of sub oscillator
- **Octave**: Transpose by octaves (-1, -2)

---

## Filters

### Filter Types

| Type | Character | Best For |
|------|-----------|----------|
| SVF (State Variable) | Clean, flexible | General purpose |
| Moog Ladder | Warm, smooth | Basses, vintage sounds |
| MS-20 | Aggressive, screaming | Acid, aggressive leads |
| Prophet | Punchy, musical | Poly-style sounds |
| SEM | Smooth, sweet | Pads, gentle filtering |
| TB-303 | Resonant, squelchy | Acid bass |

### Filter Controls

- **Cutoff**: Filter frequency (20Hz - 20kHz)
- **Resonance**: Filter resonance/Q (adds "peak" at cutoff)
- **Drive**: Input drive for saturation
- **Env Amount**: How much ENV 2 affects the filter
- **Key Track**: How much filter follows keyboard (0-36 semitones)
- **Decay**: Filter envelope decay time

### Filter Tips

1. **Self-oscillation**: Crank resonance to 90%+ for the filter to oscillate
2. **Filter sweeps**: Use ENV 2 Amount to create sweeps
3. **Key tracking**: For bright sounds, increase key tracking so higher notes stay bright
4. **Drive**: Add grit by driving the filter input before resonance

---

## Envelopes

### Envelope Controls

Each envelope has:
- **Attack**: Time to reach peak
- **Decay**: Time to reach sustain level
- **Sustain**: Level to maintain while note is held
- **Release**: Time to fade out after note release

### Envelope Curves

- **Linear**: Even rate throughout
- **Exponential**: Fast initial change, slower at end (natural)
- **Logarithmic**: Slow initial change, faster at end

### Envelope Assignments

- **ENV 1**: Amplitude (volume)
- **ENV 2**: Filter cutoff
- **ENV 3**: Available for modulation routing
- **ENV 4**: Available for modulation routing

### Envelope Tips

1. **Percussive sounds**: Fast attack, fast decay, zero sustain
2. **Pad sounds**: Slow attack, high sustain, long release
3. **Pluck sounds**: Instant attack, medium decay, no sustain
4. **Click removal**: Minimum attack is automatically smoothed to prevent clicks

---

## LFOs & Modulation

### LFO Controls

- **Type**: Waveform shape (Sine, Triangle, Saw, Square, S&H, Random)
- **Rate**: LFO speed in Hz
- **Depth**: Modulation amount
- **Target**: What the LFO modulates
- **Sync**: Sync to tempo
- **Sync Rate**: Note division (1/1, 1/2, 1/4, 1/8, 1/16, 1/32)
- **Retrigger**: Restart LFO on each note
- **Phase**: Starting phase

### LFO Targets

1. Pitch (vibrato)
2. Filter Cutoff (wah, sweep)
3. Resonance (resonance pulse)
4. OSC 1 PWM (pulse width mod)
5. OSC 2 PWM
6. Pan (autopan)

### Modulation Tips

1. **Vibrato**: LFO 1 → Pitch, sine wave, 5-7Hz
2. **Wah**: LFO 1 → Filter, triangle, 0.5-2Hz
3. **Tremolo**: LFO 1 → Amplitude (via mod wheel), triangle, 3-8Hz
4. **Random S&H**: LFO → Filter or pitch for organic movement

---

## Effects

### Distortion

- **Amount**: Effect level
- **Type**: Soft Clip, Hard Clip, Fuzz, Bitcrush

### Chorus

- **Amount**: Effect level
- **Rate**: Modulation speed
- **Depth**: Modulation depth
- **Voices**: Number of voices (2-8)

### Reverb

- **Amount**: Effect level (mix)
- **Size**: Room size
- **Decay**: Tail length

### Delay

- **Amount**: Effect level (mix)
- **Time**: Delay time
- **Feedback**: Number of repeats
- **Sync**: Sync to tempo
- **Mode**: Stereo or Ping-Pong

### Limiter

- **Amount**: Output limiting to prevent clipping

### Effect Tips

1. **Chain order**: Distortion → Filter → Chorus → Delay → Reverb → Limiter
2. **Subtle effects**: Keep amounts below 30% for subtle enhancement
3. **Creative effects**: Push amounts higher for sound design
4. **Save CPU**: Disable unused effects

---

## Arpeggiator

### Arpeggiator Modes

- **Up**: Ascending
- **Down**: Descending
- **Up/Down**: Ascending then descending
- **Order**: As played
- **Random**: Random note selection
- **Chord**: All notes at once

### Arpeggiator Controls

- **Rate**: Arpeggio speed (or sync division)
- **Octaves**: Range in octaves (1-4)
- **Gate**: Note length (short-long)
- **Pattern**: Note pattern (see below)
- **Swing**: Shuffle amount
- **Probability**: Note repeat probability

### Pattern Editor

Create custom patterns in the pattern grid:
- **Click**: Add/remove step
- **Right Click**: Edit step velocity
- **Ctrl+Click**: Tie notes

### Arpeggiator Tips

1. **Melodic arpeggios**: Use Up mode with 2-3 octaves
2. **Bass patterns**: Use Order mode for specific bass lines
3. **Random inspiration**: Use Random mode with probability < 100%
4. **Ratchet effects**: Use probability to create note repeats

---

## Presets

### Preset Browser

The preset browser includes:
- **Search**: Find presets by name, tag, or category
- **Categories**: Filter by sound type (Bass, Lead, Pad, etc.)
- **Favorites**: Star your favorite presets
- **Recently Used**: Quick access to recent presets

### Saving Presets

1. Click "Save" in the preset browser
2. Enter a name
3. Add tags (optional)
4. Select category (optional)
5. Add description (optional)

### Factory Presets

Zenith includes **600+ factory presets** covering:

- **100** Bass presets (Sub, Reese, Acid, Funk, etc.)
- **100** Lead presets (Saw, Square, Supersaw, Trance, etc.)
- **80** Pad presets (Ambient, Warm, Evolving, String, Choir)
- **80** Pluck presets (Classic, E. Piano, Clav, Synth, Metallic)
- **50** Keys presets (Organ, Piano, Clav, Synth)
- **40** FX presets (Hits, Rises, Falls, Atmospheres, Noise)
- **30** Sequencer presets (Arps, Basslines)
- **20** Drum presets (Kick, Snare, Hi-Hat, Perc)
- **20** Vocal presets (Vocoder, Choir)
- **30** Ambient presets (Drone, Texture)
- **50** Experimental presets (Glitch, FM, Wavetable)

### A/B Comparison

Compare two different sounds:
1. Click "A" to capture state A
2. Make changes to the sound
3. Click "B" to capture state B
4. Use the A/B button to switch between them
5. Use the morph slider to blend between A and B

---

## Advanced Features

### Modulation Matrix

Route any modulation source to any destination:
1. Click "Mod" button
2. Select source (LFO 1, LFO 2, ENV 3, ENV 4, Mod Wheel, etc.)
3. Select destination (any parameter)
4. Adjust amount

### Macros

Create custom controls:
1. Right-click any parameter → "Assign to Macro"
2. Select macro 1-8
3. Use the macro knob to control all assigned parameters
4. Assign multiple parameters for complex transformations

### Randomization

Create variations:
1. Click "Randomize" for complete randomization
2. Lock parameters you want to keep
3. Use the amount slider to control randomization depth
4. "Randomize" again for new variations

### MIDI Learn

1. Right-click any parameter
2. Select "MIDI Learn"
3. Move a controller on your MIDI keyboard
4. Parameter is now assigned to that controller

### Automation

Automation can be recorded:
1. Enable automation recording in your DAW
2. Automate any parameter
3. Parameters show automation indicator when automated

---

## Tips & Tricks

### Sound Design Tips

1. **Start simple**: Begin with Init preset, add elements gradually
2. **Use your ears**: Trust what you hear over numerical values
3. **Less is more**: Subtle modulation often works best
4. **Layer presets**: Combine elements from multiple presets
5. **Save variations**: Save versions of sounds you like

### Performance Tips

1. **Reduce voices**: Lower polyphony if CPU is high
2. **Disable FX**: Turn off unused effects
3. **Lower quality**: Reduce oversampling in settings if needed
4. **Use presets**: Factory presets are CPU-optimized

### Workflow Tips

1. **Keyboard shortcuts**: Learn key commands for your DAW
2. **Templates**: Save DAW templates with Zenith
3. **Favorites**: Star presets for quick access
4. **Recent presets**: Use recently used for workflow
5. **A/B testing**: Always A/B test your sounds with others

### Troubleshooting

| Problem | Solution |
|---------|----------|
| No sound | Check audio device, MIDI routing, master volume |
| Clicks on notes | Enable "Click-Free Envelopes" in settings |
| Too much CPU | Reduce polyphony, disable oversampling |
| Sound too bright | Lower filter cutoff, add high-cut |
| Sound too thin | Add detune, enable sub oscillator, add chorus |
| No modulation | Check routing, LFO rate, depth amount |

---

## Quick Reference

### Default MIDI CC Assignments

| CC | Parameter |
|----|-----------|
| 1 | Mod Wheel |
| 2 | Breath ( assignable) |
| 5 | Portamento Time |
| 64 | Sustain Pedal |
| 65 | Portamento |
| 74 | Filter Cutoff |
| 71 | Resonance |
| 76 | Vibrato Rate |

### Keyboard Shortcuts (in Plugin)

| Key | Function |
|-----|----------|
| Cmd/Ctrl+S | Save preset |
| Cmd/Ctrl+Z | Undo |
| Cmd/Ctrl+Shift+Z | Redo |
| Cmd/Ctrl+N | New preset (Init) |
| Cmd/Ctrl+F | Focus search |
| Space | Preview sound (if supported) |

---

## Support

For more information:
- Check the online documentation
- Watch tutorial videos
- Join the community forum
- Report bugs via GitHub issues

**Version**: 1.0.0
**Last Updated**: 2025
