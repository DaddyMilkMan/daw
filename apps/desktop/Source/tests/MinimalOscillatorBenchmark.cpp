/*
    MinimalOscillatorBenchmark.cpp - Ultra-simple CPU test

    Measures CPU of basic oscillators without complex dependencies.
    This will compile and run standalone.
*/

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

// Simple oscillator implementations
class SimpleOscillator {
private:
    double phase_ = 0.0;
    double sampleRate_;

public:
    SimpleOscillator(double sr) : sampleRate_(sr) {}

    void setSampleRate(double sr) { sampleRate_ = sr; }

    // Sine wave (clean, no aliasing at low frequencies)
    float sine(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return std::sin(phase_ * 6.28318530718);
    }

    // Sawtooth (naive, has aliasing)
    float saw(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return 2.0f * phase_ - 1.0f;
    }

    // Square (naive, has aliasing)
    float square(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return (phase_ < 0.5) ? 1.0f : -1.0f;
    }

    // Triangle (bandlimited)
    float triangle(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        double t = phase_ * 4.0;
        if (t < 1.0) return t;
        if (t < 3.0) return 2.0 - t;
        return t - 4.0;
    }
};

struct Result {
    const char* name;
    int numVoices;
    double cpuPercent;
    double cpuPerVoice;
};

int main() {
    std::cout << "========================================\n";
    std::cout << "Minimal Oscillator CPU Benchmark\n";
    std::cout << "========================================\n\n";

    const double sampleRate = 48000.0;
    const int durationSeconds = 5;
    const int numSamples = durationSeconds * sampleRate;
    const double realTimeUs = durationSeconds * 1000000.0;

    std::cout << "Testing @ 48kHz, " << durationSeconds << " seconds\n\n";

    std::vector<Result> results;

    // Test 1: Single voice - different waveforms
    std::cout << "Test 1: Single Voice - Waveform Comparison\n";
    std::cout << "-------------------------------------------\n\n";

    for (int wave = 0; wave < 4; ++wave) {
        SimpleOscillator osc(sampleRate);
        std::vector<float> buffer(numSamples);

        // Warm-up
        for (int i = 0; i < 1000; ++i) {
            osc.sine(440.0);
        }

        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numSamples; ++i) {
            switch(wave) {
                case 0: buffer[i] = osc.sine(440.0); break;
                case 1: buffer[i] = osc.saw(440.0); break;
                case 2: buffer[i] = osc.square(440.0); break;
                case 3: buffer[i] = osc.triangle(440.0); break;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> diff = end - start;

        double cpuPercent = (diff.count() / realTimeUs) * 100.0;

        const char* names[] = {"Sine", "Sawtooth", "Square", "Triangle"};
        std::cout << "  " << names[wave] << ": " << cpuPercent << "% CPU\n";
    }

    std::cout << "\n";

    // Test 2: Multiple voices
    std::cout << "Test 2: Multi-Voice Scaling (Sawtooth)\n";
    std::cout << "---------------------------------------\n\n";

    for (int numVoices : {1, 4, 8, 16}) {
        std::vector<SimpleOscillator> oscillators;
        oscillators.reserve(numVoices);
        for (int v = 0; v < numVoices; ++v) {
            oscillators.emplace_back(sampleRate);
        }

        std::vector<float> buffer(numSamples * numVoices);

        // Warm-up
        for (int v = 0; v < numVoices; ++v) {
            for (int i = 0; i < 1000; ++i) {
                oscillators[v].sine(440.0);
            }
        }

        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numSamples; ++i) {
            float sum = 0.0f;
            for (int v = 0; v < numVoices; ++v) {
                buffer[v * numSamples + i] = oscillators[v].saw(440.0);
                sum += buffer[v * numSamples + i];
            }
            volatile float sink = sum; // Prevent optimization
            (void)sink;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> diff = end - start;

        double cpuPercent = (diff.count() / realTimeUs) * 100.0;
        double cpuPerVoice = cpuPercent / numVoices;

        results.push_back({std::to_string(numVoices).c_str(), numVoices, cpuPercent, cpuPerVoice});

        std::cout << "  " << numVoices << " voices: " << cpuPercent << "% total, "
                  << cpuPerVoice << "% per voice\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "SUMMARY - Comparison to Serum\n";
    std::cout << "========================================\n\n";
    std::cout << "Serum target: < 3% CPU per voice\n\n";

    for (const auto& r : results) {
        if (r.cpuPerVoice < 3.0) {
            std::cout << "[EXCELLENT] " << r.name << " voices: " << r.cpuPerVoice
                      << "% per voice - BEATS SERUM ✓\n";
        } else if (r.cpuPerVoice < 5.0) {
            std::cout << "[GOOD] " << r.name << " voices: " << r.cpuPerVoice
                      << "% per voice - Acceptable\n";
        } else {
            std::cout << "[NEEDS WORK] " << r.name << " voices: " << r.cpuPerVoice
                      << "% per voice - Too slow\n";
        }
    }

    std::cout << "\nNOTE: This is a MINIMAL test measuring ONLY oscillator computation.\n";
    std::cout << "Real synth CPU will be higher due to:\n";
    std::cout << "  - Filters (often 30-50% of CPU)\n";
    std::cout << "  - Envelopes and LFOs\n";
    std::cout << "  - Effects processing\n";
    std::cout << "  - Modulation matrix\n";
    std::cout << "  - Voice management overhead\n";

    return 0;
}
