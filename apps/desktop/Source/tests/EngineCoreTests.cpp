/**
 * @file EngineCoreTests.cpp
 * @brief Unit tests for the engine core
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "engine/core/EngineCore.h"
#include "engine/core/AudioDeviceManager.h"
#include "engine/core/TrackManager.h"
#include "engine/core/TransportController.h"
#include <memory>

using namespace zenith;
using namespace testing;

class EngineCoreTests : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<EngineCore>();
    }

    void TearDown() override {
        engine.reset();
    }

    std::unique_ptr<EngineCore> engine;
};

TEST_F(EngineCoreTests, Initialization) {
    EXPECT_NO_THROW(engine->initialize());
    EXPECT_TRUE(engine->isInitialized());
}

TEST_F(EngineCoreTests, TransportControl) {
    EXPECT_NO_THROW(engine->play());
    EXPECT_TRUE(engine->isPlaying());

    EXPECT_NO_THROW(engine->stop());
    EXPECT_FALSE(engine->isPlaying());
}

TEST_F(EngineCoreTests, TransportPosition) {
    // Test initial position
    EXPECT_EQ(engine->getPlayheadSamples(), 0);

    // Test position change
    EXPECT_NO_THROW(engine->setPlayheadSamples(1000));
    EXPECT_EQ(engine->getPlayheadSamples(), 1000);
}

TEST_F(EngineCoreTests, LoopControl) {
    EXPECT_NO_THROW(engine->setLooping(true));
    EXPECT_TRUE(engine->isLooping());

    EXPECT_NO_THROW(engine->setLoopRegion(1000, 2000));
    EXPECT_EQ(engine->getLoopStart(), 1000);
    EXPECT_EQ(engine->getLoopEnd(), 2000);
}

TEST_F(EngineCoreTests, TrackManagement) {
    auto& trackManager = engine->getTrackManager();

    // Test track creation
    auto trackId = trackManager.createTrack("Test Track", "audio");
    EXPECT_FALSE(trackId.isEmpty());

    // Test track retrieval
    auto* track = trackManager.getTrackById(trackId);
    EXPECT_NE(track, nullptr);
    EXPECT_EQ(track->getName(), "Test Track");
}

TEST_F(EngineCoreTests, AudioDeviceManager) {
    auto& deviceManager = engine->getAudioDeviceManager();

    EXPECT_NO_THROW(deviceManager.initialize(44100, 512));
    EXPECT_TRUE(deviceManager.isInitialized());

    EXPECT_NO_THROW(deviceManager.shutdown());
    EXPECT_FALSE(deviceManager.isInitialized());
}

class TransportControllerTests : public ::testing::Test {
protected:
    std::unique_ptr<TransportController> controller;

    void SetUp() override {
        controller = std::make_unique<TransportController>();
    }

    void TearDown() override {
        controller.reset();
    }
};

TEST_F(TransportControllerTests, InitialState) {
    EXPECT_FALSE(controller->isPlaying());
    EXPECT_FALSE(controller->isRecording());
    EXPECT_EQ(controller->getPlayheadSamples(), 0);
}

TEST_F(TransportControllerTests, TransportOperations) {
    controller->play();
    EXPECT_TRUE(controller->isPlaying());

    controller->stop();
    EXPECT_FALSE(controller->isPlaying());
}

TEST_F(TransportControllerTests, RecordingOperations) {
    controller->record();
    EXPECT_TRUE(controller->isRecording());

    controller->stopRecording();
    EXPECT_FALSE(controller->isRecording());
}

TEST_F(TransportControllerTests, ToggleRecording) {
    controller->toggleRecording();
    // Toggle recording should arm the track
    EXPECT_TRUE(controller->isRecordingArmed());
}

TEST_F(TransportControllerTests, Panic) {
    controller->play();
    EXPECT_TRUE(controller->isPlaying());

    controller->panic();
    EXPECT_FALSE(controller->isPlaying());
    EXPECT_FALSE(controller->isRecording());
}

class TrackManagerTests : public ::testing::Test {
protected:
    std::unique_ptr<TrackManager> trackManager;

    void SetUp() override {
        trackManager = std::make_unique<TrackManager>();
    }

    void TearDown() override {
        trackManager.reset();
    }
};

TEST_F(TrackManagerTests, TrackCreation) {
    auto trackId = trackManager->createTrack("Test Track", "audio");
    EXPECT_FALSE(trackId.isEmpty());
    EXPECT_EQ(trackManager->getNumTracks(), 1);
}

TEST_F(TrackManagerTests, TrackRemoval) {
    auto trackId = trackManager->createTrack("Test Track", "audio");
    EXPECT_EQ(trackManager->getNumTracks(), 1);

    trackManager->removeTrack(trackId);
    EXPECT_EQ(trackManager->getNumTracks(), 0);
}

TEST_F(TrackManagerTests, TrackState) {
    auto trackId = trackManager->createTrack("Test Track", "audio");

    // Test track state manipulation
    trackManager->setTrackVolume(0, 0.5f);
    EXPECT_FLOAT_EQ(trackManager->getTrackLevel(0), 0.5f);

    trackManager->setTrackMute(0, true);
    EXPECT_TRUE(trackManager->getTrack(0)->isMuted());
}

class AudioDeviceManagerTests : public ::testing::Test {
protected:
    std::unique_ptr<AudioDeviceManager> deviceManager;

    void SetUp() override {
        deviceManager = std::make_unique<AudioDeviceManager>();
    }

    void TearDown() override {
        deviceManager->shutdown();
        deviceManager.reset();
    }
};

TEST_F(AudioDeviceManagerTests, Initialization) {
    EXPECT_NO_THROW(deviceManager->initialize(44100, 512));
    EXPECT_TRUE(deviceManager->isInitialized());
}

TEST_F(AudioDeviceManagerTests, SuspendResume) {
    EXPECT_NO_THROW(deviceManager->initialize(44100, 512));

    EXPECT_NO_THROW(deviceManager->setSuspended(true));
    EXPECT_TRUE(deviceManager->isSuspended());

    EXPECT_NO_THROW(deviceManager->setSuspended(false));
    EXPECT_FALSE(deviceManager->isSuspended());
}

TEST_F(AudioDeviceManagerTests, GetDeviceInfo) {
    EXPECT_NO_THROW(deviceManager->initialize(44100, 512));

    auto info = deviceManager->getDeviceInfo();
    EXPECT_FALSE(info.isEmpty());
}

// Performance benchmarks
TEST_F(EngineCoreTests, PerformanceBenchmark) {
    auto start = std::chrono::high_resolution_clock::now();

    // Simulate many track operations
    for (int i = 0; i < 1000; ++i) {
        trackManager->createTrack("Track " + std::to_string(i), "audio");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_GT(trackManager->getNumTracks(), 900); // Should have most tracks
    EXPECT_LT(duration.count(), 1000); // Should complete in under 1 second
}

// Integration test
TEST_F(EngineCoreTests, EngineIntegration) {
    // Initialize the engine
    EXPECT_TRUE(engine->initialize());

    // Test transport control
    engine->play();
    EXPECT_TRUE(engine->isPlaying());

    // Test track management
    auto& trackManager = engine->getTrackManager();
    auto trackId = trackManager.createTrack("Integration Test Track", "audio");
    EXPECT_FALSE(trackId.isEmpty());

    // Test audio device manager
    auto& deviceManager = engine->getAudioDeviceManager();
    EXPECT_NO_THROW(deviceManager.initialize(44100, 512));

    // Shutdown
    engine->shutdown();
    EXPECT_FALSE(engine->isInitialized());
}