/*
  ==============================================================================

    PhysicalModelingTest.h
    Created: [Date] Author: Claude AI
    Comprehensive test framework for physical modeling engines

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"
#include "../../modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/StringModel.h"
#include "../../modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/WindModel.h"
#include "../../modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/PercussionModel.h"

namespace Zenith
{

class PhysicalModelingTest
{
public:
    PhysicalModelingTest();
    ~PhysicalModelingTest();

    // Test categories
    enum TestCategory
    {
        AllTests,
        StringModelTests,
        WindModelTests,
        PercussionModelTests,
        PerformanceTests,
        AudioQualityTests
    };

    // Main test methods
    bool runAllTests();
    bool runTestsByCategory(TestCategory category);
    
    // String model tests
    bool testStringModelBasicFunctionality();
    bool testStringModelExcitationTypes();
    bool testStringModelMPESupport();
    bool testStringModelModalSynthesis();
    bool testStringModelPerformance();
    
    // Wind model tests
    bool testWindModelBasicFunctionality();
    bool testWindModelEmbouchureTypes();
    bool testWindModelBreathControl();
    bool testWindModelFormantFiltering();
    bool testWindModelPerformance();
    
    // Percussion model tests
    bool testPercussionModelBasicFunctionality();
    bool testPercussionModelDrumTypes();
    bool testPercussionModelModalSynthesis();
    bool testPercussionModelEnvelopeGeneration();
    bool testPercussionModelPerformance();
    
    // Audio quality tests
    bool testAudioQuality();
    bool testFrequencyResponse();
    bool testDynamicRange();
    bool testLatency();
    
    // Performance tests
    bool testCPUUsage();
    bool testMemoryUsage();
    bool testRealTimePerformance();
    bool testMultiVoicePerformance();

    // Test utilities
    void printTestResults();
    void generateTestReport();
    void exportTestAudio();
    
    // Getters
    juce::String getTestSummary() const;
    juce::Array<juce::String> getFailedTests() const;
    juce::Array<juce::String> getPassedTests() const;

private:
    // Test results
    int totalTests_;
    int passedTests_;
    int failedTests_;
    juce::Array<juce::String> failedTests_;
    juce::Array<juce::String> passedTests_;
    
    // Audio buffer for testing
    juce::AudioBuffer<float> testBuffer_;
    
    // Test parameters
    double sampleRate_;
    int bufferSize_;
    
    // Test utilities
    bool compareFloats(float a, float b, float tolerance = 1e-6f);
    bool compareBuffers(const juce::AudioBuffer<float>& buf1, const juce::AudioBuffer<float>& buf2, float tolerance = 1e-6f);
    float calculateRMS(const juce::AudioBuffer<float>& buffer);
    float calculatePeak(const juce::AudioBuffer<float>& buffer);
    
    // Test helpers
    void initializeTestBuffer();
    void createTestSignal(juce::AudioBuffer<float>& buffer, float frequency, float amplitude = 1.0f);
    void createNoiseSignal(juce::AudioBuffer<float>& buffer, float amplitude = 1.0f);
    
    // Analysis helpers
    void analyzeFrequencyResponse(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output);
    void analyzeDynamicRange(const juce::AudioBuffer<float>& buffer);
    void measureLatency(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output);
    
    // Performance helpers
    void measureCPUUsage(std::function<void()> testFunction, const juce::String& testName);
    void measureMemoryUsage(std::function<void()> testFunction, const juce::String& testName);
    
    // String model test helpers
    bool testStringModelKarplusStrong();
    bool testStringModelPhysicalParameters();
    bool testStringModelVibratoGeneration();
    
    // Wind model test helpers
    bool testWindModelReedModel();
    bool testWindModelBoreResonance();
    bool testWindModelNonlinearity();
    
    // Percussion model test helpers
    bool testPercussionModelStrikeGeneration();
    bool testPercussionModelCymbalModes();
    bool testPercussionModelSnareGeneration();
};

} // namespace Zenith
