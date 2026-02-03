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

class RealtimeAudioEngineAgentTest : public juce::UnitTest {
public:
  RealtimeAudioEngineAgentTest() : juce::UnitTest("RealtimeAudioEngineAgent Tests", "RealtimeAudioEngineAgent") {}

  void runTest() override {
    testInitialization();
    testStartStop();
    testProcessBlock();
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

    // Currently processBlock clears the buffer (placeholder behavior)
    // Verify silence
    expectEquals(buffer.getMagnitude(0, 512), 0.0f, "Buffer should be cleared by processBlock");

    // Verify metrics updated
    const auto& metrics = agent.getMetrics();
    expectEquals((int64_t)metrics.samplesProcessed, (int64_t)512, "Samples processed should be updated");
  }

  void testStopSynchronization() {
    beginTest("Stop Synchronization");

    // This test attempts to verify that stop() waits for processBlock to finish.
    // However, without modifying the agent to artificially delay processBlock,
    // we can only verify that stop() eventually returns and the state is consistent.
    // We will simulate a high-load scenario.

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

} // namespace agents
} // namespace zenith
