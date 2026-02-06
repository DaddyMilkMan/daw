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

#include "../ZenithPlugin.h"

namespace zenith {

class ZenithTremolo : public ZenithPlugin {
public:
  ZenithTremolo();
  ~ZenithTremolo() override;

  const juce::String getName() const override { return "Zenith Tremolo"; }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

private:
  // Parameters
  // Note: We use raw pointers for speed in processBlock, updated from APVTS
  std::atomic<float> *rateParam = nullptr;
  std::atomic<float> *depthParam = nullptr;

  // State
  float currentPhase = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithTremolo)
};

} // namespace zenith
