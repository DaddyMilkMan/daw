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
      if (clip != nullptr && clip->isPlaying() &&
          clip->isActiveAt(playheadSamples)) {
        if (clip->getType() == Clip::Type::Audio) {
          clipBuffer_.clear();
          juce::AudioSourceChannelInfo clipInfo(&clipBuffer_, 0,
                                                bufferToFill.numSamples);
          clip->processAudioClip(clipInfo, playheadSamples);

          const int channelsToMix =
              juce::jmin(bufferToFill.buffer->getNumChannels(),
                         clipBuffer_.getNumChannels());
          for (int ch = 0; ch < channelsToMix; ++ch) {
            bufferToFill.buffer->addFrom(ch, bufferToFill.startSample,
                                         clipBuffer_, ch, 0,
                                         bufferToFill.numSamples);
          }
        }
      }
    }
  }

  juce::AudioBuffer<float> localBuffer(
      bufferToFill.buffer->getArrayOfWritePointers(),
      bufferToFill.buffer->getNumChannels(), bufferToFill.startSample,
      bufferToFill.numSamples);

  // Process through plugin chain (Base class)
  juce::MidiBuffer dummyMidi;
  processPluginChain(localBuffer, dummyMidi, bufferToFill.numSamples,
                     sidechainBuffer);

  // Mixer processing (Base class)
  juce::AudioSourceChannelInfo mixerInfo(&localBuffer, 0,
                                         bufferToFill.numSamples);
  mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);
}

} // namespace zenith
