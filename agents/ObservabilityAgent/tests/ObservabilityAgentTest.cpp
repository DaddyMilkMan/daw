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
    testConcurrentAccess();
  }

private:
  void testConcurrentAccess() {
    beginTest("Concurrent Access");

    ObservabilityAgent agent;
    const int numThreads = 4;
    const int numOpsPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> startFlag{0};

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&agent, &startFlag, numOpsPerThread] {
            while (startFlag.load() == 0) {
                std::this_thread::yield();
            }
            for (int j = 0; j < numOpsPerThread; ++j) {
                agent.recordCounter("concurrent_counter", 1.0);
            }
        });
    }

    startFlag.store(1);

    for (auto& t : threads) {
        t.join();
    }

    // Drain the buffer using the safe test API
    int totalEvents = agent.drainRingBufferForTesting();

    expectEquals(totalEvents, numThreads * numOpsPerThread, "Should capture all concurrent events");
  }

  void testMetricsCollection() {
    beginTest("Ring Buffer Writes with Spinlock");

    ObservabilityAgent agent;

    // Test Gauge
    const char* gaugeName = "cpu_usage";
    double gaugeValue = 42.5;
    agent.recordGauge(gaugeName, gaugeValue);

    // Test Counter
    const char* counterName = "buffer_underruns";
    double counterValue = 1.0;
    agent.recordCounter(counterName, counterValue);

    // Access via const test API
    const auto& fifo = agent.getRingBufferFifoForTesting();
    const auto& ringData = agent.getRingBufferDataForTesting();

    // We expect 2 events
    expect(fifo.getNumReady() >= 2, "Should have at least 2 events in buffer");

    // Read the events (indices based on FIFO state)
    // Note: We can't directly manipulate the FIFO, so we read based on expected order
    // For more robust testing, we'd use drainRingBufferForTesting()
    
    // For now, just verify the FIFO has data and check the counter was incremented
    auto numReady = fifo.getNumReady();
    expect(numReady >= 2, "Should have 2 events queued");

    // Drain and verify we got 2 events
    int drained = agent.drainRingBufferForTesting(2);
    expectEquals(drained, 2, "Should drain exactly 2 events");

    // Verify Timer
    beginTest("Timer Recording");
    uint64_t start = agent.startTimer();
    juce::Thread::sleep(10); // Small sleep
    const char* timerName = "process_block";
    agent.endTimer(timerName, start);

    const auto& fifo2 = agent.getRingBufferFifoForTesting();
    expect(fifo2.getNumReady() >= 1, "Should have 1 timer event");
    
    int timerDrained = agent.drainRingBufferForTesting(1);
    expectEquals(timerDrained, 1, "Should drain exactly 1 timer event");
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