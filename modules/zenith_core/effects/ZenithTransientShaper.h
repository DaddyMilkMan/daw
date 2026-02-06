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

#include "../plugins/ZenithPlugin.h"
#include <juce_dsp/juce_dsp.h>

namespace zenith {

class ZenithTransientShaper : public ZenithPlugin {
public:
  ZenithTransientShaper();
  ~ZenithTransientShaper() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  const juce::String getName() const override {
    return "Zenith Transient Shaper";
  }

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  std::atomic<float> *attackGain = nullptr;
  std::atomic<float> *sustainGain = nullptr;

  // Envelope followers (dynamic channel support)
  std::vector<float> fastEnvelope;
  std::vector<float> slowEnvelope;

  float sampleRate_ = 44100.0f;

  // Pre-computed envelope coefficients
  float fastCoeff_ = 0.0f;
  float slowCoeff_ = 0.0f;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTransientShaper)
};

} // namespace zenith
