#include "MIDITrack.h"
#include "EngineConstants.h"

namespace zenith {

MIDITrack::MIDITrack(const juce::String& name) 
    : ClipTrack(name, Type::MIDI) {
}

void MIDITrack::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    ClipTrack::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MIDITrack::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
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
    
    // 4. Process Plugin Chain (Instrument)
    // Create proxy buffer for correct offset handling
    juce::AudioBuffer<float> proxyBuffer(bufferToFill.buffer->getArrayOfWritePointers(), 
                                        bufferToFill.buffer->getNumChannels(), 
                                        bufferToFill.startSample, numSamples);
                                        
    processPluginChain(proxyBuffer, midiBuffer, numSamples);
    
    // 5. Apply Mixer
    applyGainAndPan(proxyBuffer, numSamples);
    
    // 6. Metering
    updateLevelMeters(proxyBuffer, numSamples);
}

} // namespace zenith
