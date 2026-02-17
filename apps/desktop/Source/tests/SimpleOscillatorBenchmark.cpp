/*
    SimpleOscillatorBenchmark.cpp - Minimal CPU Performance Test

    Measures actual CPU usage of Zenith oscillators.
    This is a reduced benchmark that will actually compile and run.

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include <juce_core/juce_core.h>  // Must be first
#include "../instruments/ZenithOscillator.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace juce;
using namespace zenith;

//==============================================================================
// Simple Performance Timer
//==============================================================================
class PerfTimer {
public:
    using Clock = std::chrono::high_resolution_clock;

    void start() {
        startTime_ = Clock::now();
    }

    double stopMicroseconds() {
        auto endTime = Clock::now();
        std::chrono::duration<double, std::micro> diff = endTime - startTime_;
        return diff.count();
    }

private:
    Clock::time_point startTime_;
};

//==============================================================================
// Benchmark Result
//==============================================================================
struct OscBenchmarkResult {
    int waveformType;
    float frequency;
    int numSamples;
    double processingUs;
    double realTimeUs;
    double cpuPercent;

    void print() const {
        const char* waveformNames[] = {"Sine", "Sawtooth", "Square", "Triangle"};

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  " << waveformNames[waveformType] << " @ " << frequency << " Hz:\n";
        std::cout << "    Samples: " << numSamples << "\n";
        std::cout << "    Processing: " << processingUs / 1000.0 << " ms\n";
        std::cout << "    Real time: " << realTimeUs / 1000.0 << " ms\n";
        std::cout << "    CPU: " << cpuPercent << "%\n";
        std::cout << "\n";
    }
};

//==============================================================================
// Oscillator Performance Test
//==============================================================================
OscBenchmarkResult benchmarkOscillator(int waveformType, float frequency,
                                          int durationSeconds, int sampleRate) {
    OscBenchmarkResult result;
    result.waveformType = waveformType;
    result.frequency = frequency;
    result.numSamples = durationSeconds * sampleRate;
    result.realTimeUs = (durationSeconds * 1000000.0);

    // Create oscillator
    ZenithOscillator osc;
    osc.setSampleRate(sampleRate);
    osc.setWaveform(static_cast<OscillatorWaveform>(waveformType));

    // Allocate buffer
    std::vector<float> buffer(result.numSamples);

    // Warm-up
    for (int i = 0; i < sampleRate; ++i) {
        osc.getNextSample(frequency, 0.5f);
    }

    // Benchmark
    PerfTimer timer;
    timer.start();

    float phase = 0.0f;
    for (int i = 0; i < result.numSamples; ++i) {
        buffer[i] = osc.getNextSample(frequency, 0.5f);
    }

    result.processingUs = timer.stopMicroseconds();
    result.cpuPercent = (result.processingUs / result.realTimeUs) * 100.0;

    return result;
}

//==============================================================================
// Multi-Voice Test
//==============================================================================
struct MultiVoiceResult {
    int numVoices;
    double cpuPercent;
    double cpuPerVoice;
    double voicesPerPercentCPU;

    void print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  " << numVoices << " Voices:\n";
        std::cout << "    Total CPU: " << cpuPercent << "%\n";
        std::cout << "    Per Voice: " << cpuPerVoice << "%\n";
        std::cout << "    Voices/%CPU: " << voicesPerPercentCPU << "\n";
        std::cout << "\n";
    }
};

MultiVoiceResult benchmarkMultiVoice(int numVoices, int sampleRate, int durationSeconds) {
    MultiVoiceResult result;
    result.numVoices = numVoices;

    int numSamples = durationSeconds * sampleRate;
    double realTimeUs = (durationSeconds * 1000000.0);

    // Create oscillators
    std::vector<ZenithOscillator> oscillators(numVoices);
    for (int i = 0; i < numVoices; ++i) {
        oscillators[i].setSampleRate(sampleRate);
        oscillators[i].setWaveform(OscillatorWaveform::Saw);
    }

    // Allocate buffers
    std::vector<std::vector<float>> buffers(numVoices);
    for (int i = 0; i < numVoices; ++i) {
        buffers[i].resize(numSamples);
    }

    // Warm-up
    for (int i = 0; i < numVoices; ++i) {
        for (int j = 0; j < sampleRate; ++j) {
            oscillators[i].getNextSample(440.0f, 0.5f);
        }
    }

    // Benchmark
    PerfTimer timer;
    timer.start();

    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int v = 0; v < numVoices; ++v) {
            buffers[v][i] = oscillators[v].getNextSample(440.0f, 0.5f);
            sum += buffers[v][i]; // Prevent optimization
        }
        volatile float sink = sum; // Use result
        (void)sink;
    }

    double processingUs = timer.stopMicroseconds();
    result.cpuPercent = (processingUs / realTimeUs) * 100.0;
    result.cpuPerVoice = result.cpuPercent / numVoices;
    result.voicesPerPercentCPU = numVoices / result.cpuPercent;

    return result;
}

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "Zenith Oscillator CPU Benchmark\n";
    std::cout << "========================================\n\n";

    // System info
    std::cout << "System Information:\n";
    std::cout << "  CPU Cores: " << juce::SystemStats::getNumCpus() << "\n";
    std::cout << "  CPU Speed: " << juce::SystemStats::getCpuSpeedInMegahertz() << " MHz\n";
    std::cout << "  Memory: " << juce::SystemStats::getMemorySizeInMegabytes() << " MB\n\n";

    const int sampleRate = 48000;
    const int durationSeconds = 5;

    // Test 1: Single Oscillator Performance
    std::cout << "Test 1: Single Oscillator Performance (5 seconds @ 48kHz)\n";
    std::cout << "---------------------------------------------------------\n\n";

    std::vector<OscBenchmarkResult> oscResults;

    for (int waveform = 0; waveform < 4; ++waveform) {
        auto result = benchmarkOscillator(waveform, 440.0f, durationSeconds, sampleRate);
        oscResults.push_back(result);
        result.print();
    }

    // Test 2: Multi-Voice Performance
    std::cout << "Test 2: Multi-Voice Performance (5 seconds @ 48kHz)\n";
    std::cout << "------------------------------------------------\n\n";

    std::vector<MultiVoiceResult> voiceResults;

    for (int numVoices : {1, 4, 8, 16}) {
        auto result = benchmarkMultiVoice(numVoices, sampleRate, durationSeconds);
        voiceResults.push_back(result);
        result.print();
    }

    // Summary
    std::cout << "========================================\n";
    std::cout << "SUMMARY\n";
    std::cout << "========================================\n\n";

    std::cout << "Single Oscillator CPU (Average):\n";
    double avgOscCPU = 0.0;
    for (const auto& r : oscResults) {
        avgOscCPU += r.cpuPercent;
    }
    avgOscCPU /= oscResults.size();
    std::cout << "  Average: " << avgOscCPU << "%\n\n";

    std::cout << "Multi-Voice Results:\n";
    for (const auto& r : voiceResults) {
        std::cout << "  " << r.numVoices << " voices: " << r.cpuPerVoice << "% per voice\n";
    }
    std::cout << "\n";

    // Comparison to Serum
    std::cout << "Comparison to Serum (~2-3% per voice):\n";
    for (const auto& r : voiceResults) {
        if (r.cpuPerVoice < 3.0f) {
            std::cout << "  " << r.numVoices << " voices: ✓ EXCELLENT - Beats Serum!\n";
        } else if (r.cpuPerVoice < 5.0f) {
            std::cout << "  " << r.numVoices << " voices: ✓ GOOD - Acceptable\n";
        } else {
            std::cout << "  " << r.numVoices << " voices: ✗ NEEDS OPTIMIZATION\n";
        }
    }

    return 0;
}
