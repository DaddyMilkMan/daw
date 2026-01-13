/*
  ==============================================================================
    RealtimeAudioEngineAgent.h
    Coordinator agent for realtime audio backend optimization
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>

namespace zenith {
namespace agents {

/**
 * @class RealtimeAudioEngineAgent
 * 
 * Coordinates realtime audio engine optimization and monitoring.
 * Ensures RT-safe operations and low-latency performance.
 * 
 * Thread Safety:
 * - All public methods are MESSAGE THREAD ONLY unless marked otherwise
 * - Uses atomics for cross-thread performance metrics
 * - Never called from audio thread directly
 */
class RealtimeAudioEngineAgent
{
public:
    RealtimeAudioEngineAgent();
    ~RealtimeAudioEngineAgent();

    // Performance monitoring
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    // Analysis and reporting
    struct PerformanceReport {
        double cpuUsage = 0.0;
        int dropoutCount = 0;
        double averageLatency = 0.0;
        juce::String recommendations;
        bool rtSafetyViolations = false;
    };

    PerformanceReport analyzePerformance();

    // RT-safety validation
    bool validateRTSafety();
    juce::String generateOptimizationReport();

private:
    std::atomic<bool> isMonitoring_{false};
    std::atomic<int> dropoutCount_{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealtimeAudioEngineAgent)
};

} // namespace agents
} // namespace zenith
