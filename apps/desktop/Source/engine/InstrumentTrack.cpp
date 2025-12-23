#include "InstrumentTrack.h"
#include "EngineConstants.h"

namespace zenith {

<<<<<<< HEAD
InstrumentTrack::InstrumentTrack(const juce::String& name) 
    : ClipTrack(name, Type::Instrument) {
}

void InstrumentTrack::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    ClipTrack::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void InstrumentTrack::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                                        int64_t playheadSamples,
                                        const juce::MidiBuffer* incomingMidi,
                                        const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                                        const TempoMap* tempoMap) {
    juce::ignoreUnused(auxBuffers, tempoMap);
    auto numSamples = bufferToFill.numSamples;
    
    // 1. Clear Audio Buffer (Instrument will fill it)
    bufferToFill.clearActiveBufferRegion();
    
    // 2. Prepare MIDI Buffer
    juce::MidiBuffer midiBuffer;
    if (incomingMidi != nullptr) {
        midiBuffer.addEvents(*incomingMidi, 0, numSamples, 0);
    }
    
    // 3. Add Clip MIDI
    auto* snapshot = activeClipSnapshot_.load(std::memory_order_acquire);
    if (snapshot != nullptr) {
        for (auto* clip : snapshot->clips) {
             if (clip->getType() == Clip::Type::MIDI) {
                 clip->setTransportPosition(playheadSamples);
                 clip->getMidiEvents(midiBuffer, numSamples);
             }
        }
    }
    
    // 4. Process Plugin Chain (should contain a virtual instrument)
    juce::AudioBuffer<float> proxyBuffer(bufferToFill.buffer->getArrayOfWritePointers(), 
                                        bufferToFill.buffer->getNumChannels(), 
                                        bufferToFill.startSample, numSamples);
                                        
    processPluginChain(proxyBuffer, midiBuffer, numSamples);
    
    // 5. Apply Mixer
    applyGainAndPan(proxyBuffer, numSamples);
    
    // 6. Metering
    updateLevelMeters(proxyBuffer, numSamples);
=======
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

  // 1. Clear Audio Buffer (Instrument plugin will fill it)
  bufferToFill.clearActiveBufferRegion();

  // 2. Prepare MIDI Buffer
  juce::MidiBuffer midiBuffer;
  if (incomingMidi != nullptr) {
    midiBuffer.addEvents(*incomingMidi, 0, numSamples, 0);
  }

  // 3. Add Clip MIDI (from MIDI clips on this instrument track)
  auto *snapshot = activeClipSnapshot_.load(std::memory_order_acquire);
  if (snapshot != nullptr) {
    for (auto *clip : snapshot->clips) {
      if (clip->getType() == Clip::Type::MIDI) {
        clip->setTransportPosition(playheadSamples);
        clip->getMidiEvents(midiBuffer, numSamples);
      }
    }
  }

  // 4. Process Plugin Chain (first plugin should be a virtual instrument)
  // Create proxy buffer for correct offset handling
  juce::AudioBuffer<float> proxyBuffer(
      bufferToFill.buffer->getArrayOfWritePointers(),
      bufferToFill.buffer->getNumChannels(), bufferToFill.startSample,
      numSamples);

  processPluginChain(proxyBuffer, midiBuffer, numSamples);

  // 5. Apply Mixer (gain/pan)
  applyGainAndPan(proxyBuffer, numSamples);

  // 6. Update Metering
  updateLevelMeters(proxyBuffer, numSamples);
>>>>>>> origin/master
}

} // namespace zenith
