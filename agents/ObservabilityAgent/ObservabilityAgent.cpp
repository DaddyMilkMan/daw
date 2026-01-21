/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.cpp
    Lock-free observability and metrics collection implementation.
  ==============================================================================
*/

#include "ObservabilityAgent.h"

namespace zenith {
namespace agents {

//==============================================================================
ObservabilityAgent::ObservabilityAgent() : ringBufferData_(4096) {
  // Initialize metrics collection system
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
  ringBufferFifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
      ringBufferData_[start1].type = MetricEvent::Type::Counter;
      ringBufferData_[start1].name = name;
      ringBufferData_[start1].value = value;
      ringBufferFifo_.finishedWrite(1);
  }
  
  metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

void ObservabilityAgent::recordGauge(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  int start1, size1, start2, size2;
  ringBufferFifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
      ringBufferData_[start1].type = MetricEvent::Type::Gauge;
      ringBufferData_[start1].name = name;
      ringBufferData_[start1].value = value;
      ringBufferFifo_.finishedWrite(1);
  }
  
  metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::startTimer() noexcept {
  // Return high-resolution timestamp (nanoseconds)
  auto now = std::chrono::steady_clock::now();
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
}

void ObservabilityAgent::endTimer(const char* name, uint64_t startTime) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  auto now = std::chrono::steady_clock::now();
  auto endTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
  
  // Calculate duration in seconds
  double duration = (double)(endTime - startTime) * 1e-9;

  int start1, size1, start2, size2;
  ringBufferFifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
      ringBufferData_[start1].type = MetricEvent::Type::Timer;
      ringBufferData_[start1].name = name;
      ringBufferData_[start1].value = duration;
      ringBufferFifo_.finishedWrite(1);
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
  
  int numReady = ringBufferFifo_.getNumReady();
  if (numReady == 0)
      return metrics;

  int start1, size1, start2, size2;
  ringBufferFifo_.prepareToRead(numReady, start1, size1, start2, size2);

  metrics.reserve(size1 + size2);

  auto processEvent = [&](int index) {
      const auto& event = ringBufferData_[index];
      Metric m;
      m.name = event.name; // Assumes name is static char*
      m.value = event.value;
      m.timestamp = std::chrono::steady_clock::now(); // Capture read time as approximation

      switch (event.type) {
          case MetricEvent::Type::Counter: m.type = MetricType::Counter; break;
          case MetricEvent::Type::Gauge:   m.type = MetricType::Gauge; break;
          case MetricEvent::Type::Timer:   m.type = MetricType::Timer; break;
      }
      metrics.push_back(m);
  };

  for (int i = 0; i < size1; ++i)
      processEvent(start1 + i);

  for (int i = 0; i < size2; ++i)
      processEvent(start2 + i);

  ringBufferFifo_.finishedRead(size1 + size2);
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  // Drain the FIFO
  int numReady = ringBufferFifo_.getNumReady();
  if (numReady > 0) {
      int s1, z1, s2, z2;
      ringBufferFifo_.prepareToRead(numReady, s1, z1, s2, z2);
      ringBufferFifo_.finishedRead(z1 + z2);
  }
  metricsCollected_.store(0, std::memory_order_release);
}

} // namespace agents
} // namespace zenith
