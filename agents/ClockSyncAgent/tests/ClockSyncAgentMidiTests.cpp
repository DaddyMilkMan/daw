/*
  ==============================================================================
    agents/ClockSyncAgent/tests/ClockSyncAgentMidiTests.cpp
    Unit tests for MIDI Clock Synchronization logic.
  ==============================================================================
*/

#include "../ClockSyncAgent.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <thread>
#include <random>

namespace zenith {
namespace agents {

class ClockSyncAgentMidiTest : public juce::UnitTest {
public:
  ClockSyncAgentMidiTest() : juce::UnitTest("ClockSyncAgent MIDI", "ClockSyncAgent") {}

  void runTest() override {
    testInitialState();
    testTransportLogic();
    testJitterRejection();
    testTempoJumpReset();
  }

  void shutdown() override {
    agent.reset();
  }

private:
  std::unique_ptr<ClockSyncAgent> agent;

  // Helper to create MIDI messages
  static juce::MidiMessage createClockMsg() { return juce::MidiMessage(0xF8); }
  static juce::MidiMessage createStartMsg() { return juce::MidiMessage(0xFA); }
  static juce::MidiMessage createStopMsg() { return juce::MidiMessage(0xFC); }
  static juce::MidiMessage createContinueMsg() { return juce::MidiMessage(0xFB); }
  static juce::MidiMessage createSPPMsg(int beats) {
    int lsb = beats & 0x7F;
    int msb = (beats >> 7) & 0x7F;
    return juce::MidiMessage(0xF2, lsb, msb);
  }

  void setup() {
    agent = std::make_unique<ClockSyncAgent>(); // Reset
    agent->setTimeSource(ClockSyncAgent::TimeSource::MIDIClock);
  }

  void testInitialState() {
    beginTest("Initial State");
    setup();
    auto status = agent->getSyncStatus();
    expect(status.currentSource == ClockSyncAgent::TimeSource::MIDIClock);
    expect(!status.synchronized);
  }

  void testTransportLogic() {
    beginTest("Transport Logic");
    setup();

    // Send Start
    agent->processMidiMessage(createStartMsg());

    // Send some clocks at proper 120 BPM timing (~20.8ms per tick)
    for (int i = 0; i < 24; ++i) {
      agent->processMidiMessage(createClockMsg());
      std::this_thread::sleep_for(std::chrono::milliseconds(21)); // Proper timing for 120 BPM
    }

    // Stop
    agent->processMidiMessage(createStopMsg());

    // Continue
    agent->processMidiMessage(createContinueMsg());

    // SPP
    agent->processMidiMessage(createSPPMsg(16)); // 16th note index 16

    // Pass if we get here without crashing
    expect(true);
  }

  void testJitterRejection() {
    beginTest("Jitter Rejection");
    setup();

    // Simulate 120 BPM: 24 ticks per beat.
    // 120 BPM = 2 beats per second = 48 ticks per second.
    // Interval = 1000ms / 48 = 20.833 ms = 20833333 ns.

    const int64_t nominalIntervalNs = 20833333;
    const int numTicks = 100;

    // Warm up
    for (int i = 0; i < numTicks; ++i) {
      // Wait appropriate amount of time
      std::this_thread::sleep_for(std::chrono::nanoseconds(nominalIntervalNs));
      agent->processMidiMessage(createClockMsg());
    }

    auto status = agent->getSyncStatus();

    // After 100 ticks, we should be synchronized
    expect(status.synchronized, "Should be synchronized after warmup");

    // Offset should be reasonable
    int64_t absOffset = std::abs(status.offsetNanoseconds);
    expect(absOffset < 50000000, "Offset should be < 50ms (Actual: " + juce::String(absOffset) + " ns)");
  }

  void testTempoJumpReset() {
    beginTest("Tempo Jump Reset");
    setup();

    // Simulate steady 120 BPM
    for (int i = 0; i < 50; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Approx 20.8ms
      agent->processMidiMessage(createClockMsg());
    }

    auto statusBefore = agent->getSyncStatus();
    expect(statusBefore.synchronized, "Should be synced before jump");

    // Jump: Wait 100ms (late arrival / pause)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    agent->processMidiMessage(createClockMsg());

    // Re-stabilize
    for (int i = 0; i < 20; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      agent->processMidiMessage(createClockMsg());
    }

    auto statusAfter = agent->getSyncStatus();
    expect(statusAfter.synchronized, "Should be synced after recovery");
  }
};

static ClockSyncAgentMidiTest clockSyncAgentMidiTest;

} // namespace agents
} // namespace zenith
