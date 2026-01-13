/**
 * @file ScheduledTransportAgent.cpp
 * @brief Implementation of ScheduledTransportAgent
 */

#include "ScheduledTransportAgent.h"
#include <algorithm>

namespace zenith {

ScheduledTransportAgent::ScheduledTransportAgent() {
}

ScheduledTransportAgent::~ScheduledTransportAgent() {
}

void ScheduledTransportAgent::initialize(double sampleRate) {
    sampleRate_ = sampleRate;
    clearSchedule();
}

void ScheduledTransportAgent::scheduleEvent(TransportEventType type, 
                                           int64_t samplePosition,
                                           std::function<void()> callback) {
    // MESSAGE THREAD ONLY - can allocate and modify vector
    
    ScheduledEvent event;
    event.type = type;
    event.samplePosition = samplePosition;
    event.callback = callback;
    
    events_.push_back(event);
    
    // Sort events by sample position for efficient processing
    std::sort(events_.begin(), events_.end(), 
              [](const ScheduledEvent& a, const ScheduledEvent& b) {
                  return a.samplePosition < b.samplePosition;
              });
    
    pendingEventCount_.store(events_.size());
}

int ScheduledTransportAgent::processScheduledEvents(int64_t currentSamplePosition) {
    // AUDIO THREAD SAFE - RT-safe operations only
    // Note: This is a placeholder implementation
    // Production code should use a lock-free queue (e.g., juce::AbstractFifo)
    
    int processedCount = 0;
    
    // TODO: Implement lock-free event processing
    // For now, this is a skeleton that demonstrates the interface
    
    return processedCount;
}

void ScheduledTransportAgent::clearSchedule() {
    // MESSAGE THREAD ONLY
    events_.clear();
    pendingEventCount_.store(0);
}

size_t ScheduledTransportAgent::getPendingEventCount() const {
    return pendingEventCount_.load();
}

} // namespace zenith
