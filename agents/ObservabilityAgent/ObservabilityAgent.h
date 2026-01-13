/**
 * @file ObservabilityAgent.h
 * @brief Coordinator agent for system monitoring and diagnostics
 * 
 * Thread Safety:
 * - recordMetric() is AUDIO THREAD SAFE (lock-free)
 * - exportMetrics() is BACKGROUND THREAD SAFE
 * - All atomic operations for counters
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>

namespace zenith {

/**
 * @class ObservabilityAgent
 * @brief Provides monitoring and diagnostics without RT impact
 */
class ObservabilityAgent {
public:
    enum class MetricType {
        CpuUsage,
        MemoryUsage,
        AudioLatency,
        BufferUnderrun,
        Custom
    };

    ObservabilityAgent();
    ~ObservabilityAgent();

    /**
     * Initialize the agent
     */
    void initialize();

    /**
     * Record a metric value (AUDIO THREAD SAFE - lock-free)
     * @param type Type of metric
     * @param value Metric value
     * @param timestamp Optional timestamp (defaults to now)
     */
    void recordMetric(
        MetricType type, 
        double value,
        std::chrono::microseconds timestamp = std::chrono::microseconds(0));

    /**
     * Get current CPU usage (thread-safe)
     * @return CPU usage as percentage (0.0 - 100.0)
     */
    double getCpuUsage() const;

    /**
     * Get current memory usage (thread-safe)
     * @return Memory usage in bytes
     */
    size_t getMemoryUsage() const;

    /**
     * Get buffer underrun count (thread-safe)
     * @return Number of underruns since initialization
     */
    int getUnderrunCount() const;

    /**
     * Export metrics to string (BACKGROUND THREAD SAFE)
     * @return JSON-formatted metrics string
     */
    juce::String exportMetrics() const;

private:
    std::atomic<bool> initialized_{false};
    std::atomic<double> cpuUsage_{0.0};
    std::atomic<size_t> memoryUsage_{0};
    std::atomic<int> underrunCount_{0};
    
    // Lock-free ring buffer for metric events
    // TODO: Implement proper lock-free buffer structure
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObservabilityAgent)
};

} // namespace zenith
