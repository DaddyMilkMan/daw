/*
  ==============================================================================
    ClockSyncAgent.h
    Coordinator agent for clock synchronization and timing management
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>

namespace zenith {
namespace agents {

/**
 * @class ClockSyncAgent
 * 
 * Manages clock synchronization, drift correction, and latency compensation.
 * Ensures sample-accurate timing across devices and tracks.
 * 
 * Thread Safety:
 * - Configuration methods are MESSAGE THREAD ONLY
 * - Timing methods may be called from audio thread (marked RT-safe)
 * - Uses atomics for drift metrics
 */
class ClockSyncAgent
{
public:
    ClockSyncAgent();
    ~ClockSyncAgent();

    // Clock source management
    enum class ClockSource {
        Internal,
        External,
        MIDIClock,
        WordClock,
        NetworkPTP
    };

    void setClockSource(ClockSource source);
    ClockSource getCurrentClockSource() const;

    // Drift monitoring and correction
    struct DriftMetrics {
        double driftPPM = 0.0;  // Parts per million
        double averageLatency = 0.0;
        int correctionCount = 0;
        bool stableSync = false;
    };

    DriftMetrics getDriftMetrics() const;
    void calibrate();

    // Latency compensation
    void setLatencyCompensation(int samples);
    int getLatencyCompensation() const;

    // Analysis
    juce::String generateTimingReport();

private:
    ClockSource currentClockSource_{ClockSource::Internal};
    std::atomic<int> latencyCompensation_{0};
    std::atomic<int> correctionCount_{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClockSyncAgent)
};

} // namespace agents
} // namespace zenith
