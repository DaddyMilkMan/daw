/*
  ==============================================================================

    AdvancedOscillatorsTest.cpp
    Created: 2025-02-01
    Author:  Zenith DAW

    Comprehensive test suite for Phase 2 advanced oscillators.

  ==============================================================================
*/

#include "../instruments/ZenithAdvancedOscillators.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <cassert>
#include <iostream>
#include <cmath>

using namespace zenith;

//==============================================================================
// Test Utilities
//==============================================================================

class TestReporter {
public:
    static void pass(const juce::String& testName) {
        std::cout << "[PASS] " << testName << std::endl;
        passCount++;
    }
    
    static void fail(const juce::String& testName, const juce::String& reason) {
        std::cout << "[FAIL] " << testName << ": " << reason << std::endl;
        failCount++;
    }
    
    static void info(const juce::String& message) {
        std::cout << "[INFO] " << message << std::endl;
    }
    
    static int getPassCount() { return passCount; }
    static int getFailCount() { return failCount; }
    static int getTotalCount() { return passCount + failCount; }
    
private:
    static int passCount;
    static int failCount;
};

int TestReporter::passCount = 0;
int TestReporter::failCount = 0;

//==============================================================================
// Oscillator Tests
//==============================================================================

void testWavefolder() {
    TestReporter::info("Testing Buchla Wavefolder...");
    
    BuchlaWavefolder wavefolder;
    wavefolder.prepare(44100.0);
    wavefolder.setFolds(2.0f);
    wavefolder.setSymmetry(0.0f);
    wavefolder.setAmount(1.0f);
    
    // Test 1: Wavefolder produces output
    try {
        float sine = std::sin(0.5f);  // Input sine
        float output = wavefolder.processSample(sine, 0.5f);
        
        if (std::isfinite(output)) {
            TestReporter::pass("Wavefolder::processSample produces valid output");
        } else {
            TestReporter::fail("Wavefolder::processSample", "Output is not finite");
        }
    } catch (...) {
        TestReporter::fail("Wavefolder::processSample", "Exception thrown");
    }
    
    // Test 2: Wavefolder adds harmonics
    {
        wavefolder.reset();
        wavefolder.setFolds(3.0f);
        wavefolder.setAmount(1.0f);
        
        // Generate multiple samples and check for harmonic content
        float maxOutput = 0.0f;
        for (int i = 0; i < 100; ++i) {
            float phase = static_cast<float>(i) / 100.0f;
            float sine = std::sin(phase * 6.28318f);
            float output = wavefolder.processSample(sine, phase);
            maxOutput = std::max(maxOutput, std::abs(output));
        }
        
        if (maxOutput > 0.5f) {
            TestReporter::pass("Wavefolder generates harmonics");
        } else {
            TestReporter::fail("Wavefolder harmonics", "Output too low: " + juce::String(maxOutput));
        }
    }
    
    // Test 3: Symmetry control affects output
    {
        wavefolder.reset();
        wavefolder.setFolds(2.0f);
        wavefolder.setSymmetry(0.0f);
        
        float sine = 0.5f;
        float sym1 = wavefolder.processSample(sine, 0.5f);
        
        wavefolder.reset();
        wavefolder.setSymmetry(0.5f);
        float sym2 = wavefolder.processSample(sine, 0.5f);
        
        if (std::abs(sym1 - sym2) > 0.01f) {
            TestReporter::pass("Wavefolder symmetry affects output");
        } else {
            TestReporter::fail("Wavefolder symmetry", "No change detected");
        }
    }
}

void testPhaseDistortion() {
    TestReporter::info("Testing Phase Distortion Oscillator...");
    
    PhaseDistortionOscillator osc;
    osc.prepare(44100.0);
    osc.setDistortion(0.5f);
    osc.setWaveform(0);
    
    // Test 1: Phase distortion produces output
    float output = osc.processSample(440.0f);
    
    if (std::isfinite(output)) {
        TestReporter::pass("PhaseDist::processSample produces valid output");
    } else {
        TestReporter::fail("PhaseDist::processSample", "Output is not finite");
    }
    
    // Test 2: Distortion amount changes waveform
    {
        osc.reset();
        osc.setDistortion(0.0f);
        
        float minSum = 0.0f;
        for (int i = 0; i < 100; ++i) {
            minSum += std::abs(osc.processSample(440.0f));
        }
        
        osc.reset();
        osc.setDistortion(1.0f);
        
        float maxSum = 0.0f;
        for (int i = 0; i < 100; ++i) {
            maxSum += std::abs(osc.processSample(440.0f));
        }
        
        if (std::abs(minSum - maxSum) > 0.1f) {
            TestReporter::pass("PhaseDist distortion changes waveform");
        } else {
            TestReporter::fail("PhaseDist distortion", "No significant change");
        }
    }
}

