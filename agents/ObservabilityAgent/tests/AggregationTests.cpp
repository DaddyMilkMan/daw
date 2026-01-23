/*
  ==============================================================================
    agents/ObservabilityAgent/tests/AggregationTests.cpp
    Tests for ObservabilityAgent consumer and aggregation logic.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../ObservabilityAgent.h"

namespace zenith {
namespace agents {

class AggregationTests : public juce::UnitTest {
public:
    AggregationTests() : juce::UnitTest("ObservabilityAgent Aggregation Tests", "ObservabilityAgent") {}

    void runTest() override {
        testCounterAggregation();
        testGaugeAggregation();
        testMixedAggregation();
    }

private:
    void testCounterAggregation() {
        beginTest("Counter Aggregation");

        ObservabilityAgent agent;
        const char* name = "test_counter";

        // Record 3 increments
        agent.recordCounter(name, 1.0);
        agent.recordCounter(name, 2.0);
        agent.recordCounter(name, 0.5);

        // Get metrics (triggers processPendingMetrics)
        auto metrics = agent.getMetrics();

        // Find the metric
        bool found = false;
        for (const auto& m : metrics) {
            if (m.name == name) {
                found = true;
                expectEquals(m.value, 3.5, "Counter value should be sum of increments");
                expect(m.type == ObservabilityAgent::MetricType::Counter, "Type should be Counter");
            }
        }
        expect(found, "Metric not found in aggregation");

        // Verify buffer is drained (cannot easily access private ringBufferFifo_ here without friendship,
        // but getting metrics again should result in same value if nothing added)
        auto metrics2 = agent.getMetrics();
        for (const auto& m : metrics2) {
            if (m.name == name) {
                expectEquals(m.value, 3.5, "Counter value should persist");
            }
        }
    }

    void testGaugeAggregation() {
        beginTest("Gauge Aggregation");

        ObservabilityAgent agent;
        const char* name = "test_gauge";

        agent.recordGauge(name, 10.0);
        agent.recordGauge(name, 20.0); // Should overwrite

        auto metrics = agent.getMetrics();

        bool found = false;
        for (const auto& m : metrics) {
            if (m.name == name) {
                found = true;
                expectEquals(m.value, 20.0, "Gauge value should be the last one recorded");
                expect(m.type == ObservabilityAgent::MetricType::Gauge, "Type should be Gauge");
            }
        }
        expect(found, "Gauge metric not found");
    }

    void testMixedAggregation() {
        beginTest("Mixed Aggregation");

        ObservabilityAgent agent;

        agent.recordCounter("c1", 1.0);
        agent.recordGauge("g1", 5.0);
        agent.recordCounter("c1", 1.0);
        agent.recordGauge("g1", 10.0);

        auto metrics = agent.getMetrics();

        double c1_val = 0;
        double g1_val = 0;
        int foundCount = 0;

        for (const auto& m : metrics) {
            if (m.name == "c1") { c1_val = m.value; foundCount++; }
            if (m.name == "g1") { g1_val = m.value; foundCount++; }
        }

        expectEquals(foundCount, 2, "Should find both metrics");
        expectEquals(c1_val, 2.0, "Counter should be 2.0");
        expectEquals(g1_val, 10.0, "Gauge should be 10.0");
    }
};

static AggregationTests aggregationTests;

} // namespace agents
} // namespace zenith
