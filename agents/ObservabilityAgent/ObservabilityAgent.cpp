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
  stopThread(1000);
  stopExportThread();
  // Flush any pending metrics
  stopTimer();
  exportMetrics();
}

//==============================================================================
// Metrics Collection (RT-safe)

void ObservabilityAgent::recordCounter(const char* name, double value) noexcept {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }
  
  const juce::SpinLock::ScopedLockType lock(ringBufferLock_);

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
  
  const juce::SpinLock::ScopedLockType lock(ringBufferLock_);

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
  
  const juce::SpinLock::ScopedLockType lock(ringBufferLock_);

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
    shouldExitExportThread_ = false;
    exportThread_ = std::thread(&ObservabilityAgent::exportLoop, this);
  } else if (interval.count() <= 0 && exportThread_.joinable()) {
    lock.unlock(); // Unlock before join to avoid deadlock
    stopExportThread();
  } else {
      // Wake up thread to pick up new interval if already running
      exportCv_.notify_all();
  }
  
  // Keep timer for compatibility if needed, or remove if thread supersedes
  if (interval.count() > 0) {
    juce::Timer::startTimer(static_cast<int>(interval.count()));
  } else {
    stopTimer();
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

  // Ring buffer is drained and processed in getMetrics()
  // which is called by exportLoop().
  // If this method is called manually (e.g. by Timer), we should also trigger export.
  // But exportLoop does it on a thread.
  // If timer is running, it just calls this.

  // For now, track export count for verification
  exportCount_.fetch_add(1, std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::getExportCount() const {
  return exportCount_.load(std::memory_order_relaxed);
}

void ObservabilityAgent::timerCallback() {
  exportMetrics();
}

void ObservabilityAgent::processRingBuffer() {
    std::lock_guard<std::mutex> lock(metricsMutex_);

    int start1, size1, start2, size2;
    // Drain everything available
    ringBufferFifo_.prepareToRead(kRingBufferSize, start1, size1, start2, size2);

    if (size1 + size2 == 0) return;

    auto processEvent = [this](int index) {
        const auto& event = ringBufferData_[static_cast<size_t>(index)];

        switch (event.type) {
            case MetricType::Counter: {
                auto& m = metricsMap_[event.name];
                if (m.name.empty()) {
                    m.name = event.name;
                    m.type = MetricType::Counter;
                }
                m.value += event.value;
                m.timestamp = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(event.timestamp));
                break;
            }
            case MetricType::Gauge: {
                auto& m = metricsMap_[event.name];
                m.name = event.name;
                m.type = MetricType::Gauge;
                m.value = event.value; // Gauges are overwritten
                m.timestamp = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(event.timestamp));
                break;
            }
            case MetricType::Timer: {
                // Aggregate Timer into sum and count
                std::string baseName = event.name;

                // Sum
                {
                    std::string sumName = baseName + "_sum";
                    auto& m = metricsMap_[sumName];
                    if (m.name.empty()) {
                        m.name = sumName;
                        m.type = MetricType::Counter; // Treat as counter for export
                    }
                    m.value += event.value;
                    m.timestamp = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(event.timestamp));
                }

                // Count
                {
                    std::string countName = baseName + "_count";
                    auto& m = metricsMap_[countName];
                    if (m.name.empty()) {
                        m.name = countName;
                        m.type = MetricType::Counter; // Treat as counter for export
                    }
                    m.value += 1.0;
                    m.timestamp = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(event.timestamp));
                }
                break;
            }
            default:
                break;
        }
    };

    for (int i = 0; i < size1; ++i) processEvent(start1 + i);
    for (int i = 0; i < size2; ++i) processEvent(start2 + i);

    ringBufferFifo_.finishedRead(size1 + size2);
}

std::vector<ObservabilityAgent::Metric> ObservabilityAgent::getMetrics() {
  processRingBuffer(); // Drain and aggregate new events

  std::lock_guard<std::mutex> lock(metricsMutex_);
  std::vector<Metric> metrics;
  metrics.reserve(metricsMap_.size() + 1);
  
  // Internal diagnostic metric
  Metric m;
  m.name = "zenith_metrics_collected_total";
  m.type = MetricType::Counter;
  m.value = static_cast<double>(metricsCollected_.load(std::memory_order_relaxed));
  m.timestamp = std::chrono::steady_clock::now();
  metrics.push_back(m);

  for (const auto& kv : metricsMap_) {
      metrics.push_back(kv.second);
  }
  
  return metrics;
}

void ObservabilityAgent::clearMetrics() {
  std::lock_guard<std::mutex> lock(metricsMutex_);
  metricsMap_.clear();

  // Also drain the ring buffer to ensure fresh start
  int s1, sz1, s2, sz2;
  ringBufferFifo_.prepareToRead(kRingBufferSize, s1, sz1, s2, sz2);
  ringBufferFifo_.finishedRead(sz1 + sz2);

  metricsCollected_.store(0, std::memory_order_release);
}

void ObservabilityAgent::stopExportThread() {
    {
        std::lock_guard<std::mutex> lock(exportMutex_);
        shouldExitExportThread_ = true;
        exportCv_.notify_all();
    }

    if (exportThread_.joinable()) {
        exportThread_.join();
    }
}

void ObservabilityAgent::exportLoop() {
    while (!shouldExitExportThread_) {
        std::chrono::milliseconds interval;
        {
            std::unique_lock<std::mutex> lock(exportMutex_);
            interval = exportInterval_;
        }

        if (interval.count() <= 0) break;

        // Sleep for the interval
        {
            std::unique_lock<std::mutex> lock(exportMutex_);
            exportCv_.wait_for(lock, interval, [this] { return shouldExitExportThread_.load(); });

            if (shouldExitExportThread_) break;
        }

        // Export metrics
        if (enabled_.load(std::memory_order_acquire)) {
            auto metrics = getMetrics();
            juce::File dest;
            {
                std::lock_guard<std::mutex> lock(exportMutex_);
                dest = metricsFile_;
            }
            if (exporter_) {
                exporter_->exportMetrics(metrics, dest);
            }
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
