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

#include "AuxBusTrack.h"

namespace zenith {


AuxBusTrack::AuxBusTrack(const juce::String &name) : Track(name, Type::Bus) {}

void AuxBusTrack::prepareToPlay(int samplesPerBlockExpected,
                                double sampleRate) {

  Track::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AuxBusTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    std::span<juce::AudioBuffer<float> * const> auxBuffers,
    const TempoMap *tempoMap, const juce::AudioBuffer<float> *sidechainBuffer) {
  juce::ignoreUnused(playheadSamples, incomingMidi, tempoMap);
  auto numSamples = bufferToFill.numSamples;

  // 1. Start with silence
  bufferToFill.clearActiveBufferRegion();

  // 2. Sum Aux Buffers (Inputs)
  for (auto *inputBuffer : auxBuffers) {
    if (inputBuffer != nullptr && inputBuffer->getNumChannels() > 0) {
      for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch) {
        // Map input channels to output (simple mono/stereo logic)
        int inputCh = (ch < inputBuffer->getNumChannels()) ? ch : 0;

        bufferToFill.buffer->addFrom(ch, bufferToFill.startSample, *inputBuffer,
                                     inputCh, 0, numSamples);
      }
    }
  }

  // 3. Process Plugins and Mixer (delegated to Processor)
  juce::AudioSourceChannelInfo blockInfo(bufferToFill.buffer, bufferToFill.startSample, numSamples);
  processor->processBlock(blockInfo, processor->getEmptyMidiBuffer(), {}, sidechainBuffer);
}

} // namespace zenith
