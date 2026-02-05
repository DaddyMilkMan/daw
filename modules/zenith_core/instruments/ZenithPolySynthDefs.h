/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    ZenithPolySynthDefs.h
    Created: 2025-12-06
    Author:  Zenith DAW

    Shared definitions, enums, and structures for ZenithPolySynth.

  ==============================================================================

*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
// Enum Definitions
//==============================================================================

/**
    Waveform types for oscillators
*/
enum class OscillatorWaveform {
  Sine = 0,
  Saw,
  Square,
  Triangle,
  Noise,
  Supersaw,
  Wavetable,
  Wavefolder,
  PhaseDist,
  Additive,
  Granular,
  NumWaveforms
};

/**
    Filter types
*/
enum class FilterType { Lowpass = 0, Bandpass, Highpass, NumTypes };

/**
    Filter model types
*/
enum class FilterModelType { 
    SVF = 0, 
    Ladder,
    MoogLadder,
    MS20,
    Prophet,
    SEM,
    TB303,
    NumModels
};

/**
    Quality preset for CPU optimization
*/
enum class QualityPreset { Low = 0, Medium, High, NumPresets };

/**
    LFO waveform shapes
*/
enum class LFOWaveform {
  Sine = 0,
  Triangle,
  Saw,
  Square,
  SampleAndHold,
  NumWaveforms
};

/**
    LFO target parameters (legacy - now part of modulation matrix)
*/
enum class LFOTarget {
  FilterCutoff = 0,
  Osc1Pitch,
  Osc2Pitch,
  Osc1Mix,
  Osc2Mix,
  AmpGain,
  Osc1Shape,
  NumTargets
};

/**
    Filter key tracking modes
*/
enum class FilterKeyTrack {
  Off = 0,
  Half, // 50% tracking
  Full, // 100% tracking
  NumModes
};

/**
    Rhythmic sync rates
*/
enum class SyncRate {
  Free = 0, // Frequency in Hz
  _1_64,
  _1_32,
  _1_16,
  _1_8,
  _1_4,
  _1_2,
  _1_1,
  _2_1,
  _4_1,
  NumRates
};

//==============================================================================
// Modulation System
//==============================================================================

/**
    Modulation sources available in the matrix
*/
enum class ModulationSource {
  None = 0,   // No modulation
  LFO1,       // Low-frequency oscillator 1 (sine wave, -1 to +1)
  LFO2,       // Low-frequency oscillator 2 (sine wave, -1 to +1)
  StepLFO1,   // Step sequencer LFO 1 (-1 to +1)
  StepLFO2,   // Step sequencer LFO 2 (-1 to +1)
  StepLFO3,   // Step sequencer LFO 3 (-1 to +1)
  StepLFO4,   // Step sequencer LFO 4 (-1 to +1)
  Env1,       // Amplitude envelope (0 to 1, ADSR)
  Env2,       // Modulation envelope (0 to 1, ADSR)
  Velocity,   // Note-on velocity (0 to 1)
  ModWheel,   // MIDI mod wheel CC#1 (0 to 1)
  Aftertouch, // MIDI channel pressure (0 to 1)
  Timbre,     // MPE Y-axis (CC#74) (0 to 1)
  Arp,        // Arpeggiator gate (0 to 1)
  NumSources
};

/**
    Modulation destinations available in the matrix
*/
enum class ModulationDestination {
  None = 0,
  Osc1Pitch,
  Osc2Pitch,
  Osc3Pitch,
  Osc1Mix,
  Osc2Mix,
  Osc3Mix,
  FilterCutoff,
  FilterResonance,
  AmpGain,
  Osc1Shape,
  Osc2Shape,
  Osc3Shape,
  LFO1Rate,
  LFO2Rate,
  StepLFO1Rate,
  StepLFO2Rate,
  StepLFO3Rate,
  StepLFO4Rate,
  UnisonDetune,
  UnisonSpread,
  NumDestinations
};

/**
    Single modulation routing slot
*/
struct ModulationSlot {
  ModulationSource source = ModulationSource::None;
  ModulationDestination destination = ModulationDestination::None;
  float amount = 0.0f; // Modulation depth/amount (-1 to +1)

  bool isActive() const {
    return source != ModulationSource::None &&
           destination != ModulationDestination::None;
  }
};

/**
    RT-safe modulation state per voice
*/
struct ModulationState {
  // Pre-computed modulation amounts for each destination
  juce::Array<float> values;

  ModulationState() {
    values.resize(static_cast<int>(ModulationDestination::NumDestinations));
    reset();
  }

  void reset() { values.fill(0.0f); }

  float get(ModulationDestination dest) const {
    return values[static_cast<int>(dest)];
  }

  void set(ModulationDestination dest, float value) {
    values.set(static_cast<int>(dest), value);
  }

  void add(ModulationDestination dest, float value) {
    values.set(static_cast<int>(dest), get(dest) + value);
  }
};

} // namespace zenith
