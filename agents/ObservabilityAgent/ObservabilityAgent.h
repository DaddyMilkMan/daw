/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.h
    Lock-free observability and metrics collection for real-time audio.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <array>

namespace zenith {
namespace agents {

//==============================================================================
/**
    ObservabilityAgent provides lock-free metrics collection and monitoring
    for real-time audio systems without impacting RT thread performance.
*/
class ObservabilityAgent : public juce::Thread {
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
  
  /// Record counter increment (RT-safe, lock-free)
  void recordCounter(const char* name, double value = 1.0) noexcept;
  
  /// Record gauge value (RT-safe, lock-free)
  void recordGauge(const char* name, double value) noexcept;
  
  /// Start timing measurement (RT-safe)
  uint64_t startTimer() noexcept;
  
  /// End timing measurement and record (RT-safe)
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
  
  //==============================================================================
  // Configuration
  
  /// Enable/disable metrics collection
  void setEnabled(bool enabled);
  
  /// Set metrics export interval
  void setExportInterval(std::chrono::milliseconds interval);
  
  /// Get collected metrics (non-RT)
  std::vector<Metric> getMetrics();
  
  /// Clear collected metrics
  void clearMetrics();

private:
  //==============================================================================
  void run() override;

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
  
  // Async Log Queue
  static constexpr int kLogQueueSize = 1024;
  juce::AbstractFifo logFifo_{kLogQueueSize};
  std::vector<LogEntry> logBuffer_;

  // Lock-free ring buffer for RT metrics
  static constexpr int kRingBufferSize = 4096;
  juce::AbstractFifo ringBufferFifo_{kRingBufferSize};
  std::vector<RawMetricEvent> ringBufferData_;

  // TODO: Add metrics exporter (Prometheus, OpenTelemetry)
  // TODO: Add trace context propagation
  // TODO: Add log aggregation
  
  friend class ObservabilityAgentTest; // Allow tests to access ring buffer

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObservabilityAgent)
};

} // namespace agents
} // namespace zenith