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

void AudioTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,

    std::span<juce::AudioBuffer<float> * const> auxBuffers,
    const TempoMap *tempoMap, const juce::AudioBuffer<float> *sidechainBuffer) {
  juce::ignoreUnused(incomingMidi);

  // Clear the buffer first
  bufferToFill.clearActiveBufferRegion();

  if (!enabled.load())
    return;

  // Apply automation (Base class logic would be better here, but for now we
  // follow old implementation) Actually, TrackBase should handle automation.
  // I'll need to move that back to TrackBase.

  const ClipSnapshot *currentSnapshot =
      activeClipSnapshot_.load(std::memory_order_acquire);

  if (currentSnapshot) {
    for (auto *clip : currentSnapshot->clips) {
      if (clip == nullptr || !clip->isPlaying() || clip->getType() != Clip::Type::Audio)
        continue;

      const int64_t clipStart = clip->getStartPosition();
      const int64_t blockEnd = playheadSamples + bufferToFill.numSamples;

      // Since clips are sorted by start position, we can stop early if this clip starts after the current block
      if (clipStart >= blockEnd)
          break;

      const int64_t clipEnd = clip->getEndPosition();
      const int64_t blockStart = playheadSamples;

      // Check for overlap
      if (blockEnd > clipStart && blockStart < clipEnd) {
        const int64_t overlapStart = std::max(blockStart, clipStart);
        const int64_t overlapEnd = std::min(blockEnd, clipEnd);
        const int numToProcess = static_cast<int>(overlapEnd - overlapStart);

        if (numToProcess > 0) {
          const int startOffsetInBuffer = static_cast<int>(overlapStart - blockStart);
          
          clipBuffer_.clear();
          juce::AudioSourceChannelInfo clipInfo(&clipBuffer_, 0, numToProcess);
          
          // Process the intersecting part of the clip
          clip->processAudioClip(clipInfo, overlapStart);

          const int channelsToMix = juce::jmin(bufferToFill.buffer->getNumChannels(),
                                               clipBuffer_.getNumChannels());
          for (int ch = 0; ch < channelsToMix; ++ch) {
            juce::FloatVectorOperations::add(bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample + startOffsetInBuffer),
                                           clipBuffer_.getReadPointer(ch), 
                                           numToProcess);
          }
        }
      }
    }
  }

  // 3. Process through plugin chain and mixer (delegated to Processor)
  juce::AudioSourceChannelInfo blockInfo(bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);
  processor->processBlock(blockInfo, processor->getEmptyMidiBuffer(), auxBuffers, sidechainBuffer);
}

} // namespace zenith
