/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTests.cpp
    Unit tests for ObservabilityAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../ObservabilityAgent.h"

class ObservabilityAgentTest : public juce::UnitTest
{
public:
    ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent") {}

    void runTest() override
    {
        beginTest("Counter Recording");
        {
            zenith::agents::ObservabilityAgent agent;
            agent.recordCounter("test_counter", 1.0);

            auto metrics = agent.getMetrics();
            expect(metrics.size() >= 1, "Should have at least one metric");

            bool found = false;
            for (const auto& m : metrics)
            {
                if (m.name == "test_counter" && m.value == 1.0)
                {
                    found = true;
                    break;
                }
            }
            expect(found, "Counter metric not found or incorrect value");
        }

        beginTest("Gauge Recording");
        {
            zenith::agents::ObservabilityAgent agent;
            agent.recordGauge("test_gauge", 42.0);

            auto metrics = agent.getMetrics();
            bool found = false;
            for (const auto& m : metrics)
            {
                if (m.name == "test_gauge" && m.value == 42.0)
                {
                    found = true;
                    break;
                }
            }
            expect(found, "Gauge metric not found");
        }

        beginTest("Timer Recording");
        {
            zenith::agents::ObservabilityAgent agent;
            auto start = agent.startTimer();
            juce::Thread::sleep(10);
            agent.endTimer("test_timer", start);

            auto metrics = agent.getMetrics();
            bool found = false;
            for (const auto& m : metrics)
            {
                if (m.name == "test_timer")
                {
                    found = true;
                    expect(m.value > 0.0, "Timer duration should be positive");
                    break;
                }
            }
            expect(found, "Timer metric not found");
        }

        beginTest("Buffer Overflow Handling");
        {
            zenith::agents::ObservabilityAgent agent;
            // Fill buffer beyond 4096 capacity
            for (int i = 0; i < 5000; ++i)
            {
                agent.recordCounter("overflow_test", 1.0);
            }

            auto metrics = agent.getMetrics();
            expect(metrics.size() <= 4096, "Should not exceed buffer size");
            // We expect at least some metrics
            expect(metrics.size() > 0, "Should have captured some metrics");
        }
    }
};

static ObservabilityAgentTest observabilityAgentTest;

int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
