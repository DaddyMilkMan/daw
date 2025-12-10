/*
  ==============================================================================

    GlobalLFO.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Global Low Frequency Oscillator for modulation.
    Supports tempo-sync and multiple waveforms.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>
#include <random>

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
    // Initialize random generator
    std::random_device rd;
    randomGen_ = std::mt19937(rd());
    randomDist_ = std::uniform_real_distribution<float>(-1.0f, 1.0f);
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

  // Tempo-sync division (e.g., 1.0 = quarter note, 0.5 = eighth note)
  void setSyncDivision(float division) { syncDivision_.store(division); }
  float getSyncDivision() const { return syncDivision_.load(); }

  void setPhaseOffset(float offset) { phaseOffset_.store(offset); }
  float getPhaseOffset() const { return phaseOffset_.load(); }

  void setAmplitude(float amp) { amplitude_.store(amp); }
  float getAmplitude() const { return amplitude_.load(); }

  //==========================================================================
  // Audio Thread Interface
  //==========================================================================

  void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }

  // Process one block, advancing the LFO phase
  // Returns the current value at the END of the block
  float process(int numSamples, double tempo = 120.0) {
    if (sampleRate_ <= 0.0)
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

    // Advance phase
    double phaseIncrement = freq * numSamples / sampleRate_;
    phase_ += phaseIncrement;

    // Wrap phase
    while (phase_ >= 1.0) {
      phase_ -= 1.0;
      // Update S&H on wrap
      if (static_cast<Waveform>(waveform_.load()) == Waveform::Random) {
        lastRandomValue_ = randomDist_(randomGen_);
      }
    }

    // Calculate output value
    float value = calculateValue(phase_ + phaseOffset_.load());
    value *= amplitude_.load();

    currentValue_.store(value);
    return value;
  }

  // Get current value (RT-safe, lock-free)
  float getValue() const { return currentValue_.load(); }

  // Reset phase (e.g., on transport start)
  void reset() {
    phase_ = 0.0;
    lastRandomValue_ = 0.0f;
    currentValue_.store(0.0f);
  }

private:
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
      return lastRandomValue_;

    default:
      return 0.0f;
    }
  }

  //==========================================================================
  // State
  //==========================================================================

  double sampleRate_ = 48000.0;
  double phase_ = 0.0;
  float lastRandomValue_ = 0.0f;

  // Random generator for S&H
  mutable std::mt19937 randomGen_;
  mutable std::uniform_real_distribution<float> randomDist_;

  // Atomic parameters (thread-safe)
  std::atomic<int> waveform_{static_cast<int>(Waveform::Sine)};
  std::atomic<float> frequency_{1.0f}; // Hz (when not tempo-synced)
  std::atomic<bool> tempoSync_{false};
  std::atomic<float> syncDivision_{1.0f}; // 1.0 = quarter note
  std::atomic<float> phaseOffset_{0.0f};  // 0.0 - 1.0
  std::atomic<float> amplitude_{1.0f};    // Output scaling

  std::atomic<float> currentValue_{0.0f}; // Latest output value
};

} // namespace dsp
} // namespace zenith
