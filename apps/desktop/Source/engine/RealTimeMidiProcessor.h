/*
  ==============================================================================

    RealTimeMidiProcessor.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #10)

    Ensures thread-safe real-time MIDI processing.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Real-time MIDI message with priority
 */
struct PrioritizedMidiMessage {
    juce::MidiMessage message;
    int samplePosition = 0;
    int priority = 0;                // 0=highest, 10=lowest
    double timestamp = 0.0;         // Creation time

    juce::String toString() const {
        return "MIDI[prio=" + juce::String(priority) +
               ", pos=" + juce::String(samplePosition) + "]";
    }
};

//==============================================================================
/**
 * @brief Real-time MIDI processing issue
 */
struct RealTimeMidiIssue {
    enum Type {
        ThreadSafetyViolation,      // Concurrent access detected
        PriorityInversion,          // Low priority blocked high
        DeadlineMissed,             // Processing too slow
        QueueOverflow,               // Message queue overflow
        DataCorruption,             // MIDI data corrupted
        ProcessingStalled            // Processing stopped
    };

    Type type;
    juce::String description;
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case ThreadSafetyViolation: typeStr = "Thread Safety"; break;
            case PriorityInversion: typeStr = "Priority Inversion"; break;
            case DeadlineMissed: typeStr = "Deadline Missed"; break;
            case QueueOverflow: typeStr = "Queue Overflow"; break;
            case DataCorruption: typeStr = "Data Corruption"; break;
            case ProcessingStalled: typeStr = "Processing Stalled"; break;
        }
        return "[" + typeStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Processing statistics
 */
struct MidiProcessingStatistics {
    std::atomic<int> totalMessagesProcessed{0};
    std::atomic<int> messagesDropped{0};
    std::atomic<int> queueOverflows{0};
    std::atomic<double> averageProcessingTime{0.0};  // microseconds
    std::atomic<int> threadSafetyViolations{0};

    juce::String toString() const {
        return "MIDI Processing: " +
               juce::String(totalMessagesProcessed.load()) + " processed, " +
               juce::String(messagesDropped.load()) + " dropped, " +
               juce::String(queueOverflows.load()) + " overflows";
    }
};

//==============================================================================
/**
 * @brief Thread-safe real-time MIDI processor
 *
 * Features:
 * - Thread-safe message queues
 * - Priority-based processing
 * - Lock-free algorithms where possible
 * - Real-time safety validation
 * - Message batching
 * - Processing time monitoring
 */
class RealTimeMidiProcessor {
public:
    //==========================================================================
    RealTimeMidiProcessor();
    ~RealTimeMidiProcessor();

    //==========================================================================
    /**
     * @brief Submit message for processing (thread-safe)
     * @param message Message to process
     * @param priority Message priority (0=highest)
     * @return true if queued successfully
     */
    bool submitMessage(const juce::MidiMessage& message,
                       int priority = 5);

    //==========================================================================
    /**
     * @brief Process pending messages
     * @param maxMessages Maximum number to process
     * @param sampleRate Sample rate for timing calculations
     * @return Number of messages processed
     */
    int processMessages(int maxMessages = 100,
                       double sampleRate = 48000.0);

    //==========================================================================
    /**
     * @brief Set maximum queue size
     * @param size Maximum number of messages in queue
     */
    void setMaxQueueSize(int size) {
        maxQueueSize_ = juce::jmax(100, size);
    }

    /**
     * @brief Get maximum queue size
     * @return Maximum size
     */
    int getMaxQueueSize() const { return maxQueueSize_; }

    //==========================================================================
    /**
     * @brief Get current queue size
     * @return Number of pending messages
     */
    int getQueueSize() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return static_cast<int>(messageQueue_.size());
    }

    //==========================================================================
    /**
     * @brief Enable/disable priority-based processing
     * @param enable true to use priority system
     */
    void setPriorityProcessingEnabled(bool enable) {
        priorityProcessingEnabled_ = enable;
    }

    //==========================================================================
    /**
     * @brief Get processing statistics
     * @return Current statistics
     */
    MidiProcessingStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        statistics_ = MidiProcessingStatistics{};
    }

    //==========================================================================
    /**
     * @brief Register callback for issues
     * @param callback Function to call when issue detected
     */
    void setIssueCallback(std::function<void(const RealTimeMidiIssue&)> callback) {
        issueCallback_ = callback;
    }

private:
    //==========================================================================
    int calculatePriority(const juce::MidiMessage& message) const;
    void reportIssue(const RealTimeMidiIssue& issue);

    //==========================================================================
    // Thread-safe message queue (sorted by priority)
    std::priority_queue<PrioritizedMidiMessage,
                        std::vector<PrioritizedMidiMessage>,
                        std::greater<PrioritizedMidiMessage>> messageQueue_;
    mutable std::mutex queueMutex_;

    // Settings
    int maxQueueSize_ = 1000;
    bool priorityProcessingEnabled_ = true;

    // Callbacks
    std::function<void(const RealTimeMidiIssue&)> issueCallback_;

    // Statistics
    MidiProcessingStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeMidiProcessor)
};

//==============================================================================
/**
 * @brief Singleton accessor for real-time MIDI processor
 */
class RealTimeMidiProcessorHolder {
public:
    static RealTimeMidiProcessor& getInstance() {
        static RealTimeMidiProcessor instance;
        return instance;
    }

    RealTimeMidiProcessorHolder(const RealTimeMidiProcessorHolder&) = delete;
    RealTimeMidiProcessorHolder& operator=(const RealTimeMidiProcessorHolder&) = delete;

private:
    RealTimeMidiProcessorHolder() = default;
    ~RealTimeMidiProcessorHolder() = default;
};

} // namespace zenith

// Comparison for priority queue (lower priority number = higher priority)
inline bool operator>(const PrioritizedMidiMessage& a,
                       const PrioritizedMidiMessage& b) {
    return a.priority < b.priority;
}
