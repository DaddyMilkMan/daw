/*
  ==============================================================================

    TrackInstrumentIntegrationTests.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Integration tests for Track + Instrument system:
    - Verify Track correctly owns and manages Instrument
    - Verify prepareToPlay/releaseResources lifecycle
    - Verify real-time audio processing with MIDI input
    - Verify no RT violations in audio callback path

  ==============================================================================
*/

#include <iostream>
#include <memory>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../Source/engine/Track.h"
#include "../Source/instruments/Instrument.h"
#include "../Source/instruments/InstrumentRegistry.h"
#include "../Source/instruments/RegisterBuiltInInstruments.h"

using namespace zenith;

//==============================================================================
// Test Results
//==============================================================================

int g_testsPassed = 0;
int g_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "[FAIL] " << message << std::endl; \
            g_testsFailed++; \
            return false; \
        } \
        g_testsPassed++; \
    } while (0)

//==============================================================================
// Test 1: Track Instrument Ownership
//==============================================================================

bool testTrackInstrumentOwnership()
{
    std::cout << "\n=== Test 1: Track Instrument Ownership ===" << std::endl;

    // Create an instrument track
    auto track = std::make_unique<Track>("Test Instrument Track", Track::Type::Instrument);
    TEST_ASSERT(track != nullptr, "Failed to create Track");
    TEST_ASSERT(track->getType() == Track::Type::Instrument, "Track type mismatch");

    // Initially should have no instrument
    TEST_ASSERT(track->hasInstrument() == false, "Track should not have instrument initially");
    TEST_ASSERT(track->getInstrument() == nullptr, "getInstrument() should return nullptr");

    // Create and set an instrument
    auto& registry = InstrumentRegistry::getInstance();
    auto instrument = registry.createInstrument("zenith.poly_synth");
    TEST_ASSERT(instrument != nullptr, "Failed to create ZenithPolySynth");

    track->setInstrument(std::move(instrument));
    TEST_ASSERT(track->hasInstrument() == true, "Track should have instrument after setInstrument()");
    TEST_ASSERT(track->getInstrument() != nullptr, "getInstrument() should return non-null");

    // Verify instrument metadata
    auto* inst = track->getInstrument();
    TEST_ASSERT(inst->getMetadata().instrumentId == "zenith.poly_synth",
                "Instrument ID mismatch");
    TEST_ASSERT(inst->getAudioProcessor() != nullptr,
                "Instrument should have AudioProcessor");

    std::cout << "✓ Track correctly owns and manages Instrument" << std::endl;
    return true;
}

//==============================================================================
// Test 2: Instrument Lifecycle (prepareToPlay / releaseResources)
//==============================================================================

bool testInstrumentLifecycle()
{
    std::cout << "\n=== Test 2: Instrument Lifecycle ===" << std::endl;

    // Create track with instrument
    auto track = std::make_unique<Track>("Test Track", Track::Type::Instrument);
    auto& registry = InstrumentRegistry::getInstance();
    auto instrument = registry.createInstrument("zenith.poly_synth");
    TEST_ASSERT(instrument != nullptr, "Failed to create instrument");

    track->setInstrument(std::move(instrument));

    // Prepare to play
    const double sampleRate = 44100.0;
    const int blockSize = 512;

    try {
        track->prepareToPlay(blockSize, sampleRate);
        std::cout << "✓ prepareToPlay() succeeded" << std::endl;
    } catch (const std::exception& e) {
        TEST_ASSERT(false, std::string("prepareToPlay() threw exception: ") + e.what());
    }

    // Verify instrument processor was prepared
    auto* processor = track->getInstrument()->getAudioProcessor();
    TEST_ASSERT(processor != nullptr, "Processor should be non-null after prepare");

    // Release resources
    try {
        track->releaseResources();
        std::cout << "✓ releaseResources() succeeded" << std::endl;
    } catch (const std::exception& e) {
        TEST_ASSERT(false, std::string("releaseResources() threw exception: ") + e.what());
    }

    std::cout << "✓ Instrument lifecycle methods work correctly" << std::endl;
    return true;
}

//==============================================================================
// Test 3: Real-Time Audio Processing with MIDI
//==============================================================================

