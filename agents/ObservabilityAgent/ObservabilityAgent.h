/*
  ==============================================================================
    agents/ObservabilityAgent/ObservabilityAgent.h
    Lock-free observability and metrics collection for real-time audio.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <string>
#include <map>
#include <chrono>

namespace zenith {
namespace agents {

class ObservabilityAgent {
public:
  struct Metric {
    std::string name;
    double value;
    std::map<std::string, std::string> labels;
    enum class Type { Counter, Gauge, Timer } type;
  };

  ObservabilityAgent();
  ~ObservabilityAgent();

  // RT-Safe: name must be a static string literal
  void recordCounter(const char* name, int increment = 1);
  void recordGauge(const char* name, double value);
  void startTimer(const char* name);
  void endTimer(const char* name);

  // Non-RT: Reads from ring buffer and returns full metrics
  std::vector<Metric> getMetrics();
  
  // Configuration
  void setEnabled(bool enabled);
  void setExportInterval(std::chrono::milliseconds interval);
  void clearMetrics();

  // Logging (async, non-RT)
  enum class LogLevel { Debug, Info, Warning, Error, Critical };
  void log(LogLevel level, const juce::String& message);
  void logStructured(LogLevel level, const juce::String& message, const juce::var& data);

private:
  struct RawMetricEvent {
    enum class Type { Counter, Gauge, Timer } type;
    const char* name; // CONTRACT: Must be static
    double value;
    int64_t timestamp;
  };

  // Lock-free Queue
  juce::AbstractFifo fifo_{ 4096 };
  std::vector<RawMetricEvent> eventBuffer_;

  // Helper to push to FIFO
  void pushToFifo(RawMetricEvent::Type type, const char* name, double value);

  std::atomic<bool> enabled_{true};
  std::atomic<int> metricsCollected_{ 0 };

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObservabilityAgent)
};

} // namespace agents
} // namespace zenith
