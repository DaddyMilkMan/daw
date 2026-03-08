/*
    CompleteSynthBenchmark.cpp - Full Synth Voice CPU Test

    Measures realistic synth performance with:
    - 2 Detuned oscillators
    - Filter with envelope modulation
    - ADSR Amplitude envelope
    - LFO modulation
    - Stereo output

    This approximates real-world synth usage.

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
// Simple Oscillator
//==============================================================================
class Oscillator {
private:
    double phase_ = 0.0;
    double sampleRate_;

public:
    Oscillator(double sr) : sampleRate_(sr) {}

    void setSampleRate(double sr) { sampleRate_ = sr; }

    inline float saw(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return 2.0f * static_cast<float>(phase_) - 1.0f;
    }

    inline float square(float freq) {
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return (phase_ < 0.5) ? 1.0f : -1.0f;
    }
};

//==============================================================================
// State Variable Filter
//==============================================================================
class Filter {
private:
    double sampleRate_;
    double frequency_ = 1000.0;
    double resonance_ = 0.5;
    double low_ = 0.0;
    double high_ = 0.0;
    double band_ = 0.0;

public:
    Filter(double sr) : sampleRate_(sr) {}

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setCutoff(double f) { frequency_ = f; }
    void setResonance(double r) { resonance_ = r; }

    inline float processLow(float input, double mod = 0.0) {
        double fc = frequency_ + mod;
        fc = std::max(20.0, std::min(20000.0, fc));
        double f = fc / sampleRate_;
        double r = resonance_;

        low_ += f * band_;
        high_ = input - low_ - r * band_;
        band_ += f * high_;

        return static_cast<float>(low_);
    }
};

//==============================================================================
// ADSR Envelope
//==============================================================================
class Envelope {
private:
    double sampleRate_;
    double attack_ = 0.01;
    double decay_ = 0.3;
    double sustain_ = 0.7;
    double release_ = 0.5;
    double level_ = 0.0;
    int stage_ = 0; // 0=attack, 1=decay, 2=sustain, 3=release, 4=idle
    double timeInStage_ = 0.0;

public:
    Envelope(double sr) : sampleRate_(sr) {}

    void setADSR(double a, double d, double s, double r) {
        attack_ = a;
        decay_ = d;
        sustain_ = s;
        release_ = r;
    }

    void trigger() {
        stage_ = 0;
        timeInStage_ = 0.0;
    }

    void release() {
        if (stage_ != 4) {
            stage_ = 3;
            timeInStage_ = 0.0;
        }
    }

    inline float process() {
        double inc = 1.0 / sampleRate_;
        timeInStage_ += inc;

        switch (stage_) {
            case 0: // Attack
                level_ = timeInStage_ / attack_;
                if (timeInStage_ >= attack_) {
                    stage_ = 1;
                    timeInStage_ = 0.0;
                }
                break;
            case 1: // Decay
                level_ = 1.0 - (1.0 - sustain_) * (timeInStage_ / decay_);
                if (timeInStage_ >= decay_) {
                    stage_ = 2;
                    timeInStage_ = 0.0;
                }
                break;
            case 2: // Sustain
                level_ = sustain_;
                break;
            case 3: // Release
                level_ = sustain_ * (1.0 - timeInStage_ / release_);
                if (timeInStage_ >= release_) {
                    stage_ = 4;
                    level_ = 0.0;
                }
                break;
            case 4: // Idle
                level_ = 0.0;
                break;
        }

        return static_cast<float>(level_);
    }

    bool isActive() const { return stage_ != 4; }
};

//==============================================================================
// LFO
//==============================================================================
class LFO {
private:
    double phase_ = 0.0;
    double sampleRate_;
    double rate_ = 5.0;
    double amount_ = 0.5;

public:
    LFO(double sr) : sampleRate_(sr) {}

    void setRate(double r) { rate_ = r; }
    void setAmount(double a) { amount_ = a; }

    inline float process() {
        phase_ += rate_ / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return std::sin(phase_ * 6.28318530718) * static_cast<float>(amount_);
    }
};

//==============================================================================
// Complete Synth Voice
//==============================================================================
class SynthVoice {
private:
    Oscillator osc1_, osc2_;
    Filter filter_;
    Envelope ampEnv_, modEnv_;
    LFO lfo_;
    double sampleRate_;
    float baseFreq_ = 440.0f;
    float detune1_ = -5.0f;
    float detune2_ = 5.0f;

public:
    SynthVoice(double sr)
        : osc1_(sr), osc2_(sr), filter_(sr), ampEnv_(sr), modEnv_(sr), lfo_(sr), sampleRate_(sr) {}

    void setFreq(float freq) {
        baseFreq_ = freq;
    }

    void setDetune(float d1, float d2) {
        detune1_ = d1;
        detune2_ = d2;
    }

    void setFilter(double cutoff, double resonance) {
        filter_.setCutoff(cutoff);
        filter_.setResonance(resonance);
    }

    void setADSR(double a, double d, double s, double r) {
        ampEnv_.setADSR(a, d, s, r);
    }

    void trigger() {
        ampEnv_.trigger();
        modEnv_.trigger();
    }

    void release() {
        ampEnv_.release();
        modEnv_.release();
    }

    inline float process() {
        float amp = ampEnv_.process();
        float mod = modEnv_.process();
        float lfoVal = lfo_.process();

        // Oscillators with detune
        float f1 = baseFreq_ * std::pow(2.0, detune1_ / 1200.0);
        float f2 = baseFreq_ * std::pow(2.0, detune2_ / 1200.0);

        float sample = (osc1_.saw(f1) + osc2_.saw(f2)) * 0.5f;

        // Filter with envelope + LFO modulation
        double modAmount = mod * 2000.0 + lfoVal * 500.0;
        sample = filter_.processLow(sample, modAmount);

        // Apply amplitude envelope
        sample *= amp;

        return sample;
    }

    bool isActive() const { return ampEnv_.isActive(); }
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
// Synth Benchmark Result
//==============================================================================
struct SynthBenchmarkResult {
    int numVoices;
    double cpuPercent;
    double cpuPerVoice;
    double voicesPerPercentCPU;

    void print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  " << numVoices << " voices:\n";
        std::cout << "    Total CPU: " << cpuPercent << "%\n";
        std::cout << "    Per Voice: " << cpuPerVoice << "%\n";
        std::cout << "    Voices/%CPU: " << voicesPerPercentCPU << "\n\n";

        // Assessment
        if (cpuPerVoice < 3.0f) {
            std::cout << "    ✓ EXCELLENT - Beats Serum target!\n";
        } else if (cpuPerVoice < 5.0f) {
            std::cout << "    ✓ GOOD - Acceptable performance\n";
        } else {
            std::cout << "    ✗ NEEDS OPTIMIZATION\n";
        }
        std::cout << "\n";
    }
};

//==============================================================================
// Benchmark Complete Synth Voice
//==============================================================================
SynthBenchmarkResult benchmarkSynth(int numVoices, int sampleRate, int durationSeconds) {
    SynthBenchmarkResult result;
    result.numVoices = numVoices;

    int numSamples = durationSeconds * sampleRate;
    double realTimeUs = (durationSeconds * 1000000.0);

    // Create voices
    std::vector<SynthVoice> voices;
    voices.reserve(numVoices);
    for (int v = 0; v < numVoices; ++v) {
        voices.emplace_back(sampleRate);
        voices[v].setFreq(220.0f + v * 20.0f);
        voices[v].setDetune(-5.0f, 5.0f);
        voices[v].setFilter(2000.0, 0.5);
        voices[v].setADSR(0.02, 0.3, 0.7, 0.5);
    }

    // Trigger all voices
    for (auto& voice : voices) {
        voice.trigger();
    }

    // Warm-up
    for (int i = 0; i < 1000; ++i) {
        for (auto& voice : voices) {
            volatile float output = voice.process();
            (void)output;
        }
    }

    // Benchmark
    PerfTimer timer;
    timer.start();

    for (int i = 0; i < numSamples; ++i) {
        float mix = 0.0f;
        for (auto& voice : voices) {
            mix += voice.process();
        }
        volatile float sink = mix / numVoices; // Prevent optimization
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
int main() {
    std::cout << "========================================\n";
    std::cout << "Complete Synth Performance Benchmark\n";
    std::cout << "========================================\n\n";

    std::cout << "Testing realistic synth voice:\n";
    std::cout << "  - 2 Detuned saw oscillators\n";
    std::cout << "  - State Variable Filter\n";
    std::cout << "  - ADSR Amplitude Envelope\n";
    std::cout << "  - ADSR Modulation Envelope\n";
    std::cout << "  - LFO modulating filter\n\n";

    const int sampleRate = 48000;
    const int durationSeconds = 5;

    std::cout << "Test parameters:\n";
    std::cout << "  Sample rate: " << sampleRate << " Hz\n";
    std::cout << "  Duration: " << durationSeconds << " seconds\n\n";

    std::cout << "Testing voice scaling...\n\n";

    std::vector<SynthBenchmarkResult> results;

    for (int numVoices : {1, 4, 8, 16, 32}) {
        auto result = benchmarkSynth(numVoices, sampleRate, durationSeconds);
        results.push_back(result);
        result.print();
    }

    // Summary
    std::cout << "========================================\n";
    std::cout << "SUMMARY - Comparison to Serum\n";
    std::cout << "========================================\n\n";
    std::cout << "Serum target: < 3% CPU per voice\n\n";

    std::cout << "Component breakdown (from previous tests):\n";
    std::cout << "  Oscillators: 0.006% per voice\n";
    std::cout << "  Filter: 0.01% per voice\n";
    std::cout << "  Envelopes + LFO: ~0.01% per voice (estimated)\n";
    std::cout << "  Expected total: ~0.03% per voice\n\n";

    std::cout << "Actual measured (complete voice):\n";
    for (const auto& r : results) {
        std::cout << "  " << r.numVoices << " voices: " << r.cpuPerVoice << "% per voice";
        if (r.cpuPerVoice < 3.0f) {
            std::cout << " ✓ BEATS SERUM\n";
        } else if (r.cpuPerVoice < 5.0f) {
            std::cout << " - Acceptable\n";
        } else {
            std::cout << " - Too slow\n";
        }
    }

    std::cout << "\nProfessional assessment:\n";
    double avgCpuPerVoice = results.back().cpuPerVoice;
    if (avgCpuPerVoice < 3.0) {
        std::cout << "  ✅ EXCELLENT - Matches or beats Serum performance\n";
        std::cout << "  → Proceed to Phase 2B (Unique Strengths)\n";
    } else if (avgCpuPerVoice < 5.0) {
        std::cout << "  ✅ GOOD - Acceptable performance\n";
        std::cout << "  → Consider optimization OR proceed to Phase 2B\n";
    } else {
        std::cout << "  ⚠️  NEEDS OPTIMIZATION\n";
        std::cout << "  → Profile to find bottlenecks\n";
        std::cout << "  → Implement targeted fixes\n";
    }

    return 0;
}
