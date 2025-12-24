/*
  ==============================================================================

    ZenithOscillator.h
    Created: 2025-12-06
    Refactored: 2025-12-20 (Pro Wavetable Update)
    Author:  Zenith DAW

    Oscillator component for ZenithPolySynth.
    Now includes REAL wavetable support with MIP-mapping.

  ==============================================================================
*/

#pragma once

#include "WavetableData.h"
#include "ZenithPolySynthDefs.h"
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
*/
class ZenithOscillator {
public:
  ZenithOscillator() = default;

  void setWaveform(OscillatorWaveform waveform) { waveform_ = waveform; }
  OscillatorWaveform getWaveform() const { return waveform_; }
  void setDetune(float detuneCents);
  void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
  void reset() { phase_ = 0.0; }
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

  float processSine(float frequency);
  float processSaw(float frequency);
  float processSquare(float frequency, float pulseWidth);
  float processTriangle(float frequency);
  float processNoise();
  float processSupersaw(float frequency);
  float processWavetable(float frequency, float shape);
  float processRealWavetable(float frequency,
                             float shape); // NEW: Real wavetable playback

  // Supersaw state
  std::array<double, 7> supersawPhases_ = {0.0};
  std::array<float, 7> supersawDetunes_ = {0.0f};
  std::array<float, 7> supersawRatios_ = {
      1.0f}; // Precalculated frequency multipliers
  bool supersawInit_ = false;

  // Random number generator for noise and phase randomization
  juce::Random random_;

public:
  // Wavetable management (Pro Upgrade)
  void setWavetable(const Wavetable *wt) { wavetable_ = wt; }
  const Wavetable *getWavetable() const { return wavetable_; }
  bool hasWavetable() const {
    return wavetable_ != nullptr && wavetable_->isValid();
  }

private:
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
