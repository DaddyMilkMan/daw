/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.cpp
    Lock-free observability and metrics collection implementation.
  ==============================================================================
*/

#include "ObservabilityAgent.h"
#include <cstring>

namespace zenith {
namespace agents {

//==============================================================================
ObservabilityAgent::ObservabilityAgent() : juce::Thread("ObservabilityAgent") {
  // Initialize metrics collection system
  logBuffer_.resize(kLogQueueSize);
  startThread();
}

ObservabilityAgent::~ObservabilityAgent() {
  stopThread(1000);
  // Flush any pending metrics
}

//==============================================================================
// Metrics Collection (RT-safe)

void ObservabilityAgent::recordCounter(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  // TODO: Write to lock-free ring buffer
  // TODO: Avoid string allocations
  
  metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

void ObservabilityAgent::recordGauge(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  // TODO: Write to lock-free ring buffer
  
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
  
  // TODO: Record timer metric with duration
  // TODO: Write to lock-free ring buffer
  
  metricsCollected_.fetch_add(1, std::memory_order_relaxed);
}

//==============================================================================
// Logging (async, non-RT)

void ObservabilityAgent::log(LogLevel level, const char* message) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }

  int start1, size1, start2, size2;
  logFifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
    auto& entry = logBuffer_[static_cast<size_t>(start1)];
    entry.level = level;
    entry.timestamp = startTimer();

    // RT-safe string copy
    std::strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    logFifo_.finishedWrite(1);
  }
}

void ObservabilityAgent::log(LogLevel level, const juce::String& message) {
  log(level, message.toRawUTF8());
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
  
  // TODO: Read from lock-free ring buffer
  // TODO: Aggregate metrics by name
  // TODO: Apply time-based windowing
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  // TODO: Clear lock-free ring buffer
  metricsCollected_.store(0, std::memory_order_release);
}

void ObservabilityAgent::run() {
  while (!threadShouldExit()) {
    int start1, size1, start2, size2;
    logFifo_.prepareToRead(kLogQueueSize, start1, size1, start2, size2);

    if (size1 + size2 > 0) {
      auto process = [this](int index) {
        const auto& entry = logBuffer_[static_cast<size_t>(index)];
        DBG("[" << static_cast<int>(entry.level) << "] "
            << entry.timestamp << ": "
            << entry.message);
      };

      for (int i = 0; i < size1; ++i) process(start1 + i);
      for (int i = 0; i < size2; ++i) process(start2 + i);

      logFifo_.finishedRead(size1 + size2);
    }

    wait(100);
  }
}

} // namespace agents
} // namespace zenith
