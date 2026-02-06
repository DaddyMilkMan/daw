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

#pragma once

#include "WavetableData.h"
#include "ZenithPolySynthDefs.h"
#include "ZenithAdvancedOscillators.h"
#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

// Forward declaration
class Wavetable;

/**
    Single oscillator with multiple waveforms, wavetables, and detune
    Now with professional-grade advanced oscillators.
*/
class ZenithOscillator {
public:
  ZenithOscillator() {
    // Initialize advanced oscillators
    advancedEngine_.prepare(44100.0);
  }

  void setWaveform(OscillatorWaveform waveform) { waveform_ = waveform; }
  OscillatorWaveform getWaveform() const { return waveform_; }
  void setDetune(float detuneCents);
  void setSampleRate(double sampleRate) { 
    sampleRate_ = sampleRate; 
    advancedEngine_.prepare(sampleRate);
  }
  void reset() { 
    phase_ = 0.0; 
    advancedEngine_.reset();
  }
  void randomizePhase() { phase_ = random_.nextFloat(); }

  /**
   * @brief Generate next sample
   * @param frequency Base frequency in Hz
   * @param shape Shape parameter (Pulse Width for Square, etc.)
   * @return Sample value in range [-1, 1]
   */
  float getNextSample(float frequency, float shape = 0.5f);

  /**
   * @brief Update supersaw frequency ratios after detune change
   * Must be called after setDetune() to update cached ratios.
   */
  void updateSupersawRatios();

  // Flagship Features
  void setSync(bool enabled) { syncEnabled_ = enabled; }
  void resetPhase() { phase_ = 0.0; }
  double getPhase() const { return phase_; }
  void reducePhase(double amount) {
    phase_ -= amount;
  } // For adjusting phase after sync reset

  // Wavetable management (Pro Upgrade)
  void setWavetable(const Wavetable *wt) { wavetable_ = wt; }
  const Wavetable *getWavetable() const { return wavetable_; }
  bool hasWavetable() const {
    return wavetable_ != nullptr && wavetable_->isValid();
  }
  
  // Access to advanced oscillators
  AdvancedOscillatorEngine& getAdvancedEngine() { return advancedEngine_; }

private:
  OscillatorWaveform waveform_ = OscillatorWaveform::Saw;
  double phase_ = 0.0;
  double sampleRate_ = 44100.0;
  float detuneCents_ = 0.0f;
  float lastTriangleValue_ = 0.0f;

  // Flagship State
  bool syncEnabled_ = false;

  // Wavetable State (Pro Upgrade)
  const Wavetable *wavetable_ =
      nullptr;                     // Non-owning pointer to loaded wavetable
  float lastWavetableFreq_ = 0.0f; // For MIP level calculation
  
  // Advanced oscillator engine
  AdvancedOscillatorEngine advancedEngine_;

  float processSine(float frequency);
  float processSaw(float frequency);
  float processSquare(float frequency, float pulseWidth);
  float processTriangle(float frequency);
  float processNoise();
  float processSupersaw(float frequency);
  float processWavetable(float frequency, float shape);
  float processRealWavetable(float frequency,
                              float shape); // NEW: Real wavetable playback
  float processWavefolder(float frequency);
  float processPhaseDist(float frequency);
  float processAdditive(float frequency);
  float processGranular(float frequency);

  // Supersaw state
  std::array<double, 7> supersawPhases_ = {0.0};
  std::array<float, 7> supersawDetunes_ = {0.0f};
  std::array<float, 7> supersawRatios_ = {
      1.0f}; // Precalculated frequency multipliers
  bool supersawInit_ = false;

  // Random number generator for noise and phase randomization
  juce::Random random_;

  // PolyBLEP anti-aliasing helper
  // t: current phase (0..1)
  // dt: phase increment per sample
  inline float poly_blep(float t, float dt) {
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
      t = (t - 1.0f) / dt;
      return t * t + t + t + 1.0f;
    }
    return 0.0f;
  }
};

} // namespace zenith
