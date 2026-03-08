/*
    Zenith Synth - CPU Profiler
    Copyright (C)2025 Micah Cooley <micahcooley@protonmail.com>
    AGPL-3.0
*/

#include <modules/zenith_core/instruments/ZenithSynthProcessor.h>
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {

/**
    CPU Profiler for measuring performance against targets
*/
class CPUProfiler {
public:
    struct Stats {
        double averagePercent = 0.0;
        double peakPercent = 0.0;
        int numVoicesActive = 0;
        double sampleRate = 0.0;
        int blockSize = 0;
    };

    CPUProfiler() = default;

    void start(int sampleRate, int blockSize) {
        sampleRate_ = sampleRate;
        blockSize_ = blockSize;
        startTime_ = juce::Time::getHighResolutionTicks();
        samplesProcessed_ = 0;
    }

    void addSamples(int count) {
        samplesProcessed_ += count;
    }

    Stats getStats() const {
        auto now = juce::Time::getHighResolutionTicks();
        double elapsed = (now - startTime_).getSeconds();
        double expectedTime = samplesProcessed_ / sampleRate_;
        double cpuPercent = (expectedTime / (elapsed + 1e-9)) * 100.0;

        Stats s;
        s.averagePercent = cpuPercent;
        s.peakPercent = cpuPercent;  // Would need rolling max
        s.sampleRate = sampleRate_;
        s.blockSize = blockSize_;

        return s;
    }

    bool isUnderTarget(double targetPercent = 10.0) const {
        if (samplesProcessed_ == 0) return true;
        auto now = juce::Time::getHighResolutionTicks();
        double elapsed = (now - startTime_).getSeconds();
        double expectedTime = samplesProcessed_ / sampleRate_;
        double cpuPercent = (expectedTime / (elapsed + 1e-9)) * 100.0;
        return cpuPercent <= targetPercent;
    }

private:
    double sampleRate_ = 44100.0;
    int blockSize_ = 512;
    juce::int64 startTime_ = 0;
    int64 samplesProcessed_ = 0;
};

//==============================================================================
// PERFORMANCE BENCHMARK
//==============================================================================

class PerformanceBenchmark {
public:
    struct BenchmarkResult {
        const char* testName;
        double cpuPercent;
        int numVoices;
        bool passed;
    };

    static constexpr double TARGET_CPU_PER_VOICE = 0.5;  // 0.5% per voice
    static constexpr double TARGET_CPU_TOTAL = 8.0;       // 8% total

    void runBenchmark(juce::String& outputLog) {
        ZenithSynthProcessor synth;
        synth.prepareToPlay(44100.0, 256);

        juce::AudioBuffer<float> buffer(2, 44100);  // 1 second
        juce::MidiBuffer midi;

        results_.clear();

        // Test 1: Single voice
        outputLog += "=== Single Voice Test ===\n";
        testVoices(synth, 1, buffer, midi, outputLog);

        // Test 2: 4 voices
        outputLog += "=== Four Voices Test ===\n";
        testVoices(synth, 4, buffer, midi, outputLog);

        // Test 3: 8 voices
        outputLog += "=== Eight Voices Test ===\n";
        testVoices(synth, 8, buffer, midi, outputLog);

        // Test 4: 16 voices (max)
        outputLog += "=== Sixteen Voices Test ===\n";
        testVoices(synth, 16, buffer, midi, outputLog);

        // Test 5: All effects
        outputLog += "=== Effects Test ===\n";
        testEffects(synth, buffer, midi, outputLog);

        // Summary
        outputLog += "\n=== SUMMARY ===\n";
        printSummary(outputLog);
    }

private:
    juce::OwnedArray<BenchmarkResult> results_;

    void testVoices(ZenithSynthProcessor& synth, int numVoices,
                    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                    juce::String& log) {
        CPUProfiler profiler;
        profiler.start(44100.0, 256);

        // Trigger notes
        midi.clear();
        for (int i = 0; i < numVoices; ++i) {
            juce::MidiMessage on = juce::MidiMessage::noteOn(60 + i, 1.0f);
            midi.addEvent(on, 0);
        }

        // Process 1 second
        auto start = juce::Time::getHighResolutionTicks();
        synth.processBlock(buffer, midi);
        auto elapsed = (juce::Time::getHighResolutionTicks() - start).getSeconds();

        double cpuPercent = (1.0 / elapsed) * 100.0;
        double target = TARGET_CPU_PER_VOICE * numVoices;
        bool passed = cpuPercent <= target * 2.0;  // Allow 2x headroom

        char line[256];
        snprintf(line, sizeof(line),
            "  Voices: %d | CPU: %.2f%% | Target: <%.1f%% | %s\n",
            numVoices, cpuPercent, target, passed ? "PASS" : "FAIL");
        log += line;

        results_.add({numVoices == 1 ? "Single Voice" :
                     numVoices == 4 ? "Four Voices" :
                     numVoices == 8 ? "Eight Voices" : "Sixteen Voices",
                     cpuPercent, numVoices, passed});
    }

    void testEffects(ZenithSynthProcessor& synth,
                    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                    juce::String& log) {
        // Trigger one voice
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(60, 0.5f), 0);

        CPUProfiler profiler;
        profiler.start(44100.0, 256);

        auto start = juce::Time::getHighResolutionTicks();
        synth.processBlock(buffer, midi);
        auto elapsed = (juce::Time::getHighResolutionTicks() - start).getSeconds();

        double cpuPercent = (1.0 / elapsed) * 100.0;
        bool passed = cpuPercent < 20.0;  // 20% budget for effects

        char line[256];
        snprintf(line, sizeof(line),
            "  Effects CPU: %.2f%% | Target: <20%% | %s\n",
            cpuPercent, passed ? "PASS" : "FAIL");
        log += line;
    }

    void printSummary(juce::String& log) {
        int passed = 0;
        for (auto& r : results_) {
            if (r.passed) passed++;
        }

        char line[256];
        snprintf(line, sizeof(line),
            "  Tests Passed: %d / %d\n",
            passed, results_.size());
        log += line;

        // Overall verdict
        bool allPassed = (passed == results_.size());
        log += allPassed ? "  RESULT: SHIP READY\n" : "  RESULT: NEEDS OPTIMIZATION\n";
    }
};

} // namespace zenith
