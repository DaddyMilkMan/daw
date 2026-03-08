/*
    FilterPerformanceBenchmark.cpp - Filter CPU Measurement

    Measures actual CPU usage of filter processing.
    Filters are typically 30-50% of total synth CPU.

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

//==============================================================================
// Simple State Variable Filter (Commonly used in synths)
//==============================================================================
class StateVariableFilter {
private:
    double sampleRate_;
    double frequency_ = 1000.0;
    double resonance_ = 0.5;
    double low_ = 0.0;
    double high_ = 0.0;
    double band_ = 0.0;
    double notch_ = 0.0;

public:
    StateVariableFilter(double sr) : sampleRate_(sr) {}

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double f) { frequency_ = f; }
    void setResonance(double r) { resonance_ = r; }

    // Process one sample (most CPU-intensive operation)
    inline float processLow(float input) {
        double f = frequency_ / sampleRate_;
        double r = resonance_;

        // State variable filter equations
        low_ += f * band_;
        high_ = input - low_ - r * band_;
        band_ += f * high_;
        notch_ = high_ + low_;

        return static_cast<float>(low_);
    }

    inline float processBand(float input) {
        double f = frequency_ / sampleRate_;
        double r = resonance_;

        low_ += f * band_;
        high_ = input - low_ - r * band_;
        band_ += f * high_;
        notch_ = high_ + low_;

        return static_cast<float>(band_);
    }
};

//==============================================================================
// Simple Moog Ladder Filter (More complex, realistic CPU usage)
//==============================================================================
class MoogLadderFilter {
private:
    double sampleRate_;
    double frequency_ = 1000.0;
    double resonance_ = 0.5;
    double z1_[4] = {0, 0, 0, 0};
    double z2_[4] = {0, 0, 0, 0};

public:
    MoogLadderFilter(double sr) : sampleRate_(sr) {}

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double f) { frequency_ = f; }
    void setResonance(double r) { resonance_ = r; }

    // Process one sample (4 poles = more CPU)
    inline float process(float input) {
        double fc = frequency_ / sampleRate_;
        double r = resonance_;
        double x = input;

        // 4 pole ladder filter
        for (int i = 0; i < 4; ++i) {
            double g = 0.5 * fc;
            double gamma = g + g;
            double delta = gamma + gamma;
            double epsilon = delta - 1.0;

            double t = (x - epsilon * z1_[i]) / (1.0 + gamma);
            double u = t + z1_[i];
            z1_[i] = u + t;
            x = z1_[i] + z2_[i];
            z2_[i] = x;
        }

        return static_cast<float>(x);
    }
};

//==============================================================================
// Performance Timer
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
// Filter Benchmark Result
//==============================================================================
struct FilterBenchmarkResult {
    const char* filterType;
    int numVoices;
    double cpuPercent;
    double cpuPerVoice;

    void print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  " << filterType << " (" << numVoices << " voices):\n";
        std::cout << "    Total CPU: " << cpuPercent << "%\n";
        std::cout << "    Per Voice: " << cpuPerVoice << "%\n";
        std::cout << "\n";
    }
};

//==============================================================================
// Benchmark State Variable Filter
//==============================================================================
FilterBenchmarkResult benchmarkSVFilter(int numVoices, int sampleRate, int durationSeconds) {
    FilterBenchmarkResult result;
    result.filterType = "State Variable Filter";
    result.numVoices = numVoices;

    int numSamples = durationSeconds * sampleRate;
    double realTimeUs = (durationSeconds * 1000000.0);

    // Create filters (one per voice)
    std::vector<StateVariableFilter> filters;
    filters.reserve(numVoices);
    for (int v = 0; v < numVoices; ++v) {
        filters.emplace_back(sampleRate);
        filters[v].setFrequency(1000.0 + v * 100.0);
        filters[v].setResonance(0.5);
    }

    // Generate test signal
    std::vector<float> input(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        input[i] = std::sin(2.0 * 3.14159 * 440.0 * i / sampleRate);
    }

    // Warm-up
    for (int i = 0; i < 1000; ++i) {
        for (int v = 0; v < numVoices; ++v) {
            volatile float output = filters[v].processLow(input[i % numSamples]);
            (void)output;
        }
    }

    // Benchmark
    PerfTimer timer;
    timer.start();

    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int v = 0; v < numVoices; ++v) {
            sum += filters[v].processLow(input[i]);
        }
        volatile float sink = sum; // Prevent optimization
        (void)sink;
    }

    double processingUs = timer.stopMicroseconds();
    result.cpuPercent = (processingUs / realTimeUs) * 100.0;
    result.cpuPerVoice = result.cpuPercent / numVoices;

    return result;
}

//==============================================================================
// Benchmark Moog Ladder Filter
//==============================================================================
FilterBenchmarkResult benchmarkMoogFilter(int numVoices, int sampleRate, int durationSeconds) {
    FilterBenchmarkResult result;
    result.filterType = "Moog Ladder Filter";
    result.numVoices = numVoices;

    int numSamples = durationSeconds * sampleRate;
    double realTimeUs = (durationSeconds * 1000000.0);

    // Create filters (one per voice)
    std::vector<MoogLadderFilter> filters;
    filters.reserve(numVoices);
    for (int v = 0; v < numVoices; ++v) {
        filters.emplace_back(sampleRate);
        filters[v].setFrequency(1000.0 + v * 100.0);
        filters[v].setResonance(0.5);
    }

    // Generate test signal
    std::vector<float> input(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        input[i] = std::sin(2.0 * 3.14159 * 440.0 * i / sampleRate);
    }

    // Warm-up
    for (int i = 0; i < 1000; ++i) {
        for (int v = 0; v < numVoices; ++v) {
            volatile float output = filters[v].process(input[i % numSamples]);
            (void)output;
        }
    }

    // Benchmark
    PerfTimer timer;
    timer.start();

    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int v = 0; v < numVoices; ++v) {
            sum += filters[v].process(input[i]);
        }
        volatile float sink = sum; // Prevent optimization
        (void)sink;
    }

    double processingUs = timer.stopMicroseconds();
    result.cpuPercent = (processingUs / realTimeUs) * 100.0;
    result.cpuPerVoice = result.cpuPercent / numVoices;

    return result;
}

//==============================================================================
// Main
//==============================================================================
int main() {
    std::cout << "========================================\n";
    std::cout << "Filter Performance Benchmark\n";
    std::cout << "========================================\n\n";

    std::cout << "Testing realistic filter CPU usage:\n";
    std::cout << "  - State Variable Filter (2-pole, efficient)\n";
    std::cout << "  - Moog Ladder Filter (4-pole, realistic)\n\n";

    const int sampleRate = 48000;
    const int durationSeconds = 5;

    std::cout << "Test parameters:\n";
    std::cout << "  Sample rate: " << sampleRate << " Hz\n";
    std::cout << "  Duration: " << durationSeconds << " seconds\n\n";

    // Test different voice counts
    std::vector<FilterBenchmarkResult> results;

    std::cout << "Test 1: State Variable Filter (2-pole)\n";
    std::cout << "-----------------------------------------\n\n";

    for (int numVoices : {1, 4, 8, 16}) {
        auto result = benchmarkSVFilter(numVoices, sampleRate, durationSeconds);
        results.push_back(result);
        result.print();
    }

    std::cout << "\nTest 2: Moog Ladder Filter (4-pole)\n";
    std::cout << "-------------------------------------\n\n";

    for (int numVoices : {1, 4, 8, 16}) {
        auto result = benchmarkMoogFilter(numVoices, sampleRate, durationSeconds);
        results.push_back(result);
        result.print();
    }

    // Summary
    std::cout << "========================================\n";
    std::cout << "SUMMARY - Filter vs Oscillator\n";
    std::cout << "========================================\n\n";

    std::cout << "Previous oscillator benchmark: 0.0057% CPU per voice\n\n";

    std::cout << "Filter results (16 voices):\n";
    for (const auto& r : results) {
        if (r.numVoices == 16) {
            std::cout << "  " << r.filterType << ": " << r.cpuPerVoice << "% per voice\n";
        }
    }

    std::cout << "\nComparison to Serum (~2-3% per voice for full synth):\n";
    std::cout << "  Oscillators: ~0.006% per voice (0.3% of total)\n";
    std::cout << "  Filters: TBD (likely 30-50% of total)\n";
    std::cout << "  Remaining: Envelopes, LFOs, FX, modulation\n\n";

    return 0;
}
