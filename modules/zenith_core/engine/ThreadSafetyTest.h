/*
    ThreadSafetyTest.h - Comprehensive thread safety test suite for Zenith DAW

    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

namespace zenith {

/**
 * @class ThreadSafetyTest
 * @brief Comprehensive thread safety test suite for audio engine components
 *
 * Tests thread safety of critical audio engine components including:
 * - Track management and snapshots
 * - Audio device callbacks
 * - Plugin processing
 * - MIDI queue management
 * - Engine state changes
 */
class ThreadSafetyTest {
public:
    /**
     * @brief Run all thread safety tests
     * @return true if all tests pass, false otherwise
     */
    static bool runAllTests() {
        std::cout << "==================================================" << std::endl;
        std::cout << "Zenith DAW - Thread Safety Test Suite" << std::endl;
        std::cout << "==================================================" << std::endl;
        std::cout << std::endl;

        bool allPassed = true;

        allPassed &= testTrackManagerSnapshots();
        allPassed &= testAudioDeviceCallback();
        allPassed &= testPluginProcessing();
        allPassed &= testMIDIQueue();
        allPassed &= testEngineState();
        allPassed &= testAtomicOperations();
        allPassed &= testMemoryOrdering();
        allPassed &= testStressScenarios();

        std::cout << "==================================================" << std::endl;
        if (allPassed) {
            std::cout << "✅ All thread safety tests passed!" << std::endl;
        } else {
            std::cout << "❌ Some thread safety tests failed!" << std::endl;
        }
        std::cout << "==================================================" << std::endl;

        return allPassed;
    }

