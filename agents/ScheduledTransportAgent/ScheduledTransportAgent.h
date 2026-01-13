/**
 * @file ScheduledTransportAgent.h
 * @brief Agent for managing scheduled transport operations and timing
 * 
 * Thread Safety:
 * - scheduleEvent() is MESSAGE THREAD ONLY
 * - processScheduledEvents() can be called from AUDIO THREAD (RT-safe)
 * - Uses lock-free data structures for audio thread access
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace zenith {

/**
 * @enum TransportEventType
 * @brief Types of scheduled transport events
 */
enum class TransportEventType {
    Play,
    Stop,
    Record,
    PunchIn,
    PunchOut,
    SetPosition
};

/**
 * @struct ScheduledEvent
 * @brief Represents a scheduled transport event
 */
struct ScheduledEvent {
    TransportEventType type;
    int64_t samplePosition;
    std::function<void()> callback;
};

/**
 * @class ScheduledTransportAgent
 * @brief Manages scheduled transport operations with sample-accurate timing
 */
class ScheduledTransportAgent {
public:
    ScheduledTransportAgent();
    ~ScheduledTransportAgent();

    /**
     * Initialize the agent
     * MESSAGE THREAD ONLY
     */
    void initialize(double sampleRate);

    /**
     * Schedule a transport event at a specific sample position
     * MESSAGE THREAD ONLY
     * 
     * @param type Event type
     * @param samplePosition Absolute sample position for event
     * @param callback Optional callback to invoke when event fires
     */
    void scheduleEvent(TransportEventType type, int64_t samplePosition, 
                      std::function<void()> callback = nullptr);

    /**
     * Process scheduled events up to current playhead position
     * AUDIO THREAD SAFE - RT-safe, lock-free
     * 
     * @param currentSamplePosition Current playhead position
     * @return Number of events processed
     */
    int processScheduledEvents(int64_t currentSamplePosition);

    /**
     * Clear all scheduled events
     * MESSAGE THREAD ONLY
     */
    void clearSchedule();

    /**
     * Get number of pending events
     * Thread-safe via atomic
     */
    size_t getPendingEventCount() const;

private:
    double sampleRate_{44100.0};
    
    // Atomic counter for pending events
    std::atomic<size_t> pendingEventCount_{0};
    
    // TODO: Replace with lock-free queue for production use
    // Currently using vector as placeholder
    std::vector<ScheduledEvent> events_;
};

} // namespace zenith
