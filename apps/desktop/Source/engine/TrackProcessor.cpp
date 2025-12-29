/*
  ==============================================================================

    TrackProcessor.cpp
    Created: 2025
    Author:  Zenith DAW

  ==============================================================================
*/

#include "TrackProcessor.h"

namespace zenith {

TrackProcessor::TrackProcessor() {
}

TrackProcessor::~TrackProcessor() {
}

void TrackProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

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

void TrackProcessor::processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                                  juce::MidiBuffer& midiMessages,
                                  std::span<juce::AudioBuffer<float>* const> auxBuffers,
                                  const juce::AudioBuffer<float>* sidechain) {
    
    // 1. Process Plugin Chain
    // Using a local buffer wrapper implies we process in-place on bufferToFill.buffer
    // BUT Track::processPluginChain took a reference to a buffer.
    // Let's match typical Track behavior: process the buffer directly.
    
    // Create a local AudioBuffer referencing the detailed part of bufferToFill
    // to avoid messing with offsets manually inside pluginChain
    juce::AudioBuffer<float> localBuffer(
        bufferToFill.buffer->getArrayOfWritePointers(),
        bufferToFill.buffer->getNumChannels(),
        bufferToFill.startSample,
        bufferToFill.numSamples
    );

    pluginChain.process(localBuffer, midiMessages, sidechain);

    // 2. Mixer Processing (Vol, Pan, Sends)
    // MixerChannel expects AudioSourceChannelInfo
    juce::AudioSourceChannelInfo mixerInfo(&localBuffer, 0, bufferToFill.numSamples);
    mixerChannel.getNextAudioBlock(mixerInfo, auxBuffers);
}

void TrackProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    // Convenience overload - create AudioSourceChannelInfo and call full version
    juce::AudioSourceChannelInfo info(&buffer, 0, buffer.getNumSamples());
    processBlock(info, midiMessages, {}, nullptr);
}

} // namespace zenith
