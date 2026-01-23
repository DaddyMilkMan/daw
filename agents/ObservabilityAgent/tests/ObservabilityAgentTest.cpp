/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTest.cpp
    Unit tests for ObservabilityAgent and PrometheusExporter.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "../ObservabilityAgent.h"
#include "../PrometheusExporter.h"

namespace zenith {
namespace agents {

class ObservabilityAgentTest : public juce::UnitTest {
public:
  ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent Tests", "ObservabilityAgent") {}

  void runTest() override {
    testMetricsCollection();
    testPeriodicExport();
  }

private:
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

class ObservabilityAggregationTests : public juce::UnitTest {
public:
    ObservabilityAggregationTests() : juce::UnitTest("ObservabilityAggregationTests", "Observability") {}

    void runTest() override {
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
            }

            // Cleanup
            tempFile.deleteFile();
        }

        beginTest("Timer Aggregation Test");
        {
            ObservabilityAgent agent;
            agent.setEnabled(true);

            // Record a timer
            auto t0 = agent.startTimer();
            juce::Thread::sleep(10); // Sleep 10ms
            agent.endTimer("test_timer", t0);

            // Record another timer
            auto t1 = agent.startTimer();
            juce::Thread::sleep(20); // Sleep 20ms
            agent.endTimer("test_timer", t1);

            // Get metrics (this should drain the buffer and aggregate)
            auto metrics = agent.getMetrics();

            bool foundSum = false;
            bool foundCount = false;
            double sumValue = 0.0;
            double countValue = 0.0;

            for (const auto& m : metrics) {
                if (m.name == "test_timer_sum") {
                    foundSum = true;
                    sumValue = m.value;
                }
                if (m.name == "test_timer_count") {
                    foundCount = true;
                    countValue = m.value;
                }
            }

            expect(foundSum, "Timer sum metric not found");
            expect(foundCount, "Timer count metric not found");
            expectEquals(countValue, 2.0, "Timer count incorrect");
            expect(sumValue > 0.0, "Timer sum value invalid");
        }

        beginTest("Counter Aggregation Test");
        {
            ObservabilityAgent agent;
            agent.setEnabled(true);

            agent.recordCounter("test_counter", 1.0);
            agent.recordCounter("test_counter", 2.5);

            auto metrics = agent.getMetrics();

            bool found = false;
            for (const auto& m : metrics) {
                if (m.name == "test_counter") {
                    found = true;
                    expectEquals(m.value, 3.5, "Counter aggregation incorrect");
                }
            }
            expect(found, "Counter metric not found");
        }
    }
};

static ObservabilityAgentTest observabilityAgentTest;
static PrometheusExporterTests prometheusExporterTests;
static ObservabilityAggregationTests observabilityAggregationTests;

} // namespace agents
} // namespace zenith