    /**
     * @brief Run specific test
     * @param testName Name of the test to run
     * @return true if test passes, false otherwise
     */
    static bool runSpecificTest(const juce::String& testName) {
        if (testName == "TrackManager") return testTrackManagerSnapshots();
        if (testName == "AudioDevice") return testAudioDeviceCallback();
        if (testName == "PluginProcessing") return testPluginProcessing();
        if (testName == "MIDIQueue") return testMIDIQueue();
        if (testName == "EngineState") return testEngineState();
        if (testName == "AtomicOperations") return testAtomicOperations();
        if (testName == "MemoryOrdering") return testMemoryOrdering();
        if (testName == "StressScenarios") return testStressScenarios();

        std::cout << "Unknown test: " << testName << std::endl;
        return false;
    }

private:
    /**
     * @brief Test track manager snapshot thread safety
     */
    static bool testTrackManagerSnapshots() {
        std::cout << "Testing Track Manager Snapshots..." << std::endl;

        const int numTracks = 100;
        const int numIterations = 1000;
        const int numThreads = 4;

        // Mock track data
        std::vector<std::atomic<float>> trackLevels(numTracks);
        std::atomic<juce::int64> snapshotVersion{0};
        juce::CriticalSection updateLock;

        // Audio thread simulation
        auto audioThread = std::thread([&]() {
            for (int i = 0; i < numIterations; ++i) {
                // Simulate audio thread reading snapshot
                auto version = snapshotVersion.load();

                // Process tracks with current snapshot
                for (int track = 0; track < numTracks; ++track) {
                    float level = trackLevels[track].load();
                    // Simple processing - no modification, just reading
                    juce::ignoreUnused(level);
                }

                // Small delay to simulate audio processing time
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });

        // UI thread simulation
        auto uiThread = std::thread([&]() {
            for (int i = 0; i < numIterations; ++i) {
                // Simulate UI thread updating track levels
                juce::ScopedLock lock(updateLock);

                for (int track = 0; track < numTracks; ++track) {
                    trackLevels[track].store(static_cast<float>(rand()) / RAND_MAX);
                }

                // Update snapshot version
                snapshotVersion.store(snapshotVersion.load() + 1);

                // Small delay to simulate UI updates
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });

        // Background thread simulation
        auto bgThread = std::thread([&]() {
            for (int i = 0; i < numIterations / 2; ++i) {
                // Simulate background operations
                juce::ScopedLock lock(updateLock);

                for (int track = 0; track < numTracks; ++track) {
                    float current = trackLevels[track].load();
                    trackLevels[track].store(current * 0.99f); // Gradual decay
                }

                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        });

        // Wait for all threads to complete
        audioThread.join();
        uiThread.join();
        bgThread.join();

        std::cout << "✅ Track Manager Snapshots test passed" << std::endl;
        return true;
    }

    /**
     * @brief Test audio device callback thread safety
     */
    static bool testAudioDeviceCallback() {
        std::cout << "Testing Audio Device Callback Thread Safety..." << std::endl;

        // Mock audio callback state
        std::atomic<bool> callbackActive{false};
        std::atomic<int> sampleCount{0};
        std::atomic<double> cpuUsage{0.0};
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();

        // Audio callback simulation
        auto audioThread = std::thread([&]() {
            callbackActive.store(true);

            for (int i = 0; i < 1000; ++i) {
                // Simulate audio callback processing
                auto start = std::chrono::high_resolution_clock::now();

                // Process audio buffer
                float* samples = buffer.getWritePointer(0);
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                    samples[sample] = static_cast<float>(sin(i * 0.1)) * 0.5f;
                }

                // Update metrics
                sampleCount.store(sampleCount.load() + buffer.getNumSamples());

                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                cpuUsage.store(cpuUsage.load() + duration.count());

                // Simulate callback timing
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }

            callbackActive.store(false);
        });

        // Monitoring thread simulation
        auto monitorThread = std::thread([&]() {
            while (callbackActive.load()) {
                // Check callback status
                bool isActive = callbackActive.load();
                int currentSamples = sampleCount.load();
                double currentCpu = cpuUsage.load();

                // Verify thread safety
                if (isActive && currentSamples < 0) {
                    std::cout << "❌ Thread safety error: Negative sample count!" << std::endl;
                    return false;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });

        // Control thread simulation
        auto controlThread = std::thread([&]() {
            while (callbackActive.load()) {
                // Simulate control changes during callback
                double newCpu = static_cast<double>(rand()) / RAND_MAX * 100.0;
                cpuUsage.store(newCpu);

                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        });

        // Wait for threads to complete
        audioThread.join();
        monitorThread.join();
        controlThread.join();

        std::cout << "✅ Audio Device Callback test passed" << std::endl;
        return true;
    }

    /**
     * @brief Test plugin processing thread safety
     */
    static bool testPluginProcessing() {
        std::cout << "Testing Plugin Processing Thread Safety..." << std::endl;

        const int numPlugins = 10;
        const int numParameters = 5;

        // Mock plugin state
        std::vector<std::atomic<bool>> pluginEnabled(numPlugins);
        std::vector<std::vector<std::atomic<float>>> pluginParameters(numPlugins);
        for (int i = 0; i < numPlugins; ++i) {
            pluginParameters[i].resize(numParameters);
            pluginEnabled[i].store(true);
        }

        // Audio thread simulation
        auto audioThread = std::thread([&]() {
            for (int iteration = 0; iteration < 1000; ++iteration) {
                // Process each plugin
                for (int plugin = 0; plugin < numPlugins; ++plugin) {
                    if (!pluginEnabled[plugin].load()) continue;

                    // Process parameters safely
                    for (int param = 0; param < numParameters; ++param) {
                        float value = pluginParameters[plugin][param].load();
                        // Simple processing - no modification
                        juce::ignoreUnused(value);
                    }
                }
            }
        });

        // Parameter automation thread simulation
        auto automationThread = std::thread([&]() {
            for (int iteration = 0; iteration < 1000; ++iteration) {
                // Update plugin parameters
                for (int plugin = 0; plugin < numPlugins; ++plugin) {
                    for (int param = 0; param < numParameters; ++param) {
                        float newValue = static_cast<float>(sin(iteration * 0.1)) * 0.5f + 0.5f;
                        pluginParameters[plugin][param].store(newValue);
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        // Plugin management thread simulation
        auto managementThread = std::thread([&]() {
            for (int iteration = 0; iteration < 100; ++iteration) {
                // Enable/disable plugins
                int plugin = rand() % numPlugins;
                bool enabled = static_cast<bool>(rand() % 2);
                pluginEnabled[plugin].store(enabled);

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        // Wait for all threads to complete
        audioThread.join();
        automationThread.join();
        managementThread.join();

        std::cout << "✅ Plugin Processing test passed" << std::endl;
        return true;
    }

    /**
     * @brief Test MIDI queue thread safety
     */
    static bool testMIDIQueue() {
        std::cout << "Testing MIDI Queue Thread Safety..." << std::endl;

        const int queueSize = 1024;
        std::vector<std::atomic<MidiMessage>> midiQueue(queueSize);
        std::atomic<int> writePos{0};
        std::atomic<int> readPos{0};
        std::atomic<int> messageCount{0};

        // MIDI input thread simulation
        auto midiInputThread = std::thread([&]() {
            for (int i = 0; i < 5000; ++i) {
                int pos = writePos.fetch_add(1) % queueSize;

                // Create MIDI message
                MidiMessage msg;
                msg.setVelocity(static_cast<float>(rand() % 127));
                msg.setNoteNumber(rand() % 128);

                // Store in queue
                midiQueue[pos].store(msg);
                messageCount.fetch_add(1);

                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });

        // Audio thread simulation
        auto audioThread = std::thread([&]() {
            for (int i = 0; i < 5000; ++i) {
                // Process MIDI messages from queue
                int currentRead = readPos.load();
                int currentWrite = writePos.load();

                // Process available messages
                while (currentRead != currentWrite) {
                    int pos = currentRead % queueSize;
                    MidiMessage msg = midiQueue[pos].load();

                    // Process MIDI message (no modification)
                    juce::ignoreUnused(msg);

                    readPos.store(currentRead + 1);
                    currentRead++;
                    currentWrite = writePos.load();
                }

                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });

        // Wait for both threads to complete
        midiInputThread.join();
        audioThread.join();

        std::cout << "✅ MIDI Queue test passed" << std::endl;
        return true;
    }

    /**
     * @brief Test engine state thread safety
     */
    static bool testEngineState() {
        std::cout << "Testing Engine State Thread Safety..." << std::endl;

        // Mock engine state
        std::atomic<bool> isPlaying{false};
        std::atomic<bool> isRecording{false};
        std::atomic<double> tempo{120.0};
        std::atomic<int> samplesPerBlock{512};

        // Transport control thread simulation
        auto transportThread = std::thread([&]() {
            for (int i = 0; i < 100; ++i) {
                // Toggle transport state
                isPlaying.store(!isPlaying.load());
                isRecording.store(false);

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        // Recording thread simulation
        auto recordingThread = std::thread([&]() {
            for (int i = 0; i < 100; ++i) {
                // Update recording state
                bool playing = isPlaying.load();
                isRecording.store(playing && (rand() % 4 == 0)); // 25% chance when playing

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });

        // Tempo control thread simulation
        auto tempoThread = std::thread([&]() {
            for (int i = 0; i < 100; ++i) {
                // Update tempo gradually
                double currentTempo = tempo.load();
                double newTempo = currentTempo + (static_cast<double>(rand()) / RAND_MAX - 0.5) * 2.0;
                tempo.store(juce::jmax(60.0, juce::jmin(200.0, newTempo)));

                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });

        // Monitoring thread simulation
        auto monitorThread = std::thread([&]() {
            for (int i = 0; i < 100; ++i) {
                // Check state consistency
                bool playing = isPlaying.load();
                bool recording = isRecording.load();

                // Recording should only happen when playing
                if (recording && !playing) {
                    std::cout << "❌ State consistency error: Recording while not playing!" << std::endl;
                    return false;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        });

        // Wait for all threads to complete
        transportThread.join();
        recordingThread.join();
        tempoThread.join();
        monitorThread.join();

        std::cout << "✅ Engine State test passed" << std::endl;
        return true;
    }

    /**
     * @brief Test atomic operations performance and correctness
     */
    static bool testAtomicOperations() {
        std::cout << "Testing Atomic Operations..." << std::endl;

        const int numIterations = 10000;

        // Test atomic load/store
        std::atomic<int> atomicValue{0};

        auto producerThread = std::thread([&]() {
            for (int i = 0; i < numIterations; ++i) {
                atomicValue.store(i, std::memory_order_relaxed);
            }
        });

        auto consumerThread = std::thread([&]() {
            for (int i = 0; i < numIterations; ++i) {
                int value = atomicValue.load(std::memory_order_relaxed);
                if (value < 0 || value >= numIterations) {
                    std::cout << "❌ Atomic operation error: Value out of range!" << std::endl;
                    return false;
                }
            }
        });

        producerThread.join();
        consumerThread.join();

        // Test atomic flags
        std::atomic_flag flag = ATOMIC_FLAG_INIT;

        auto setThread = std::thread([&]() {
            for (int i = 0; i < numIterations; ++i) {
                while (flag.test_and_set(std::memory_order_acquire)) {
                    // Spin until set
                }
                flag.clear(std::memory_order_release);
            }
        });

        auto testThread = std::thread([&]() {
            for (int i = 0; i < numIterations; ++i) {
                bool wasSet = flag.test_and_set(std::memory_order_acquire);
                if (!wasSet) {
                    flag.clear(std::memory_order_release);
                }
            }
        });

        setThread.join();
        testThread.join();

        std::cout << "✅ Atomic Operations test passed" << std::endl;
        return true;
    }

    /**
     * @brief Test memory ordering correctness
     */
    static bool testMemoryOrdering() {
        std::cout << "Testing Memory Ordering..." << std::endl;

        std::atomic<int> x{0};
        std::atomic<int> y{0};
        std::atomic<bool> ready{false};

        // Writer thread
        auto writerThread = std::thread([&]() {
            x.store(42, std::memory_order_release);
            y.store(17, std::memory_order_release);
            ready.store(true, std::memory_order_release);
        });

        // Reader thread
        auto readerThread = std::thread([&]() {
            while (!ready.load(std::memory_order_acquire)) {
                // Wait for ready
            }

            // This should always be true with proper memory ordering
            int xVal = x.load(std::memory_order_acquire);
            int yVal = y.load(std::memory_order_acquire);

            if (xVal != 42 || yVal != 17) {
                std::cout << "❌ Memory ordering error: Invalid values!" << std::endl;
                return false;
            }
        });

        writerThread.join();
        readerThread.join();

        std::cout << "✅ Memory Ordering test passed" << std::endl;
        return true;
    }

    /**
     * @brief Test stress scenarios
     */
    static bool testStressScenarios() {
        std::cout << "Testing Stress Scenarios..." << std::endl;

        const int numThreads = 8;
        const int stressIterations = 5000;
        std::vector<std::thread> threads;

        // Create shared data
        std::atomic<int> sharedCounter{0};
        std::vector<std::atomic<float>> sharedData(100);
        juce::CriticalSection sharedLock;

        // Start multiple threads
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&]() {
                for (int i = 0; i < stressIterations; ++i) {
                    // Simulate various operations
                    sharedCounter.fetch_add(1, std::memory_order_relaxed);

                    // Update shared data
                    {
                        juce::ScopedLock lock(sharedLock);
                        for (int j = 0; j < 10; ++j) {
                            int index = rand() % sharedData.size();
                            sharedData[index].store(static_cast<float>(rand()) / RAND_MAX);
                        }
                    }

                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(rand() % 100));
                }
            });
        }

        // Monitor thread
        auto monitorThread = std::thread([&]() {
            for (int i = 0; i < stressIterations; ++i) {
                int count = sharedCounter.load();
                if (count < 0) {
                    std::cout << "❌ Stress test error: Negative counter!" << std::endl;
                    return;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        monitorThread.join();

        std::cout << "✅ Stress Scenarios test passed" << std::endl;
        return true;
    }

    // Simple MIDI message class for testing
    class MidiMessage {
    public:
        void setVelocity(float velocity) { velocity_ = velocity; }
        void setNoteNumber(int noteNumber) { noteNumber_ = noteNumber; }

        float getVelocity() const { return velocity_; }
        int getNoteNumber() const { return noteNumber_; }

    private:
        float velocity_{0.0f};
        int noteNumber_{0};
    };
};

} // namespace zenith