/**
 * @file ObservabilityAgent.h
 * @brief Agent for system observability, metrics, and tracing
 * 
 * Thread Safety:
 * - recordMetric() can be called from AUDIO THREAD (RT-safe)
 * - Metrics export and aggregation are MESSAGE THREAD ONLY
 * - Uses lock-free data structures for audio thread metrics
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace zenith {

/**
 * @enum MetricType
 * @brief Types of metrics that can be collected
 */
enum class MetricType {
    Counter,     // Monotonically increasing value
    Gauge,       // Point-in-time value
    Histogram,   // Distribution of values
    Timer        // Duration measurements
};

/**
 * @struct Metric
 * @brief Represents a collected metric
 */
struct Metric {
    std::string name;
    MetricType type;
    double value;
    uint64_t timestamp;
    std::map<std::string, std::string> labels;
};

/**
 * @struct TraceSpan
 * @brief Represents a distributed tracing span
 */
struct TraceSpan {
    std::string name;
    std::string spanId;
    std::string traceId;
    uint64_t startTimestamp;
    uint64_t endTimestamp;
    std::map<std::string, std::string> attributes;
};

/**
 * @class ObservabilityAgent
 * @brief Provides comprehensive observability for the DAW
 */
class ObservabilityAgent {
public:
    ObservabilityAgent();
    ~ObservabilityAgent();

    /**
     * Initialize the agent
     * MESSAGE THREAD ONLY
     */
    void initialize();

    /**
     * Record a metric value
     * AUDIO THREAD SAFE - RT-safe for counters and gauges
     * 
     * @param name Metric name
     * @param value Metric value
     * @param type Metric type
     */
    void recordMetric(const char* name, double value, MetricType type = MetricType::Gauge);

    /**
     * Start a trace span
     * AUDIO THREAD SAFE - RT-safe
     * 
     * @param spanName Name of the span
     * @return Span ID for ending the span later
     */
    uint64_t startSpan(const char* spanName);

    /**
     * End a trace span
     * AUDIO THREAD SAFE - RT-safe
     * 
     * @param spanId Span ID from startSpan()
     */
    void endSpan(uint64_t spanId);

    /**
     * Get current metric value
     * Thread-safe via atomic (for gauge metrics)
     * 
     * @param name Metric name
     * @return Current value, or 0.0 if not found
     */
    double getMetric(const std::string& name) const;

    /**
     * Export metrics in Prometheus format
     * MESSAGE THREAD ONLY
     * 
     * @return Prometheus-formatted metrics string
     */
    std::string exportPrometheusMetrics();

    /**
     * Export trace spans
     * MESSAGE THREAD ONLY
     * 
     * @return Vector of completed trace spans
     */
    std::vector<TraceSpan> exportTraceSpans();

    /**
     * Generate observability report
     * MESSAGE THREAD ONLY
     * 
     * @return Formatted report string
     */
    std::string generateReport();

private:
    // Atomic metrics (RT-safe)
    std::atomic<double> audioLoad_{0.0};
    std::atomic<double> cpuUsage_{0.0};
    std::atomic<uint64_t> bufferUnderrunCount_{0};
    std::atomic<uint64_t> totalCallbacks_{0};
    
    // Message thread state
    std::map<std::string, Metric> metrics_;
    std::vector<TraceSpan> completedSpans_;
    
    // Span tracking
    std::atomic<uint64_t> nextSpanId_{1};
    std::map<uint64_t, TraceSpan> activeSpans_;
    
    uint64_t getTimestampMicros() const;
    std::string generateTraceId() const;
};

/**
 * @class ScopedTrace
 * @brief RAII wrapper for trace spans
 * 
 * Usage:
 *   {
 *       ScopedTrace trace(agent, "processAudio");
 *       // ... audio processing ...
 *   } // Span automatically ended
 */
class ScopedTrace {
public:
    ScopedTrace(ObservabilityAgent* agent, const char* spanName)
        : agent_(agent), spanId_(0) {
        if (agent_) {
            spanId_ = agent_->startSpan(spanName);
        }
    }
    
    ~ScopedTrace() {
        if (agent_ && spanId_ != 0) {
            agent_->endSpan(spanId_);
        }
    }
    
    // Non-copyable
    ScopedTrace(const ScopedTrace&) = delete;
    ScopedTrace& operator=(const ScopedTrace&) = delete;

private:
    ObservabilityAgent* agent_;
    uint64_t spanId_;
};

} // namespace zenith