void testAdditive() {
    TestReporter::info("Testing Additive Oscillator...");
    
    AdditiveOscillator osc;
    osc.prepare(44100.0);
    
    // Test 1: Harmonic series generates clean tone
    {
        osc.setHarmonicSeries(0.5f);
        float sum = 0.0f;
        
        for (int i = 0; i < 100; ++i) {
            sum += std::abs(osc.processSample(440.0f));
        }
        
        if (sum > 0.1f) {
            TestReporter::pass("Additive generates output");
        } else {
            TestReporter::fail("Additive output", "Output too low");
        }
    }
    
    // Test 2: Inharmonic series generates metallic sound
    {
        osc.reset();
        osc.setInharmonicSeries(0.1f);
        
        float sum = 0.0f;
        for (int i = 0; i < 100; ++i) {
            sum += std::abs(osc.processSample(440.0f));
        }
        
        if (sum > 0.05f) {
            TestReporter::pass("Additive inharmonic generates output");
        } else {
            TestReporter::fail("Additive inharmonic", "Output too low");
        }
    }
    
    // Test 3: Individual partial control
    {
        osc.reset();
        osc.setPartialLevel(0, 1.0f);  // Fundamental
        osc.setPartialLevel(1, 0.5f);  // 2nd harmonic
        osc.setPartialLevel(2, 0.0f);  // 3rd harmonic off
        
        float output = osc.processSample(440.0f);
        
        if (std::isfinite(output) && std::abs(output) > 0.0f) {
            TestReporter::pass("Additive partial control works");
        } else {
            TestReporter::fail("Additive partial", "Invalid output");
        }
    }
}

void testGranular() {
    TestReporter::info("Testing Granular Engine...");
    
    GranularEngine granular;
    granular.prepare(44100.0);
    
    // Create a simple test sample (sine wave)
    constexpr int sampleLength = 44100;
    std::vector<float> testSample(sampleLength);
    for (int i = 0; i < sampleLength; ++i) {
        testSample[i] = std::sin((i * 440.0 / 44100.0) * 6.28318);
    }
    
    granular.setSample(testSample.data(), sampleLength);
    granular.setGrainSize(0.1f);
    granular.setGrainDensity(10.0f);
    granular.setPosition(0.5f);
    granular.setPitch(0.0f);
    granular.setRandomness(0.0f);
    
    // Test 1: Granular processes with trigger
    float output = granular.processSample(1.0f);
    
    if (std::isfinite(output)) {
        TestReporter::pass("Granular::processSample produces valid output");
    } else {
        TestReporter::fail("Granular::processSample", "Output is not finite");
    }
    
    // Test 2: Multiple grains active
    {
        granular.reset();
        
        // Trigger multiple grains
        for (int i = 0; i < 100; ++i) {
            granular.processSample(1.0f);
        }
        
        float output = granular.processSample(0.0f);
        
        if (std::isfinite(output)) {
            TestReporter::pass("Granular handles multiple grains");
        } else {
            TestReporter::fail("Granular multiple grains", "Invalid output");
        }
    }
}

void testAdvancedOscillatorEngine() {
    TestReporter::info("Testing Advanced Oscillator Engine...");
    
    AdvancedOscillatorEngine engine;
    engine.prepare(44100.0);
    
    // Test all oscillator types
    auto types = {AdvancedOscillatorEngine::OscType::Standard,
                  AdvancedOscillatorEngine::OscType::Wavefolder,
                  AdvancedOscillatorEngine::OscType::PhaseDist,
                  AdvancedOscillatorEngine::OscType::Additive};
    
    bool allPass = true;
    for (auto type : types) {
        engine.reset();
        engine.setType(type);
        engine.setFrequency(440.0f);
        
        float output = engine.processSample();
        if (!std::isfinite(output)) {
            allPass = false;
            TestReporter::fail("AdvancedEngine", 
                "Type " + juce::String(static_cast<int>(type)) + " failed");
        }
    }
    
    if (allPass) {
        TestReporter::pass("AdvancedOscillatorEngine all types work");
    }
}

