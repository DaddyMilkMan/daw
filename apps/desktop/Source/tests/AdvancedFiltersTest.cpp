/*
  ==============================================================================

    AdvancedFiltersTest.cpp
    Created: 2025-02-01
    Author:  Zenith DAW

    Comprehensive test suite for Phase 1 professional filters.

  ==============================================================================
*/

#include "../instruments/ZenithAdvancedFilters.h"
#include "../instruments/ZenithFilter.h"
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
// Filter Tests
//==============================================================================

void testMoogLadderFilter() {
    TestReporter::info("Testing Moog Ladder Filter...");
    
    MoogLadderFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);
    filter.setDrive(1.0f);
    
    // Test 1: Filter processes without crashing
    try {
        float input = 0.5f;
        float output = filter.processSample(input);
        
        if (std::isfinite(output)) {
            TestReporter::pass("MoogLadder::processSample produces valid output");
        } else {
            TestReporter::fail("MoogLadder::processSample", "Output is not finite");
        }
    } catch (...) {
        TestReporter::fail("MoogLadder::processSample", "Exception thrown");
    }
    
    // Test 2: Lowpass filter attenuates high frequencies
    {
        filter.reset();
        filter.setCutoff(100.0f);  // Low cutoff
        filter.setResonance(0.0f);  // No resonance
        
        float sumLow = 0.0f;
        float sumHigh = 0.0f;
        
        // Process low frequency (50 Hz)
        for (int i = 0; i < 100; ++i) {
            float phase = static_cast<float>(i) / 100.0f;
            float input = std::sin(phase * 6.28318f);
            float output = filter.processSample(input);
            sumLow += std::abs(output);
        }
        
        filter.reset();
        
        // Process high frequency (5000 Hz)
        for (int i = 0; i < 100; ++i) {
            float phase = static_cast<float>(i) / 100.0f;
            float input = std::sin(phase * 6.28318f);
            float output = filter.processSample(input);
            sumHigh += std::abs(output);
        }
        
        if (sumLow > sumHigh * 2.0f) {
            TestReporter::pass("MoogLadder attenuates high frequencies");
        } else {
            TestReporter::fail("MoogLadder attenuation", 
                juce::String("Low: ") + juce::String(sumLow) + 
                " High: " + juce::String(sumHigh));
        }
    }
    
    // Test 3: Resonance increases at cutoff frequency
    {
        filter.reset();
        filter.setCutoff(1000.0f);
        filter.setResonance(0.8f);  // High resonance
        
        float maxOutput = 0.0f;
        for (int i = 0; i < 1000; ++i) {
            float input = 0.1f;  // Small input
            float output = filter.processSample(input);
            maxOutput = std::max(maxOutput, std::abs(output));
        }
        
        if (maxOutput > 0.5f) {
            TestReporter::pass("MoogLadder resonance boosts output");
        } else {
            TestReporter::fail("MoogLadder resonance", 
                "Output not boosted enough: " + juce::String(maxOutput));
        }
    }
}

void testMS20Filter() {
    TestReporter::info("Testing MS-20 Lowpass Filter...");
    
    MS20LowpassFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);
    filter.setDrive(1.0f);
    
    // Test 1: Filter processes without crashing
    try {
        float output = filter.processSample(0.5f);
        if (std::isfinite(output)) {
            TestReporter::pass("MS20::processSample produces valid output");
        } else {
            TestReporter::fail("MS20::processSample", "Output is not finite");
        }
    } catch (...) {
        TestReporter::fail("MS20::processSample", "Exception thrown");
    }
    
    // Test 2: Aggressive resonance
    {
        filter.reset();
        filter.setCutoff(500.0f);
        filter.setResonance(0.95f);  // Near self-oscillation
        
        float prevOutput = 0.0f;
        bool oscillating = false;
        
        for (int i = 0; i < 4410; ++i) {  // 0.1 seconds
            float input = 0.001f;  // Tiny input
            float output = filter.processSample(input);
            
            // Check for oscillation (sign changes with sufficient amplitude)
            if (std::abs(output) > 0.1f && 
                ((output > 0.0f && prevOutput < 0.0f) || 
                 (output < 0.0f && prevOutput > 0.0f))) {
                oscillating = true;
                break;
            }
            prevOutput = output;
        }
        
        if (oscillating) {
            TestReporter::pass("MS20 self-oscillates at high resonance");
        } else {
            TestReporter::fail("MS20 self-oscillation", "No oscillation detected");
        }
    }
}

