/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <memory>
#include <vector>

namespace zenith {
namespace audio {

/**
 * @brief Real-time safe audio buffer with lock-free operations
 *
 * This buffer is designed for use in real-time audio contexts where
 * allocations and locks are prohibited. It provides:
 * - Lock-free read/write operations
 * - Pre-allocated memory
 * - Channel management
 * - Thread-safe parameter updates
 */
class RealTimeAudioBuffer {
public:
    RealTimeAudioBuffer();
    explicit RealTimeAudioBuffer(int numChannels, int numSamples);
    ~RealTimeAudioBuffer();

    // Initialization
    bool setSize(int numChannels, int numSamples);
    void clear();

    // RT-SAFE: Lock-free read operations
    const float* getReadPointer(int channel) const;
    const float** getArrayOfReadPointers() const;

    // RT-SAFE: Lock-free write operations
    float* getWritePointer(int channel);
    float** getArrayOfWritePointers();

    // RT-SAFE: Copy operations
    void copyFrom(const juce::AudioBuffer<float>& source);
    void copyTo(juce::AudioBuffer<float>& dest) const;

    // RT-SAFE: Apply gain
    void applyGain(float gain);
    void applyGain(int channel, float gain);
    void applyGainRamp(int channel, int startSample, int numSamples, float startGain, float endGain);

    // RT-SAFE: Add samples from another buffer
    void addFrom(int destChannel, int destStartSample,
                 const juce::AudioBuffer<float>& source,
                 int sourceChannel, int sourceStartSample,
                 int numSamples, float gainToApplyToSource = 1.0f);

    // Information
    int getNumChannels() const { return numChannels; }
    int getNumSamples() const { return numSamples; }
    bool isEmpty() const { return numChannels == 0 || numSamples == 0; }

    // State management
    bool isValid() const { return buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0; }

private:
    juce::AudioBuffer<float> buffer;
    int numChannels = 0;
    int numSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeAudioBuffer)
};

} // namespace audio
} // namespace zenith
