/**
 * @file TestUtils.cpp
 * @brief Testing utilities for Zenith DAW
 */

#include "TestUtils.h"
#include "engine/core/EngineCore.h"
#include "engine/core/TrackManager.h"
#include "engine/core/TransportController.h"
#include <memory>

namespace zenith {
namespace test {

// Mock Track for testing
class MockTrack : public zenith::Track {
public:
    MockTrack() {
        setName("Mock Track");
        setVolume(1.0f);
        setPan(0.0f);
        setMuted(false);
        setSolo(false);
        setArmed(false);
        setInputChannel(0);
    }

    void prepareToPlay(int samplesPerBlock, double sampleRate) override {
        // Mock implementation
    }

    void releaseResources() override {
        // Mock implementation
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        // Mock implementation - just pass through
    }
};

// Test utilities
TestUtils::TestUtils() {
    // Initialize test environment
}

TestUtils::~TestUtils() {
    // Clean up test environment
}

std::unique_ptr<EngineCore> TestUtils::createTestEngine() {
    auto engine = std::make_unique<EngineCore>();

    // Initialize the engine for testing
    if (!engine->initialize()) {
        throw std::runtime_error("Failed to initialize test engine");
    }

    return engine;
}

std::unique_ptr<TrackManager> TestUtils::createTestTrackManager() {
    auto trackManager = std::make_unique<TrackManager>();
    return trackManager;
}

std::unique_ptr<TransportController> TestUtils::createTestTransportController() {
    auto controller = std::make_unique<TransportController>();
    return controller;
}

std::unique_ptr<MockTrack> TestUtils::createMockTrack() {
    return std::make_unique<MockTrack>();
}

// Test configuration
TestConfiguration::TestConfiguration() {
    // Set default test configuration
    sampleRate = 44100.0;
    bufferSize = 512;
    numChannels = 2;
    enableDebugLogging = false;
    enableVerboseOutput = false;
    enablePerformanceMetrics = true;
}

TestConfiguration::~TestConfiguration() {
    // Clean up configuration
}

void TestConfiguration::applyToEngine(EngineCore& engine) {
    // Apply configuration to engine
    engine.setSampleRate(sampleRate);
    engine.setBufferSize(bufferSize);
}

void TestConfiguration::print() const {
    std::cout << "Test Configuration:" << std::endl;
    std::cout << "  Sample Rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << bufferSize << " samples" << std::endl;
    std::cout << "  Num Channels: " << numChannels << std::endl;
    std::cout << "  Debug Logging: " << (enableDebugLogging ? "On" : "Off") << std::endl;
    std::cout << "  Verbose Output: " << (enableVerboseOutput ? "On" : "Off") << std::endl;
    std::cout << "  Performance Metrics: " << (enablePerformanceMetrics ? "On" : "Off") << std::endl;
}

// Performance metrics
PerformanceMetrics::PerformanceMetrics() {
    reset();
}

PerformanceMetrics::~PerformanceMetrics() {
    // Clean up metrics
}

void PerformanceMetrics::startTiming(const std::string& operation) {
    auto it = timings.find(operation);
    if (it == timings.end()) {
        timings[operation] = {0.0, 0};
    }
    currentStartTimes[operation] = std::chrono::high_resolution_clock::now();
}

void PerformanceMetrics::stopTiming(const std::string& operation) {
    auto startIt = currentStartTimes.find(operation);
    if (startIt != currentStartTimes.end()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startIt->second);

        auto metricsIt = timings.find(operation);
        if (metricsIt != timings.end()) {
            metricsIt->second.first += duration.count();
            metricsIt->second.second++;
        }

        currentStartTimes.erase(startIt);
    }
}

double PerformanceMetrics::getAverageTime(const std::string& operation) const {
    auto it = timings.find(operation);
    if (it != timings.end() && it->second.second > 0) {
        return it->second.first / it->second.second;
    }
    return 0.0;
}

void PerformanceMetrics::printMetrics() const {
    std::cout << "Performance Metrics:" << std::endl;
    for (const auto& [operation, metrics] : timings) {
        std::cout << "  " << operation << ": "
                  << metrics.first << "s total, "
                  << metrics.second << " calls, "
                  << (metrics.second > 0 ? metrics.first / metrics.second : 0.0)
                  << "s average" << std::endl;
    }
}

void PerformanceMetrics::reset() {
    timings.clear();
    currentStartTimes.clear();
}

// Test assertions
TestAssertions::TestAssertions() {
    // Initialize assertions
}

TestAssertions::~TestAssertions() {
    // Clean up assertions
}

bool TestAssertions::assertEngineState(EngineCore& engine, EngineState expectedState) {
    bool success = true;

    if (engine.isPlaying() != (expectedState == EngineState::PLAYING)) {
        std::cout << "ERROR: Engine play state mismatch" << std::endl;
        success = false;
    }

    if (engine.isRecording() != (expectedState == EngineState::RECORDING)) {
        std::cout << "ERROR: Engine record state mismatch" << std::endl;
        success = false;
    }

    if (engine.isInitialized() != (expectedState != EngineState::UNINITIALIZED)) {
        std::cout << "ERROR: Engine initialized state mismatch" << std::endl;
        success = false;
    }

    return success;
}

bool TestAssertions::assertTrackCount(TrackManager& trackManager, int expectedCount) {
    if (trackManager.getNumTracks() != expectedCount) {
        std::cout << "ERROR: Track count mismatch. Expected: " << expectedCount
                  << ", Actual: " << trackManager.getNumTracks() << std::endl;
        return false;
    }
    return true;
}

bool TestAssertions::assertAudioBuffer(const juce::AudioBuffer<float>& buffer, int expectedNumChannels, int expectedNumSamples) {
    if (buffer.getNumChannels() != expectedNumChannels) {
        std::cout << "ERROR: Number of channels mismatch. Expected: " << expectedNumChannels
                  << ", Actual: " << buffer.getNumChannels() << std::endl;
        return false;
    }

    if (buffer.getNumSamples() != expectedNumSamples) {
        std::cout << "ERROR: Number of samples mismatch. Expected: " << expectedNumSamples
                  << ", Actual: " << buffer.getNumSamples() << std::endl;
        return false;
    }

    return true;
}

// Test fixtures
TestFixture::TestFixture() {
    // Initialize test fixture
    engine = TestUtils::createTestEngine();
    trackManager = TestUtils::createTestTrackManager();
    transportController = TestUtils::createTestTransportController();
}

TestFixture::~TestFixture() {
    // Clean up test fixture
    engine->shutdown();
    engine.reset();
    trackManager.reset();
    transportController.reset();
}

EngineCore& TestFixture::getEngine() {
    return *engine;
}

TrackManager& TestFixture::getTrackManager() {
    return *trackManager;
}

TransportController& TestFixture::getTransportController() {
    return *transportController;
}

// Integration test helper
IntegrationTestHelper::IntegrationTestHelper() {
    // Initialize integration test helper
}

IntegrationTestHelper::~IntegrationTestHelper() {
    // Clean up integration test helper
}

void IntegrationTestHelper::createSimpleProject(EngineCore& engine, int numTracks) {
    auto& trackManager = engine.getTrackManager();

    // Create simple project with tracks
    for (int i = 0; i < numTracks; ++i) {
        auto trackId = trackManager.createTrack("Track " + std::to_string(i + 1), "audio");
        if (!trackId.isEmpty()) {
            trackManager.setTrackVolume(i, 0.8f);
            trackManager.setTrackPan(i, 0.0f);
        }
    }
}

void IntegrationTestHelper::playProject(EngineCore& engine, double durationSeconds) {
    engine.play();

    // Simulate playback for the given duration
    auto startTime = std::chrono::high_resolution_clock::now();
    while (true) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - startTime);

        if (elapsed.count() >= durationSeconds) {
            break;
        }

        // Simulate audio processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void IntegrationTestHelper::stopProject(EngineCore& engine) {
    engine.stop();
}

} // namespace test
} // namespace zenith