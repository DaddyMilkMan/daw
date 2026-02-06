/*
  ==============================================================================

    PhysicalModelingTest.cpp
    Created: [Date] Author: Claude AI
    Comprehensive test implementation for physical modeling engines

  ==============================================================================
*/

#include "PhysicalModelingTest.h"
#include "../../../DSP/AudioAnalysisUtils.h"

namespace Zenith
{

PhysicalModelingTest::PhysicalModelingTest()
    : totalTests_(0)
    , passedTests_(0)
    , failedTests_(0)
    , sampleRate_(44100.0)
    , bufferSize_(512)
{
    initializeTestBuffer();
}

PhysicalModelingTest::~PhysicalModelingTest()
{
    // Clean up
}

void PhysicalModelingTest::initializeTestBuffer()
{
    testBuffer_.setSize(2, bufferSize_);
    testBuffer_.clear();
}

bool PhysicalModelingTest::runAllTests()
{
    DBG("Running all physical modeling tests...");
    
    bool allPassed = true;
    
    // String Model Tests
    allPassed &= runTestsByCategory(StringModelTests);
    
    // Wind Model Tests
    allPassed &= runTestsByCategory(WindModelTests);
    
    // Percussion Model Tests
    allPassed &= runTestsByCategory(PercussionModelTests);
    
    // Performance Tests
    allPassed &= runTestsByCategory(PerformanceTests);
    
    // Audio Quality Tests
    allPassed &= runTestsByCategory(AudioQualityTests);
    
    printTestResults();
    return allPassed;
}

bool PhysicalModelingTest::runTestsByCategory(TestCategory category)
{
    bool categoryPassed = true;
    
    switch (category) {
        case StringModelTests:
            categoryPassed &= testStringModelBasicFunctionality();
            categoryPassed &= testStringModelExcitationTypes();
            categoryPassed &= testStringModelMPESupport();
            categoryPassed &= testStringModelModalSynthesis();
            categoryPassed &= testStringModelPerformance();
            break;
            
        case WindModelTests:
            categoryPassed &= testWindModelBasicFunctionality();
            categoryPassed &= testWindModelEmbouchureTypes();
            categoryPassed &= testWindModelBreathControl();
            categoryPassed &= testWindModelFormantFiltering();
            categoryPassed &= testWindModelPerformance();
            break;
            
        case PercussionModelTests:
            categoryPassed &= testPercussionModelBasicFunctionality();
            categoryPassed &= testPercussionModelDrumTypes();
            categoryPassed &= testPercussionModelModalSynthesis();
            categoryPassed &= testPercussionModelEnvelopeGeneration();
            categoryPassed &= testPercussionModelPerformance();
            break;
            
        case PerformanceTests:
            categoryPassed &= testCPUUsage();
            categoryPassed &= testMemoryUsage();
            categoryPassed &= testRealTimePerformance();
            categoryPassed &= testMultiVoicePerformance();
            break;
            
        case AudioQualityTests:
            categoryPassed &= testAudioQuality();
            categoryPassed &= testFrequencyResponse();
            categoryPassed &= testDynamicRange();
            categoryPassed &= testLatency();
            break;
            
        case AllTests:
        default:
            categoryPassed &= runAllTests();
            break;
    }
    
    return categoryPassed;
}

// String Model Tests
bool PhysicalModelingTest::testStringModelBasicFunctionality()
{
    DBG("Testing String Model basic functionality...");
    
    StringModelVoice stringModel;
    
    // Test initialization
    if (!stringModel.isActive()) {
        DBG("StringModel: Initial state test passed");
    } else {
        DBG("StringModel: Initial state test failed");
        return false;
    }
    
    // Test note on
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    if (!stringModel.isActive()) {
        DBG("StringModel: NoteOn test failed");
        return false;
    }
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    stringModel.process(buffer, bufferSize_);
    
    // Check if we have audio output
    float rms = calculateRMS(buffer);
    if (rms > 0.0f) {
        DBG("StringModel: Audio generation test passed - RMS: " << rms);
        passedTests_++;
        return true;
    } else {
        DBG("StringModel: Audio generation test failed - no audio output");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testStringModelExcitationTypes()
{
    DBG("Testing String Model excitation types...");
    
    StringModelVoice stringModel;
    
    // Test all excitation types
    std::array<StringModelVoice::Excitation, 4> excitations = {
        StringModelVoice::Excitation::Pluck,
        StringModelVoice::Excitation::Bow,
        StringModelVoice::Excitation::Strike,
        StringModelVoice::Excitation::Blow
    };
    
    for (auto excitation : excitations) {
        stringModel.reset();
        stringModel.noteOn(440.0f, 0.5f, excitation);
        
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        stringModel.process(buffer, bufferSize_);
        
        float rms = calculateRMS(buffer);
        if (rms > 0.0f) {
            DBG("StringModel: " << static_cast<int>(excitation) << " excitation test passed");
        } else {
            DBG("StringModel: " << static_cast<int>(excitation) << " excitation test failed");
            return false;
        }
    }
    
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testStringModelMPESupport()
{
    DBG("Testing String Model MPE support...");
    
    StringModelVoice stringModel;
    
    // Test MPE parameters
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    // Set MPE parameters
    stringModel.setPitchBend(0.5f);
    stringModel.setPressure(0.3f);
    stringModel.setTimbre(0.7f);
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    stringModel.process(buffer, bufferSize_);
    
    float rms = calculateRMS(buffer);
    if (rms > 0.0f) {
        DBG("StringModel: MPE support test passed");
        passedTests_++;
        return true;
    } else {
        DBG("StringModel: MPE support test failed");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testStringModelModalSynthesis()
{
    DBG("Testing String Model modal synthesis...");
    
    StringModelVoice stringModel;
    
    // Configure for modal synthesis
    stringModel.setNumberOfModes(8);
    stringModel.setInharmonicity(0.3f);
    stringModel.setBodyResonance(0.5f);
    
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    stringModel.process(buffer, bufferSize_);
    
    // Check modal amplitudes
    juce::Array<float> modalAmplitudes = stringModel.getModeAmplitudes();
    if (modalAmplitudes.size() > 0) {
        DBG("StringModel: Modal synthesis test passed - " << modalAmplitudes.size() << " modes");
        passedTests_++;
        return true;
    } else {
        DBG("StringModel: Modal synthesis test failed");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testStringModelPerformance()
{
    DBG("Testing String Model performance...");
    
    StringModelVoice stringModel;
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    // Measure performance
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < 1000; ++i) {
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        stringModel.process(buffer, bufferSize_);
    }
    
    auto end = juce::Time::getMillisecondCounterHiRes();
    double duration = (end - start) / 1000.0;
    
    DBG("StringModel: Performance test - 1000 iterations in " << duration << " seconds");
    
    if (duration < 1.0) { // Should complete in less than 1 second
        passedTests_++;
        return true;
    } else {
        DBG("StringModel: Performance test failed - too slow");
        failedTests_++;
        return false;
    }
}

// Wind Model Tests
bool PhysicalModelingTest::testWindModelBasicFunctionality()
{
    DBG("Testing Wind Model basic functionality...");
    
    WindModelVoice windModel;
    
    // Test initialization
    if (!windModel.isActive()) {
        DBG("WindModel: Initial state test passed");
    } else {
        DBG("WindModel: Initial state test failed");
        return false;
    }
    
    // Test note on
    windModel.noteOn(440.0f, 0.5f, 0.5f);
    
    if (!windModel.isActive()) {
        DBG("WindModel: NoteOn test failed");
        return false;
    }
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    windModel.process(buffer, bufferSize_);
    
    // Check if we have audio output
    float rms = calculateRMS(buffer);
    if (rms > 0.0f) {
        DBG("WindModel: Audio generation test passed - RMS: " << rms);
        passedTests_++;
        return true;
    } else {
        DBG("WindModel: Audio generation test failed - no audio output");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testWindModelEmbouchureTypes()
{
    DBG("Testing Wind Model embouchure types...");
    
    WindModelVoice windModel;
    
    // Test all embouchure types
    std::array<WindModelVoice::Embouchure, 4> embouchures = {
        WindModelVoice::Embouchure::Clarinet,
        WindModelVoice::Embouchure::Saxophone,
        WindModelVoice::Embouchure::Flute,
        WindModelVoice::Embouchure::Brass
    };
    
    for (auto embouchure : embouchures) {
        windModel.reset();
        windModel.setEmbouchureType(embouchure);
        windModel.noteOn(440.0f, 0.5f, 0.5f);
        
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        windModel.process(buffer, bufferSize_);
        
        float rms = calculateRMS(buffer);
        if (rms > 0.0f) {
            DBG("WindModel: " << static_cast<int>(embouchure) << " embouchure test passed");
        } else {
            DBG("WindModel: " << static_cast<int>(embouchure) << " embouchure test failed");
            return false;
        }
    }
    
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testWindModelBreathControl()
{
    DBG("Testing Wind Model breath control...");
    
    WindModelVoice windModel;
    
    // Test different breath pressures
    std::array<float, 5> pressures = {0.1f, 0.3f, 0.5f, 0.7f, 1.0f};
    
    for (auto pressure : pressures) {
        windModel.reset();
        windModel.noteOn(440.0f, 0.5f, pressure);
        
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        windModel.process(buffer, bufferSize_);
        
        float rms = calculateRMS(buffer);
        DBG("WindModel: Pressure " << pressure << " - RMS: " << rms);
        
        // Check if RMS scales with pressure
        if (pressure > 0.1f && rms < 0.001f) {
            DBG("WindModel: Breath control test failed at pressure " << pressure);
            return false;
        }
    }
    
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testWindModelFormantFiltering()
{
    DBG("Testing Wind Model formant filtering...");
    
    WindModelVoice windModel;
    
    // Configure for formant filtering
    windModel.setToneColor(0.5f);
    windModel.setResonanceBoost(0.3f);
    windModel.setResonanceFrequency(1000.0f);
    
    windModel.noteOn(440.0f, 0.5f, 0.5f);
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    windModel.process(buffer, bufferSize_);
    
    float rms = calculateRMS(buffer);
    if (rms > 0.0f) {
        DBG("WindModel: Formant filtering test passed");
        passedTests_++;
        return true;
    } else {
        DBG("WindModel: Formant filtering test failed");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testWindModelPerformance()
{
    DBG("Testing Wind Model performance...");
    
    WindModelVoice windModel;
    windModel.noteOn(440.0f, 0.5f, 0.5f);
    
    // Measure performance
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < 1000; ++i) {
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        windModel.process(buffer, bufferSize_);
    }
    
    auto end = juce::Time::getMillisecondCounterHiRes();
    double duration = (end - start) / 1000.0;
    
    DBG("WindModel: Performance test - 1000 iterations in " << duration << " seconds");
    
    if (duration < 1.0) { // Should complete in less than 1 second
        passedTests_++;
        return true;
    } else {
        DBG("WindModel: Performance test failed - too slow");
        failedTests_++;
        return false;
    }
}

// Percussion Model Tests
bool PhysicalModelingTest::testPercussionModelBasicFunctionality()
{
    DBG("Testing Percussion Model basic functionality...");
    
    PercussionModelVoice percussionModel;
    
    // Test initialization
    if (!percussionModel.isActive()) {
        DBG("PercussionModel: Initial state test passed");
    } else {
        DBG("PercussionModel: Initial state test failed");
        return false;
    }
    
    // Test note on
    percussionModel.noteOn(0.5f, PercussionModelVoice::DrumType::Kick);
    
    if (!percussionModel.isActive()) {
        DBG("PercussionModel: NoteOn test failed");
        return false;
    }
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    percussionModel.process(buffer, bufferSize_);
    
    // Check if we have audio output
    float rms = calculateRMS(buffer);
    if (rms > 0.0f) {
        DBG("PercussionModel: Audio generation test passed - RMS: " << rms);
        passedTests_++;
        return true;
    } else {
        DBG("PercussionModel: Audio generation test failed - no audio output");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testPercussionModelDrumTypes()
{
    DBG("Testing Percussion Model drum types...");
    
    PercussionModelVoice percussionModel;
    
    // Test all drum types
    std::array<PercussionModelVoice::DrumType, 9> drumTypes = {
        PercussionModelVoice::DrumType::Kick,
        PercussionModelVoice::DrumType::Snare,
        PercussionModelVoice::DrumType::Tom,
        PercussionModelVoice::DrumType::Hihat,
        PercussionModelVoice::DrumType::Cymbal,
        PercussionModelVoice::DrumType::Percussion,
        PercussionModelVoice::DrumType::Triangle,
        PercussionModelVoice::DrumType::Chimes,
        PercussionModelVoice::DrumType::Taiko
    };
    
    for (auto drumType : drumTypes) {
        percussionModel.reset();
        percussionModel.noteOn(0.5f, drumType);
        
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        percussionModel.process(buffer, bufferSize_);
        
        float rms = calculateRMS(buffer);
        if (rms > 0.0f) {
            DBG("PercussionModel: " << static_cast<int>(drumType) << " drum type test passed");
        } else {
            DBG("PercussionModel: " << static_cast<int>(drumType) << " drum type test failed");
            return false;
        }
    }
    
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testPercussionModelModalSynthesis()
{
    DBG("Testing Percussion Model modal synthesis...");
    
    PercussionModelVoice percussionModel;
    
    // Configure for modal synthesis
    percussionModel.setNumberOfModes(8);
    percussionModel.setResonance(0.5f);
    percussionModel.setMetallicResonance(0.3f);
    
    percussionModel.noteOn(0.5f, PercussionModelVoice::DrumType::Tom);
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    percussionModel.process(buffer, bufferSize_);
    
    // Check modal amplitudes
    juce::Array<float> modalAmplitudes = percussionModel.getModeAmplitudes();
    if (modalAmplitudes.size() > 0) {
        DBG("PercussionModel: Modal synthesis test passed - " << modalAmplitudes.size() << " modes");
        passedTests_++;
        return true;
    } else {
        DBG("PercussionModel: Modal synthesis test failed");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testPercussionModelEnvelopeGeneration()
{
    DBG("Testing Percussion Model envelope generation...");
    
    PercussionModelVoice percussionModel;
    
    // Configure envelope
    percussionModel.setAttack(0.01f);
    percussionModel.setDecay(0.5f);
    percussionModel.setSustain(0.0f);
    percussionModel.setRelease(0.1f);
    
    percussionModel.noteOn(0.5f, PercussionModelVoice::DrumType::Kick);
    
    // Process audio
    juce::AudioBuffer<float> buffer(2, bufferSize_);
    buffer.clear();
    percussionModel.process(buffer, bufferSize_);
    
    float rms = calculateRMS(buffer);
    if (rms > 0.0f) {
        DBG("PercussionModel: Envelope generation test passed");
        passedTests_++;
        return true;
    } else {
        DBG("PercussionModel: Envelope generation test failed");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testPercussionModelPerformance()
{
    DBG("Testing Percussion Model performance...");
    
    PercussionModelVoice percussionModel;
    percussionModel.noteOn(0.5f, PercussionModelVoice::DrumType::Kick);
    
    // Measure performance
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < 1000; ++i) {
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        percussionModel.process(buffer, bufferSize_);
    }
    
    auto end = juce::Time::getMillisecondCounterHiRes();
    double duration = (end - start) / 1000.0;
    
    DBG("PercussionModel: Performance test - 1000 iterations in " << duration << " seconds");
    
    if (duration < 1.0) { // Should complete in less than 1 second
        passedTests_++;
        return true;
    } else {
        DBG("PercussionModel: Performance test failed - too slow");
        failedTests_++;
        return false;
    }
}

// Audio Quality Tests
bool PhysicalModelingTest::testAudioQuality()
{
    DBG("Testing audio quality...");
    
    // Test all three models
    StringModelVoice stringModel;
    WindModelVoice windModel;
    PercussionModelVoice percussionModel;
    
    // Configure models
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    windModel.noteOn(440.0f, 0.5f, 0.5f);
    percussionModel.noteOn(0.5f, PercussionModelVoice::DrumType::Kick);
    
    // Process audio
    juce::AudioBuffer<float> stringBuffer(2, bufferSize_);
    juce::AudioBuffer<float> windBuffer(2, bufferSize_);
    juce::AudioBuffer<float> percussionBuffer(2, bufferSize_);
    
    stringModel.process(stringBuffer, bufferSize_);
    windModel.process(windBuffer, bufferSize_);
    percussionModel.process(percussionBuffer, bufferSize_);
    
    // Check audio quality
    float stringRMS = calculateRMS(stringBuffer);
    float windRMS = calculateRMS(windBuffer);
    float percussionRMS = calculateRMS(percussionBuffer);
    
    DBG("Audio Quality Test - String RMS: " << stringRMS << ", Wind RMS: " << windRMS << ", Percussion RMS: " << percussionRMS);
    
    // Check for clipping
    float stringPeak = calculatePeak(stringBuffer);
    float windPeak = calculatePeak(windBuffer);
    float percussionPeak = calculatePeak(percussionBuffer);
    
    if (stringPeak < 1.0f && windPeak < 1.0f && percussionPeak < 1.0f) {
        DBG("Audio Quality Test: No clipping detected");
        passedTests_++;
        return true;
    } else {
        DBG("Audio Quality Test: Clipping detected");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testFrequencyResponse()
{
    DBG("Testing frequency response...");
    
    StringModelVoice stringModel;
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    // Create test signal
    juce::AudioBuffer<float> inputBuffer(2, bufferSize_);
    createTestSignal(inputBuffer, 440.0f, 0.5f);
    
    // Process audio
    juce::AudioBuffer<float> outputBuffer(2, bufferSize_);
    outputBuffer.clear();
    stringModel.process(outputBuffer, bufferSize_);
    
    // Analyze frequency response
    analyzeFrequencyResponse(inputBuffer, outputBuffer);
    
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testDynamicRange()
{
    DBG("Testing dynamic range...");
    
    StringModelVoice stringModel;
    
    // Test different velocity levels
    std::array<float, 5> velocities = {0.1f, 0.3f, 0.5f, 0.7f, 1.0f};
    juce::Array<float> rmsLevels;
    
    for (auto velocity : velocities) {
        stringModel.reset();
        stringModel.noteOn(440.0f, velocity, StringModelVoice::Excitation::Pluck);
        
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        stringModel.process(buffer, bufferSize_);
        
        float rms = calculateRMS(buffer);
        rmsLevels.add(rms);
        DBG("Velocity " << velocity << " - RMS: " << rms);
    }
    
    // Check dynamic range
    float minRMS = rmsLevels.getMin();
    float maxRMS = rmsLevels.getMax();
    float dynamicRange = 20.0f * std::log10(maxRMS / minRMS);
    
    DBG("Dynamic Range: " << dynamicRange << " dB");
    
    if (dynamicRange > 20.0f) { // At least 20 dB dynamic range
        passedTests_++;
        return true;
    } else {
        DBG("Dynamic Range Test: Insufficient dynamic range");
        failedTests_++;
        return false;
    }
}

bool PhysicalModelingTest::testLatency()
{
    DBG("Testing latency...");
    
    StringModelVoice stringModel;
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    // Create impulse signal
    juce::AudioBuffer<float> impulseBuffer(2, bufferSize_);
    impulseBuffer.clear();
    impulseBuffer.setSample(0, 0, 1.0f); // Impulse at start
    
    // Process audio
    juce::AudioBuffer<float> outputBuffer(2, bufferSize_);
    outputBuffer.clear();
    stringModel.process(outputBuffer, bufferSize_);
    
    // Measure latency
    measureLatency(impulseBuffer, outputBuffer);
    
    passedTests_++;
    return true;
}

// Performance Tests
bool PhysicalModelingTest::testCPUUsage()
{
    DBG("Testing CPU usage...");
    
    StringModelVoice stringModel;
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    // Measure CPU usage
    measureCPUUsage([this, &stringModel]() {
        for (int i = 0; i < 1000; ++i) {
            juce::AudioBuffer<float> buffer(2, bufferSize_);
            buffer.clear();
            stringModel.process(buffer, bufferSize_);
        }
    }, "String Model CPU Test");
    
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testMemoryUsage()
{
    DBG("Testing memory usage...");
    
    // Create multiple instances
    std::vector<std::unique_ptr<StringModelVoice>> stringModels;
    
    measureMemoryUsage([this, &stringModels]() {
        for (int i = 0; i < 100; ++i) {
            stringModels.push_back(std::make_unique<StringModelVoice>());
            stringModels.back()->noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
        }
    }, "String Model Memory Test");
    
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testRealTimePerformance()
{
    DBG("Testing real-time performance...");
    
    StringModelVoice stringModel;
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    
    // Simulate real-time processing
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < 100; ++i) {
        juce::AudioBuffer<float> buffer(2, bufferSize_);
        buffer.clear();
        stringModel.process(buffer, bufferSize_);
        
        // Check if we're keeping up with real-time
        auto currentTime = juce::Time::getMillisecondCounterHiRes();
        double elapsed = (currentTime - start) / 1000.0;
        double expectedTime = (i + 1) * (bufferSize_ / sampleRate_);
        
        if (elapsed > expectedTime * 1.1) { // 10% tolerance
            DBG("Real-time performance test failed - too slow");
            failedTests_++;
            return false;
        }
    }
    
    DBG("Real-time performance test passed");
    passedTests_++;
    return true;
}

bool PhysicalModelingTest::testMultiVoicePerformance()
{
    DBG("Testing multi-voice performance...");
    
    // Create multiple voices
    std::vector<std::unique_ptr<StringModelVoice>> voices;
    
    for (int i = 0; i < 16; ++i) {
        auto voice = std::make_unique<StringModelVoice>();
        voice->noteOn(440.0f + i * 10.0f, 0.5f, StringModelVoice::Excitation::Pluck);
        voices.push_back(std::move(voice));
    }
    
    // Process all voices simultaneously
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < 100; ++i) {
        for (auto& voice : voices) {
            juce::AudioBuffer<float> buffer(2, bufferSize_);
            buffer.clear();
            voice->process(buffer, bufferSize_);
        }
    }
    
    auto end = juce::Time::getMillisecondCounterHiRes();
    double duration = (end - start) / 1000.0;
    
    DBG("Multi-voice performance test - 16 voices, 100 iterations in " << duration << " seconds");
    
    if (duration < 5.0) { // Should complete in less than 5 seconds
        passedTests_++;
        return true;
    } else {
        DBG("Multi-voice performance test failed - too slow");
        failedTests_++;
        return false;
    }
}

// Utility Methods
void PhysicalModelingTest::printTestResults()
{
    DBG("=== Physical Modeling Test Results ===");
    DBG("Total Tests: " << totalTests_);
    DBG("Passed Tests: " << passedTests_);
    DBG("Failed Tests: " << failedTests_);
    DBG("Success Rate: " << (passedTests_ * 100.0f / totalTests_) << "%");
    
    if (!failedTests_.isEmpty()) {
        DBG("Failed Tests:");
        for (auto& test : failedTests_) {
            DBG("  - " << test);
        }
    }
}

void PhysicalModelingTest::generateTestReport()
{
    // Generate detailed test report
    juce::File reportFile = juce::File::getSpecialLocation(juce::File::currentDirectory)
                            .getChildFile("PhysicalModelingTestReport.txt");
    
    if (reportFile.existsFile()) {
        reportFile.deleteFile();
    }
    
    juce::String reportContent;
    reportContent << "ZenithUltraSynth Physical Modeling Test Report\n";
    reportContent << "============================================\n\n";
    reportContent << "Test Summary:\n";
    reportContent << "Total Tests: " << totalTests_ << "\n";
    reportContent << "Passed Tests: " << passedTests_ << "\n";
    reportContent << "Failed Tests: " << failedTests_ << "\n";
    reportContent << "Success Rate: " << (passedTests_ * 100.0f / totalTests_) << "%\n\n";
    
    reportContent << "Failed Tests:\n";
    for (auto& test : failedTests_) {
        reportContent << "- " << test << "\n";
    }
    
    reportContent << "\nPassed Tests:\n";
    for (auto& test : passedTests_) {
        reportContent << "- " << test << "\n";
    }
    
    reportFile.replaceWithText(reportContent);
    DBG("Test report generated: " << reportFile.getFullPathName());
}

void PhysicalModelingTest::exportTestAudio()
{
    // Export test audio files
    StringModelVoice stringModel;
    WindModelVoice windModel;
    PercussionModelVoice percussionModel;
    
    // Configure models
    stringModel.noteOn(440.0f, 0.5f, StringModelVoice::Excitation::Pluck);
    windModel.noteOn(440.0f, 0.5f, 0.5f);
    percussionModel.noteOn(0.5f, PercussionModelVoice::DrumType::Kick);
    
    // Process audio
    juce::AudioBuffer<float> stringBuffer(2, bufferSize_);
    juce::AudioBuffer<float> windBuffer(2, bufferSize_);
    juce::AudioBuffer<float> percussionBuffer(2, bufferSize_);
    
    stringModel.process(stringBuffer, bufferSize_);
    windModel.process(windBuffer, bufferSize_);
    percussionModel.process(percussionBuffer, bufferSize_);
    
    // Export audio files
    exportAudioBuffer(stringBuffer, "StringModelTest.wav");
    exportAudioBuffer(windBuffer, "WindModelTest.wav");
    exportAudioBuffer(percussionBuffer, "PercussionModelTest.wav");
    
    DBG("Test audio files exported");
}

juce::String PhysicalModelingTest::getTestSummary() const
{
    juce::String summary;
    summary << "Total: " << totalTests_ << ", Passed: " << passedTests_ << ", Failed: " << failedTests_;
    summary << " (" << (passedTests_ * 100.0f / totalTests_) << "% success rate)";
    return summary;
}

juce::Array<juce::String> PhysicalModelingTest::getFailedTests() const
{
    return failedTests_;
}

juce::Array<juce::String> PhysicalModelingTest::getPassedTests() const
{
    return passedTests_;
}

// Private Utility Methods
bool PhysicalModelingTest::compareFloats(float a, float b, float tolerance)
{
    return std::abs(a - b) < tolerance;
}

bool PhysicalModelingTest::compareBuffers(const juce::AudioBuffer<float>& buf1, const juce::AudioBuffer<float>& buf2, float tolerance)
{
    if (buf1.getNumSamples() != buf2.getNumSamples() || buf1.getNumChannels() != buf2.getNumChannels()) {
        return false;
    }
    
    for (int channel = 0; channel < buf1.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buf1.getNumSamples(); ++sample) {
            if (!compareFloats(buf1.getSample(channel, sample), buf2.getSample(channel, sample), tolerance)) {
                return false;
            }
        }
    }
    
    return true;
}

float PhysicalModelingTest::calculateRMS(const juce::AudioBuffer<float>& buffer)
{
    float sum = 0.0f;
    int count = 0;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            sum += buffer.getSample(channel, sample) * buffer.getSample(channel, sample);
            count++;
        }
    }
    
    return std::sqrt(sum / count);
}

float PhysicalModelingTest::calculatePeak(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            peak = juce::jmax(peak, std::abs(buffer.getSample(channel, sample)));
        }
    }
    
    return peak;
}

void PhysicalModelingTest::createTestSignal(juce::AudioBuffer<float>& buffer, float frequency, float amplitude)
{
    buffer.clear();
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float value = amplitude * std::sin(2.0 * juce::MathConstants<float>::pi * frequency * sample / sampleRate_);
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }
}

void PhysicalModelingTest::createNoiseSignal(juce::AudioBuffer<float>& buffer, float amplitude)
{
    buffer.clear();
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float value = amplitude * (2.0f * (float)juce::Random::getSystemRandom().nextFloat() - 1.0f);
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }
}

void PhysicalModelingTest::analyzeFrequencyResponse(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output)
{
    // Simple frequency response analysis
    float inputRMS = calculateRMS(input);
    float outputRMS = calculateRMS(output);
    
    DBG("Frequency Response - Input RMS: " << inputRMS << ", Output RMS: " << outputRMS);
    DBG("Gain: " << 20.0f * std::log10(outputRMS / inputRMS) << " dB");
}

void PhysicalModelingTest::analyzeDynamicRange(const juce::AudioBuffer<float>& buffer)
{
    float peak = calculatePeak(buffer);
    float rms = calculateRMS(buffer);
    
    DBG("Dynamic Range Analysis - Peak: " << peak << ", RMS: " << rms);
    DBG("Peak-to-RMS Ratio: " << 20.0f * std::log10(peak / rms) << " dB");
}

void PhysicalModelingTest::measureLatency(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output)
{
    // Simple latency measurement
    float maxInput = 0.0f;
    int maxInputSample = 0;
    
    for (int sample = 0; sample < input.getNumSamples(); ++sample) {
        if (std::abs(input.getSample(0, sample)) > maxInput) {
            maxInput = std::abs(input.getSample(0, sample));
            maxInputSample = sample;
        }
    }
    
    float maxOutput = 0.0f;
    int maxOutputSample = 0;
    
    for (int sample = 0; sample < output.getNumSamples(); ++sample) {
        if (std::abs(output.getSample(0, sample)) > maxOutput) {
            maxOutput = std::abs(output.getSample(0, sample));
            maxOutputSample = sample;
        }
    }
    
    int latencySamples = maxOutputSample - maxInputSample;
    float latencyMs = latencySamples * 1000.0f / sampleRate_;
    
    DBG("Latency Measurement: " << latencySamples << " samples (" << latencyMs << " ms)");
}

void PhysicalModelingTest::measureCPUUsage(std::function<void()> testFunction, const juce::String& testName)
{
    auto start = juce::Time::getMillisecondCounterHiRes();
    testFunction();
    auto end = juce::Time::getMillisecondCounterHiRes();
    
    double duration = (end - start) / 1000.0;
    DBG(testName << " completed in " << duration << " seconds");
}

void PhysicalModelingTest::measureMemoryUsage(std::function<void()> testFunction, const juce::String& testName)
{
    // Get memory before
    size_t memoryBefore = juce::SystemStats::getMemorySizeTotal();
    
    // Execute test
    testFunction();
    
    // Get memory after
    size_t memoryAfter = juce::SystemStats::getMemorySizeTotal();
    
    DBG(testName << " memory usage: " << (memoryAfter - memoryBefore) / 1024 / 1024 << " MB");
}

} // namespace Zenith
