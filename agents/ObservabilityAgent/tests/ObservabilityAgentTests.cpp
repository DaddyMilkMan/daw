/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTests.cpp
    Unit tests for ObservabilityAgent and PrometheusExporter.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../ObservabilityAgent.h"
#include "../PrometheusExporter.h"

namespace zenith {
namespace agents {
namespace test {

class PrometheusExporterTests : public juce::UnitTest {
public:
    PrometheusExporterTests() : juce::UnitTest("PrometheusExporterTests", "Observability") {}

    void runTest() override {
        beginTest("Sanitization");
        {
            // We can't access private static helpers directly, so we test via formatMetric logic or public API if available.
            // Since helpers are private, we test the public formatMetric.
            ObservabilityAgent::Metric m;
            m.name = "my.metric-name with spaces";
            m.type = ObservabilityAgent::MetricType::Counter;
            m.value = 123.45;
            m.timestamp = std::chrono::steady_clock::now();
            m.labels = {{"label.one", "value\"with\"quotes"}, {"label-two", "value\\with\\backslashes"}};

            auto output = PrometheusExporter::formatMetric(m);

            // Expected name: my_metric_name_with_spaces
            expect(output.contains("my_metric_name_with_spaces"), "Name sanitization failed");

            // Expected label 1: label_one="value\"with\"quotes"
            expect(output.contains("label_one=\"value\\\"with\\\"quotes\""), "Label 1 sanitization failed");

            // Expected label 2: label_two="value\\with\\backslashes"
            expect(output.contains("label_two=\"value\\\\with\\\\backslashes\""), "Label 2 sanitization failed");

            // Expected value
            expect(output.contains("123.45"), "Value missing");

            // Expected metadata
            expect(output.contains("# TYPE my_metric_name_with_spaces counter"), "Type metadata missing");
            expect(output.contains("# HELP my_metric_name_with_spaces"), "Help metadata missing");
        }

        beginTest("Atomic File Write");
        {
            PrometheusExporter exporter;
            auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("zenith_test_metrics.prom");

            if (tempFile.exists()) tempFile.deleteFile();

            std::vector<ObservabilityAgent::Metric> metrics;
            ObservabilityAgent::Metric m;
            m.name = "test_metric";
            m.type = ObservabilityAgent::MetricType::Gauge;
            m.value = 42.0;
            metrics.push_back(m);

            auto result = exporter.exportMetrics(metrics, tempFile);

            expect(result.wasOk(), "Export failed: " + result.getErrorMessage());
            expect(tempFile.exists(), "Destination file not created");

            auto content = tempFile.loadFileAsString();
            expect(content.contains("test_metric 42"), "Content missing in file");

            // Cleanup
            tempFile.deleteFile();
        }
    }
};

class ObservabilityAgentTests : public juce::UnitTest {
public:
    ObservabilityAgentTests() : juce::UnitTest("ObservabilityAgentTests", "Observability") {}

    void runTest() override {
        beginTest("Aggregation Logic");
        {
            ObservabilityAgent agent;
            agent.setEnabled(true);

            // 1. Counter Test
            agent.recordCounter("test_counter", 10.0);
            agent.recordCounter("test_counter", 5.0);

            // 2. Gauge Test
            agent.recordGauge("test_gauge", 100.0);
            agent.recordGauge("test_gauge", 200.0); // Should overwrite

            // 3. Timer Test
            auto start = agent.startTimer();
            juce::Thread::sleep(10); // Sleep 10ms to ensure non-zero duration
            agent.endTimer("test_timer", start);

            // Trigger processing via getMetrics
            auto metrics = agent.getMetrics();

            // Verify Counter
            bool foundCounter = false;
            for (const auto& m : metrics) {
                if (m.name == "test_counter") {
                    expectEquals(m.value, 15.0, "Counter aggregation failed");
                    expect(m.type == ObservabilityAgent::MetricType::Counter);
                    foundCounter = true;
                }
            }
            expect(foundCounter, "Counter metric not found");

            // Verify Gauge
            bool foundGauge = false;
            for (const auto& m : metrics) {
                if (m.name == "test_gauge") {
                    expectEquals(m.value, 200.0, "Gauge aggregation failed");
                    expect(m.type == ObservabilityAgent::MetricType::Gauge);
                    foundGauge = true;
                }
            }
            expect(foundGauge, "Gauge metric not found");

            // Verify Timer
            bool foundTimerSum = false;
            bool foundTimerCount = false;
            for (const auto& m : metrics) {
                if (m.name == "test_timer_sum") {
                    expect(m.value > 0.0, "Timer sum should be positive");
                    foundTimerSum = true;
                }
                if (m.name == "test_timer_count") {
                    expectEquals(m.value, 1.0, "Timer count should be 1");
                    foundTimerCount = true;
                }
            }
            expect(foundTimerSum, "Timer sum metric not found");
            expect(foundTimerCount, "Timer count metric not found");
        }

        beginTest("Integration Test");
        {
            ObservabilityAgent agent;
            auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("zenith_agent_integration.prom");

            if (tempFile.exists()) tempFile.deleteFile();

            agent.setMetricsFile(tempFile);
            agent.setEnabled(true);

            // Record some metrics (internal counter)
            agent.recordCounter("ignored_for_now");

            // Start export thread with short interval
            agent.setExportInterval(std::chrono::milliseconds(100));

            // Wait for a bit
            juce::Thread::sleep(300);

            // Stop export
            agent.setExportInterval(std::chrono::milliseconds(0));

            // Verify file exists
            expect(tempFile.exists(), "Metrics file not created by agent");

            if (tempFile.exists()) {
                auto content = tempFile.loadFileAsString();
                expect(content.contains("zenith_metrics_collected_total"), "Internal metric missing");
                // We recorded one counter above, so total should be at least 1 (plus potential others)
                // The implementation increments 'metricsCollected_' on recordCounter.
                // internal 'metricsCollected_' is exported.
                // We called recordCounter once.
                // The sample metric value should be >= 1.
            }

            // Cleanup
            tempFile.deleteFile();
        }
    }
};

static PrometheusExporterTests prometheusExporterTests;
static ObservabilityAgentTests observabilityAgentTests;

} // namespace test
} // namespace agents
} // namespace zenith
