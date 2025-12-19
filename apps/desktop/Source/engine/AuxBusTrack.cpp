#include "AuxBusTrack.h"

namespace zenith {

void AuxBusTrack::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill, 
                                   int64_t playheadSamples,
                                   const juce::MidiBuffer* incomingMidi,
                                   const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                                   const TempoMap* tempoMap) {
    juce::ignoreUnused(playheadSamples, incomingMidi, auxBuffers, tempoMap);
    
    if (!enabled.load()) {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    juce::AudioBuffer<float> localBuffer(bufferToFill.buffer->getArrayOfWritePointers(), 
                                         bufferToFill.buffer->getNumChannels(), 
                                         bufferToFill.startSample, 
                                         bufferToFill.numSamples);

    // Process through plugin chain
    juce::MidiBuffer dummyMidi;
    processPluginChain(localBuffer, dummyMidi, bufferToFill.numSamples);

    // Mixer processing
    juce::AudioSourceChannelInfo mixerInfo(&localBuffer, 0, bufferToFill.numSamples);
    mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);
}

} // namespace zenith
