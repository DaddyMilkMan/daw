/**
 * @file ObservabilityAgent.h
 * @brief Coordinator agent for system observability and monitoring
 * 
 * Thread Safety:
 * - recordMetric() is AUDIO THREAD SAFE (RT-safe, lock-free)
 * - Log writing happens on BACKGROUND thread
 * - Configuration is MESSAGE THREAD ONLY
 * 
 * Note: In release builds, RT-thread recording has minimal overhead.
 * In debug builds, can detect RT violations.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace zenith {

/**
 * @enum MetricType
 * @brief Types of metrics that can be recorded
 */
enum class MetricType {
    CpuUsage,
    BufferUnderrun,
    ProcessingLatency,
    MemoryAllocation,
    PluginLatency,
    NetworkLatency
};

/**
 * @struct PerformanceMetrics
 * @brief Aggregated performance statistics
 */
struct PerformanceMetrics {
    float avgCpuPercent{0.0f};
    float maxCpuPercent{0.0f};
    int underrunCount{0};
    float avgLatencyMs{0.0f};
    float maxLatencyMs{0.0f};
    int64_t totalSamplesProcessed{0};
};

/**
 * @class ObservabilityAgent
 * @brief Manages system monitoring and diagnostics
 * 
 * This agent provides comprehensive observability for the audio engine
 * with minimal impact on real-time performance.
 */
class ObservabilityAgent {
public:
    ObservabilityAgent();
    ~ObservabilityAgent();

    /**
     * @brief Initialize observability system
     * @param enableRtMetrics Enable RT-thread metric collection
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void initialize(bool enableRtMetrics);

    /**
     * @brief Record a metric value (RT-safe)
     * @param type Metric type
     * @param value Metric value
     * 
     * Thread: AUDIO THREAD (RT-safe, lock-free)
     * Note: In release builds, this is a fast atomic operation
     */
    void recordMetric(MetricType type, float value);

    /**
     * @brief Record a trace event (RT-safe in release)
     * @param eventName Event identifier
     * @param timestamp Sample timestamp
     * 
     * Thread: AUDIO THREAD (RT-safe via lock-free FIFO)
     */
    void recordTraceEvent(const char* eventName, int64_t timestamp);

    /**
     * @brief Get aggregated performance metrics
     * @return Current performance statistics
     * 
     * Thread: Any (uses atomics)
     */
    PerformanceMetrics getMetrics() const;

    /**
     * @brief Write log message (async, background thread)
     * @param message Log message
     * 
     * Thread: Any (queued to background thread)
     */
    void logMessage(const std::string& message);

    /**
     * @brief Enable/disable real-time metric collection
     * @param enable True to enable
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void setRtMetricsEnabled(bool enable);

private:
    std::atomic<float> avgCpuUsage_{0.0f};
    std::atomic<float> maxCpuUsage_{0.0f};
    std::atomic<int> underrunCount_{0};
    std::atomic<int64_t> totalSamples_{0};
    std::atomic<bool> rtMetricsEnabled_{false};
    
    // TODO: Add lock-free metric queue
    // TODO: Add background log writer thread
    // TODO: Add performance histogram collectors
};

} // namespace zenith
