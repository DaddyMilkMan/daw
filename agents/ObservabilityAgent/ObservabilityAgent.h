/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.h
    Lock-free observability and metrics collection for real-time audio.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <array>

namespace zenith {
namespace agents {

class PrometheusExporter;

//==============================================================================
/**
    ObservabilityAgent provides lock-free metrics collection and monitoring
    for real-time audio systems without impacting RT thread performance.
*/
class ObservabilityAgent : public juce::Thread, private juce::Timer {
public:
  //==============================================================================
  using Timestamp = std::chrono::steady_clock::time_point;
  
  enum class MetricType {
    Counter,
    Gauge,
    Histogram,
    Timer
  };
  
  enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
  };
  
  struct Metric {
    std::string name;
    MetricType type;
    double value;
    Timestamp timestamp;
    std::vector<std::pair<std::string, std::string>> labels;
  };

  //==============================================================================
  ObservabilityAgent();
  ~ObservabilityAgent() override;

  //==============================================================================
  // Metrics Collection (RT-safe)
  
  /// Record counter increment (RT-safe, uses spinlock for MPSC safety)
  void recordCounter(const char* name, double value = 1.0) noexcept;
  
  /// Record gauge value (RT-safe, uses spinlock for MPSC safety)
  void recordGauge(const char* name, double value) noexcept;
  
  /// Start timing measurement (RT-safe, lock-free)
  uint64_t startTimer() noexcept;
  
  /// End timing measurement and record (RT-safe, uses spinlock for MPSC safety)
  void endTimer(const char* name, uint64_t startTime) noexcept;
  
  //==============================================================================
  // Logging (async, non-RT)
  
  /// Log message at specified level (RT-safe)
  void log(LogLevel level, const char* message) noexcept;

  /// Log message at specified level (Helper)
  void log(LogLevel level, const juce::String& message);
  
  /// Log structured data
  void logStructured(LogLevel level, 
                     const juce::String& message,
                     const juce::var& data);

  /// Low-level logging (RT-safe)
  void log(const char* rawMessage) noexcept;
  
  //==============================================================================
  // Configuration
  
  /// Enable/disable metrics collection
  void setEnabled(bool enabled);
  
  /// Set metrics export interval
  void setExportInterval(std::chrono::milliseconds interval);
  
  /// Set the destination file for metrics export
  void setMetricsFile(const juce::File& file);

  /// Get collected metrics (non-RT)
  std::vector<Metric> getMetrics();
  
  /// Clear collected metrics
  void clearMetrics();

  /// Export accumulated metrics (called by timer or manually)
  void exportMetrics();

  /// Get number of export cycles completed (for testing)
  uint64_t getExportCount() const;

#if JUCE_UNIT_TESTS
  /// Get ring buffer FIFO for testing (not RT-safe, test only)
  const juce::AbstractFifo& getRingBufferFifoForTesting() const { return ringBufferFifo_; }
  
  /// Get ring buffer data for testing (not RT-safe, test only)
  const std::vector<RawMetricEvent>& getRingBufferDataForTesting() const { return ringBufferData_; }
  
  /// Drain ring buffer for testing (not RT-safe, test only)
  /// @param numToDrain Number of events to drain, or -1 for all available
  /// @return Number of events actually drained
  int drainRingBufferForTesting(int numToDrain = -1) {
    const juce::SpinLock::ScopedLockType lock(ringBufferLock_);
    int s1, s2, num1, num2;
    int toDrain = numToDrain < 0 ? ringBufferFifo_.getNumReady() : numToDrain;
    ringBufferFifo_.prepareToRead(toDrain, s1, num1, s2, num2);
    int actuallyDrained = num1 + num2;
    ringBufferFifo_.finishedRead(actuallyDrained);
    return actuallyDrained;
  }
#endif

private:
  //==============================================================================
  void run() override;
  void timerCallback() override;
  void exportLoop();
  void stopExportThread();

  struct LogEntry {
      LogLevel level;
      uint64_t timestamp;
      char message[512];
  };

  struct RawMetricEvent {
    MetricType type;
    const char* name; // Points to static literal
    double value;
    uint64_t timestamp;
  };

  std::atomic<bool> enabled_{true};
  std::atomic<uint64_t> metricsCollected_{0};
  std::atomic<uint64_t> exportCount_{0};
  
  std::unique_ptr<PrometheusExporter> exporter_;
  juce::File metricsFile_;

  std::thread exportThread_;
  std::atomic<bool> shouldExitExportThread_{false};
  std::mutex exportMutex_;
  std::condition_variable exportCv_;
  std::chrono::milliseconds exportInterval_{0};

  // Async Log Queue
  static constexpr int kLogQueueSize = 1024;
  juce::AbstractFifo logFifo_{kLogQueueSize};
  std::vector<LogEntry> logBuffer_;

  // MPSC ring buffer for RT metrics (uses spinlock for multi-producer safety)
  // Critical section: ~20-50 CPU cycles (well under 100-cycle RT-safety limit)
  static constexpr int kRingBufferSize = 4096;
  juce::SpinLock ringBufferLock_;  // Protects AbstractFifo write operations
  juce::AbstractFifo ringBufferFifo_{kRingBufferSize};
  std::vector<RawMetricEvent> ringBufferData_;

  // TODO: Add metrics exporter (Prometheus, OpenTelemetry)
  // TODO: Add trace context propagation
  // TODO: Add log aggregation

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObservabilityAgent)
};

} // namespace agents
} // namespace zenith
