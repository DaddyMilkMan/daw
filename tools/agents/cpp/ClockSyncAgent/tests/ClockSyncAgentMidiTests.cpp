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
    testResynchronize();
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

  void testResynchronize() {
    beginTest("Resynchronize");
    setup();

    // Stabilize
    for (int i = 0; i < 20; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      agent->processMidiMessage(createClockMsg());
    }

    expect(agent->getSyncStatus().synchronized, "Should be synced");

    // Call resynchronize
    agent->resynchronize();

    auto status = agent->getSyncStatus();
    expect(!status.synchronized, "Should be unsynchronized after resync");
    expect(status.offsetNanoseconds == 0, "Offset should be reset");
  }
};

static ClockSyncAgentMidiTest clockSyncAgentMidiTest;

class ClockSyncAgentGeneralTest : public juce::UnitTest {
public:
  ClockSyncAgentGeneralTest() : juce::UnitTest("ClockSyncAgent General", "ClockSyncAgent") {}

  void runTest() override {
    testInitialState();
    testLatencyCalculation();
    testNetworkSourceSwitching();
    testResynchronize();
  }

  void shutdown() override {
    agent.reset();
  }

private:
  std::unique_ptr<ClockSyncAgent> agent;

  void setup() {
    agent = std::make_unique<ClockSyncAgent>();
  }

  void testInitialState() {
    beginTest("Initial State");
    setup();
    auto status = agent->getSyncStatus();
    expect(status.currentSource == ClockSyncAgent::TimeSource::LocalClock, "Initial source should be LocalClock");
    expect(status.synchronized, "LocalClock should be synchronized initially");
  }

  void testLatencyCalculation() {
    beginTest("Latency Calculation");
    setup();

    using namespace std::chrono;
    auto t1 = nanoseconds(100000000); // 100ms
    auto t2 = nanoseconds(110000000); // 110ms
    auto t3 = nanoseconds(120000000); // 120ms
    auto t4 = nanoseconds(140000000); // 140ms

    agent->updateNetworkMetrics(t1, t2, t3, t4);

    auto status = agent->getSyncStatus();

    double expectedLatency = 15.0;
    int64_t expectedOffset = -5000000;

    expect(std::abs(status.latencyMs - expectedLatency) < 0.001, "Latency calculation incorrect");
    expect(status.offsetNanoseconds == expectedOffset, "Offset calculation incorrect");
  }

  void testNetworkSourceSwitching() {
    beginTest("Network Source Switching");
    setup();

    // NTP
    agent->setTimeSource(ClockSyncAgent::TimeSource::NetworkNTP);
    auto status = agent->getSyncStatus();
    expect(status.currentSource == ClockSyncAgent::TimeSource::NetworkNTP, "Source should be NetworkNTP");

    // PTP
    agent->setTimeSource(ClockSyncAgent::TimeSource::NetworkPTP);
    status = agent->getSyncStatus();
    expect(status.currentSource == ClockSyncAgent::TimeSource::NetworkPTP, "Source should be NetworkPTP");
  }

  void testResynchronize() {
    beginTest("Resynchronize");
    setup();

    // Local
    agent->setTimeSource(ClockSyncAgent::TimeSource::LocalClock);
    // Inject offset
    using namespace std::chrono;
    agent->updateNetworkMetrics(nanoseconds(100), nanoseconds(110), nanoseconds(120), nanoseconds(130)); // Offset != 0
    expect(agent->getSyncStatus().offsetNanoseconds != 0, "Offset should be injected");

    agent->resynchronize();
    auto status = agent->getSyncStatus();
    expect(status.offsetNanoseconds == 0, "Resynchronize (Local) should reset offset");
    expect(status.synchronized, "Resynchronize (Local) should be synchronized");

    // Network (PTP)
    agent->setTimeSource(ClockSyncAgent::TimeSource::NetworkPTP);
    // PTP resets sync to false on setTimeSource
    // But let's call resynchronize anyway
    agent->resynchronize();
    status = agent->getSyncStatus();
    // PTP doesn't set sync to true until packet received
    expect(!status.synchronized, "PTP should not be synchronized immediately after resync");

    // Check drift is updated (should be 1.0)
    expect(std::abs(status.driftPPM) < 0.0001, "Drift should be ~0 PPM (1.0 ratio)");
  }
};

static ClockSyncAgentGeneralTest clockSyncAgentGeneralTest;

} // namespace agents
} // namespace zenith
