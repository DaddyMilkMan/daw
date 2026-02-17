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

#include "RealTimeAudioBuffer.h"

namespace zenith {
namespace audio {

// Lock-free ring buffer for real-time audio
template<typename T, size_t Size>
class RealTimeAudioProcessor {
public:
    RealTimeAudioProcessor();
    ~RealTimeAudioProcessor();

    // Initialization
    bool initialize(int numChannels, double sampleRate, int bufferSize);
    void shutdown();

    // Processing
    void processAudio(juce::AudioBuffer<float>& buffer);

    // RT-SAFE: Template version eliminates std::function heap allocation
    // C++20 constraint REMOVED for C++17 compatibility
    template<typename ProcessorFunc>
    void processAudioWithCallback(juce::AudioBuffer<float>& buffer, ProcessorFunc&& processor) {
        // Check real-time safety
        if (!checkRealTimeSafety()) {
            underruns.fetch_add(1);
        }

        // Execute the actual processing callback - INLINED, no allocation
        std::forward<ProcessorFunc>(processor)(buffer);

        // Update CPU usage metrics with atomic operations only
        updatePerformanceMetrics();
    }

    // Buffer management
    bool setInputBuffer(const juce::AudioBuffer<float>& buffer);
    bool getOutputBuffer(juce::AudioBuffer<float>& buffer);

    // Latency management
    int getLatencySamples() const;
    double getLatencySeconds() const;
    void setTargetLatency(double seconds);

    // Performance monitoring
    float getCpuUsage() const;
    int getUnderruns() const;
    int getOverruns() const;
    void resetPerformanceCounters();

    // Real-time safety
    bool isRealTimeSafe() const;
    void setRealTimePriority(bool enabled);

private:
    std::unique_ptr<RealTimeAudioBuffer> inputBuffer;
    std::unique_ptr<RealTimeAudioBuffer> outputBuffer;
    std::unique_ptr<SampleRateConverter> sampleRateConverter;

    int numChannels = 2;
    double sampleRate = 44100.0;
    int bufferSize = 512;
    double targetLatency = 0.01;  // 10ms

    // Performance monitoring
    std::atomic<float> cpuUsage{0.0f};
    std::atomic<int> underruns{0};
    std::atomic<int> overruns{0};

    // Timing
    juce::Time lastProcessTime;
    std::atomic<bool> realTimePriority{false};

    void updatePerformanceMetrics();
    bool checkRealTimeSafety() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeAudioProcessor)
};

// Audio interface for hardware integration

} // namespace
