/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include <cstdint>

namespace zenith {

//==============================================================================
// OSCILLATOR WAVEFORMS
//==============================================================================
enum class OscillatorWaveform {
    Saw,        ///< Standard sawtooth wave (rising ramp)
    Square,      ///< Square wave (50% duty)
    Triangle,    ///< Triangle wave
    Sine,        ///< Sine wave
    Wavetable,   ///< User wavetable
    Noise        ///< White noise
};

//==============================================================================
// NOISE COLOR TYPES
//==============================================================================
enum class NoiseColor {
    White,       ///< Flat spectrum, equal energy per Hz
    Pink,         ///< -3dB/octave slope, natural sounding
    Brown,         ///< -6dB/octave slope, warmer and deeper
    Sampled        ///< Use sampled noise buffer
};

//==============================================================================
// ENVELOPE CURVE TYPES
//==============================================================================
enum class EnvelopeCurve {
    Linear,        ///< Linear transitions
    Exponential,    ///< Exponential attack/decay
    Logarithmic,    ///< Logarithmic decay
    TCurve,          ///< T-shaped curve (slow attack)
    SCurve,          ///< S-shaped curve (slow release)
    Step           ///< Instant attack
    Slow            ///< Slow attack/release
};

//==============================================================================
// OSCILLATOR OVERSAMPLING QUALITY
//==============================================================================
/**
 * Per-oscillator oversampling quality matching Xfer Serum
 */
enum class OscillatorOversamplingQuality {
    Clean = 1,   ///< 1x (no oversampling)
    Good = 2,     ///< 2x oversampling
    Ultra = 4,    ///< 4x oversampling
    Extreme = 8   ///< 8x oversampling
};

//==============================================================================
// FILTER TYPES
//==============================================================================
enum class FilterType {
    LowPass,
    HighPass,
    BandPass,
    Notch,
    Resonant,
    // Professional modes
    MoogLadder,
    MS20,
    Proportion,
    SEM,
    TB303
};

//==============================================================================
// FILTER MODEL TYPE
//==============================================================================
enum class FilterModelType {
    SVF = 0,        ///< State Variable Filter
    Moog = 1,       ///< Moog Ladder emulation
    MS20 = 2,        ///< Korg MS-20 emulation
    SEM = 3,         ///< Oberheim SEM emulation
    TB303 = 4,       ///< Roland TB-303 emulation
    DiodeLadder = 5  ///< Diode ladder filter
};

//==============================================================================
// FILTER KEY TRACKING
//==============================================================================
enum class FilterKeyTracking {
    Off = 0,       ///< No key tracking
    Half = 1,       ///< 50% key tracking
    Full = 2         ///< 100% key tracking (filter tracks pitch 1:1)
};

//==============================================================================
// QUALITY PRESETS
//==============================================================================
enum class QualityPreset {
    Clean,     ///< Lowest CPU, basic quality
    Medium,    ///< Balanced quality/CPU
    High,       ///< High quality with oversampling
    Ultra       ///< Maximum quality, highest CPU
};

//==============================================================================
// LFO WAVEFORMS
//==============================================================================
enum class LFOWaveform {
    Sine,
    Triangle,
    Square,
    Saw,
    SampleAndHold,
    Random
};

//==============================================================================
// LFO TARGETS
//==============================================================================
enum class LFOTarget {
    None = 0,
    Osc1Pitch,
    Osc2Pitch,
    Osc3Pitch,
    Osc1Mix,
    Osc2Mix,
    Osc3Mix,
    FilterCutoff,
    FilterResonance,
    Osc1PulseWidth,
    Osc2PulseWidth,
    Osc3PulseWidth
};

//==============================================================================
// SYNC RATES
//==============================================================================
enum class SyncRate {
    _1_32 = 0,
    _1_16 = 1,
    _1_8 = 2,
    _1_4 = 3,
    _1_2 = 4,
    _1_1 = 5,
    _2_1 = 6,
    _4_1 = 7,
    _8_1 = 8
};

//==============================================================================
// MODULATION SOURCE
//==============================================================================
enum class ModulationSource {
    None = 0,
    LFO1,
    LFO2,
    StepLFO1,
    StepLFO2,
    StepLFO3,
    StepLFO4,
    ModEnvelope,
    AmpEnvelope,
    Velocity,
    ModWheel,
    Aftertouch,
    PitchBend,
    Timbre,
    NoteNumber
};

//==============================================================================
// MODULATION DESTINATION
//==============================================================================
enum class ModulationDestination {
    None = 0,
    Osc1Pitch,
    Osc2Pitch,
    Osc3Pitch,
    Osc1Mix,
    Osc2Mix,
    Osc3Mix,
    Osc1PulseWidth,
    Osc2PulseWidth,
    Osc3PulseWidth,
    Osc1Shape,
    Osc2Shape,
    Osc3Shape,
    FilterCutoff,
    FilterResonance,
    FilterDrive,
    Filter2Cutoff,
    Filter2Resonance,
    GlobalTune,
    UnisonDetune,
    Pan
};

//==============================================================================
// AFTERTOUCH CURVE
//==============================================================================
enum class AftertouchCurve {
    Linear,         ///< Linear response
    Exponential,     ///< Exponential response
    Logarithmic,     ///< Logarithmic response
    TCurve,          ///< T-shaped curve
    SCurve           ///< S-shaped curve
};

//==============================================================================
// MODULATION SLOT
//==============================================================================
struct ModulationSlot {
    ModulationSource source = ModulationSource::None;
    ModulationDestination destination = ModulationDestination::None;
    float amount = 0.0f;
    bool active = false;

    ModulationSlot() = default;
    ModulationSlot(ModulationSource s, ModulationDestination d, float a)
        : source(s), destination(d), amount(a), active(a != 0.0f) {}
};

//==============================================================================
// MODULATION STATE
//==============================================================================
struct ModulationState {
    float lfo1 = 0.0f;
    float lfo2 = 0.0f;
    float modEnv = 0.0f;
    float ampEnv = 0.0f;
    float velocity = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float pitchBend = 0.0f;
    float timbre = 0.0f;
    float noteNumber = 0.0f;
};

//==============================================================================
// ARPEGGIATOR PATTERN MODES
//==============================================================================
enum class PatternMode {
    Forward,       ///< Play forward through steps
    Reverse,       ///< Play backward through steps
    Alt,           ///< Alternating forward/reverse
    Random,         ///< Random step order
    Chord,          ///< Play all notes at once
    AsPlayed,       ///< Preserve note input order
    Order            ///< Custom step order
};

//==============================================================================
// ARPEGGIATOR CONSTANTS
//==============================================================================
static constexpr int MAX_PATTERNS = 8;

} // namespace zenith
