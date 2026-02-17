/*
    CompleteVoiceBenchmark.cpp - Full Synth Performance Test

    Measures actual CPU of complete voice rendering with:
    - Multiple oscillators
    - Filter processing
    - Envelopes
    - LFO modulation
    - Unison voices
    - All features combined

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include <juce_core/juce_core.h>
#include "zenith_core/instruments/ZenithPolySynth.h"
#include "zenith_core/instruments/ZenithPolySynthVoice.h"
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace juce;
using namespace zenith;

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

struct VoiceTestResult {
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

VoiceTestResult benchmarkFullVoice(int numVoices, int sampleRate, int blockSize, int durationMs) {
    VoiceTestResult result;
    result.numVoices = numVoices;

    // Create synth
    MPESynthesiser synth;

    // Add voices
    for (int i = 0; i < numVoices; ++i) {
        synth.addVoice(new ZenithPolySynthVoice());
    }

    synth.setCurrentPlaybackSampleRate(sampleRate);

    // Configure a realistic voice setup (warm pad preset)
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(synth.getVoice(i))) {
            // Oscillators: Detuned saws
            voice->setOsc1Waveform(OscillatorWaveform::Saw); // Sawtooth
            voice->setOsc1Mix(0.7f);
            voice->setOsc1Detune(-5.0f);
            voice->setOsc2Waveform(OscillatorWaveform::Saw); // Sawtooth
            voice->setOsc2Mix(0.7f);
            voice->setOsc2Detune(5.0f);

            // Filter
            voice->setFilterCutoff(2000.0f);
            voice->setFilterResonance(0.4f);
            voice->setFilterEnvAmount(0.5f);

            // Envelopes
            voice->setAmpEnvelope(0.02f, 0.3f, 0.7f, 0.5f);
            voice->setModEnvelope(0.02f, 0.2f, 0.6f, 0.4f);

            // LFO
            voice->setLFO1(3.0f, 0.2f, LFOTarget::FilterCutoff, LFOWaveform::Sine);

            // Unison (increases CPU significantly)
            voice->setUnisonVoices(4);
            voice->setUnisonDetune(15.0f);
            voice->setUnisonSpread(0.8f);
        }
    }

    // Prepare audio buffer
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    const int numBlocks = (sampleRate * durationMs / 1000) / blockSize;

    // Warm-up
    for (int b = 0; b < 10; ++b) {
        synth.renderNextBlock(buffer, midi, 0, blockSize);
    }

    // Benchmark
    double totalSamples = static_cast<double>(numBlocks * blockSize);
    double audioTimeSeconds = totalSamples / sampleRate;
    double realTimeUs = audioTimeSeconds * 1000000.0;

    PerfTimer timer;
    timer.start();

    for (int b = 0; b < numBlocks; ++b) {
        // Trigger notes for all voices
        for (int v = 0; v < numVoices; ++v) {
            auto midiNote = 60 + (v % 24); // Spread across 2 octaves
            auto velocity = 0.8f;
            synth.handleMidiEvent(juce::MidiMessage::noteOn(1, midiNote, velocity));
        }

        synth.renderNextBlock(buffer, midi, 0, blockSize);
    }

    double processingUs = timer.stopMicroseconds();
    result.cpuPercent = (processingUs / realTimeUs) * 100.0;
    result.cpuPerVoice = result.cpuPercent / numVoices;
    result.voicesPerPercentCPU = numVoices / result.cpuPercent;

    // Stop all notes
    synth.turnOffAllVoices(true);

    return result;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Complete Voice Performance Benchmark\n";
    std::cout << "========================================\n\n";

    std::cout << "Testing realistic voice usage:\n";
    std::cout << "  - Detuned saw oscillators (2)\n";
    std::cout << "  - Filter with envelope modulation\n";
    std::cout << "  - ADSR envelopes\n";
    std::cout << "  - LFO modulation\n";
    std::cout << "  - 4x unison voices per voice\n\n";

    const int sampleRate = 48000;
    const int blockSize = 256;
    const int durationMs = 2000; // 2 seconds

    std::cout << "Test parameters:\n";
    std::cout << "  Sample rate: " << sampleRate << " Hz\n";
    std::cout << "  Block size: " << blockSize << " samples\n";
    std::cout << "  Duration: " << durationMs << " ms\n\n";

    // Test different voice counts
    std::cout << "Testing voice scaling...\n\n";

    std::vector<VoiceTestResult> results;

    for (int numVoices : {1, 4, 8, 16}) {
        auto result = benchmarkFullVoice(numVoices, sampleRate, blockSize, durationMs);
        results.push_back(result);
        result.print();
    }

    // Summary
    std::cout << "========================================\n";
    std::cout << "SUMMARY - Comparison to Serum\n";
    std::cout << "========================================\n\n";
    std::cout << "Serum target: < 3% CPU per voice\n\n";

    for (const auto& r : results) {
        if (r.cpuPerVoice < 3.0f) {
            std::cout << "[EXCELLENT] " << r.numVoices << " voices: "
                      << r.cpuPerVoice << "% per voice - BEATS SERUM ✓\n";
        } else if (r.cpuPerVoice < 5.0f) {
            std::cout << "[GOOD] " << r.numVoices << " voices: "
                      << r.cpuPerVoice << "% per voice - Acceptable\n";
        } else {
            std::cout << "[NEEDS WORK] " << r.numVoices << " voices: "
                      << r.cpuPerVoice << "% per voice - Too slow\n";
        }
    }

    return 0;
}
