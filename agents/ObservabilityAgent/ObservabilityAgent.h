/*
  ==============================================================================
    ObservabilityAgent.h
    Coordinator agent for system monitoring, logging, and telemetry
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>
#include <map>
#include <string>

namespace zenith {
namespace agents {

/**
 * @class ObservabilityAgent
 * 
 * Provides system-wide monitoring, logging, and telemetry collection.
 * Ensures minimal overhead and RT-safe metric gathering.
 * 
 * Thread Safety:
 * - Configuration is MESSAGE THREAD ONLY
 * - Metric recording is RT-safe (lock-free counters)
 * - Report generation is MESSAGE THREAD ONLY
 */
class ObservabilityAgent
{
public:
    ObservabilityAgent();
    ~ObservabilityAgent();

    // Monitoring control
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    // Metric recording (RT-safe)
    void recordMetric(const std::string& name, double value);
    void incrementCounter(const std::string& name);

    // System metrics
    struct SystemMetrics {
        double cpuUsage = 0.0;
        double memoryUsage = 0.0;
        int audioDropouts = 0;
        double averageLatency = 0.0;
        int activeThreads = 0;
        juce::String systemInfo;
    };

    SystemMetrics getSystemMetrics();

    // Reporting
    juce::String generateDiagnosticReport();
    void exportTelemetry(const juce::File& outputFile);

    // Profiling
    void startProfiling();
    void stopProfiling();
    juce::String getProfilingResults();

private:
    std::atomic<bool> isMonitoring_{false};
    std::atomic<bool> isProfiling_{false};
    std::map<std::string, std::atomic<int>> counters_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObservabilityAgent)
};

} // namespace agents
} // namespace zenith
