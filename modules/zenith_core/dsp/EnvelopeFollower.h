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

    EnvelopeFollower.h
    Created: 2025-12-09
    Author:  Zenith DAW

    Simple A/R envelope follower for modulation control signals.

  ==============================================================================

*/

#pragma once

#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>


namespace zenith {
namespace dsp {

class EnvelopeFollower {
public:
  EnvelopeFollower() = default;

  void setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
  }

  void setAttack(float attackMs) {
    attackMs_ = attackMs;
    updateCoefficients();
  }

  void setRelease(float releaseMs) {
    releaseMs_ = releaseMs;
    updateCoefficients();
  }

  // Process a block of audio and return the final envelope value
  // (We adhere to block-accurate modulation for Phase 1)
  float process(const float *input, int numSamples) {
    float current = envelope_.load();

    for (int i = 0; i < numSamples; ++i) {
      float in = std::abs(input[i]);
      if (in > current) {
        current = attackCoef_ * (current - in) + in;
      } else {
        current = releaseCoef_ * (current - in) + in;
      }
    }

    envelope_.store(current);
    return current;
  }

  float getCurrentValue() const { return envelope_.load(); }

private:
  void updateCoefficients() {
    if (sampleRate_ <= 0.0)
      return;

    attackCoef_ = std::exp(-1000.0f / (attackMs_ * sampleRate_));
    releaseCoef_ = std::exp(-1000.0f / (releaseMs_ * sampleRate_));
  }

  double sampleRate_ = 48000.0;
  float attackMs_ = 10.0f;
  float releaseMs_ = 100.0f;

  float attackCoef_ = 0.0f;
  float releaseCoef_ = 0.0f;

  std::atomic<float> envelope_{0.0f};
};

} // namespace dsp
} // namespace zenith
