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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace zenith {
namespace agents {

class PrometheusExporter;

//==============================================================================
/**
    ObservabilityAgent provides lock-free metrics collection and monitoring
    for real-time audio systems without impacting RT thread performance.
*/
class ObservabilityAgent {
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
  ~ObservabilityAgent();

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
  
  /// Log message at specified level
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
  
  /// Set the destination file for metrics export
  void setMetricsFile(const juce::File& file);

  /// Get collected metrics (non-RT)
  std::vector<Metric> getMetrics();
  
  /// Clear collected metrics
  void clearMetrics();

private:
  //==============================================================================
  void exportLoop();
  void stopExportThread();

  std::atomic<bool> enabled_{true};
  std::atomic<uint64_t> metricsCollected_{0};
  
  std::unique_ptr<PrometheusExporter> exporter_;
  juce::File metricsFile_;

  std::thread exportThread_;
  std::atomic<bool> shouldExitExportThread_{false};
  std::mutex exportMutex_;
  std::condition_variable exportCv_;
  std::chrono::milliseconds exportInterval_{0};

  // TODO: Add lock-free ring buffer for RT metrics
  // TODO: Add trace context propagation
  // TODO: Add log aggregation
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObservabilityAgent)
};

} // namespace agents
} // namespace zenith
