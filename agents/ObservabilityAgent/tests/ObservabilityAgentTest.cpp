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
    ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgentTest") {}

    void runTest() override
    {
        beginTest("LockFreeRingBuffer_WritesAndReads");

        zenith::agents::ObservabilityAgent agent;

        // Simulate Audio Thread Writes
        agent.recordGauge("audio_callback_duration", 0.05);
        agent.recordCounter("buffer_underruns", 1);

        // Simulate Message Thread Read
        auto metrics = agent.getMetrics();

        expectEquals((int)metrics.size(), 2);

        if (metrics.size() >= 2) {
            expect(metrics[0].name == "audio_callback_duration");
            expectEquals(metrics[0].value, 0.05);
            expect(metrics[0].type == zenith::agents::ObservabilityAgent::Metric::Type::Gauge);

            expect(metrics[1].name == "buffer_underruns");
            expectEquals(metrics[1].value, 1.0);
            expect(metrics[1].type == zenith::agents::ObservabilityAgent::Metric::Type::Counter);
        }

        // Verify Buffer is drained
        auto emptyMetrics = agent.getMetrics();
        expectEquals((int)emptyMetrics.size(), 0);
    }
};

static ObservabilityAgentTest observabilityAgentTest;

// Main entry point for standalone test app
int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return 0;
}
