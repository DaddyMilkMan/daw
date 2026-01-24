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
    testLoggingConcurrency();
    testLogTruncation();
  }

private:
  void testLoggingConcurrency() {
    beginTest("Concurrent Logging");

    ObservabilityAgent agent;
    juce::File logFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                         .getChildFile("test_observability_log.txt");
    logFile.deleteFile();
    agent.setLogFile(logFile);

    const int numThreads = 4;
    const int numLogsPerThread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&agent, i, numLogsPerThread] {
            for (int j = 0; j < numLogsPerThread; ++j) {
                juce::String msg = "Thread " + juce::String(i) + " Log " + juce::String(j);
                agent.log(ObservabilityAgent::LogLevel::Info, msg);
                // Small yield to encourage interleaving
                if (j % 10 == 0) std::this_thread::yield();
            }
        });
    }

    for (auto& t : threads) t.join();

    // Wait for consumer to process
    // We poll until we see enough lines or timeout
    int maxRetries = 50; // 5 seconds
    size_t totalProcessed = 0;
    size_t expected = numThreads * numLogsPerThread;

    while (maxRetries-- > 0) {
        juce::Thread::sleep(100);

        juce::StringArray lines;
        logFile.readLines(lines);
        totalProcessed = lines.size() + agent.getDroppedLogCount();

        if (totalProcessed >= expected) break;
    }

    // Read file final state
    juce::StringArray lines;
    logFile.readLines(lines);

    // removeEmptyStrings returns the number of strings removed, but modifies the array in place
    lines.removeEmptyStrings();

    totalProcessed = lines.size() + agent.getDroppedLogCount();

    // Verify
    expect(lines.size() > 0, "Log file should not be empty");

    if (lines.size() > 0) {
        juce::String firstLine = lines[0];
        // Expect ISO8601...
        // e.g. 2023-10-27T... INFO [tid=...] Thread X Log Y
        expect(firstLine.contains("INFO"), "Line should contain level");
        expect(firstLine.contains("tid="), "Line should contain thread ID");
        expect(firstLine.contains("Thread"), "Line should contain message body");
    }

    expectEquals((int)totalProcessed, (int)expected, "Total logs (written + dropped) should match expected");

    logFile.deleteFile();
  }

  void testLogTruncation() {
      beginTest("Log Truncation");
      ObservabilityAgent agent;
      
      // Test Truncation
      juce::String longMsg;
      for (int i=0; i<3000; ++i) longMsg += "a";
      agent.log(ObservabilityAgent::LogLevel::Warning, longMsg);

      // Wait for consumer
      int retries = 10;
      while (retries-- > 0 && agent.getTruncatedLogCount() == 0) {
          juce::Thread::sleep(50);
      }
      
      expectEquals(agent.getTruncatedLogCount(), (uint64_t)1, "Should record truncated log");
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
