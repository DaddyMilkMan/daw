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
  // Ensure the logging thread wakes up to exit
  signalThreadShouldExit();
  logEvent_.signal();
  stopThread(1000);

  stopExportThread();
  // Flush any pending metrics
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
    event.timestamp = startTimer(); // Capture timestamp
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

  // Try to acquire the lock - drop if contended (RT requirement)
  if (!logSpinLock_.tryEnter()) {
    droppedLogCount_.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  // FIFO logic - protected by spinlock for MPSC safety
  int start1, size1, start2, size2;
  logFifo_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
    auto& entry = logBuffer_[static_cast<size_t>(start1)];
    entry.level = level;
    entry.timestamp = juce::Time::currentTimeMillis();
    entry.threadId = (uint64_t)(uintptr_t)juce::Thread::getCurrentThreadId();

    // RT-safe string copy with truncation handling
    size_t maxLen = sizeof(entry.message) - 1;
    size_t msgLen = std::strlen(message);

    if (msgLen > maxLen) {
        std::memcpy(entry.message, message, maxLen - 3);
        std::memcpy(entry.message + maxLen - 3, "...", 3);
        entry.message[maxLen] = '\0';
        truncatedLogCount_.fetch_add(1, std::memory_order_relaxed);
    } else {
        std::memcpy(entry.message, message, msgLen);
        entry.message[msgLen] = '\0';
    }

    logFifo_.finishedWrite(1);

    // Release lock before signaling to minimize hold time
    logSpinLock_.exit();
    logEvent_.signal();
  } else {
    // Queue full
    logSpinLock_.exit();
    droppedLogCount_.fetch_add(1, std::memory_order_relaxed);
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

void ObservabilityAgent::setLogFile(const juce::File& file) {
    {
        std::lock_guard<std::mutex> lock(logFileMutex_);
        logFile_ = file;
    }
    logEvent_.signal(); // Wake up consumer to handle file switch
}

uint64_t ObservabilityAgent::getDroppedLogCount() const {
    return droppedLogCount_.load(std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::getTruncatedLogCount() const {
    return truncatedLogCount_.load(std::memory_order_relaxed);
}

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
}

void ObservabilityAgent::setMetricsFile(const juce::File& file) {
    std::lock_guard<std::mutex> lock(exportMutex_);
    metricsFile_ = file;
}

void ObservabilityAgent::exportMetrics() {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }

  // Fetch metrics (this calls processRingBuffer internally)
  auto metrics = getMetrics();

  juce::File dest;
  {
      std::lock_guard<std::mutex> lock(exportMutex_);
      dest = metricsFile_;
  }

  if (exporter_) {
      exporter_->exportMetrics(metrics, dest);
  }

  // Track export count for verification
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
        exportMetrics();
    }
}

void ObservabilityAgent::run() {
  // Local state for file handling
  juce::File currentLogFile;

  while (!threadShouldExit()) {
    // Check for file changes
    juce::File targetFile;
    {
        std::lock_guard<std::mutex> lock(logFileMutex_);
        targetFile = logFile_;
    }

    if (targetFile != currentLogFile) {
        currentLogFile = targetFile;
        logStream_.reset(); // Close existing

        if (currentLogFile != juce::File()) {
            logStream_ = currentLogFile.createOutputStream();
            if (!logStream_) {
                // Failed to open - fallback to debug
                DBG("ObservabilityAgent: Failed to open log file " << currentLogFile.getFullPathName());
            }
        }
    }

    // Process queue
    int start1, size1, start2, size2;
    logFifo_.prepareToRead(kLogQueueSize, start1, size1, start2, size2);

    if (size1 + size2 > 0) {
      auto process = [this](int index) {
        const auto& entry = logBuffer_[static_cast<size_t>(index)];

        // Format: ISO8601 LEVEL [tid=...] Message
        auto time = juce::Time(entry.timestamp);
        juce::String levelStr;
        switch (entry.level) {
            case LogLevel::Debug:    levelStr = "DEBUG"; break;
            case LogLevel::Info:     levelStr = "INFO"; break;
            case LogLevel::Warning:  levelStr = "WARN"; break;
            case LogLevel::Error:    levelStr = "ERROR"; break;
            case LogLevel::Critical: levelStr = "CRIT"; break;
        }

        juce::String output = time.toISO8601(true) + " " +
                              levelStr + " [tid=" +
                              juce::String(entry.threadId) + "] " +
                              juce::String(entry.message);

        // Output to Debug
        DBG(output);

        // Output to File
        if (logStream_) {
            *logStream_ << output << juce::newLine;
            logStream_->flush();
        }
      };

      for (int i = 0; i < size1; ++i) process(start1 + i);
      for (int i = 0; i < size2; ++i) process(start2 + i);

      logFifo_.finishedRead(size1 + size2);
    }

    // Wait for event or timeout
    logEvent_.wait(1000);
  }
}

} // namespace agents
} // namespace zenith
