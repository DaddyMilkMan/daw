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

#include <juce_dsp/juce_dsp.h>

namespace zenith {
namespace effects {

class ConsoleEmulation {
public:
  enum class Mode {
    Clean,
    Vintage, // Warm saturation + roll-off
    Modern,  // Slight saturation, bright
    Tube     // Heavy harmonics
  };

  ConsoleEmulation() = default;

  void prepare(juce::dsp::ProcessSpec &spec);
  void reset();

  // Process a block of audio (in-place)
  void process(juce::AudioBuffer<float> &buffer);

  // Parameters
  void setMode(Mode newMode) {
    if (mode != newMode) {
      mode = newMode;
      coefficientsDirty = true;
    }
  }
  void setDrive(float newDrive) { drive = juce::jlimit(0.0f, 1.0f, newDrive); }
  void setCharacter(float newChar) {
    character = juce::jlimit(0.0f, 1.0f, newChar);
  }

private:
  Mode mode = Mode::Clean;
  float drive = 0.0f;     // 0.0 to 1.0
  float character = 0.0f; // 0.0 to 1.0 (mix or intensity)

  float sampleRate = 44100.0f;
  bool coefficientsDirty = true;

  // Filters for tonal shaping
  juce::dsp::IIR::Filter<float> lowPass;

  void updateCoefficients();
  float applySaturation(float input, float driveAmount);
};

} // namespace effects
} // namespace zenith