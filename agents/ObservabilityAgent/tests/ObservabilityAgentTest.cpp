/*
  ==============================================================================
    agents/ObservabilityAgent/tests/ObservabilityAgentTest.cpp
    Tests for ObservabilityAgent and Lock-Free Ring Buffer.
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../ObservabilityAgent.h"

namespace zenith {
namespace tests {

class ObservabilityAgentTest : public juce::UnitTest {
public:
  ObservabilityAgentTest() : juce::UnitTest("ObservabilityAgent Tests", "Observability") {}

  void runTest() override {
    beginTest("MPSC Ring Buffer Basic Operations");
    {
      using Buffer = agents::ObservabilityAgent::MpscRingBuffer;
      using Event = agents::ObservabilityAgent::RingBufferEvent;
      using Type = agents::ObservabilityAgent::MetricType;

      Buffer buffer(16); // Small buffer for testing

      // Test Write
      bool result = buffer.write(Type::Counter, "test_counter", 1.0, 100);
      expect(result, "Write should succeed on empty buffer");

      // Test Read
      Event event;
      result = buffer.read(event);
      expect(result, "Read should succeed");
      expect(event.type == Type::Counter);
      expectEquals(event.value, 1.0);
      expectEquals(std::string(event.name), std::string("test_counter"));
      expectEquals(static_cast<int>(event.timestamp), 100);

      // Test Empty Read
      result = buffer.read(event);
      expect(!result, "Read should fail on empty buffer");
    }

    beginTest("MPSC Ring Buffer Overflow");
    {
      using Buffer = agents::ObservabilityAgent::MpscRingBuffer;
      using Type = agents::ObservabilityAgent::MetricType;

      Buffer buffer(4); // Very small buffer

      // Fill buffer
      for (int i = 0; i < 4; ++i) {
        bool res = buffer.write(Type::Counter, "fill", static_cast<double>(i), 0);
        expect(res, "Write should succeed while filling");
      }

      // Try to overfill
      bool overflowRes = buffer.write(Type::Counter, "overflow", 99.0, 0);
      expect(!overflowRes, "Write should fail (drop) when full");

      // Read one
      agents::ObservabilityAgent::RingBufferEvent event;
      bool readRes = buffer.read(event);
      expect(readRes, "Should be able to read after full");
      expectEquals(event.value, 0.0);

      // Write again
      bool writeAgain = buffer.write(Type::Counter, "new", 100.0, 0);
      expect(writeAgain, "Should be able to write after reading one");
    }

    beginTest("ObservabilityAgent Public API");
    {
      agents::ObservabilityAgent agent;

      // Record some metrics
      agent.recordCounter("counter1", 10.0);
      agent.recordGauge("gauge1", 5.5);

      uint64_t start = agent.startTimer();
      juce::Thread::sleep(1); // Ensure some time passes
      agent.endTimer("timer1", start);

      // Retrieve metrics
      auto metrics = agent.getMetrics();

      expect(metrics.size() == 3, "Should have 3 metrics recorded");

      // Verify order and content
      if (metrics.size() >= 3) {
        expectEquals(metrics[0].name, std::string("counter1"));
        expectEquals(metrics[0].value, 10.0);
        expect(metrics[0].type == agents::ObservabilityAgent::MetricType::Counter);

        expectEquals(metrics[1].name, std::string("gauge1"));
        expectEquals(metrics[1].value, 5.5);
        expect(metrics[1].type == agents::ObservabilityAgent::MetricType::Gauge);

        expectEquals(metrics[2].name, std::string("timer1"));
        expect(metrics[2].value > 0.0, "Timer duration should be positive");
        expect(metrics[2].type == agents::ObservabilityAgent::MetricType::Timer);
      }

      // Verify drain
      auto metricsEmpty = agent.getMetrics();
      expect(metricsEmpty.empty(), "Metrics should be drained after getMetrics()");
    }

    beginTest("Multi-Threaded Writer Stress Test");
    {
      using Buffer = agents::ObservabilityAgent::MpscRingBuffer;
      using Type = agents::ObservabilityAgent::MetricType;

      Buffer buffer(4096);
      std::atomic<bool> stop{false};
      std::atomic<int> writeCount{0};

      // Start 4 writer threads
      std::vector<std::thread> writers;
      for (int i = 0; i < 4; ++i) {
        writers.emplace_back([&]() {
          while (!stop) {
            if (buffer.write(Type::Counter, "stress", 1.0, 0)) {
              writeCount++;
            }
            juce::Thread::yield();
          }
        });
      }

      // Read for a bit
      int readCount = 0;
      auto start = juce::Time::getMillisecondCounter();
      while (juce::Time::getMillisecondCounter() - start < 100) {
        agents::ObservabilityAgent::RingBufferEvent evt;
        if (buffer.read(evt)) {
          readCount++;
        } else {
          juce::Thread::yield();
        }
      }

      stop = true;
      for (auto& t : writers) t.join();

      // Drain remaining
      agents::ObservabilityAgent::RingBufferEvent evt;
      while (buffer.read(evt)) {
        readCount++;
      }

      // Since we drop on full, readCount might be less than writeCount if the reader was slow.
      // But we should verify we didn't crash and readCount > 0.
      expect(readCount > 0, "Should have read some events");
      expectEquals(readCount, writeCount.load());
    }
  }
};

static ObservabilityAgentTest observabilityAgentTest;

} // namespace tests
} // namespace zenith
