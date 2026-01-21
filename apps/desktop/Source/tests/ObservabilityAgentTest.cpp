/*
  ==============================================================================
    ObservabilityAgentTest.cpp
    Unit tests for ObservabilityAgent
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../../../../agents/ObservabilityAgent/ObservabilityAgent.h"

using namespace zenith::agents;

class ObservabilityAgentTest : public juce::UnitTest {
public:
    ObservabilityAgentTest() : UnitTest("ObservabilityAgent Tests", "ObservabilityAgent") {}

    void runTest() override {
        beginTest("Basic Metrics Recording");
        testBasicRecording();

        beginTest("Buffer Wrapping");
        testBufferWrapping();

        beginTest("Aggregation");
        testAggregation();

        beginTest("Clear Metrics");
        testClearMetrics();
    }

private:
    void testBasicRecording() {
        ObservabilityAgent agent;
        agent.setEnabled(true);

        agent.recordCounter("test.counter", 1.0);
        agent.recordGauge("test.gauge", 42.0);

        uint64_t start = agent.startTimer();
        juce::Thread::sleep(10);
        agent.endTimer("test.timer", start);

        auto metrics = agent.getMetrics();
        expectEquals((int)metrics.size(), 3);

        bool foundCounter = false;
        bool foundGauge = false;
        bool foundTimer = false;

        for (const auto& m : metrics) {
            if (m.name == "test.counter") {
                expectEquals(m.value, 1.0);
                expect(m.type == ObservabilityAgent::MetricType::Counter);
                foundCounter = true;
            } else if (m.name == "test.gauge") {
                expectEquals(m.value, 42.0);
                expect(m.type == ObservabilityAgent::MetricType::Gauge);
                foundGauge = true;
            } else if (m.name == "test.timer") {
                expect(m.value >= 0.0);
                expect(m.type == ObservabilityAgent::MetricType::Timer);
                foundTimer = true;
            }
        }

        expect(foundCounter);
        expect(foundGauge);
        expect(foundTimer);
    }

    void testBufferWrapping() {
        ObservabilityAgent agent;
        agent.setEnabled(true);

        // Buffer is 4096. Write 5000 items.
        for (int i = 0; i < 5000; ++i) {
            agent.recordCounter("test.wrap", 1.0);
        }

        auto metrics = agent.getMetrics();
        // Since we aggregate counters, we should get 1 metric.
        // The value should be around 4096 because AbstractFifo doesn't overwrite, it stops accepting writes when full.

        expectEquals((int)metrics.size(), 1);
        if (metrics.size() == 1) {
            expect(metrics[0].value <= 4096.0);
            expect(metrics[0].value > 4000.0); // Should be close to capacity
        }
    }

    void testAggregation() {
        ObservabilityAgent agent;
        agent.setEnabled(true);

        agent.recordCounter("cnt", 1.0);
        agent.recordCounter("cnt", 2.0);
        agent.recordGauge("gauge", 10.0);
        agent.recordGauge("gauge", 20.0); // should be 20

        auto metrics = agent.getMetrics();

        bool foundCnt = false;
        bool foundGauge = false;

        for (const auto& m : metrics) {
            if (m.name == "cnt") {
                expectEquals(m.value, 3.0);
                foundCnt = true;
            } else if (m.name == "gauge") {
                expectEquals(m.value, 20.0);
                foundGauge = true;
            }
        }
        expect(foundCnt);
        expect(foundGauge);
    }

    void testClearMetrics() {
        ObservabilityAgent agent;
        agent.setEnabled(true);
        agent.recordCounter("clear_me", 1.0);

        agent.clearMetrics();

        auto metrics = agent.getMetrics();
        expectEquals((int)metrics.size(), 0);
    }
};

static ObservabilityAgentTest observabilityAgentTest;
