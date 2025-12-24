#include "AudioTrack.h"
#include "Clip.h"
#include "TakeFolder.h"
#include "TempoMap.h"

namespace zenith {

void AudioTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    const std::vector<juce::AudioBuffer<float> *> &auxBuffers,
    const TempoMap *tempoMap) {
  juce::ignoreUnused(incomingMidi, tempoMap);

  // Clear the buffer first
  bufferToFill.clearActiveBufferRegion();

  if (!enabled.load())
    return;

  const ClipSnapshot *currentSnapshot =
      activeClipSnapshot_.load(std::memory_order_acquire);

  if (currentSnapshot) {
    // Render regular clips
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

    // Render TakeFolders (comping)
    for (auto *folder : currentSnapshot->takeFolders) {
      if (folder != nullptr) {
        int64_t folderStart = folder->getStartPosition();
        int64_t folderEnd = folder->getEndPosition();

        // Check if we're within the folder's range
        if (playheadSamples >= folderStart && playheadSamples < folderEnd) {
          clipBuffer_.clear();
          folder->getNextAudioBlock(clipBuffer_, playheadSamples,
                                    bufferToFill.numSamples, currentSampleRate);

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
  processPluginChain(localBuffer, dummyMidi, bufferToFill.numSamples);

  // Mixer processing (Base class)
  juce::AudioSourceChannelInfo mixerInfo(&localBuffer, 0,
                                         bufferToFill.numSamples);
  mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);
}

} // namespace zenith
