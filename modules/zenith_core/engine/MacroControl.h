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

#include <array>
#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>

namespace zenith {

class MacroControl {
public:
  MacroControl() = default;
  explicit MacroControl(const juce::String &name) { setName(name); }

  //==========================================================================
  // Configuration (Message Thread)
  //==========================================================================

  void setName(const juce::String &name) {
    juce::ScopedLock lock(nameLock_);
    name_ = name;
  }

  juce::String getName() const {
    juce::ScopedLock lock(nameLock_);
    return name_;
  }

  // Set the target value (user input) - can be called from any thread
  void setValue(float value) {
    targetValue_.store(juce::jlimit(0.0f, 1.0f, value));
  }

  float getTargetValue() const { return targetValue_.load(); }

  // Smoothing time in milliseconds
  void setSmoothingTime(float ms) {
    smoothingTimeMs_.store(juce::jmax(0.0f, ms));
    updateSmoothingCoefficient();
  }
  float getSmoothingTime() const { return smoothingTimeMs_.load(); }

  // Default value (for reset)
  void setDefaultValue(float value) {
    defaultValue_.store(juce::jlimit(0.0f, 1.0f, value));
  }
  float getDefaultValue() const { return defaultValue_.load(); }

  void reset() {
    float def = defaultValue_.load();
    targetValue_.store(def);
    currentValue_.store(def);
  }

  //==========================================================================
  // Audio Thread Interface
  //==========================================================================

  void setSampleRate(double sampleRate) {
    sampleRate_.store(sampleRate);
    updateSmoothingCoefficient();
  }

  // Process one block, smoothing towards target - O(1) complexity
  float process(int numSamples) {
    float target = targetValue_.load();
    float current = currentValue_.load();
    float coef = smoothingCoef_.load();

    // O(1) exponential smoothing: current = target + (current - target) *
    // coef^n Where coef = 1 - smoothingCoef (per-sample decay)
    if (numSamples > 0 && coef < 1.0f) {
      float decay = std::pow(1.0f - coef, static_cast<float>(numSamples));
      current = target + (current - target) * decay;
    } else if (coef >= 1.0f) {
      // Instant (no smoothing)
      current = target;
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
    double sr = sampleRate_.load();
    if (sr <= 0.0) {
      smoothingCoef_.store(1.0f);
      return;
    }

    float timeMs = smoothingTimeMs_.load();
    if (timeMs <= 0.0f) {
      smoothingCoef_.store(1.0f); // Instant
    } else {
      // Time constant for one-pole filter
      float timeSamples = static_cast<float>(timeMs * 0.001 * sr);
      smoothingCoef_.store(1.0f - std::exp(-1.0f / timeSamples));
    }
  }

  //==========================================================================
  // State
  //==========================================================================

  mutable juce::CriticalSection nameLock_; // Only for name string
  juce::String name_{"Macro"};

  std::atomic<double> sampleRate_{48000.0};
  std::atomic<float> defaultValue_{0.5f};

  // Atomic parameters
  std::atomic<float> targetValue_{0.5f};
  std::atomic<float> currentValue_{0.5f};
  std::atomic<float> smoothingTimeMs_{10.0f}; // 10ms default smoothing
  std::atomic<float> smoothingCoef_{0.1f};
};

//==============================================================================
// Convenience struct for managing multiple macros
//==============================================================================
struct MacroBank {
  static constexpr int kNumMacros = 8;

  std::array<MacroControl, kNumMacros> macros;

  MacroBank() {
    for (int i = 0; i < kNumMacros; ++i) {
      macros[static_cast<size_t>(i)].setName("Macro " + juce::String(i + 1));
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