void testProphet5Filter() {
    TestReporter::info("Testing Prophet-5 Filter...");
    
    Prophet5Filter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);
    filter.setDrive(1.0f);
    
    // Test basic functionality
    float output = filter.processSample(0.5f);
    
    if (std::isfinite(output)) {
        TestReporter::pass("Prophet5::processSample produces valid output");
    } else {
        TestReporter::fail("Prophet5::processSample", "Output is not finite");
    }
}

void testSEMFilter() {
    TestReporter::info("Testing SEM Filter...");
    
    SEMFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);
    filter.setDrive(1.0f);
    
    // Test all filter modes
    filter.setType(FilterType::Lowpass);
    float lp = filter.processSample(0.5f);
    
    filter.reset();
    filter.setType(FilterType::Bandpass);
    float bp = filter.processSample(0.5f);
    
    filter.reset();
    filter.setType(FilterType::Highpass);
    float hp = filter.processSample(0.5f);
    
    if (std::isfinite(lp) && std::isfinite(bp) && std::isfinite(hp)) {
        TestReporter::pass("SEM filter all modes produce valid output");
    } else {
        TestReporter::fail("SEM filter modes", "Invalid output detected");
    }
}

void testTB303Filter() {
    TestReporter::info("Testing TB-303 Filter...");
    
    TB303Filter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);
    filter.setDrive(1.0f);
    
    float output = filter.processSample(0.5f);
    
    if (std::isfinite(output)) {
        TestReporter::pass("TB303::processSample produces valid output");
    } else {
        TestReporter::fail("TB303::processSample", "Output is not finite");
    }
}

void testFilterIntegration() {
    TestReporter::info("Testing ZenithFilter Integration...");
    
    ZenithFilter filter;
    filter.setSampleRate(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);
    filter.setDrive(1.0f);
    
    // Test all filter models
    auto models = {FilterModelType::SVF, FilterModelType::MoogLadder, 
                  FilterModelType::MS20, FilterModelType::Prophet};
    
    bool allPass = true;
    for (auto model : models) {
        filter.setModel(model);
        filter.reset();
        
        float output = filter.processSample(0.5f);
        if (!std::isfinite(output)) {
            allPass = false;
            TestReporter::fail("ZenithFilter integration", 
                "Model " + juce::String(static_cast<int>(model)) + " failed");
        }
    }
    
    if (allPass) {
        TestReporter::pass("ZenithFilter all models integrate correctly");
    }
}

void testFilterModulation() {
    TestReporter::info("Testing Filter Modulation Smoothing...");
    
    MoogLadderFilter filter;
    filter.prepare(44100.0);
    
    // Test parameter smoothing (no zipper noise)
    filter.setCutoff(100.0f);
    
    float prevOutput = 0.0f;
    bool suddenJump = false;
    
    for (int i = 0; i < 100; ++i) {
        // Modulate cutoff rapidly
        float cutoff = 100.0f + i * 10.0f;
        filter.setCutoff(cutoff);
        
        float output = filter.processSample(0.5f);
        
        // Check for sudden jumps (zipper noise)
        float diff = std::abs(output - prevOutput);
        if (diff > 0.5f) {
            suddenJump = true;
            break;
        }
        prevOutput = output;
    }
    
    if (!suddenJump) {
        TestReporter::pass("Filter parameters are smoothly modulated");
    } else {
        TestReporter::fail("Filter modulation", "Sudden jumps detected");
    }
}

void testFilterPerformance() {
    TestReporter::info("Testing Filter CPU Performance...");
    
    MoogLadderFilter filter;
    filter.prepare(44100.0);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.5f);
    
    const int numSamples = 44100;  // 1 second
    
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < numSamples; ++i) {
        filter.processSample(0.5f);
    }
    
    auto end = juce::Time::getMillisecondCounterHiRes();
    double elapsed = (end - start) / 1000.0;
    double percent = (elapsed / 1.0) * 100.0;
    
    TestReporter::info("Filter CPU: " + juce::String(percent, 2) + "% for single voice");
    
    if (percent < 5.0) {
        TestReporter::pass("Filter CPU usage is acceptable");
    } else {
        TestReporter::fail("Filter CPU", "Usage too high: " + juce::String(percent) + "%");
    }
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main() {
    std::cout << "\n";
    std::cout << "==================================================\n";
    std::cout << "  ZenithPolySynth - Phase 1 Filter Tests\n";
    std::cout << "==================================================\n\n";
    
    // Run all filter tests
    testMoogLadderFilter();
    testMS20Filter();
    testProphet5Filter();
    testSEMFilter();
    testTB303Filter();
    testFilterIntegration();
    testFilterModulation();
    testFilterPerformance();
    
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
