#include "AudioTrack.h"
#include "Clip.h"
#include "TempoMap.h"

namespace zenith {

void AudioTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    const std::vector<juce::AudioBuffer<float> *> &auxBuffers,
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
      if (clip != nullptr && clip->isPlaying() && clip->getType() == Clip::Type::Audio) {
        const int64_t clipStart = clip->getStartPosition();
        const int64_t clipEnd = clip->getEndPosition();
        const int64_t blockStart = playheadSamples;
        const int64_t blockEnd = playheadSamples + bufferToFill.numSamples;

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
              bufferToFill.buffer->addFrom(ch, bufferToFill.startSample + startOffsetInBuffer,
                                           clipBuffer_, ch, 0, numToProcess);
            }
          }
        }
      }
    }
  }

  // 3. Process through plugin chain and mixer (delegated to Processor)
  juce::AudioSourceChannelInfo blockInfo(bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);
  juce::MidiBuffer dummyMidi;
  processor->processBlock(blockInfo, dummyMidi, auxBuffers, sidechainBuffer);
}

} // namespace zenith
