#include "AuxBusTrack.h"
#include "EngineConstants.h"

namespace zenith {

AuxBusTrack::AuxBusTrack(const juce::String& name) 
    : Track(name, Type::Bus) {
}

void AuxBusTrack::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    Track::prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void AuxBusTrack::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                                  int64_t playheadSamples,
                                  const juce::MidiBuffer* incomingMidi,
                                  const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                                  const TempoMap* tempoMap) {
    juce::ignoreUnused(playheadSamples, incomingMidi, tempoMap);
    auto numSamples = bufferToFill.numSamples;
    
    // 1. Start with silence
    bufferToFill.clearActiveBufferRegion();

    // Verify rigorous thread safety
    jassert(juce::MessageManager::getInstance()->currentThreadHasLockedMessageManager() == false);

    // Process any pending cross-thread events/notes safely
    processPendingNotes();
    
    // 2. Sum Aux Buffers (Inputs)
    for (auto* inputBuffer : auxBuffers) {
        if (inputBuffer != nullptr && inputBuffer->getNumChannels() > 0) {
            for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch) {
                // Map input channels to output (simple mono/stereo logic)
                int inputCh = (ch < inputBuffer->getNumChannels()) ? ch : 0;
                
                bufferToFill.buffer->addFrom(ch, bufferToFill.startSample, 
                                            *inputBuffer, inputCh, 0, numSamples);
            }
        }
    }
    
    // 3. Process Plugins (Effects)
    juce::MidiBuffer emptyMidi;
    juce::AudioBuffer<float> proxyBuffer(bufferToFill.buffer->getArrayOfWritePointers(), 
                                        bufferToFill.buffer->getNumChannels(), 
                                        bufferToFill.startSample, numSamples);
    processPluginChain(proxyBuffer, emptyMidi, numSamples);
    
    // 4. Mixer (Volume/Pan for bus output)
    applyGainAndPan(proxyBuffer, numSamples);
    
    // 5. Metering
    updateLevelMeters(proxyBuffer, numSamples);
}

} // namespace zenith
