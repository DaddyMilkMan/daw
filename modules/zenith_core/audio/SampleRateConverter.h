/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <chrono>
#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace zenith {
namespace audio {

// Lock-free ring buffer for real-time audio
template<typename T, size_t Size>
class SampleRateConverter {
public:
    enum class Quality {
        Fast,
        Good,
        Best,
        Linear,
        Sinc
    };

    SampleRateConverter();
    ~SampleRateConverter();

    // Conversion
    bool convert(const juce::AudioBuffer<float>& input,
                juce::AudioBuffer<float>& output,
                double inputSampleRate,
                double outputSampleRate,
                Quality quality = Quality::Good);

    // Configuration
    void setQuality(Quality quality);
    Quality getQuality() const { return currentQuality; }

    // Information
    double getLatency() const;
    bool isInPlaceSupported() const { return false; }

private:
    Quality currentQuality = Quality::Good;

    // Conversion algorithms
    bool convertLinear(const juce::AudioBuffer<float>& input,
                      juce::AudioBuffer<float>& output,
                      double ratio);

    bool convertSinc(const juce::AudioBuffer<float>& input,
                    juce::AudioBuffer<float>& output,
                    double ratio);

    // Sinc filter
    std::vector<float> sincKernel;
    int kernelSize = 64;
    void buildSincKernel(double cutoff);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRateConverter)
};

// Real-time audio processor with low latency

} // namespace
