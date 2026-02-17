/*
    SynthPerformanceBenchmark.cpp - Comprehensive CPU Profiling Test for Zenith PolySynth

    This test measures ACTUAL CPU performance to answer the critical question:
    "Can Zenith beat Serum's ~2-3% CPU per voice?"

    Usage:
    - Run with different voice counts (1, 4, 8, 16, 32)
    - Test at different sample rates (44.1k, 48k, 96k)
    - Test all quality presets (None, 2x, 4x oversampling)
    - Identify bottlenecks (oscillators, filters, envelopes, modulation)

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "zenith_core/instruments/ZenithPolySynth.h"
#include "zenith_core/instruments/ZenithPolySynthVoice.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

using namespace juce;
using namespace zenith;

//==============================================================================
// Performance Timer - High precision timing
//==============================================================================
class PerformanceTimer {
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

    double stopMilliseconds() {
        return stopMicroseconds() / 1000.0;
    }

private:
    Clock::time_point startTime_;
};

//==============================================================================
// Benchmark Result - Store metrics for each test
//==============================================================================
struct BenchmarkResult {
    int numVoices;
    double sampleRate;
    int blockSize;
    QualityPreset quality;
    double realTimeSeconds;       // Wall clock time for test
    double audioTimeSeconds;      // Actual audio duration rendered
    double cpuPercent;            // CPU usage percentage
    double cpuPercentPerVoice;    // CPU per voice
    double voicesPerPercent;      // How many voices per 1% CPU
    std::chrono::microseconds processingTime;  // Total processing time
    size_t totalSamples;          // Total samples processed

    void print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Voices: " << numVoices << " | ";
        std::cout << "Sample Rate: " << sampleRate/1000.0 << "kHz | ";
        std::cout << "Quality: ";
        switch (quality) {
            case QualityPreset::Low: std::cout << "None"; break;
            case QualityPreset::Medium: std::cout << "2x"; break;
            case QualityPreset::High: std::cout << "4x"; break;
            default: std::cout << "Unknown"; break;
        }
        std::cout << "\n";
        std::cout << "    CPU: " << cpuPercent << "% | ";
        std::cout << "Per Voice: " << cpuPercentPerVoice << "%/voice | ";
        std::cout << "Voices/%CPU: " << voicesPerPercent << "\n";
        std::cout << "    Processing: " << processingTime.count() / 1000.0 << "ms | ";
        std::cout << "Audio: " << audioTimeSeconds << "s\n";
    }
};

//==============================================================================
// Synth Configuration - Different preset configurations to test
//==============================================================================
struct SynthConfiguration {
    const char* name;
    std::function<void(ZenithPolySynthVoice&)> apply;
};

std::vector<SynthConfiguration> getTestConfigurations() {
    return {
        {
            "Minimal - Sine Only",
            [](ZenithPolySynthVoice& voice) {
                voice.setOsc1Waveform(OscillatorWaveform::Sine); // Sine
                voice.setOsc1Mix(1.0f);
                voice.setOsc2Mix(0.0f);
                voice.setOsc3Mix(0.0f);
                voice.setFilterCutoff(20000.0f); // Wide open
                voice.setFilterResonance(0.0f);
                voice.setLFO1(1.0f, 0.0f, LFOTarget::FilterCutoff, LFOWaveform::Sine);
                voice.setLFO2(1.0f, 0.0f, LFOTarget::FilterCutoff, LFOWaveform::Sine);
                voice.setUnisonVoices(1);
                voice.setAmpEnvelope(0.01f, 0.1f, 0.7f, 0.1f);
            }
        },
        {
            "Bass - Saw + Filter",
            [](ZenithPolySynthVoice& voice) {
                voice.setOsc1Waveform(OscillatorWaveform::Saw); // Sawtooth
                voice.setOsc1Mix(1.0f);
                voice.setOsc2Mix(0.0f);
                voice.setOsc3Mix(0.0f);
                voice.setFilterCutoff(800.0f);
                voice.setFilterResonance(0.3f);
                voice.setFilterEnvAmount(0.7f);
                voice.setLFO1(1.0f, 0.0f, LFOTarget::FilterCutoff, LFOWaveform::Sine);
                voice.setLFO2(1.0f, 0.0f, LFOTarget::FilterCutoff, LFOWaveform::Sine);
                voice.setUnisonVoices(1);
                voice.setAmpEnvelope(0.01f, 0.2f, 0.5f, 0.3f);
                voice.setModEnvelope(0.01f, 0.1f, 0.0f, 0.1f);
            }
        },
        {
            "Lead - 2 Osc + Unison",
            [](ZenithPolySynthVoice& voice) {
                voice.setOsc1Waveform(OscillatorWaveform::Saw); // Sawtooth
                voice.setOsc2Waveform(OscillatorWaveform::Sine); // Sine (detuned)
                voice.setOsc1Mix(0.7f);
                voice.setOsc2Mix(0.3f);
                voice.setOsc3Mix(0.0f);
                voice.setOsc1Detune(0.0f);
                voice.setOsc2Detune(10.0f); // Detune
                voice.setFilterCutoff(3000.0f);
                voice.setFilterResonance(0.2f);
                voice.setLFO1(5.0f, 0.3f, LFOTarget::FilterCutoff, LFOWaveform::Sine);
                voice.setUnisonVoices(4);
                voice.setUnisonDetune(15.0f);
                voice.setUnisonSpread(0.8f);
                voice.setAmpEnvelope(0.01f, 0.2f, 0.6f, 0.2f);
            }
        },
        {
            "Pad - 3 Osc + Unison + Effects",
            [](ZenithPolySynthVoice& voice) {
                voice.setOsc1Waveform(OscillatorWaveform::Sine); // Sine
                voice.setOsc2Waveform(OscillatorWaveform::Saw); // Saw
                voice.setOsc3Waveform(OscillatorWaveform::Square); // Square
                voice.setOsc1Mix(0.4f);
                voice.setOsc2Mix(0.4f);
                voice.setOsc3Mix(0.2f);
                voice.setOsc1Detune(-5.0f);
                voice.setOsc2Detune(0.0f);
                voice.setOsc3Detune(5.0f);
                voice.setFilterCutoff(2000.0f);
                voice.setFilterResonance(0.4f);
                voice.setFilterEnvAmount(0.5f);
                voice.setLFO1(0.5f, 0.4f, LFOTarget::FilterCutoff, LFOWaveform::Sine);
                voice.setLFO2(3.0f, 0.2f, LFOTarget::Osc1Pitch, LFOWaveform::Sine);
                voice.setUnisonVoices(7);
                voice.setUnisonDetune(20.0f);
                voice.setUnisonSpread(1.0f);
                voice.setAmpEnvelope(0.3f, 0.5f, 0.7f, 1.0f);
                voice.setModEnvelope(0.1f, 0.3f, 0.5f, 0.5f);
            }
        },
        {
            "Complex - Full Modulation",
            [](ZenithPolySynthVoice& voice) {
                voice.setOsc1Waveform(OscillatorWaveform::Saw);
                voice.setOsc2Waveform(OscillatorWaveform::Saw);
                voice.setOsc3Waveform(OscillatorWaveform::Sine);
                voice.setOsc1Mix(0.5f);
                voice.setOsc2Mix(0.3f);
                voice.setOsc3Mix(0.2f);
                voice.setOsc2FM(0.3f);
                voice.setRingMod(0.2f);
                voice.setSubOscLevel(0.3f);
                voice.setSubOscOctave(-2);
                voice.setNoiseLevel(0.05f);
                voice.setFilterCutoff(1500.0f);
                voice.setFilterResonance(0.6f);
                voice.setFilterEnvAmount(0.8f);
                voice.setFilterKeyTrack(FilterKeyTrack::Full);
                voice.setLFO1(3.0f, 0.5f, LFOTarget::Osc1Pitch, LFOWaveform::Sine);
                voice.setLFO2(7.0f, 0.4f, LFOTarget::Osc1Shape, LFOWaveform::Sine);
                voice.setUnisonVoices(7);
                voice.setUnisonDetune(25.0f);
                voice.setUnisonSpread(1.0f);
                voice.setUnisonPanRandom(true);
                voice.setAmpEnvelope(0.01f, 0.3f, 0.6f, 0.4f);
                voice.setModEnvelope(0.01f, 0.2f, 0.0f, 0.3f);
                // Set up modulation matrix
                voice.setModulationSlot(0, ModulationSource::Velocity,
                                       ModulationDestination::FilterCutoff, 0.3f);
                voice.setModulationSlot(1, ModulationSource::Aftertouch,
                                       ModulationDestination::AmpGain, 0.2f);
            }
        }
    };
}

//==============================================================================
// Benchmark Test Runner
//==============================================================================
class SynthBenchmark {
public:
    SynthBenchmark() {
        // Create synth
        for (int i = 0; i < 16; ++i)
            synth_.addVoice(new ZenithPolySynthVoice());

        synth_.setCurrentPlaybackSampleRate(44100.0);
    }

    BenchmarkResult runBenchmark(int numVoices, double sampleRate, int blockSize,
                                 QualityPreset quality,
                                 const SynthConfiguration& config,
                                 double durationSeconds = 5.0) {
        // Setup
        synth_.setCurrentPlaybackSampleRate(sampleRate);

        // Configure all voices
        for (int i = 0; i < synth_.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<ZenithPolySynthVoice*>(synth_.getVoice(i))) {
                voice->setQualityPreset(quality);
                voice->setSampleRate(sampleRate);
                config.apply(*voice);
            }
        }

        // Start notes
        for (int i = 0; i < numVoices; ++i) {
            auto midiNote = 60 + (i % 24); // Spread across 2 octaves
            auto velocity = 0.8f;
            synth_.handleMidiEvent(juce::MidiMessage::noteOn(1, midiNote, velocity));
        }

        // Prepare buffer
        const int numSamples = static_cast<int>(durationSeconds * sampleRate);
        const int totalBlocks = (numSamples + blockSize - 1) / blockSize;
        AudioBuffer<float> buffer(2, blockSize);
        MidiBuffer midi;

        // Warm-up (avoid cache misses affecting first measurement)
        for (int i = 0; i < 10; ++i) {
            synth_.renderNextBlock(buffer, midi, 0, blockSize);
        }

        // Benchmark
        PerformanceTimer timer;
        timer.start();

        for (int block = 0; block < totalBlocks; ++block) {
            int samplesInBlock = jmin(blockSize, numSamples - block * blockSize);
            synth_.renderNextBlock(buffer, midi, 0, samplesInBlock);
        }

        auto elapsedUs = timer.stopMicroseconds();
        auto elapsedSeconds = elapsedUs / 1'000'000.0;

        // Calculate metrics
        BenchmarkResult result;
        result.numVoices = numVoices;
        result.sampleRate = sampleRate;
        result.blockSize = blockSize;
        result.quality = quality;
        result.realTimeSeconds = elapsedSeconds;
        result.audioTimeSeconds = durationSeconds;
        result.processingTime = std::chrono::microseconds(static_cast<int64>(elapsedUs));
        result.totalSamples = numSamples * 2; // Stereo

        // CPU% = (processing time / real time) * 100
        result.cpuPercent = (elapsedSeconds / durationSeconds) * 100.0;
        result.cpuPercentPerVoice = result.cpuPercent / numVoices;
        result.voicesPerPercent = numVoices / result.cpuPercent;

        // Stop all notes
        synth_.turnOffAllVoices(true);

        return result;
    }

    void runComprehensiveSuite() {
        std::cout << "=================================================\n";
        std::cout << "Zenith PolySynth CPU Performance Benchmark\n";
        std::cout << "=================================================\n\n";

        auto configs = getTestConfigurations();

        // Test 1: Voice scaling at 48kHz, medium quality
        std::cout << "Test 1: Voice Scaling (48kHz, 2x oversampling)\n";
        std::cout << "-------------------------------------------------\n";
        for (auto& config : configs) {
            std::cout << "\nConfiguration: " << config.name << "\n";
            for (int voices : {1, 4, 8, 16}) {
                auto result = runBenchmark(voices, 48000.0, 256,
                                          QualityPreset::Medium,
                                          config, 2.0);
                result.print();
            }
        }

        // Test 2: Sample rate comparison
        std::cout << "\n\nTest 2: Sample Rate Comparison (8 voices)\n";
        std::cout << "--------------------------------------------\n";
        for (auto& config : configs) {
            std::cout << "\nConfiguration: " << config.name << "\n";
            for (double sampleRate : {44100.0, 48000.0, 96000.0}) {
                auto result = runBenchmark(8, sampleRate, 256,
                                          QualityPreset::Medium,
                                          config, 2.0);
                result.print();
            }
        }

        // Test 3: Quality preset comparison
        std::cout << "\n\nTest 3: Quality Preset Comparison (8 voices, 48kHz)\n";
        std::cout << "------------------------------------------------------\n";
        for (auto& config : configs) {
            std::cout << "\nConfiguration: " << config.name << "\n";
            for (auto quality : {QualityPreset::Low,
                                 QualityPreset::Medium,
                                 QualityPreset::High}) {
                auto result = runBenchmark(8, 48000.0, 256, quality, config, 2.0);
                result.print();
            }
        }

        // Test 4: Realistic workload - 16 voices at 48kHz
        std::cout << "\n\nTest 4: Realistic Workload (16 voices, 48kHz)\n";
        std::cout << "------------------------------------------------\n";
        for (auto& config : configs) {
            std::cout << "\nConfiguration: " << config.name << "\n";
            auto result = runBenchmark(16, 48000.0, 256,
                                      QualityPreset::Medium,
                                      config, 5.0);
            result.print();

            // Compare to Serum target
            std::cout << "\n    Comparison to Serum (~2-3% per voice):\n";
            if (result.cpuPercentPerVoice < 2.0f) {
                std::cout << "    ✓ EXCELLENT - Better than Serum!\n";
            } else if (result.cpuPercentPerVoice < 3.0f) {
                std::cout << "    ✓ GOOD - Matches Serum performance\n";
            } else if (result.cpuPercentPerVoice < 5.0f) {
                std::cout << "    ⚠ ACCEPTABLE - Slightly worse than Serum\n";
            } else {
                std::cout << "    ✗ NEEDS OPTIMIZATION - Significantly worse than Serum\n";
            }
        }
    }

private:
    MPESynthesiser synth_;
};

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    std::cout << "Zenith PolySynth Performance Benchmark\n";
    std::cout << "======================================\n\n";

    // Print system info
    std::cout << "System Information:\n";
    std::cout << "  CPU Cores: " << SystemStats::getNumCpus() << "\n";
    std::cout << "  CPU Speed: " << SystemStats::getCpuSpeedInMegahertz() << " MHz\n";
    std::cout << "  Memory: " << SystemStats::getMemorySizeInMegabytes() << " MB\n\n";

    SynthBenchmark benchmark;
    benchmark.runComprehensiveSuite();

    std::cout << "\n\nBenchmark complete!\n";
    std::cout << "\nKey Findings:\n";
    std::cout << "- If CPU per voice > 5%, optimization is needed\n";
    std::cout << "- Target: < 3% per voice (Serum performance)\n";
    std::cout << "- If voices per %CPU < 2, voice stealing may be needed\n";
    std::cout << "- Higher quality presets increase CPU significantly\n";

    return 0;
}
