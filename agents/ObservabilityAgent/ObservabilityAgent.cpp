/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.cpp
    Lock-free observability and metrics collection implementation.
  ==============================================================================
*/

#include "ObservabilityAgent.h"
#include "PrometheusExporter.h"
#include <cstring>
#include <cstdio>

namespace zenith {
namespace agents {

//==============================================================================
ObservabilityAgent::ObservabilityAgent() : juce::Thread("ObservabilityAgent") {
  // Initialize metrics collection system
  logBuffer_.resize(kLogQueueSize);
  ringBufferData_.resize(kRingBufferSize);
  
  exporter_ = std::make_unique<PrometheusExporter>();

  // Default metrics file location (temp directory)
  metricsFile_ = juce::File::getSpecialLocation(juce::File::tempDirectory)
                 .getChildFile("zenith_metrics.prom");

  startThread();
}

ObservabilityAgent::~ObservabilityAgent() {
  // Stop export thread first (it may be waiting to acquire exportMutex_)
  stopExportThread();
  // Then stop the juce::Thread for log processing
  stopThread(1000);
  // Flush any pending metrics (safe now that threads are stopped)
  exportMetrics();
}

//==============================================================================
// Metrics Collection (RT-safe)

void ObservabilityAgent::recordCounter(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  auto s1 = 0, s2 = 0, num1 = 0, num2 = 0;
  ringBufferFifo_.prepareToWrite(1, s1, num1, s2, num2);
  
  if (num1 > 0) {
    auto& event = ringBufferData_[s1];
    event.type = MetricType::Counter;
    event.name = name;
    event.value = value;
    event.timestamp = startTimer(); // Reuse for current timestamp
    ringBufferFifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  }
}

void ObservabilityAgent::recordGauge(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  auto s1 = 0, s2 = 0, num1 = 0, num2 = 0;
  ringBufferFifo_.prepareToWrite(1, s1, num1, s2, num2);
  
  if (num1 > 0) {
    auto& event = ringBufferData_[s1];
    event.type = MetricType::Gauge;
    event.name = name;
    event.value = value;
    event.timestamp = startTimer(); // Reuse for current timestamp
    ringBufferFifo_.finishedWrite(1);
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
  
  auto s1 = 0, s2 = 0, num1 = 0, num2 = 0;
  ringBufferFifo_.prepareToWrite(1, s1, num1, s2, num2);
  
  if (num1 > 0) {
    auto& event = ringBufferData_[s1];
    event.type = MetricType::Timer;
    event.name = name;
    event.value = static_cast<double>(duration);
    event.timestamp = endTime;
    ringBufferFifo_.finishedWrite(1);
    metricsCollected_.fetch_add(1, std::memory_order_relaxed);
  }
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

void ObservabilityAgent::log(const char* rawMessage) noexcept {
    // Treat raw log as Info level
    log(LogLevel::Info, rawMessage);
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
  std::unique_lock<std::mutex> lock(exportMutex_);
  exportInterval_ = interval;

  if (interval.count() > 0 && !exportThread_.joinable()) {
    shouldExitExportThread_.store(false, std::memory_order_release);
    lock.unlock();
    exportThread_ = std::thread(&ObservabilityAgent::exportLoop, this);
  } else if (interval.count() <= 0 && exportThread_.joinable()) {
    lock.unlock(); // Unlock before stopping to avoid deadlock
    stopExportThread();
  } else {
      lock.unlock();
      // Wake up thread to pick up new interval if already running
      // Notify after unlocking for better performance (avoids immediate re-block)
      exportCv_.notify_all();
  }
}

void ObservabilityAgent::setMetricsFile(const juce::File& file) {
    std::lock_guard<std::mutex> lock(exportMutex_);
    metricsFile_ = file;
}

void ObservabilityAgent::exportMetrics() {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }

  // TODO: Drain lock-free ring buffer
  // TODO: Send to configured exporters

  auto metrics = getMetrics();
  juce::File dest;
  {
      std::lock_guard<std::mutex> lock(exportMutex_);
      dest = metricsFile_;
  }
  if (exporter_) {
      exporter_->exportMetrics(metrics, dest);
  }

  // For now, track export count for verification
  exportCount_.fetch_add(1, std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::getExportCount() const {
  return exportCount_.load(std::memory_order_relaxed);
}

std::vector<ObservabilityAgent::Metric> ObservabilityAgent::getMetrics() {
  std::vector<Metric> metrics;
  
  // Create a sample metric from the internal counter for testing/demonstration
  // until the ring buffer is implemented.
  Metric m;
  m.name = "zenith_metrics_collected_total";
  m.type = MetricType::Counter;
  m.value = static_cast<double>(metricsCollected_.load(std::memory_order_relaxed));
  m.timestamp = std::chrono::steady_clock::now();

  metrics.push_back(m);

  // TODO: Read from lock-free ring buffer
  // TODO: Aggregate metrics by name
  // TODO: Apply time-based windowing
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  // TODO: Clear lock-free ring buffer
  metricsCollected_.store(0, std::memory_order_release);
}

void ObservabilityAgent::stopExportThread() {
    shouldExitExportThread_.store(true, std::memory_order_release);
    
    {
        std::lock_guard<std::mutex> lock(exportMutex_);
        exportCv_.notify_all();
    }

    if (exportThread_.joinable()) {
        exportThread_.join();
    }
}

void ObservabilityAgent::exportLoop() {
    while (!shouldExitExportThread_.load(std::memory_order_acquire)) {
        std::chrono::milliseconds interval;
        bool shouldExport = false;
        
        {
            std::unique_lock<std::mutex> lock(exportMutex_);
            interval = exportInterval_;
            
            if (interval.count() <= 0) break;
            
            // Wait for the interval or until signaled to exit
            // We check shouldExitExportThread_ in the predicate for responsiveness
            // Note: We don't check exportInterval_ in the predicate to avoid races
            // with setExportInterval(). Instead, we wake on any notification and
            // re-check the interval at the start of the next loop iteration.
            // wait_for returns false on timeout, true if predicate becomes true before timeout
            bool predicateTrue = exportCv_.wait_for(lock, interval, [this] { 
                return shouldExitExportThread_.load(std::memory_order_acquire); 
            });
            
            // On timeout (predicate false), it's time to export
            // On predicate true (woken by notification), loop will re-check conditions
            if (!predicateTrue) {
                shouldExport = true;
            }
        }

        // Export metrics if it's time (outside the lock to avoid blocking)
        if (shouldExport) {
            exportMetrics();
        }
    }
}

void ObservabilityAgent::run() {
  while (!threadShouldExit()) {
    int start1, size1, start2, size2;
    logFifo_.prepareToRead(kLogQueueSize, start1, size1, start2, size2);

    if (size1 + size2 > 0) {
      auto process = [this](int index) {
        const auto& entry = logBuffer_[static_cast<size_t>(index)];
        // Simple console output for now
        // In production this would write to a file or aggregation service
        // DBG("[" << static_cast<int>(entry.level) << "] "
        //     << entry.timestamp << ": "
        //     << entry.message);
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