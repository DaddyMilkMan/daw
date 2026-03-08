#include "AuxBusTrack.h"
#include "EngineConstants.h"

namespace zenith {

AuxBusTrack::AuxBusTrack(const juce::String &name) : Track(name, Type::Bus) {}

void AuxBusTrack::prepareToPlay(int samplesPerBlockExpected,
                                double sampleRate) {
  Track::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AuxBusTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    const std::vector<juce::AudioBuffer<float> *> &auxBuffers,
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
