#include "InstrumentTrack.h"
#include "EngineConstants.h"

namespace zenith {

InstrumentTrack::InstrumentTrack(const juce::String &name)
    : ClipTrack(name, Type::Instrument) {}

void InstrumentTrack::prepareToPlay(int samplesPerBlockExpected,
                                    double sampleRate) {
  ClipTrack::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void InstrumentTrack::getNextAudioBlock(
    const juce::AudioSourceChannelInfo &bufferToFill, int64_t playheadSamples,
    const juce::MidiBuffer *incomingMidi,
    const std::vector<juce::AudioBuffer<float> *> &auxBuffers,
    const TempoMap *tempoMap) {
  juce::ignoreUnused(auxBuffers, tempoMap);
  auto numSamples = bufferToFill.numSamples;

  bufferToFill.clearActiveBufferRegion();

  juce::MidiBuffer midiBuffer;
  if (incomingMidi != nullptr) {
    midiBuffer.addEvents(*incomingMidi, 0, numSamples, 0);
  }

  auto *snapshot = activeClipSnapshot_.load(std::memory_order_acquire);
  if (snapshot != nullptr) {
    for (auto *clip : snapshot->clips) {
      if (clip->getType() == Clip::Type::MIDI) {
        clip->setTransportPosition(playheadSamples);
        clip->getMidiEvents(midiBuffer, numSamples);
      }
    }
  }

  juce::AudioBuffer<float> proxyBuffer(
      bufferToFill.buffer->getArrayOfWritePointers(),
      bufferToFill.buffer->getNumChannels(), bufferToFill.startSample,
      numSamples);

  processPluginChain(proxyBuffer, midiBuffer, numSamples);

  applyGainAndPan(proxyBuffer, numSamples);

  updateLevelMeters(proxyBuffer, numSamples);
}

} // namespace zenith