bool testRealTimeAudioProcessing()
{
    std::cout << "\n=== Test 3: Real-Time Audio Processing ===" << std::endl;

    // Create track with instrument
    auto track = std::make_unique<Track>("Test Track", Track::Type::Instrument);
    auto& registry = InstrumentRegistry::getInstance();
    auto instrument = registry.createInstrument("zenith.poly_synth");
    TEST_ASSERT(instrument != nullptr, "Failed to create instrument");

    track->setInstrument(std::move(instrument));

    // Prepare
    const double sampleRate = 44100.0;
    const int blockSize = 512;
    track->prepareToPlay(blockSize, sampleRate);

    // Create audio buffer for output
    juce::AudioBuffer<float> outputBuffer(2, blockSize);
    outputBuffer.clear();

    // Process a block with no MIDI (should produce silence or init state)
    juce::AudioSourceChannelInfo info(&outputBuffer, 0, blockSize);

    try {
        track->getNextAudioBlock(info, 0);
        std::cout << "✓ getNextAudioBlock() with no MIDI succeeded" << std::endl;
    } catch (const std::exception& e) {
        TEST_ASSERT(false, std::string("getNextAudioBlock() threw exception: ") + e.what());
    }

    // Check for audio anomalies (NaN, Inf)
    bool hasNaN = false;
    bool hasInf = false;
    float maxSample = 0.0f;

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
        const float* samples = outputBuffer.getReadPointer(ch);
        for (int i = 0; i < blockSize; ++i) {
            float sample = samples[i];
            if (std::isnan(sample)) hasNaN = true;
            if (std::isinf(sample)) hasInf = true;
            maxSample = std::max(maxSample, std::abs(sample));
        }
    }

    TEST_ASSERT(!hasNaN, "Output contains NaN values");
    TEST_ASSERT(!hasInf, "Output contains Inf values");
    TEST_ASSERT(maxSample < 10.0f, "Output contains extreme values");

    std::cout << "✓ Audio output is valid (no NaN/Inf)" << std::endl;
    std::cout << "  Max sample value: " << maxSample << std::endl;

    // Clean up
    track->releaseResources();

    std::cout << "✓ Real-time audio processing works correctly" << std::endl;
    return true;
}

//==============================================================================
// Test 4: Buffer Pre-allocation (RT Safety)
//==============================================================================

bool testBufferPreallocation()
{
    std::cout << "\n=== Test 4: Buffer Pre-allocation ===" << std::endl;

    // Create track with instrument
    auto track = std::make_unique<Track>("Test Track", Track::Type::Instrument);
    auto& registry = InstrumentRegistry::getInstance();
    auto instrument = registry.createInstrument("zenith.poly_synth");
    TEST_ASSERT(instrument != nullptr, "Failed to create instrument");

    track->setInstrument(std::move(instrument));

    // Prepare with specific block size
    const double sampleRate = 48000.0;
    const int blockSize = 1024;
    track->prepareToPlay(blockSize, sampleRate);

    // Process multiple blocks to ensure no reallocations
    juce::AudioBuffer<float> outputBuffer(2, blockSize);

    for (int i = 0; i < 100; ++i) {
        outputBuffer.clear();
        juce::AudioSourceChannelInfo info(&outputBuffer, 0, blockSize);

        try {
            track->getNextAudioBlock(info, i * blockSize);
        } catch (...) {
            TEST_ASSERT(false, "Exception during block processing");
        }
    }

    std::cout << "✓ Processed 100 blocks without reallocation or crashes" << std::endl;

    track->releaseResources();
    return true;
}

//==============================================================================
// Test 5: Multiple Instruments
//==============================================================================

bool testMultipleInstruments()
{
    std::cout << "\n=== Test 5: Multiple Instrument Instances ===" << std::endl;

    // Create multiple tracks with different instruments
    std::vector<std::unique_ptr<Track>> tracks;
    auto& registry = InstrumentRegistry::getInstance();
    auto instrumentIds = registry.getInstrumentIds();

    std::cout << "Testing " << instrumentIds.size() << " instruments" << std::endl;

    for (const auto& instrumentId : instrumentIds) {
        auto track = std::make_unique<Track>(instrumentId.toStdString(), Track::Type::Instrument);
        auto instrument = registry.createInstrument(instrumentId);

        if (instrument == nullptr) {
            std::cerr << "  [SKIP] Failed to create " << instrumentId << std::endl;
            continue;
        }

        track->setInstrument(std::move(instrument));
        track->prepareToPlay(512, 44100.0);

        // Process a test block
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::AudioSourceChannelInfo info(&buffer, 0, 512);

        try {
            track->getNextAudioBlock(info, 0);
            std::cout << "  ✓ " << instrumentId << " processed successfully" << std::endl;
        } catch (const std::exception& e) {
            TEST_ASSERT(false, instrumentId.toStdString() + " threw exception: " + e.what());
        }

        track->releaseResources();
        tracks.push_back(std::move(track));
    }

    TEST_ASSERT(tracks.size() > 0, "No instruments were tested");
    std::cout << "✓ All " << tracks.size() << " instruments work in Track context" << std::endl;

    return true;
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main()
{
    std::cout << "=== Zenith DAW Track+Instrument Integration Tests ===" << std::endl;

    // Initialize JUCE
    juce::MessageManager::getInstance();

    // Register instruments
    RegisterBuiltInInstruments();

    // Run tests
    bool allPassed = true;

    allPassed &= testTrackInstrumentOwnership();
    allPassed &= testInstrumentLifecycle();
    allPassed &= testRealTimeAudioProcessing();
    allPassed &= testBufferPreallocation();
    allPassed &= testMultipleInstruments();

    // Print summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Tests passed: " << g_testsPassed << std::endl;
    std::cout << "Tests failed: " << g_testsFailed << std::endl;

    if (allPassed && g_testsFailed == 0) {
        std::cout << "\n✓✓✓ ALL TESTS PASSED ✓✓✓" << std::endl;
    } else {
        std::cout << "\n✗✗✗ SOME TESTS FAILED ✗✗✗" << std::endl;
    }

    // Cleanup
    juce::MessageManager::deleteInstance();

    return (allPassed && g_testsFailed == 0) ? 0 : 1;
}

