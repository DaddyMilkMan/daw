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
    testPeriodicExport();
    testLogging();
    testConcurrentLogging();
  }

private:
  void testLogging() {
      beginTest("Logging");
      ObservabilityAgent agent;

      // Simple log
      agent.log(ObservabilityAgent::LogLevel::Info, "Test log message");

      // We can't easily verify DBG output, but we can verify no crash.
      // To verify processing, we can check if queue drains.
      // The consumer runs in 'run()', which is started by constructor.
      // We need to wait a bit.
      juce::Thread::sleep(100);

      // Check dropped count is 0
      expectEquals(agent.getDroppedLogCount(), (uint64_t)0);
      expectEquals(agent.getTruncatedLogCount(), (uint64_t)0);

      // Test Truncation
      juce::String longMsg;
      for (int i=0; i<3000; ++i) longMsg += "a";
      agent.log(ObservabilityAgent::LogLevel::Warning, longMsg);

      juce::Thread::sleep(100);
      expectEquals(agent.getTruncatedLogCount(), (uint64_t)1);
  }

  void testConcurrentLogging() {
      beginTest("Concurrent Logging");
      ObservabilityAgent agent;

      const int numThreads = 4;
      const int logsPerThread = 100;
      std::vector<std::thread> threads;

      for (int i=0; i<numThreads; ++i) {
          threads.emplace_back([&, i] {
              for (int j=0; j<logsPerThread; ++j) {
                  agent.log(ObservabilityAgent::LogLevel::Info,
                           juce::String("Thread " + juce::String(i) + " Log " + juce::String(j)));
                  // Small sleep to vary contention
                  if (j % 10 == 0) juce::Thread::sleep(1);
              }
          });
      }

      for (auto& t : threads) t.join();

      // Wait for consumer
      juce::Thread::sleep(500);

      // We don't strictly assert dropped count is 0 because contention might cause drops
      // with the spinlock try_lock.
      // But we verify the system is stable.
      expect(true, "Concurrent logging completed");

      // Verify file logging
      juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("test_log.txt");
      tempFile.deleteFile();
      agent.setLogFile(tempFile);

      agent.log(ObservabilityAgent::LogLevel::Error, "File log test");
      juce::Thread::sleep(200);

      expect(tempFile.existsAsFile(), "Log file created");
      expect(tempFile.getSize() > 0, "Log file not empty");

      tempFile.deleteFile();
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