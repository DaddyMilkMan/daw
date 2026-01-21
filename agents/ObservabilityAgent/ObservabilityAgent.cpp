/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.cpp
    Lock-free observability and metrics collection implementation.
  ==============================================================================
*/

#include "ObservabilityAgent.h"
#include <map>

namespace zenith {
namespace agents {

//==============================================================================
ObservabilityAgent::ObservabilityAgent() {
  // Initialize metrics collection system
  eventBuffer_.resize(4096);
}

ObservabilityAgent::~ObservabilityAgent() {
  // Flush any pending metrics
}

//==============================================================================
// Metrics Collection (RT-safe)

void ObservabilityAgent::recordCounter(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 + size2 > 0)
  {
      MetricEvent e;
      e.type = MetricEvent::Type::Counter;
      e.name = name;
      e.value = value;
      e.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

      if (size1 > 0)
          eventBuffer_[start1] = e;
      else
          eventBuffer_[start2] = e;

      fifo_.finishedWrite(1);
  }
  
  metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

void ObservabilityAgent::recordGauge(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 + size2 > 0)
  {
      MetricEvent e;
      e.type = MetricEvent::Type::Gauge;
      e.name = name;
      e.value = value;
      e.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

      if (size1 > 0)
          eventBuffer_[start1] = e;
      else
          eventBuffer_[start2] = e;

      fifo_.finishedWrite(1);
  }
  
  metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::startTimer() noexcept {
  // Return high-resolution timestamp
  auto now = std::chrono::steady_clock::now();
  return static_cast<uint64_t>(now.time_since_epoch().count());
}

void ObservabilityAgent::endTimer(const char* name, uint64_t startTime) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  auto now = std::chrono::steady_clock::now();
  auto endTime = static_cast<uint64_t>(now.time_since_epoch().count());
  auto duration = endTime - startTime;
  
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 + size2 > 0)
  {
      MetricEvent e;
      e.type = MetricEvent::Type::TimerEnd;
      e.name = name;
      e.value = static_cast<double>(duration);
      e.timestamp = static_cast<int64_t>(endTime);

      if (size1 > 0)
          eventBuffer_[start1] = e;
      else
          eventBuffer_[start2] = e;

      fifo_.finishedWrite(1);
  }
  
  metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

//==============================================================================
// Logging (async, non-RT)

void ObservabilityAgent::log(LogLevel level, const juce::String& message) {
  // TODO: Queue log message for async processing
  // TODO: Format with timestamp and level
  // TODO: Write to log sink
  
  DBG("[" << static_cast<int>(level) << "] " << message);
}

void ObservabilityAgent::logStructured(LogLevel level,
                                       const juce::String& message,
                                       const juce::var& data) {
  // TODO: Serialize structured data to JSON
  // TODO: Queue for async processing
  
  log(level, message);
}

//==============================================================================
// Configuration

void ObservabilityAgent::setEnabled(bool enabled) {
  enabled_.store(enabled, std::memory_order_release);
}

void ObservabilityAgent::setExportInterval(std::chrono::milliseconds interval) {
  // TODO: Configure periodic metrics export
  // TODO: Start/restart export timer
}

std::vector<ObservabilityAgent::Metric> ObservabilityAgent::getMetrics() {
  std::vector<Metric> metrics;
  
  int numReady = fifo_.getNumReady();
  if (numReady == 0) return metrics;

  int start1, size1, start2, size2;
  fifo_.prepareToRead(numReady, start1, size1, start2, size2);

  std::map<std::string, Metric> aggregated;

  auto processEvent = [&](const MetricEvent& e) {
      // Find or create metric
      auto& m = aggregated[e.name];

      // Initialize if new
      if (m.name.empty()) {
          m.name = e.name;
          m.timestamp = Timestamp(std::chrono::steady_clock::duration(e.timestamp));

          switch (e.type) {
              case MetricEvent::Type::Counter: m.type = MetricType::Counter; break;
              case MetricEvent::Type::Gauge: m.type = MetricType::Gauge; break;
              case MetricEvent::Type::TimerEnd: m.type = MetricType::Timer; break;
          }
      } else {
          // Update timestamp to latest
          m.timestamp = Timestamp(std::chrono::steady_clock::duration(e.timestamp));
      }

      // Aggregate value
      if (e.type == MetricEvent::Type::Counter) {
          m.value += e.value;
      } else {
          // For Gauge and Timer, last value wins in this batch
          m.value = e.value;
      }
  };

  for (int i = 0; i < size1; ++i) processEvent(eventBuffer_[start1 + i]);
  for (int i = 0; i < size2; ++i) processEvent(eventBuffer_[start2 + i]);

  fifo_.finishedRead(size1 + size2);

  metrics.reserve(aggregated.size());
  for (const auto& pair : aggregated) {
      metrics.push_back(pair.second);
  }
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  // Clear lock-free ring buffer by draining it
  // reset() is not thread-safe if the producer is active
  int numReady = fifo_.getNumReady();
  if (numReady > 0)
  {
      int start1, size1, start2, size2;
      fifo_.prepareToRead(numReady, start1, size1, start2, size2);
      fifo_.finishedRead(size1 + size2);
  }

  metricsCollected_.store(0, std::memory_order_release);
}

} // namespace agents
} // namespace zenith
