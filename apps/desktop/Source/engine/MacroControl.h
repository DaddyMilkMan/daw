/*
  ==============================================================================

    MacroControl.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Macro Control for user-driven modulation.
    Simple float value (0.0-1.0) with optional smoothing.

  ==============================================================================
*/

#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>

namespace zenith {

class MacroControl {
public:
  MacroControl() = default;
  explicit MacroControl(const juce::String &name) : name_(name) {}

  //==========================================================================
  // Configuration (Message Thread)
  //==========================================================================

  void setName(const juce::String &name) { name_ = name; }
  const juce::String &getName() const { return name_; }

  // Set the target value (user input)
  void setValue(float value) {
    targetValue_.store(juce::jlimit(0.0f, 1.0f, value));
  }

  float getTargetValue() const { return targetValue_.load(); }

  // Smoothing time in milliseconds
  void setSmoothingTime(float ms) { smoothingTimeMs_.store(ms); }
  float getSmoothingTime() const { return smoothingTimeMs_.load(); }

  // Default value (for reset)
  void setDefaultValue(float value) {
    defaultValue_ = juce::jlimit(0.0f, 1.0f, value);
  }
  float getDefaultValue() const { return defaultValue_; }

  void reset() {
    targetValue_.store(defaultValue_);
    currentValue_.store(defaultValue_);
  }

  //==========================================================================
  // Audio Thread Interface
  //==========================================================================

  void setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updateSmoothingCoefficient();
  }

  // Process one block, smoothing towards target
  float process(int numSamples) {
    float target = targetValue_.load();
    float current = currentValue_.load();
    float smoothing = smoothingCoef_.load();

    // One-pole smoothing
    for (int i = 0; i < numSamples; ++i) {
      current += (target - current) * smoothing;
    }

    currentValue_.store(current);
    return current;
  }

  // Get current smoothed value (RT-safe)
  float getValue() const { return currentValue_.load(); }

  // Get value scaled to a range
  float getValueScaled(float min, float max) const {
    return min + (max - min) * currentValue_.load();
  }

  // Get bipolar value (-1.0 to 1.0)
  float getValueBipolar() const { return currentValue_.load() * 2.0f - 1.0f; }

private:
  void updateSmoothingCoefficient() {
    if (sampleRate_ <= 0.0)
      return;

    float timeMs = smoothingTimeMs_.load();
    if (timeMs <= 0.0f) {
      smoothingCoef_.store(1.0f); // Instant
    } else {
      // Time constant for one-pole filter
      float timeSamples = static_cast<float>(timeMs * 0.001 * sampleRate_);
      smoothingCoef_.store(1.0f - std::exp(-1.0f / timeSamples));
    }
  }

  //==========================================================================
  // State
  //==========================================================================

  juce::String name_{"Macro"};
  double sampleRate_ = 48000.0;
  float defaultValue_ = 0.5f;

  // Atomic parameters
  std::atomic<float> targetValue_{0.5f};
  std::atomic<float> currentValue_{0.5f};
  std::atomic<float> smoothingTimeMs_{10.0f}; // 10ms default smoothing
  std::atomic<float> smoothingCoef_{0.1f};
};

// Convenience struct for managing multiple macros
struct MacroBank {
  static constexpr int kNumMacros = 8;

  std::array<MacroControl, kNumMacros> macros;

  MacroBank() {
    for (int i = 0; i < kNumMacros; ++i) {
      macros[i].setName("Macro " + juce::String(i + 1));
    }
  }

  void setSampleRate(double sampleRate) {
    for (auto &macro : macros) {
      macro.setSampleRate(sampleRate);
    }
  }

  void process(int numSamples) {
    for (auto &macro : macros) {
      macro.process(numSamples);
    }
  }

  void reset() {
    for (auto &macro : macros) {
      macro.reset();
    }
  }

  MacroControl &operator[](int index) {
    jassert(index >= 0 && index < kNumMacros);
    return macros[static_cast<size_t>(index)];
  }

  const MacroControl &operator[](int index) const {
    jassert(index >= 0 && index < kNumMacros);
    return macros[static_cast<size_t>(index)];
  }
};

} // namespace zenith
