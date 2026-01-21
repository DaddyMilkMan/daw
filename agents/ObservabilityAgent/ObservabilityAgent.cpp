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
ObservabilityAgent::ObservabilityAgent() {
  // Initialize metrics collection system
  eventBuffer_.resize(bufferSize);
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
    eventBuffer_[start1] = {
        name,
        value,
        MetricType::Counter,
        static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count())
    };
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

  if (size1 > 0) {
    eventBuffer_[start1] = {
        name,
        value,
        MetricType::Gauge,
        static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count())
    };
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

  if (size1 > 0) {
    eventBuffer_[start1] = {
        name,
        static_cast<double>(duration),
        MetricType::Timer,
        static_cast<int64_t>(endTime)
    };
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
  
  auto numReady = fifo_.getNumReady();
  if (numReady == 0)
    return metrics;

  int start1, size1, start2, size2;
  fifo_.prepareToRead(numReady, start1, size1, start2, size2);

  auto readFromBuffer = [&](int start, int size) {
    for (int i = 0; i < size; ++i) {
      const auto& event = eventBuffer_[start + i];
      metrics.push_back({
          event.name, // Converts const char* to std::string
          event.type,
          event.value,
          Timestamp(std::chrono::steady_clock::duration(event.timestamp)),
          {}
      });
    }
  };

  if (size1 > 0)
    readFromBuffer(start1, size1);
  if (size2 > 0)
    readFromBuffer(start2, size2);

  fifo_.finishedRead(size1 + size2);

  // TODO: Aggregate metrics by name
  // TODO: Apply time-based windowing
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  // Clear lock-free ring buffer
  fifo_.reset();
  metricsCollected_.store(0, std::memory_order_release);
}

} // namespace agents
} // namespace zenith
