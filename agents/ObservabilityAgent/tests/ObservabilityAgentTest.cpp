/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTest.cpp
    Unit tests for ObservabilityAgent.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "../ObservabilityAgent.h"

namespace zenith {
namespace agents {

class ObservabilityAgentTest : public juce::UnitTest {
public:
  ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent Tests", "ObservabilityAgent") {}

  void runTest() override {
    testMetricsCollection();
    testAggregation();
    testPeriodicExport();
  }

private:
  void testAggregation() {
    beginTest("Metric Aggregation");

    ObservabilityAgent agent;
    agent.setEnabled(true);

    // 1. Counters
    agent.recordCounter("request_count", 1.0);
    agent.recordCounter("request_count", 2.0);

    // 2. Gauges
    agent.recordGauge("memory_usage", 100.0);
    agent.recordGauge("memory_usage", 200.0); // Should overwrite

    // 3. Timers
    uint64_t start = agent.startTimer();
    // We expect 2 timer events
    agent.endTimer("process_time", start);
    agent.endTimer("process_time", start);

    // Trigger processing (drains buffer)
    auto metrics = agent.getMetrics();

    // Verify
    bool foundCounter = false;
    bool foundGauge = false;
    bool foundTimerCount = false;
    bool foundTimerSum = false;

    for (const auto& m : metrics) {
        if (m.name == "request_count") {
            expectEquals(m.value, 3.0, "Counter aggregation failed");
            foundCounter = true;
        } else if (m.name == "memory_usage") {
            expectEquals(m.value, 200.0, "Gauge aggregation failed");
            foundGauge = true;
        } else if (m.name == "process_time_count") {
            expectEquals(m.value, 2.0, "Timer count aggregation failed");
            foundTimerCount = true;
        } else if (m.name == "process_time_sum") {
            expect(m.value >= 0.0, "Timer sum invalid");
            foundTimerSum = true;
        }
    }

    expect(foundCounter, "Counter metric missing");
    expect(foundGauge, "Gauge metric missing");
    expect(foundTimerCount, "Timer count metric missing");
    expect(foundTimerSum, "Timer sum metric missing");
  }

  void testMetricsCollection() {
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

  void testPeriodicExport() {
    beginTest("Periodic Export");

    ObservabilityAgent agent;

    // Initial state
    expectEquals(agent.getExportCount(), (uint64_t)0);

    // Set interval to 50ms
    agent.setExportInterval(std::chrono::milliseconds(50));

    // Wait for at least one export (allow 150ms to be safe)
    // We must pump the message loop to allow the Timer to fire

    // Ensure message manager is initialized
    if (auto* mm = juce::MessageManager::getInstance()) {
        juce::Timer::callAfterDelay(150, [mm] { mm->stopDispatchLoop(); });
        mm->runDispatchLoop();
    } else {
        // Fallback if no message manager (shouldn't happen with correct runner)
        // But for Timer to work, MessageManager MUST be present.
        expect(false, "MessageManager not initialized, Timer cannot run");
    }

    // Check that exports happened
    expect(agent.getExportCount() > 0, "Metrics should have been exported at least once");

    // Stop export
    agent.setExportInterval(std::chrono::milliseconds(0));
    uint64_t countAfterStop = agent.getExportCount();

    // Wait again
    if (auto* mm = juce::MessageManager::getInstance()) {
        juce::Timer::callAfterDelay(150, [mm] { mm->stopDispatchLoop(); });
        mm->runDispatchLoop();
    }

    expectEquals(agent.getExportCount(), countAfterStop, "Metrics should not be exported after stopping");
  }
};

static ObservabilityAgentTest observabilityAgentTest;

} // namespace agents
} // namespace zenith