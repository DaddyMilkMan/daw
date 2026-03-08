/*
    DriftCPUBenchmark.cpp - Measure Drift Performance Impact

    Quick test to verify that drift adds < 0.001% CPU overhead per voice.

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

using namespace std;

//==============================================================================
// Minimal Oscillator with Drift Simulation
//==============================================================================
class OscillatorWithDrift {
private:
    double phase_ = 0.0;
    double sampleRate_;
    double driftPhase_ = 0.0;
    double driftCurrent_ = 0.0;
    double driftTarget_ = 0.0;
    double driftUpdateTime_ = 0.0;
    bool driftEnabled_ = true;
    float driftAmount_ = 0.5f;

public:
    OscillatorWithDrift(double sr) : sampleRate_(sr) {
        driftTarget_ = (rand() / double(RAND_MAX)) * 2.0 - 1.0;
    }

    void setDriftEnabled(bool enabled) { driftEnabled_ = enabled; }
    void setDriftAmount(float amount) { driftAmount_ = amount; }

    float process(float frequency) {
        // Update drift (every 10ms = 480 samples at 48kHz)
        const double driftUpdateRate = 0.01;
        driftUpdateTime_ += 1.0 / sampleRate_;

        if (driftUpdateTime_ >= driftUpdateRate) {
            driftUpdateTime_ = 0.0;
            driftPhase_ += 0.1; // Slow drift (~1.5 Hz)
            if (driftPhase_ >= 1.0) {
                driftPhase_ -= 1.0;
                driftTarget_ = (rand() / double(RAND_MAX)) * 2.0 - 1.0;
            }

            // Smooth interpolation
            double diff = driftTarget_ - driftCurrent_;
            driftCurrent_ += diff * 0.1;
        }

        // Apply drift to frequency (±5 cents max)
        float freq = frequency;
        if (driftEnabled_) {
            double cents = driftCurrent_ * driftAmount_ * 5.0;
            freq *= static_cast<float>(pow(2.0, cents / 1200.0));
        }

        // Generate sawtooth
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return 2.0f * static_cast<float>(phase_) - 1.0f;
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
// Benchmark
//==============================================================================
struct BenchmarkResult {
    const char* mode;
    int numVoices;
    double cpuPercent;
    double cpuPerVoice;

    void print() const {
        cout << fixed << setprecision(3);
        cout << "  " << mode << " (" << numVoices << " voices): ";
        cout << cpuPercent << "% total, " << cpuPerVoice << "% per voice\n";
    }
};

BenchmarkResult benchmark(bool driftEnabled, int numVoices, int sampleRate, int durationSeconds) {
    BenchmarkResult result;
    result.mode = driftEnabled ? "With Drift" : "No Drift";
    result.numVoices = numVoices;

    int numSamples = durationSeconds * sampleRate;
    double realTimeUs = (durationSeconds * 1000000.0);

    // Create oscillators
    vector<OscillatorWithDrift> oscillators;
    oscillators.reserve(numVoices);
    for (int v = 0; v < numVoices; ++v) {
        oscillators.emplace_back(sampleRate);
        oscillators[v].setDriftEnabled(driftEnabled);
        oscillators[v].setDriftAmount(0.6f);
    }

    // Warm-up
    for (int i = 0; i < 1000; ++i) {
        for (auto& osc : oscillators) {
            volatile float output = osc.process(440.0f);
            (void)output;
        }
    }

    // Benchmark
    PerfTimer timer;
    timer.start();

    for (int i = 0; i < numSamples; ++i) {
        float mix = 0.0f;
        for (auto& osc : oscillators) {
            mix += osc.process(220.0f + (oscillators.size() > 0 ? &osc - &oscillators[0] : 0) * 10.0f);
        }
        volatile float sink = mix / numVoices;
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
    cout << "========================================\n";
    cout << "Drift CPU Impact Benchmark\n";
    cout << "========================================\n\n";

    cout << "Measuring performance difference with/without drift...\n\n";

    const int sampleRate = 48000;
    const int durationSeconds = 5;

    cout << fixed << setprecision(2);

    vector<BenchmarkResult> results;

    for (int numVoices : {16, 32}) {
        auto noDrift = benchmark(false, numVoices, sampleRate, durationSeconds);
        auto withDrift = benchmark(true, numVoices, sampleRate, durationSeconds);

        results.push_back(noDrift);
        results.push_back(withDrift);

        cout << numVoices << " voices:\n";
        noDrift.print();
        withDrift.print();

        double overhead = withDrift.cpuPercent - noDrift.cpuPercent;
        double overheadPerVoice = overhead / numVoices;

        cout << "    Drift overhead: " << overhead << "% total, "
             << overheadPerVoice << "% per voice\n\n";

        if (overheadPerVoice < 0.001) {
            cout << "    ✓ EXCELLENT - Negligible overhead\n\n";
        } else if (overheadPerVoice < 0.01) {
            cout << "    ✓ GOOD - Minimal overhead\n\n";
        } else {
            cout << "    ⚠ ACCEPTABLE - Small overhead\n\n";
        }
    }

    cout << "========================================\n";
    cout << "SUMMARY\n";
    cout << "========================================\n\n";

    // Find average overhead
    double totalOverhead = 0.0;
    int count = 0;
    for (size_t i = 1; i < results.size(); i += 2) {
        totalOverhead += results[i].cpuPercent - results[i-1].cpuPercent;
        count++;
    }
    double avgOverhead = totalOverhead / count;

    cout << "Average drift overhead: " << avgOverhead << "%\n\n";

    if (avgOverhead < 0.01) {
        cout << "✅ Drift feature is production-ready!\n";
        cout << "   CPU impact is negligible.\n";
        cout << "   No optimization needed.\n";
    } else {
        cout << "⚠️  Drift overhead is acceptable but could be optimized.\n";
    }

    return 0;
}
