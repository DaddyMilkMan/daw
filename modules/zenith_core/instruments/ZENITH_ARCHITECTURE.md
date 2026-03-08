# Zenith Professional Synthesizer - Architecture Documentation

## Overview

Zenith is a professional-grade synthesizer designed to compete with Xfer Serum and Vital. The architecture is built around these core principles:

1. **Real-Time Safety**: All audio path code is RT-safe (no allocations, no blocking)
2. **Modulation First**: Deep modulation matrix with 32 slots and macro controls
3. **Professional DSP**: Oversampling, PolyBLEP anti-aliasing, circuit-emulated filters
4. **Sonic Flexibility**: 3 oscillators with sync, FM, ring mod, and wavetables

## Module Structure

```
modules/zenith_core/instruments/
├── ZenithPolySynthDefs.h       # Enums and type definitions
├── ZenithOscillator.h/.cpp     # Professional oscillator with PolyBLEP
├── ZenithFilter.h/.cpp          # Multi-model filters
├── ZenithPolySynthVoice.h/.cpp # MPE voice implementation
├── ZenithModulationMatrix.h/.cpp # 32-slot modulation system
├── ZenithEffects.h/.cpp         # Effects chain (reverb, delay, etc.)
├── WavetableData.h             # Wavetable data structures
├── WavetableLoader.h/.cpp      # Wavetable file loading
└── CMakeLists.txt              # Build configuration
```

## Oscillator Features

- **16-voice unison** with symmetric stereo spread (Serum standard)
- **Per-oscillator oversampling**: Clean (1x), Good (2x), Ultra (4x), Extreme (8x)
- **PolyBLEP anti-aliasing** for saw, square, triangle waves
- **Hard sync** with BLEP correction for clean transients
- **Analog drift** simulation for vintage warmth
- **Wavetable playback** with MIP mapping and frame interpolation

## Filter Models

1. **SVF** (State Variable Filter) - Clean, precise
2. **Moog Ladder** - Warm, musical 4-pole
3. **Korg MS-20** - Aggressive resonance
4. **Oberheim SEM** - Smooth 2-pole character
5. **Roland TB-303** - Acid resonance

All filters support:
- Oversampling (2x, 4x, 8x) for anti-aliasing
- Key tracking (Off, Half, Full)
- Drive with soft clipping
- Serial or parallel routing

## Modulation Matrix

- **32 modulation slots** (expandable from 8)
- **8 sources**: LFO1/2, 4 Step LFOs, 2 Envelopes, Velocity, ModWheel, Aftertouch, PitchBend, Timbre, Note, Macros
- **Curving options**: Linear, Concave, Convex
- **Range control**: Min/max scaling per slot

## Effects Chain

Process order: Reverb → Delay → Chorus → Phaser → Distortion → Compressor → Limiter

- **Reverb**: JUCE reverb with predelay, room size, damping
- **Delay**: Ping-pong stereo delay with BPM sync
- **Chorus**: 8-voice ensemble with stereo spread
- **Phaser**: 2-12 stage allpass with feedback
- **Distortion**: 7 algorithms including soft/hard clip, bitcrush, wavefold
- **Compressor**: RMS detector with soft knee, auto-makeup
- **Limiter**: Brickwall lookahead limiter (safety)

## Voice Architecture

Each voice (ZenithPolySynthVoice) contains:
- 3 oscillators with individual wavetable, shape, mix, detune
- Oscillator 2: Sync to OSC1, FM from OSC1, Ring Mod (OSC1 × OSC2)
- 2 filters with serial/parallel routing
- 2 ADSR envelopes (amp + mod)
- 2 LFOs with 6 waveforms each
- Full MPE support (pressure, pitchbend, timbre)

## RT-Safe Design Rules

1. No heap allocations in audio thread
2. All memory pre-allocated or using fixed-size containers
3. No blocking I/O in render path
4. Use SmoothedValue for parameter smoothing
5. Atomic operations for cross-thread communication

## Sample Rate Support

All components support variable sample rates from 44.1kHz to 192kHz.

## Building

```cmake
# In main CMakeLists.txt
add_subdirectory(modules/zenith_core/instruments)
target_link_libraries(YourApp PRIVATE ZenithInstruments)
```

## Quality Targets

| Feature | Zenith | Serum | Vital |
|---------|----------|--------|-------|
| Max Unison Voices | 16 | 16 | 16 |
| Oversampling Options | 4 | 4 | 4 |
| Filter Models | 5 | 3 | 5+ |
| Modulation Slots | 32 | 4 (×3 modulatable) | 8+ |
| MPE Support | Full | Limited | Full |
| Wavetable Import | Professional | Yes | Yes |

## License

GNU Affero General Public License v3.0

Copyright (c) 2025 Micah Cooley <micahcooley@protonmail.com>
