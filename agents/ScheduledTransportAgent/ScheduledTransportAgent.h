/**
 * @file ScheduledTransportAgent.h
 * @brief Coordinator agent for scheduled transport operations
 * 
 * Thread Safety:
 * - scheduleEvent() is MESSAGE THREAD ONLY
 * - processScheduledEvents() can be called from AUDIO THREAD
 * - All scheduling uses lock-free queues
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

namespace zenith {

/**
 * @class ScheduledTransportAgent
 * @brief Coordinates scheduled transport operations with sample accuracy
 */
class ScheduledTransportAgent {
public:
    ScheduledTransportAgent();
    ~ScheduledTransportAgent();

    /**
     * Initialize the agent with transport parameters
     * @param sampleRate Sample rate in Hz
     */
    void initialize(double sampleRate);

    /**
     * Schedule a transport event (MESSAGE THREAD ONLY)
     * @param eventTimeSamples Sample position when event should occur
     * @return true if event was scheduled successfully
     */
    bool scheduleTransportEvent(juce::int64 eventTimeSamples);

    /**
     * Process scheduled events for current audio buffer (AUDIO THREAD SAFE)
     * @param currentSamplePosition Current playhead position
     * @param numSamples Number of samples in current buffer
     */
    void processScheduledEvents(juce::int64 currentSamplePosition, int numSamples);

    /**
     * Get current tempo (thread-safe)
     * @return Current tempo in BPM
     */
    double getCurrentTempo() const;

private:
    std::atomic<double> sampleRate_{44100.0};
    std::atomic<double> currentTempo_{120.0};
    
    // Lock-free FIFO for scheduled events
    // TODO: Implement proper lock-free queue structure
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScheduledTransportAgent)
};

} // namespace zenith
