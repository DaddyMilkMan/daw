/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTest.cpp
    Unit tests for ObservabilityAgent.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../ObservabilityAgent.h"

namespace zenith {
namespace agents {

class ObservabilityAgentTest : public juce::UnitTest {
public:
  ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent") {}

  void runTest() override {
    beginTest("Lock-free Ring Buffer Writes");

    ObservabilityAgent agent;

    // Test Gauge
    const char* gaugeName = "cpu_usage";
    double gaugeValue = 42.5;
    agent.recordGauge(gaugeName, gaugeValue);

    // Test Counter
    const char* counterName = "buffer_underruns";
    double counterValue = 1.0;
    agent.recordCounter(counterName, counterValue);

    // Simulate Consumer
    // We expect 2 events
    int s1, s2, num1, num2;
    agent.ringBufferFifo_.prepareToRead(2, s1, num1, s2, num2);

    expect(num1 + num2 == 2, "Should have 2 events in buffer");

    if (num1 > 0) {
      // First event: Gauge
      const auto& event1 = agent.ringBufferData_[s1];
      expect(event1.type == ObservabilityAgent::MetricType::Gauge, "First event should be Gauge");
      expect(event1.name == gaugeName, "Gauge name should match");
      expectEquals(event1.value, gaugeValue, "Gauge value should match");

      // Second event: Counter
      // Calculate index for second event (handle wrapping if ring buffer was small, but it's 4096)
      int idx2 = (s1 + 1) % agent.kRingBufferSize;
      const auto& event2 = agent.ringBufferData_[idx2];

      expect(event2.type == ObservabilityAgent::MetricType::Counter, "Second event should be Counter");
      expect(event2.name == counterName, "Counter name should match");
      expectEquals(event2.value, counterValue, "Counter value should match");
    }

    agent.ringBufferFifo_.finishedRead(2);

    // Verify Timer
    beginTest("Timer Recording");
    uint64_t start = agent.startTimer();
    juce::Thread::sleep(10); // Small sleep
    const char* timerName = "process_block";
    agent.endTimer(timerName, start);

    agent.ringBufferFifo_.prepareToRead(1, s1, num1, s2, num2);
    expect(num1 + num2 == 1, "Should have 1 timer event");

    if (num1 > 0) {
      const auto& event = agent.ringBufferData_[s1];
      expect(event.type == ObservabilityAgent::MetricType::Timer, "Event should be Timer");
      expect(event.name == timerName, "Timer name should match");
      expectGreaterThan(event.value, 0.0, "Timer duration should be positive");
    }
    agent.ringBufferFifo_.finishedRead(1);
  }
};

static ObservabilityAgentTest observabilityAgentTest;

} // namespace agents
} // namespace zenith
