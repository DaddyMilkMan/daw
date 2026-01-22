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
#include <memory>

namespace zenith {
namespace agents {

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
    Timer,
    None
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
  // MPSC Lock-Free Ring Buffer
  //==============================================================================
  struct RingBufferEvent {
    std::atomic<bool> isReady{false};
    MetricType type{MetricType::None};
    int64_t timestamp{0};
    const char* name{nullptr};
    double value{0.0};
  };

  class MpscRingBuffer {
  public:
    explicit MpscRingBuffer(size_t capacity = 4096)
      : capacity_(capacity), mask_(capacity - 1) {
      // Ensure capacity is power of 2
      jassert((capacity & (capacity - 1)) == 0);
      buffer_.reset(new RingBufferEvent[capacity]);

      // Initialize all slots
      for (size_t i = 0; i < capacity; ++i) {
        buffer_[i].isReady.store(false, std::memory_order_relaxed);
      }
    }

    bool write(MetricType type, const char* name, double value, int64_t timestamp) {
      size_t writeIdx;
      size_t readIdx;

      // CAS loop to reserve a slot.
      // This ensures we check capacity before incrementing writeIndex, avoiding "holes".
      do {
        writeIdx = writeIndex_.load(std::memory_order_relaxed);
        readIdx = readIndex_.load(std::memory_order_acquire);

        // Check if full
        // Use wrapping arithmetic safety:
        // If writeIdx is significantly larger than readIdx, it might wrap.
        // But assuming uint64_t and reasonable capacity, writeIdx - readIdx is safe.
        if (writeIdx - readIdx >= capacity_) {
          return false; // Drop event
        }
      } while (!writeIndex_.compare_exchange_weak(writeIdx, writeIdx + 1,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed));

      // Reserve successful.
      RingBufferEvent& event = buffer_[writeIdx & mask_];

      event.type = type;
      event.name = name;
      event.value = value;
      event.timestamp = timestamp;

      // Commit
      event.isReady.store(true, std::memory_order_release);
      return true;
    }

    bool read(RingBufferEvent& outEvent) {
      size_t readIdx = readIndex_.load(std::memory_order_relaxed);
      size_t writeIdx = writeIndex_.load(std::memory_order_acquire);

      if (readIdx >= writeIdx) {
        return false; // Empty
      }

      RingBufferEvent& event = buffer_[readIdx & mask_];

      if (!event.isReady.load(std::memory_order_acquire)) {
        // Contention: Writer reserved but hasn't committed yet.
        // Return false to try again later (don't block).
        return false;
      }

      // Copy data
      outEvent.type = event.type;
      outEvent.name = event.name;
      outEvent.value = event.value;
      outEvent.timestamp = event.timestamp;

      // Reset slot
      event.isReady.store(false, std::memory_order_release);

      // Advance reader
      readIndex_.store(readIdx + 1, std::memory_order_release);
      return true;
    }

  private:
    size_t capacity_;
    size_t mask_;
    std::unique_ptr<RingBufferEvent[]> buffer_;
    std::atomic<size_t> writeIndex_{0};
    std::atomic<size_t> readIndex_{0};
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
  
  /// Get collected metrics (non-RT)
  std::vector<Metric> getMetrics();
  
  /// Clear collected metrics
  void clearMetrics();

private:
  //==============================================================================
  std::atomic<bool> enabled_{true};
  std::atomic<uint64_t> metricsCollected_{0};
  
  MpscRingBuffer ringBuffer_;

  // TODO: Add metrics exporter (Prometheus, OpenTelemetry)
  // TODO: Add trace context propagation
  // TODO: Add log aggregation
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObservabilityAgent)
};

} // namespace agents
} // namespace zenith
