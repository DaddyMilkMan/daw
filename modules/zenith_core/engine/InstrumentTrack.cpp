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

#include "InstrumentTrack.h"

namespace zenith {


void InstrumentTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    std::span<juce::AudioBuffer<float> * const> auxBuffers,
    const TempoMap *tempoMap, const juce::AudioBuffer<float> *sidechainBuffer) {

  juce::ignoreUnused(auxBuffers, tempoMap);
  auto numSamples = bufferToFill.numSamples;

  // 1. Clear Audio Buffer (Instrument plugin will fill it)
  bufferToFill.clearActiveBufferRegion();

  // 2. Prepare MIDI Buffer
  midiBuffer_.clear();
  if (incomingMidi != nullptr) {
    midiBuffer_.addEvents(*incomingMidi, 0, numSamples, 0);
  }

  // 3. Add Clip MIDI (from MIDI clips on this instrument track)
  const ClipSnapshot *snapshot =
      activeClipSnapshot_.load(std::memory_order_acquire);

  if (snapshot != nullptr) {
    for (auto *clip : snapshot->clips) {
      if (clip != nullptr && clip->isPlaying() && clip->getType() == Clip::Type::MIDI) {
        const int64_t clipStart = clip->getStartPosition();
        const int64_t clipEnd = clip->getEndPosition();
        const int64_t blockStart = playheadSamples;
        const int64_t blockEnd = playheadSamples + numSamples;

        // Check for overlap
        if (blockEnd > clipStart && blockStart < clipEnd) {
          const int64_t overlapStart = std::max(blockStart, clipStart);
          const int64_t overlapEnd = std::min(blockEnd, clipEnd);
          const int numToProcess = static_cast<int>(overlapEnd - overlapStart);

          if (numToProcess > 0) {
            // Calculate how many samples from the start of the block we should
            // wait before starting clip processing
            const int startOffsetInBuffer = static_cast<int>(overlapStart - blockStart);
            
            // Process the intersecting part of the MIDI clip
            // processMidiClip takes numSamples as a constraint
            clip->processMidiClip(midiBuffer_, overlapStart, numToProcess);
          }
        }
      }
    }
  }

  // 4. Process through plugin chain and mixer (delegated to Processor)
  juce::AudioSourceChannelInfo blockInfo(bufferToFill.buffer, bufferToFill.startSample, numSamples);
  processor->processBlock(blockInfo, midiBuffer_, auxBuffers, sidechainBuffer);

  // 5. Update Metering
  // TrackProcessor currently doesn't explicitly 'update' meters separate from processBlock, 
  // but MixerChannel (inside processor) does it during getNextAudioBlock.
}

} // namespace zenith