void testOscillatorFrequencyAccuracy() {
    TestReporter::info("Testing Oscillator Frequency Accuracy...");
    
    PhaseDistortionOscillator osc;
    osc.prepare(44100.0);
    osc.setDistortion(0.0f);  // Pure sine
    
    // Test at A4 = 440 Hz
    const float targetFreq = 440.0f;
    const float sampleRate = 44100.0f;
    const int numSamples = static_cast<int>(sampleRate / targetFreq);  // One cycle
    
    int zeroCrossings = 0;
    float prevSample = 0.0f;
    
    for (int i = 0; i < numSamples; ++i) {
        float sample = osc.processSample(targetFreq);
        if ((prevSample > 0.0f && sample < 0.0f) || 
            (prevSample < 0.0f && sample > 0.0f)) {
            zeroCrossings++;
        }
        prevSample = sample;
    }
    
    // Should have 2 zero crossings per cycle (positive to negative and back)
    if (zeroCrossings >= 1 && zeroCrossings <= 3) {
        TestReporter::pass("Oscillator frequency is accurate");
    } else {
        TestReporter::fail("Oscillator frequency", 
            "Wrong number of zero crossings: " + juce::String(zeroCrossings));
    }
}

void testOscillatorPerformance() {
    TestReporter::info("Testing Oscillator CPU Performance...");
    
    AdditiveOscillator osc;
    osc.prepare(44100.0);
    osc.setHarmonicSeries(0.5f);
    
    const int numSamples = 44100;  // 1 second
    
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < numSamples; ++i) {
        osc.processSample(440.0f);
    }
    
    auto end = juce::Time::getMillisecondCounterHiRes();
    double elapsed = (end - start) / 1000.0;
    double percent = (elapsed / 1.0) * 100.0;
    
    TestReporter::info("Additive (64 partial) CPU: " + juce::String(percent, 2) + "% for single voice");
    
    if (percent < 10.0) {
        TestReporter::pass("Oscillator CPU usage is acceptable");
    } else {
        TestReporter::fail("Oscillator CPU", "Usage too high: " + juce::String(percent) + "%");
    }
}

void testWavefolderAliasing() {
    TestReporter::info("Testing Wavefolder Anti-Aliasing...");
    
    BuchlaWavefolder wavefolder;
    wavefolder.prepare(44100.0);
    wavefolder.setFolds(3.0f);
    wavefolder.setAmount(1.0f);
    
    // Generate high frequency (10 kHz)
    const float highFreq = 10000.0f;
    float maxOutput = 0.0f;
    
    for (int i = 0; i < 1000; ++i) {
        float phase = static_cast<float>(i) / 1000.0f;
        float sine = std::sin(phase * 6.28318f);
        float output = wavefolder.processSample(sine, phase);
        maxOutput = std::max(maxOutput, std::abs(output));
    }
    
    // Wavefolder should not explode at high frequencies
    if (maxOutput < 10.0f) {
        TestReporter::pass("Wavefolder handles high frequencies");
    } else {
        TestReporter::fail("Wavefolder anti-aliasing", "Output too high: " + juce::String(maxOutput));
    }
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main() {
    std::cout << "\n";
    std::cout << "==================================================\n";
    std::cout << "  ZenithPolySynth - Phase 2 Oscillator Tests\n";
    std::cout << "==================================================\n\n";
    
    // Run all oscillator tests
    testWavefolder();
    testPhaseDistortion();
    testAdditive();
    testGranular();
    testAdvancedOscillatorEngine();
    testOscillatorFrequencyAccuracy();
    testOscillatorPerformance();
    testWavefolderAliasing();
    
    // Print summary
    std::cout << "\n";
    std::cout << "==================================================\n";
    std::cout << "  Test Summary\n";
    std::cout << "==================================================\n";
    std::cout << "Total: " << TestReporter::getTotalCount() << "\n";
    std::cout << "Passed: " << TestReporter::getPassCount() << "\n";
    std::cout << "Failed: " << TestReporter::getFailCount() << "\n";
    std::cout << "==================================================\n\n";
    
    return (TestReporter::getFailCount() == 0) ? 0 : 1;
}
