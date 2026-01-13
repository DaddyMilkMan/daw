/*
  ==============================================================================
    ScheduledTransportAgent.h
    Coordinator agent for scheduled transport and timeline management
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>

namespace zenith {
namespace agents {

/**
 * @class ScheduledTransportAgent
 * 
 * Coordinates transport operations, timeline management, and tempo sync.
 * Ensures accurate playback, recording, and external synchronization.
 * 
 * Thread Safety:
 * - All public methods are MESSAGE THREAD ONLY
 * - Coordinates with TransportController atomics
 * - Never modifies transport state from audio thread
 */
class ScheduledTransportAgent
{
public:
    ScheduledTransportAgent();
    ~ScheduledTransportAgent();

    // Transport coordination
    void analyzeTransportState();
    void optimizeScheduling();
    
    // Sync management
    enum class SyncProtocol {
        Internal,
        MIDIClock,
        MTC,
        AbletonLink,
        LTC
    };

    void setSyncProtocol(SyncProtocol protocol);
    SyncProtocol getCurrentSyncProtocol() const;

    // Analysis and reporting
    struct TransportReport {
        double timingAccuracy = 0.0;
        int schedulingConflicts = 0;
        juce::String syncStatus;
        juce::String recommendations;
    };

    TransportReport generateReport();

private:
    SyncProtocol currentSyncProtocol_{SyncProtocol::Internal};
    std::atomic<int> schedulingConflicts_{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScheduledTransportAgent)
};

} // namespace agents
} // namespace zenith
