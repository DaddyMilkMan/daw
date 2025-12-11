/*
  ==============================================================================

    GlobalLFO.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Global Low Frequency Oscillator for modulation.
    Supports tempo-sync and multiple waveforms.

    RT-SAFETY: All operations are lock-free and allocation-free.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>
#include <juce_core/juce_core.h>

namespace zenith {
namespace dsp {

class GlobalLFO {
public:
  enum class Waveform {
    Sine,
    Triangle,
    Saw,
    Square,
    Random // Sample & Hold
  };

  GlobalLFO() {
    // Seed RT-safe RNG with address-based entropy
    rngState_.store(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this) ^ 0xDEADBEEF));
  }

  //==========================================================================
  // Configuration (Message Thread)
  //==========================================================================

  void setWaveform(Waveform wf) { waveform_.store(static_cast<int>(wf)); }
  Waveform getWaveform() const {
    return static_cast<Waveform>(waveform_.load());
  }

  void setFrequency(float hz) { frequency_.store(hz); }
  float getFrequency() const { return frequency_.load(); }

  void setTempoSync(bool sync) { tempoSync_.store(sync); }
  bool isTempoSync() const { return tempoSync_.load(); }

  // Tempo-sync division (e.g., 1.0 = quarter note, 0.5 = eighth note, 4.0 =
  // whole note)
  void setSyncDivision(float division) { syncDivision_.store(division); }
  float getSyncDivision() const { return syncDivision_.load(); }

  void setPhaseOffset(float offset) {
    phaseOffset_.store(juce::jlimit(0.0f, 1.0f, offset));
  }
  float getPhaseOffset() const { return phaseOffset_.load(); }

  void setAmplitude(float amp) { amplitude_.store(amp); }
  float getAmplitude() const { return amplitude_.load(); }

  // Retrigger on transport start
  void setRetrigger(bool retrig) { retrigger_.store(retrig); }
  bool getRetrigger() const { return retrigger_.load(); }

  //==========================================================================
  // Audio Thread Interface
  //==========================================================================

  void setSampleRate(double sampleRate) {
    sampleRate_.store(static_cast<float>(sampleRate));
  }

  // Process one block, advancing the LFO phase
  // Returns the current value at the END of the block
  float process(int numSamples, double tempo = 120.0) {
    float sr = sampleRate_.load();
    if (sr <= 0.0f)
      return 0.0f;

    float freq = frequency_.load();

    // Tempo sync: convert division to frequency
    if (tempoSync_.load() && tempo > 0.0) {
      // syncDivision_ of 1.0 = quarter note = 1 beat
      // At 120 BPM, quarter note duration = 0.5 seconds
      // Frequency = tempo / 60.0 / syncDivision
      float division = syncDivision_.load();
      if (division > 0.0f) {
        freq = static_cast<float>(tempo / 60.0) / division;
      }
    }

    // Load current phase (atomic)
    float currentPhase = phase_.load();

    // Advance phase
    float phaseIncrement = freq * static_cast<float>(numSamples) / sr;
    float newPhase = currentPhase + phaseIncrement;

    // Check for phase wrap (for S&H update)
    bool wrapped = (newPhase >= 1.0f);

    // Wrap phase
    while (newPhase >= 1.0f) {
      newPhase -= 1.0f;
    }

    // Update S&H on wrap using RT-safe RNG
    if (wrapped &&
        static_cast<Waveform>(waveform_.load()) == Waveform::Random) {
      lastRandomValue_.store(generateRandomFloat());
    }

    // Store new phase (atomic)
    phase_.store(newPhase);

    // Calculate output value
    float offset = phaseOffset_.load();
    float value = calculateValue(newPhase + offset);
    value *= amplitude_.load();

    currentValue_.store(value);
    return value;
  }

  // Get current value (RT-safe, lock-free)
  float getValue() const { return currentValue_.load(); }

  // Reset phase (e.g., on transport start)
  void reset() {
    phase_.store(0.0f);
    lastRandomValue_.store(0.0f);
    currentValue_.store(0.0f);
  }

  // Call when transport starts (if retrigger enabled)
  void onTransportStart() {
    if (retrigger_.load()) {
      reset();
    }
  }

private:
  //==========================================================================
  // RT-Safe Linear Congruential Generator
  // Constants from Numerical Recipes (fast, deterministic, no allocation)
  //==========================================================================
  float generateRandomFloat() {
    uint32_t state = rngState_.load();
    state = state * 1664525u + 1013904223u; // LCG step
    rngState_.store(state);
    // Convert to float in range [-1, 1]
    return (static_cast<float>(state) / static_cast<float>(0xFFFFFFFFu)) *
               2.0f -
           1.0f;
  }

  float calculateValue(double phase) const {
    // Normalize phase to 0-1
    phase = phase - std::floor(phase);

    Waveform wf = static_cast<Waveform>(waveform_.load());

    switch (wf) {
    case Waveform::Sine:
      return static_cast<float>(
          std::sin(phase * 2.0 * juce::MathConstants<double>::pi));

    case Waveform::Triangle: {
      // Rising 0-0.5, falling 0.5-1.0
      if (phase < 0.5)
        return static_cast<float>(phase * 4.0 - 1.0);
      else
        return static_cast<float>(3.0 - phase * 4.0);
    }

    case Waveform::Saw:
      // Rising sawtooth: -1 to 1
      return static_cast<float>(phase * 2.0 - 1.0);

    case Waveform::Square:
      return phase < 0.5 ? 1.0f : -1.0f;

    case Waveform::Random:
      // Sample & Hold - value changes on phase wrap
      return lastRandomValue_.load();

    default:
      return 0.0f;
    }
  }

  //==========================================================================
  // State (All Atomic for Thread Safety)
  //==========================================================================

  std::atomic<float> sampleRate_{0.0f};
  std::atomic<float> phase_{0.0f};
  std::atomic<float> lastRandomValue_{0.0f};
  std::atomic<uint32_t> rngState_{0x12345678}; // RT-safe RNG state

  // Atomic parameters (thread-safe)
  std::atomic<int> waveform_{static_cast<int>(Waveform::Sine)};
  std::atomic<float> frequency_{1.0f}; // Hz (when not tempo-synced)
  std::atomic<bool> tempoSync_{false};
  std::atomic<float> syncDivision_{1.0f}; // 1.0 = quarter note
  std::atomic<float> phaseOffset_{0.0f};  // 0.0 - 1.0
  std::atomic<float> amplitude_{1.0f};    // Output scaling
  std::atomic<bool> retrigger_{true};     // Retrigger on transport

  std::atomic<float> currentValue_{0.0f}; // Latest output value
};

} // namespace dsp
} // namespace zenith
