#include "InstrumentTrack.h"

namespace zenith {
<<<<<<< HEAD
// Implementation is inline in header
}
=======

void InstrumentTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    const std::vector<juce::AudioBuffer<float> *> &auxBuffers,
    const TempoMap *tempoMap, const juce::AudioBuffer<float> *sidechainBuffer) {
  juce::ignoreUnused(auxBuffers, tempoMap);
  auto numSamples = bufferToFill.numSamples;

  // 1. Clear Audio Buffer (Instrument plugin will fill it)
  bufferToFill.clearActiveBufferRegion();

  // 2. Prepare MIDI Buffer
  juce::MidiBuffer midiBuffer;
  if (incomingMidi != nullptr) {
    midiBuffer.addEvents(*incomingMidi, 0, numSamples, 0);
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
            clip->processMidiClip(midiBuffer, overlapStart, numToProcess);
          }
        }
      }
    }
  }

  // 4. Process through plugin chain and mixer (delegated to Processor)
  juce::AudioSourceChannelInfo blockInfo(bufferToFill.buffer, bufferToFill.startSample, numSamples);
  processor->processBlock(blockInfo, midiBuffer, auxBuffers, sidechainBuffer);

  // 5. Update Metering
  // TrackProcessor currently doesn't explicitly 'update' meters separate from processBlock, 
  // but MixerChannel (inside processor) does it during getNextAudioBlock.
}

} // namespace zenith
>>>>>>> origin/master
