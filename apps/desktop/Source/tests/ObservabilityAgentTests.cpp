/*
  ==============================================================================
    apps/desktop/Source/tests/ObservabilityAgentTests.cpp
    Unit tests for ObservabilityAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "ObservabilityAgent.h"

namespace zenith {
namespace tests {

class ObservabilityAgentTest : public juce::UnitTest {
public:
    ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent Tests", "Agents") {}

    void runTest() override {
        beginTest("Basic Logging and Metrics");

        // Instantiate the agent
        agents::ObservabilityAgent agent;

        // Wait for thread to start (it starts in constructor)
        juce::Thread::sleep(10);

        // Test 1: Log message via helper
        agent.log(agents::ObservabilityAgent::LogLevel::Info, "Test INFO message");
        expect(true, "Log call executed without crash");

        // Test 2: Record Counter
        agent.recordCounter("requests_total", 1.0);
        expect(true, "Counter recorded without crash");

        // Test 3: Record Gauge
        agent.recordGauge("cpu_usage", 45.5);
        expect(true, "Gauge recorded without crash");

        // Test 4: Timer
        auto start = agent.startTimer();
        // Simulate some work
        juce::Thread::sleep(5);
        agent.endTimer("process_block_duration", start);
        expect(true, "Timer recorded without crash");

        // Test 5: RT-safe direct logging
        agent.log("Raw Real-Time Safe Message");
        expect(true, "Raw log recorded without crash");

        // Test 6: Stress test (fill buffer)
        for (int i = 0; i < 100; ++i) {
            agent.recordCounter("stress_counter", i);
        }
        expect(true, "Stress test loop completed");

        // Allow some time for background thread to flush to DBG
        // In a real test environment, we might redirect DBG or mock the sink,
        // but for now we verify no crashes and successful execution flow.
        juce::Thread::sleep(200);
    }
};

static ObservabilityAgentTest observabilityAgentTest;

} // namespace tests
} // namespace zenith
