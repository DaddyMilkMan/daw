/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

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
                                  std::span<juce::AudioBuffer<float>* const> auxBuffers,
                                  const juce::AudioBuffer<float>* sidechain) {
    
    // Input validation
    if (bufferToFill.buffer == nullptr || bufferToFill.numSamples <= 0) {
        return; // Invalid buffer
    }
    
    if (bufferToFill.startSample < 0 || 
        bufferToFill.startSample + bufferToFill.numSamples > bufferToFill.buffer->getNumSamples()) {
        return; // Invalid range
    }
    
    // 1. Process Plugin Chain
    // Correctly create a local buffer wrapper by offsetting pointers to startSample
    const int numChannels = bufferToFill.buffer->getNumChannels();
    
    if (numChannels <= 0 || numChannels > 32) {
        return; // Invalid channel count
    }
    
    // Use a fixed-size array on the stack to avoid allocations
    std::array<float*, 32> pointers;
    const int safeChannels = std::min(numChannels, 32);
    
    for (int i = 0; i < safeChannels; ++i) {
        float* ptr = bufferToFill.buffer->getWritePointer(i, bufferToFill.startSample);
        if (ptr == nullptr) {
            return; // Invalid pointer
        }
        pointers[i] = ptr;
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
    
    processBlock(info, midiMessages, {}, nullptr);
}

} // namespace zenith
