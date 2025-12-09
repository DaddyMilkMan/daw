/*
  ==============================================================================

    ZenithPolySynthDefs.h
    Created: 2025-12-06
    Author:  Zenith DAW

    Shared definitions, enums, and structures for ZenithPolySynth.

  ==============================================================================
*/

#pragma once

#include <array>
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
  NumWaveforms
};

/**
    Filter types
*/
enum class FilterType { Lowpass = 0, Bandpass, Highpass, NumTypes };

/**
    Quality preset for CPU optimization
*/
enum class QualityPreset {
  Low = 0, // Max 3 unison voices, optimized for CPU
  Medium,  // Max 5 unison voices, balanced
  High,    // Max 7 unison voices, full quality
  NumPresets
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
  NumTargets
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
  Env1,       // Amplitude envelope (0 to 1, ADSR)
  Env2,       // Modulation envelope (0 to 1, ADSR)
  Velocity,   // Note-on velocity (0 to 1)
  ModWheel,   // MIDI mod wheel CC#1 (0 to 1)
  Aftertouch, // MIDI channel pressure (0 to 1)
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
  std::array<float, static_cast<size_t>(ModulationDestination::NumDestinations)>
      values;

  ModulationState() { reset(); }

  void reset() { values.fill(0.0f); }

  float get(ModulationDestination dest) const {
    return values[static_cast<size_t>(dest)];
  }

  void set(ModulationDestination dest, float value) {
    values[static_cast<size_t>(dest)] = value;
  }

  void add(ModulationDestination dest, float value) {
    values[static_cast<size_t>(dest)] += value;
  }
};

} // namespace zenith
