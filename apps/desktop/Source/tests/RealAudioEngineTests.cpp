/**
 * @file AudioEngineTests.cpp
 * @brief Real unit tests for core audio components
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "engine/ThreadSafeAudioProcessor.h"
#include "engine/AudioEngineCore.h"

using namespace zenith;

TEST_CASE("LockFreeRingBuffer basic operations", "[audio][threading]") {
    LockFreeRingBuffer<int, 4> buffer;
    
    SECTION("Empty buffer") {
        int value;
        REQUIRE_FALSE(buffer.pop(value));
    }
    
    SECTION("Push and pop") {
        REQUIRE(buffer.push(42));
        REQUIRE(buffer.push(43));
        
        int value;
        REQUIRE(buffer.pop(value));
        REQUIRE(value == 42);
        REQUIRE(buffer.pop(value));
        REQUIRE(value == 43);
    }
    
    SECTION("Buffer full") {
        REQUIRE(buffer.push(1));
        REQUIRE(buffer.push(2));
        REQUIRE(buffer.push(3));
        REQUIRE_FALSE(buffer.push(4)); // Should be full (1 slot kept free)
    }
}

TEST_CASE("AtomicPlayhead position tracking", "[audio][threading]") {
    AtomicPlayhead playhead;
    
    SECTION("Initial position") {
        REQUIRE(playhead.getPositionSamples() == 0);
        REQUIRE(playhead.getPositionSeconds() == 0.0);
    }
    
    SECTION("Position advancement") {
        playhead.advance(512, 48000.0);
        REQUIRE(playhead.getPositionSamples() == 512);
        REQUIRE(playhead.getPositionSeconds() == Catch::Approx(512.0 / 48000.0));
    }
    
    SECTION("Position setting") {
        playhead.setPosition(24000, 48000.0);
        REQUIRE(playhead.getPositionSamples() == 24000);
        REQUIRE(playhead.getPositionSeconds() == Catch::Approx(0.5));
    }
}

TEST_CASE("RTAudioBuffer operations", "[audio]") {
    RTAudioBuffer buffer(2, 512); // 2 channels, 512 samples
    
    SECTION("Initial state") {
        REQUIRE(buffer.getNumChannels() == 2);
        REQUIRE(buffer.getNumSamples() == 512);
    }
    
    SECTION("Clear buffer") {
        buffer.getWritePointer(0)[0] = 1.0f;
        buffer.getWritePointer(1)[0] = 1.0f;
        buffer.clear();
        REQUIRE(buffer.getReadPointer(0)[0] == 0.0f);
        REQUIRE(buffer.getReadPointer(1)[0] == 0.0f);
    }
    
    SECTION("Copy from source") {
        float source[] = {1.0f, 0.5f, -0.5f, -1.0f};
        buffer.copyFrom(0, 0, source, 4);
        
        REQUIRE(buffer.getReadPointer(0)[0] == 1.0f);
        REQUIRE(buffer.getReadPointer(0)[1] == 0.5f);
        REQUIRE(buffer.getReadPointer(0)[2] == -0.5f);
        REQUIRE(buffer.getReadPointer(0)[3] == -1.0f);
    }
    
    SECTION("Add from source with gain") {
        buffer.copyFrom(0, 0, std::vector<float>{1.0f, 1.0f}.data(), 2);
        buffer.addFrom(0, 0, std::vector<float>{1.0f, 2.0f}.data(), 2, 0.5f);
        
        REQUIRE(buffer.getReadPointer(0)[0] == 1.5f);
        REQUIRE(buffer.getReadPointer(0)[1] == 2.0f);
    }
}

TEST_CASE("AudioEngineCore basic functionality", "[audio][engine]") {
    AudioEngineCore engine;
    
    SECTION("Initialization") {
        REQUIRE(engine.initialize(48000.0, 512));
        REQUIRE(engine.getSampleRate() == 48000.0);
        REQUIRE(engine.getBufferSize() == 512);
    }
    
    SECTION("Transport control") {
        engine.initialize();
        
        REQUIRE_FALSE(engine.isPlaying());
        
        engine.startPlayback();
        REQUIRE(engine.isPlaying());
        
        engine.stopPlayback();
        REQUIRE_FALSE(engine.isPlaying());
    }
    
    SECTION("Position control") {
        engine.initialize();
        
        engine.setPlaybackPosition(1000);
        REQUIRE(engine.getPlaybackPosition() == 1000);
        
        engine.setPlaybackPosition(0);
        REQUIRE(engine.getPlaybackPosition() == 0);
    }
}

TEST_CASE("Thread safety stress test", "[audio][threading][stress]") {
    LockFreeRingBuffer<int, 1024> buffer;
    std::atomic<bool> stop{false};
    std::atomic<int> pushCount{0};
    std::atomic<int> popCount{0};
    
    // Producer thread
    std::thread producer([&]() {
        int value = 0;
        while (!stop.load()) {
            if (buffer.push(value)) {
                pushCount.fetch_add(1);
                value++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        int lastValue = -1;
        while (!stop.load()) {
            int value;
            if (buffer.pop(value)) {
                REQUIRE(value == lastValue + 1);
                lastValue = value;
                popCount.fetch_add(1);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
    
    // Run for 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);
    
    producer.join();
    consumer.join();
    
    // Should have processed some items without corruption
    REQUIRE(pushCount.load() > 0);
    REQUIRE(popCount.load() > 0);
    REQUIRE(pushCount.load() == popCount.load() + 1); // One item might be in buffer
}
