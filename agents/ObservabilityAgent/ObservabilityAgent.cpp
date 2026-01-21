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
ObservabilityAgent::ObservabilityAgent() : eventBuffer_(4096) {
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
  fifo_.prepareToWrite(1, start1, size1, start2, size2);
  
  if (size1 > 0) {
    eventBuffer_[start1] = {name, value, MetricType::Counter,
                            (int64_t)std::chrono::steady_clock::now().time_since_epoch().count()};
    fifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  } else if (size2 > 0) {
    eventBuffer_[start2] = {name, value, MetricType::Counter,
                            (int64_t)std::chrono::steady_clock::now().time_since_epoch().count()};
    fifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  }
}

void ObservabilityAgent::recordGauge(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(1, start1, size1, start2, size2);
  
  if (size1 > 0) {
    eventBuffer_[start1] = {name, value, MetricType::Gauge,
                            (int64_t)std::chrono::steady_clock::now().time_since_epoch().count()};
    fifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  } else if (size2 > 0) {
    eventBuffer_[start2] = {name, value, MetricType::Gauge,
                            (int64_t)std::chrono::steady_clock::now().time_since_epoch().count()};
    fifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  }
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
  double duration = static_cast<double>(endTime - startTime);
  
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(1, start1, size1, start2, size2);
  
  if (size1 > 0) {
    eventBuffer_[start1] = {name, duration, MetricType::Timer,
                            (int64_t)endTime};
    fifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  } else if (size2 > 0) {
    eventBuffer_[start2] = {name, duration, MetricType::Timer,
                            (int64_t)endTime};
    fifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  }
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
  
  int start1, size1, start2, size2;
  int numReady = fifo_.getNumReady();

  if (numReady == 0)
    return metrics;

  fifo_.prepareToRead(numReady, start1, size1, start2, size2);

  auto processEvents = [&](int start, int size) {
    for (int i = 0; i < size; ++i) {
      const auto& raw = eventBuffer_[start + i];
      Metric m;
      m.name = raw.name;
      m.type = raw.type;
      m.value = raw.value;
      // Convert int64_t count back to steady_clock::time_point
      // Note: This assumes steady_clock epoch hasn't changed or isn't relevant for display logic
      // But for Metric struct we need time_point.
      using Duration = std::chrono::steady_clock::duration;
      m.timestamp = std::chrono::steady_clock::time_point(Duration(raw.timestamp));
      metrics.push_back(m);
    }
  };

  if (size1 > 0) processEvents(start1, size1);
  if (size2 > 0) processEvents(start2, size2);

  fifo_.finishedRead(size1 + size2);
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  // Clear lock-free ring buffer
  fifo_.reset();
  metricsCollected_.store(0, std::memory_order_release);
}

} // namespace agents
} // namespace zenith
