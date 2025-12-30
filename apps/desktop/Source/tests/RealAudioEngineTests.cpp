/**
 * @file RealAudioEngineTests.cpp
 * @brief Real unit tests for core audio components using juce::UnitTest
 */

#include <juce_core/juce_core.h>
#include "engine/ThreadSafeAudioProcessor.h"
#include "engine/AudioEngineCore.h"
#include <thread>
#include <chrono>

namespace zenith {
namespace tests {

class LockFreeRingBufferTests : public juce::UnitTest {
public:
    LockFreeRingBufferTests() : juce::UnitTest("LockFreeRingBuffer", "AudioThreading") {}

    void runTest() override {
        beginTest("Empty buffer");
        {
            LockFreeRingBuffer<int, 4> buffer;
            int value;
            expect(!buffer.pop(value));
        }

        beginTest("Push and pop");
        {
            LockFreeRingBuffer<int, 4> buffer;
            expect(buffer.push(42));
            expect(buffer.push(43));

            int value;
            expect(buffer.pop(value));
            expectEquals(value, 42);
            expect(buffer.pop(value));
            expectEquals(value, 43);
        }

        beginTest("Buffer full");
        {
            LockFreeRingBuffer<int, 4> buffer;
            expect(buffer.push(1));
            expect(buffer.push(2));
            expect(buffer.push(3));
            expect(!buffer.push(4)); // Should be full (1 slot kept free)
        }
    }
};

class AtomicPlayheadTests : public juce::UnitTest {
public:
    AtomicPlayheadTests() : juce::UnitTest("AtomicPlayhead", "AudioThreading") {}

    void runTest() override {
        beginTest("Initial position");
        {
            AtomicPlayhead playhead;
            expectEquals(playhead.getPositionSamples(), (int64_t)0);
            expectEquals(playhead.getPositionSeconds(), 0.0);
        }

        beginTest("Position advancement");
        {
            AtomicPlayhead playhead;
            playhead.advance(512, 48000.0);
            expectEquals(playhead.getPositionSamples(), (int64_t)512);
            expectWithinAbsoluteError(playhead.getPositionSeconds(), 512.0 / 48000.0, 0.0001);
        }

        beginTest("Position setting");
        {
            AtomicPlayhead playhead;
            playhead.setPosition(24000, 48000.0);
            expectEquals(playhead.getPositionSamples(), (int64_t)24000);
            expectWithinAbsoluteError(playhead.getPositionSeconds(), 0.5, 0.0001);
        }
    }
};

class RTAudioBufferTests : public juce::UnitTest {
public:
    RTAudioBufferTests() : juce::UnitTest("RTAudioBuffer", "AudioEngine") {}

    void runTest() override {
        beginTest("Initial state");
        {
            RTAudioBuffer buffer(2, 512);
            expectEquals(buffer.getNumChannels(), 2);
            expectEquals(buffer.getNumSamples(), 512);
        }

        beginTest("Clear buffer");
        {
            RTAudioBuffer buffer(2, 512);
            buffer.getWritePointer(0)[0] = 1.0f;
            buffer.getWritePointer(1)[0] = 1.0f;
            buffer.clear();
            expectEquals(buffer.getReadPointer(0)[0], 0.0f);
            expectEquals(buffer.getReadPointer(1)[0], 0.0f);
        }

        beginTest("Copy from source");
        {
            RTAudioBuffer buffer(2, 512);
            float source[] = {1.0f, 0.5f, -0.5f, -1.0f};
            buffer.copyFrom(0, 0, source, 4);

            expectEquals(buffer.getReadPointer(0)[0], 1.0f);
            expectEquals(buffer.getReadPointer(0)[1], 0.5f);
            expectEquals(buffer.getReadPointer(0)[2], -0.5f);
            expectEquals(buffer.getReadPointer(0)[3], -1.0f);
        }

        beginTest("Add from source with gain");
        {
            RTAudioBuffer buffer(2, 512);
            float data1[] = {1.0f, 1.0f};
            float data2[] = {1.0f, 2.0f};
            buffer.copyFrom(0, 0, data1, 2);
            buffer.addFrom(0, 0, data2, 2, 0.5f);

            expectEquals(buffer.getReadPointer(0)[0], 1.5f);
            expectEquals(buffer.getReadPointer(0)[1], 2.0f);
        }
    }
};

class AudioEngineCoreTests : public juce::UnitTest {
public:
    AudioEngineCoreTests() : juce::UnitTest("AudioEngineCore", "AudioEngine") {}

    void runTest() override {
        beginTest("Initialization");
        {
            AudioEngineCore engine;
            expect(engine.initialize(48000.0, 512));
            expectEquals(engine.getSampleRate(), 48000.0);
            expectEquals(engine.getBufferSize(), 512);
        }

        beginTest("Transport control");
        {
            AudioEngineCore engine;
            engine.initialize();
            expect(!engine.isPlaying());

            engine.startPlayback();
            expect(engine.isPlaying());

            engine.stopPlayback();
            expect(!engine.isPlaying());
        }

        beginTest("Position control");
        {
            AudioEngineCore engine;
            engine.initialize();
            engine.setPlaybackPosition(1000);
            expectEquals(engine.getPlaybackPosition(), (int64_t)1000);

            engine.setPlaybackPosition(0);
            expectEquals(engine.getPlaybackPosition(), (int64_t)0);
        }
    }
};

class ThreadSafetyStressTests : public juce::UnitTest {
public:
    ThreadSafetyStressTests() : juce::UnitTest("Thread Safety Stress", "AudioThreading") {}

    void runTest() override {
        beginTest("RingBuffer stress test");
        {
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
                        if (value != lastValue + 1) {
                            expect(false, "Value sequence corrupted");
                        }
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

            expect(pushCount.load() > 0);
            expect(popCount.load() > 0);
            expect(pushCount.load() >= popCount.load());
        }
    }
};

static LockFreeRingBufferTests lockFreeRingBufferTests;
static AtomicPlayheadTests atomicPlayheadTests;
static RTAudioBufferTests rtAudioBufferTests;
static AudioEngineCoreTests audioEngineCoreTests;
static ThreadSafetyStressTests threadSafetyStressTests;

} // namespace tests
} // namespace zenith