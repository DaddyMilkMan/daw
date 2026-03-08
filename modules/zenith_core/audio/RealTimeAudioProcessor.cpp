/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "RealTimeAudioProcessor.h"
#include "RealTimeAudioBuffer.h"
#include "SampleRateConverter.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace audio {

//==============================================================================
RealTimeAudioProcessor::RealTimeAudioProcessor()
    : inputBuffer(std::make_unique<RealTimeAudioBuffer>()),
      outputBuffer(std::make_unique<RealTimeAudioBuffer>()),
      sampleRateConverter(std::make_unique<SampleRateConverter>()),
      lastProcessTime(juce::Time::getCurrentTime())
{
}

RealTimeAudioProcessor::~RealTimeAudioProcessor()
{
    shutdown();
}

//==============================================================================
bool RealTimeAudioProcessor::initialize(int numChannels, double sampleRate, int bufferSize)
{
    if (numChannels <= 0 || sampleRate <= 0 || bufferSize <= 0)
        return false;

    this->numChannels = numChannels;
    this->sampleRate = sampleRate;
    this->bufferSize = bufferSize;

    // Initialize buffers
    if (!inputBuffer->setSize(numChannels, bufferSize * 2))
        return false;

    if (!outputBuffer->setSize(numChannels, bufferSize * 2))
        return false;

    // Clear buffers
    inputBuffer->clear();
    outputBuffer->clear();

    // Reset performance counters
    cpuUsage.store(0.0f);
    underruns.store(0);
    overruns.store(0);

    return true;
}

void RealTimeAudioProcessor::shutdown()
{
    if (inputBuffer)
        inputBuffer->clear();

    if (outputBuffer)
        outputBuffer->clear();
}

//==============================================================================
void RealTimeAudioProcessor::processAudio(juce::AudioBuffer<float>& buffer)
{
    if (!checkRealTimeSafety())
    {
        underruns.fetch_add(1);
        return;
    }

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    // Copy input to our internal buffer
    juce::AudioBuffer<float> tempInput(buffer.getArrayOfWritePointers(),
                                       numChannels, numSamples);
    inputBuffer->copyFrom(tempInput);

    // Process (placeholder - actual processing would be done here)
    outputBuffer->copyFrom(*inputBuffer);

    // Copy output to user buffer
    outputBuffer->copyTo(buffer);

    updatePerformanceMetrics();
}

//==============================================================================
bool RealTimeAudioProcessor::setInputBuffer(const juce::AudioBuffer<float>& buffer)
{
    if (!inputBuffer->isValid())
        return false;

    inputBuffer->copyFrom(buffer);
    return true;
}

bool RealTimeAudioProcessor::getOutputBuffer(juce::AudioBuffer<float>& buffer)
{
    if (!outputBuffer->isValid())
        return false;

    outputBuffer->copyTo(buffer);
    return true;
}

//==============================================================================
int RealTimeAudioProcessor::getLatencySamples() const
{
    return static_cast<int>(targetLatency * sampleRate);
}

double RealTimeAudioProcessor::getLatencySeconds() const
{
    return targetLatency;
}

void RealTimeAudioProcessor::setTargetLatency(double seconds)
{
    if (seconds > 0 && seconds < 1.0)  // Reasonable range: 0 to 1 second
        targetLatency = seconds;
}

//==============================================================================
float RealTimeAudioProcessor::getCpuUsage() const
{
    return cpuUsage.load(std::memory_order_relaxed);
}

int RealTimeAudioProcessor::getUnderruns() const
{
    return underruns.load(std::memory_order_relaxed);
}

int RealTimeAudioProcessor::getOverruns() const
{
    return overruns.load(std::memory_order_relaxed);
}

void RealTimeAudioProcessor::resetPerformanceCounters()
{
    cpuUsage.store(0.0f);
    underruns.store(0);
    overruns.store(0);
}

//==============================================================================
bool RealTimeAudioProcessor::isRealTimeSafe() const
{
    return realTimePriority.load(std::memory_order_relaxed);
}

void RealTimeAudioProcessor::setRealTimePriority(bool enabled)
{
    realTimePriority.store(enabled, std::memory_order_relaxed);
}

//==============================================================================
void RealTimeAudioProcessor::updatePerformanceMetrics()
{
    auto currentTime = juce::Time::getCurrentTime();
    auto elapsed = currentTime - lastProcessTime;
    lastProcessTime = currentTime;

    // Calculate CPU usage as a percentage of buffer time
    double bufferTime = bufferSize / sampleRate;  // Time per buffer in seconds
    double processTime = elapsed.inSeconds();     // Actual processing time

    if (bufferTime > 0)
    {
        float usage = static_cast<float>((processTime / bufferTime) * 100.0);
        cpuUsage.store(juce::jlimit(0.0f, 100.0f, usage), std::memory_order_relaxed);
    }
}

bool RealTimeAudioProcessor::checkRealTimeSafety() const
{
    // Check for buffer underrun conditions
    if (!inputBuffer || !outputBuffer)
        return false;

    if (!inputBuffer->isValid() || !outputBuffer->isValid())
        return false;

    return true;
}

} // namespace audio
} // namespace zenith
