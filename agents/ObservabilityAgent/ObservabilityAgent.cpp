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

  // MPSC Safety: Try to acquire lock. If busy, drop message to avoid blocking RT thread.
  if (!logLock_.tryEnter()) {
      droppedLogs_.fetch_add(1, std::memory_order_relaxed);
      return;
  }

  // Critical section for producer
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
        std::strncpy(entry.message, message, maxLen - 3);
        entry.message[maxLen - 3] = '.';
        entry.message[maxLen - 2] = '.';
        entry.message[maxLen - 1] = '.';
        entry.message[maxLen] = '\0';
        truncatedLogs_.fetch_add(1, std::memory_order_relaxed);
    } else {
        std::strncpy(entry.message, message, maxLen);
        entry.message[msgLen] = '\0';
    }

    logFifo_.finishedWrite(1);

    // Signal consumer
    logLock_.exit();
    logEvent_.signal();
  } else {
      logLock_.exit();
      droppedLogs_.fetch_add(1, std::memory_order_relaxed);
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

void ObservabilityAgent::setLogFile(const juce::File& file) {
    std::lock_guard<std::mutex> lock(logFileMutex_);
    // Create new stream. If it fails, logStream_ will be null.
    logStream_ = file.createOutputStream();
}

void ObservabilityAgent::exportMetrics() {
  if (!enabled_.load(std::memory_order_acquire)) {
    return;
  }

  // TODO: Drain lock-free ring buffer
  // TODO: Send to configured exporters

  // For now, track export count for verification
  exportCount_.fetch_add(1, std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::getExportCount() const {
  return exportCount_.load(std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::getDroppedLogCount() const {
    return droppedLogs_.load(std::memory_order_relaxed);
}

uint64_t ObservabilityAgent::getTruncatedLogCount() const {
    return truncatedLogs_.load(std::memory_order_relaxed);
}

void ObservabilityAgent::timerCallback() {
  exportMetrics();
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

void ObservabilityAgent::processLogQueue() {
    int start1, size1, start2, size2;
    logFifo_.prepareToRead(kLogQueueSize, start1, size1, start2, size2);

    if (size1 + size2 > 0) {
        // Lock file mutex for the entire batch to ensure safety with setLogFile
        std::unique_lock<std::mutex> fileLock(logFileMutex_);

        auto process = [this](int index) {
            const auto& entry = logBuffer_[static_cast<size_t>(index)];

            juce::Time t(entry.timestamp);
            // Local time ISO-like format (no Z suffix as it's not UTC)
            juce::String timestamp = t.formatted("%Y-%m-%dT%H:%M:%S")
                                   + "." + juce::String::formatted("%03d", t.getMilliseconds());

            juce::String levelStr;
            switch(entry.level) {
                case LogLevel::Debug:    levelStr = "DEBUG"; break;
                case LogLevel::Info:     levelStr = "INFO"; break;
                case LogLevel::Warning:  levelStr = "WARN"; break;
                case LogLevel::Error:    levelStr = "ERROR"; break;
                case LogLevel::Critical: levelStr = "CRITICAL"; break;
            }

            juce::String logLine = "[" + timestamp + "] [" + levelStr + "] [tid="
                                 + juce::String(entry.threadId) + "] " + entry.message;

            // Debug output
            DBG(logLine);

            // File output
            if (logStream_) {
                logStream_->writeText(logLine + "\n", false, false, nullptr);
            }
        };

        for (int i = 0; i < size1; ++i) process(start1 + i);
        for (int i = 0; i < size2; ++i) process(start2 + i);

        if (logStream_) {
            logStream_->flush();
        }

        logFifo_.finishedRead(size1 + size2);
    }
}

void ObservabilityAgent::run() {
  while (!threadShouldExit()) {
    // Wait for signal or timeout (500ms for periodic flush/check)
    logEvent_.wait(500);
    processLogQueue();
  }
}

} // namespace agents
} // namespace zenith