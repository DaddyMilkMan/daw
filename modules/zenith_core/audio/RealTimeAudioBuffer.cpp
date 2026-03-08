/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "RealTimeAudioBuffer.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace audio {

//==============================================================================
RealTimeAudioBuffer::RealTimeAudioBuffer() = default;

RealTimeAudioBuffer::RealTimeAudioBuffer(int numChannels, int numSamples)
{
    setSize(numChannels, numSamples);
}

RealTimeAudioBuffer::~RealTimeAudioBuffer() = default;

//==============================================================================
bool RealTimeAudioBuffer::setSize(int numChannels, int numSamples)
{
    if (numChannels <= 0 || numSamples <= 0)
        return false;

    buffer.setSize(numChannels, numSamples, false, true, false);
    this->numChannels = numChannels;
    this->numSamples = numSamples;

    return true;
}

void RealTimeAudioBuffer::clear()
{
    buffer.clear();
}

//==============================================================================
const float* RealTimeAudioBuffer::getReadPointer(int channel) const
{
    return buffer.getReadPointer(channel);
}

const float** RealTimeAudioBuffer::getArrayOfReadPointers() const
{
    return buffer.getArrayOfReadPointers();
}

float* RealTimeAudioBuffer::getWritePointer(int channel)
{
    return buffer.getWritePointer(channel);
}

float** RealTimeAudioBuffer::getArrayOfWritePointers()
{
    return buffer.getArrayOfWritePointers();
}

//==============================================================================
void RealTimeAudioBuffer::copyFrom(const juce::AudioBuffer<float>& source)
{
    if (source.getNumChannels() != numChannels ||
        source.getNumSamples() != numSamples)
    {
        // Resize to match source
        setSize(source.getNumChannels(), source.getNumSamples());
    }

    for (int channel = 0; channel < numChannels; ++channel)
    {
        buffer.copyFrom(channel, 0, source, channel, 0, numSamples);
    }
}

void RealTimeAudioBuffer::copyTo(juce::AudioBuffer<float>& dest) const
{
    auto destChannels = dest.getNumChannels();
    auto destSamples = dest.getNumSamples();

    for (int channel = 0; channel < juce::jmin(numChannels, destChannels); ++channel)
    {
        auto samplesToCopy = juce::jmin(numSamples, destSamples);
        dest.copyFrom(channel, 0, buffer, channel, 0, samplesToCopy);
    }
}

//==============================================================================
void RealTimeAudioBuffer::applyGain(float gain)
{
    buffer.applyGain(gain);
}

void RealTimeAudioBuffer::applyGain(int channel, float gain)
{
    if (channel >= 0 && channel < numChannels)
        buffer.applyGain(channel, 0, numSamples, gain);
}

void RealTimeAudioBuffer::applyGainRamp(int channel, int startSample, int numSamples,
                                        float startGain, float endGain)
{
    if (channel >= 0 && channel < numChannels)
        buffer.applyGainRamp(channel, startSample, numSamples, startGain, endGain);
}

//==============================================================================
void RealTimeAudioBuffer::addFrom(int destChannel, int destStartSample,
                                  const juce::AudioBuffer<float>& source,
                                  int sourceChannel, int sourceStartSample,
                                  int numSamples, float gainToApplyToSource)
{
    if (destChannel >= 0 && destChannel < numChannels)
    {
        auto actualSamples = juce::jmin(numSamples, this->numSamples - destStartSample);
        buffer.addFrom(destChannel, destStartSample, source, sourceChannel,
                      sourceStartSample, actualSamples, gainToApplyToSource);
    }
}

} // namespace audio
} // namespace zenith
