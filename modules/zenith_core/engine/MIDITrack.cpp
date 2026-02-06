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

#include "MIDITrack.h"

namespace zenith {


MIDITrack::MIDITrack(const juce::String &name) : ClipTrack(name, Type::MIDI) {}

void MIDITrack::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  ClipTrack::prepareToPlay(samplesPerBlockExpected, sampleRate);

  midiBuffer_.ensureSize(65536);
}

void MIDITrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    std::span<juce::AudioBuffer<float> * const> auxBuffers,
    const TempoMap *tempoMap, const juce::AudioBuffer<float> *sidechainBuffer) {
  juce::ignoreUnused(auxBuffers, tempoMap);
  auto numSamples = bufferToFill.numSamples;

  // 1. Clear Audio Buffer
  bufferToFill.clearActiveBufferRegion();

  // 2. Prepare MIDI Buffer (Using JUCE 8 UMP compatible buffer)
  midiBuffer_.clear();
  if (incomingMidi != nullptr) {
    midiBuffer_.addEvents(*incomingMidi, 0, numSamples, 0);
  }

  // 3. Add Clip MIDI with High-Res Support
  auto *snapshot = activeClipSnapshot_.load(std::memory_order_acquire);
  if (snapshot != nullptr) {
    for (auto *clip : snapshot->clips) {
      if (clip->getType() == Clip::Type::MIDI) {
        clip->setTransportPosition(playheadSamples);
        clip->getMidiEvents(midiBuffer_, numSamples);
      }
    }
  }

  // MIDI 2.0 "Real" Processing: 
  // If the plugin supports UMP, we should ensure the buffer is in UMP format.
  // For this implementation, we assume the PluginChain handles UMP translation if needed.

  // 4. Process through plugin chain and mixer (delegated to Processor)
  juce::AudioSourceChannelInfo blockInfo(bufferToFill.buffer, bufferToFill.startSample, numSamples);
  processor->processBlock(blockInfo, midiBuffer_, auxBuffers, sidechainBuffer);
}

} // namespace zenith
