/*
  ==============================================================================
    agents/RealtimeAudioEngineAgent/tests/RealtimeAudioEngineAgentTest.cpp
    Unit tests for RealtimeAudioEngineAgent.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../RealtimeAudioEngineAgent.h"
#include <thread>
#include <atomic>
#include <chrono>

namespace zenith {
namespace agents {
namespace tests {

class RealtimeAudioEngineAgentTest : public juce::UnitTest {
public:
  RealtimeAudioEngineAgentTest() : juce::UnitTest("RealtimeAudioEngineAgent", "Agents") {}

  void runTest() override {
    testInitialization();
    testStartStop();
    testProcessBlock();
    testAudioPassThrough();
    testStopSynchronization();
  }

private:
  void testInitialization() {
    beginTest("Initialization");
    RealtimeAudioEngineAgent agent;
    expect(!agent.isRunning(), "Should not be running initially");

    agent.initialize(48000.0, 512);
    expect(!agent.isRunning(), "Should not be running after initialize");
  }

  void testStartStop() {
    beginTest("Start/Stop State");
    RealtimeAudioEngineAgent agent;
    agent.initialize(44100.0, 512);

    agent.start();
    expect(agent.isRunning(), "Should be running after start");

    agent.stop();
    expect(!agent.isRunning(), "Should not be running after stop");
  }

  void testProcessBlock() {
    beginTest("Process Block");
    RealtimeAudioEngineAgent agent;
    agent.initialize(44100.0, 512);
    agent.start();

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    // Fill with some data
    buffer.clear();
    for (int i = 0; i < 512; ++i) {
        buffer.setSample(0, i, 1.0f);
        buffer.setSample(1, i, 1.0f);
    }

    agent.processBlock(buffer, midi);

    // Verify metrics updated
    const auto& metrics = agent.getMetrics();
    expectEquals((int64_t)metrics.samplesProcessed.load(), (int64_t)512, "Samples processed should be updated");
  }

  void testAudioPassThrough() {
    beginTest("Audio Pass-through");
    RealtimeAudioEngineAgent engine;
    double sampleRate = 44100.0;
    int bufferSize = 512;
    engine.initialize(sampleRate, bufferSize);
    engine.start();

    juce::AudioBuffer<float> buffer(2, bufferSize);
    juce::MidiBuffer midi;

    // Create a test signal in the input
    for (int ch = 0; ch < 2; ++ch) {
      auto* data = buffer.getWritePointer(ch);
      for (int i = 0; i < bufferSize; ++i) {
        data[i] = 1.0f; // DC offset for simplicity
      }
    }

    // Process
    engine.processBlock(buffer, midi);

    // Verify output matches input (pass-through)
    float magnitude = buffer.getMagnitude(0, bufferSize);
    expect(magnitude > 0.0f, "Output should not be silent");

    for (int ch = 0; ch < 2; ++ch) {
        float chMag = buffer.getMagnitude(ch, 0, bufferSize);
        expectEquals(chMag, 1.0f);
    }
  }

  void testStopSynchronization() {
    beginTest("Stop Synchronization");

    RealtimeAudioEngineAgent agent;
    agent.initialize(44100.0, 512);
    agent.start();

    std::atomic<bool> threadShouldRun{true};
    std::atomic<bool> threadFinished{false};

    std::thread audioThread([&]() {
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        while (threadShouldRun) {
            agent.processBlock(buffer, midi);
            std::this_thread::yield();
        }
        threadFinished = true;
    });

    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Call stop while the thread is hammering processBlock
    agent.stop();

    expect(!agent.isRunning(), "Agent should be stopped");

    // Stop our fake audio thread
    threadShouldRun = false;
    if (audioThread.joinable())
        audioThread.join();

    expect(threadFinished, "Audio thread finished");

    // Verify no weird state
    expect(!agent.isRunning());
  }
};

static RealtimeAudioEngineAgentTest realtimeAudioEngineAgentTest;

} // namespace tests
} // namespace agents
} // namespace zenith
