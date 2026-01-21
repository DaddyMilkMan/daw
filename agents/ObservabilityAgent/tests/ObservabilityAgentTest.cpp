/*
  ==============================================================================
    ObservabilityAgentTest.cpp
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../ObservabilityAgent.h"

// Define JUCE compilation info symbols required by juce_core
namespace juce {
    extern const char* const juce_compilationDate = "2024-05-22";
    extern const char* const juce_compilationTime = "12:00:00";
}

class ObservabilityAgentTest : public juce::UnitTest
{
public:
    ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent") {}

    void runTest() override
    {
        beginTest("Lock-free Ring Buffer Operations");

        zenith::agents::ObservabilityAgent agent;

        // 1. Test recordGauge
        agent.recordGauge("cpu_load", 0.75);

        // 2. Test recordCounter
        agent.recordCounter("packets_sent", 1.0);

        // 3. Test Timer
        auto start = agent.startTimer();
        juce::Thread::sleep(10);
        agent.endTimer("process_time", start);

        // 4. Retrieve metrics
        auto metrics = agent.getMetrics();

        expectEquals((int)metrics.size(), 3);

        // Verify order and content
        if (metrics.size() >= 3)
        {
            expect(metrics[0].name == "cpu_load");
            expectEquals(metrics[0].value, 0.75);
            expect(metrics[0].type == zenith::agents::ObservabilityAgent::MetricType::Gauge);

            expect(metrics[1].name == "packets_sent");
            expectEquals(metrics[1].value, 1.0);
            expect(metrics[1].type == zenith::agents::ObservabilityAgent::MetricType::Counter);

            expect(metrics[2].name == "process_time");
            // Timer duration can be small, but should be positive
            expect(metrics[2].value >= 0.0);
            expect(metrics[2].type == zenith::agents::ObservabilityAgent::MetricType::Timer);
        }

        // 5. Test Buffer Wrap / Overflow
        // Write 5000 items (buffer is 4096). Since we drop when full, we expect the first 4096 items.
        agent.clearMetrics();
        for (int i = 0; i < 5000; ++i) {
            agent.recordCounter("stress_test", (double)i);
        }

        auto stressMetrics = agent.getMetrics();
        // AbstractFifo often holds size-1 items to distinguish full/empty
        expectEquals((int)stressMetrics.size(), 4095);

        if (stressMetrics.size() > 0)
            expectEquals(stressMetrics[0].value, 0.0);
        if (stressMetrics.size() >= 4095)
            expectEquals(stressMetrics[4094].value, 4094.0);

        // 6. Test Clear
        agent.clearMetrics();
        expectEquals((int)agent.getMetrics().size(), 0);
    }
};

static ObservabilityAgentTest test;

// Main entry point for standalone test app
int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
