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
ObservabilityAgent::ObservabilityAgent()
  : ringBuffer_(4096) // Initialize MPSC ring buffer with 4096 slots
{
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
  
  // Use high-resolution clock for timestamp
  auto now = std::chrono::steady_clock::now();
  int64_t timestamp = static_cast<int64_t>(now.time_since_epoch().count());

  if (ringBuffer_.write(MetricType::Counter, name, value, timestamp)) {
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  }
}

void ObservabilityAgent::recordGauge(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  auto now = std::chrono::steady_clock::now();
  int64_t timestamp = static_cast<int64_t>(now.time_since_epoch().count());

  if (ringBuffer_.write(MetricType::Gauge, name, value, timestamp)) {
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
  auto duration = endTime - startTime;
  
  // We record the duration as the value for the timer metric
  if (ringBuffer_.write(MetricType::Timer, name, static_cast<double>(duration), static_cast<int64_t>(endTime))) {
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
  RingBufferEvent event;
  
  // Drain the ring buffer
  while (ringBuffer_.read(event)) {
    Metric metric;
    // name is a const char* pointer (static lifetime per contract)
    if (event.name) {
      metric.name = std::string(event.name);
    }
    metric.type = event.type;
    metric.value = event.value;

    // Reconstruct timestamp from int64_t ticks
    auto duration = std::chrono::steady_clock::duration(event.timestamp);
    metric.timestamp = std::chrono::steady_clock::time_point(duration);

    metrics.push_back(std::move(metric));
  }
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  // Drain and discard
  RingBufferEvent event;
  while (ringBuffer_.read(event)) {
    // Discard
  }
  metricsCollected_.store(0, std::memory_order_release);
}

} // namespace agents
} // namespace zenith
