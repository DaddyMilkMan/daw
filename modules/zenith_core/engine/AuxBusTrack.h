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

#pragma once

#include "Track.h"

namespace zenith {

class AuxBusTrack : public Track {
public:
  AuxBusTrack(const juce::String &name);
  ~AuxBusTrack() override = default;


  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

  void getNextAudioBlock(
      const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
      const juce::MidiBuffer *incomingMidi = nullptr,
      std::span<juce::AudioBuffer<float>* const> auxBuffers = {},
      const TempoMap *tempoMap = nullptr,
      const juce::AudioBuffer<float> *sidechainBuffer = nullptr) override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxBusTrack)
};

} // namespace zenith
