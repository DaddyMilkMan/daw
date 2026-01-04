/*
  ==============================================================================

    TrackProcessor.cpp
    Created: 2025
    Author:  Zenith DAW

  ==============================================================================
*/

#include "TrackProcessor.h"
#include <array>
#include <algorithm>

namespace zenith {

TrackProcessor::TrackProcessor() {
    emptyMidi_.ensureSize(65536);
}

TrackProcessor::~TrackProcessor() {
}

void TrackProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    emptyMidi_.ensureSize(65536);

    // Resize buffers
    if (pluginBuffer.getNumSamples() != samplesPerBlock ||
        pluginBuffer.getNumChannels() != 2) {
        pluginBuffer.setSize(2, samplesPerBlock);
        pluginBuffer.clear();
    }
    
    if (sidechainBuffer.getNumSamples() != samplesPerBlock ||
        sidechainBuffer.getNumChannels() != 2) {
        sidechainBuffer.setSize(2, samplesPerBlock);
        sidechainBuffer.clear();
    }

    pluginChain.prepareToPlay(sampleRate, samplesPerBlock);
    mixerChannel.prepareToPlay(samplesPerBlock, sampleRate);
}

void TrackProcessor::releaseResources() {
    pluginChain.releaseResources();
    mixerChannel.releaseResources();
}

juce::MidiBuffer& TrackProcessor::getEmptyMidiBuffer() noexcept {
    emptyMidi_.clear();
    return emptyMidi_;
}

void TrackProcessor::processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                                  juce::MidiBuffer& midiMessages,
                                  const std::vector<juce::AudioBuffer<float>*>& auxBuffers,
                                  const juce::AudioBuffer<float>* sidechain) {
    
    // 1. Process Plugin Chain
    // Correctly create a local buffer wrapper by offsetting pointers to startSample
    const int numChannels = bufferToFill.buffer->getNumChannels();
    
    // Use a fixed-size array on the stack to avoid allocations
    std::array<float*, 32> pointers;
    const int safeChannels = std::min(numChannels, 32);
    
    for (int i = 0; i < safeChannels; ++i) {
        pointers[i] = bufferToFill.buffer->getWritePointer(i, bufferToFill.startSample);
    }

    juce::AudioBuffer<float> localBuffer(pointers.data(), safeChannels, bufferToFill.numSamples);

    pluginChain.process(localBuffer, midiMessages, sidechain);

    // 2. Mixer Processing (Vol, Pan, Sends)
    // MixerChannel expects AudioSourceChannelInfo
    juce::AudioSourceChannelInfo mixerInfo(&localBuffer, 0, bufferToFill.numSamples);
    mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);
}

void TrackProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    // Convenience overload - create AudioSourceChannelInfo and call full version
    juce::AudioSourceChannelInfo info(&buffer, 0, buffer.getNumSamples());
    
    processBlock(info, midiMessages, emptyAux_, nullptr);
}

} // namespace zenith
