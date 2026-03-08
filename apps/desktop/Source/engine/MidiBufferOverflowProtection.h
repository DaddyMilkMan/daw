/*
  ==============================================================================

    MidiBufferOverflowProtection.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #5)

    Prevents and handles MIDI buffer overflows.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>

namespace zenith {

//==============================================================================
/**
 * @brief Buffer overflow event
 */
struct MidiOverflowEvent {
    enum Type {
        BufferFull,                  // Buffer completely full
        HighUsage,                  // Buffer usage above threshold
        MessagesDropped,            // Messages had to be dropped
        PriorityDrop,               // Low-priority messages dropped
        OverflowRecovered           // Overflow condition resolved
    };

    Type type;
    juce::String description;
    int bufferSize = 0;
    int messagesInBuffer = 0;
    int messagesDropped = 0;
    double usagePercentage = 0.0;
    double severity = 0.0;          // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case BufferFull: typeStr = "Buffer Full"; break;
            case HighUsage: typeStr = "High Usage"; break;
            case MessagesDropped: typeStr = "Messages Dropped"; break;
            case PriorityDrop: typeStr = "Priority Drop"; break;
            case OverflowRecovered: typeStr = "Recovered"; break;
        }
        return "[" + typeStr + "] " + description +
               " (" + juce::String(usagePercentage, 1) + "% full)";
    }
};

//==============================================================================
/**
 * @brief Message priority
 */
enum class MidiMessagePriority {
    Critical = 0,                  // Timing, clock
    High = 1,                      // Note on/off, pitch bend
    Normal = 2,                    // CC, program change
    Low = 3                        // Aftertouch, other
};

//==============================================================================
/**
 * @brief Buffer statistics
 */
struct MidiBufferStatistics {
    int totalMessagesReceived = 0;
    int totalMessagesDropped = 0;
    int overflowsDetected = 0;
    double averageUsage = 0.0;      // Percentage
    double peakUsage = 0.0;         // Percentage
    int currentBufferSize = 0;
    int criticalDrops = 0;          // High-priority messages dropped

    juce::String toString() const {
        return "MIDI Buffer: " + juce::String(averageUsage, 1) + "% avg usage, " +
               juce::String(totalMessagesDropped) + " dropped, " +
               juce::String(overflowsDetected) + " overflows";
    }
};

//==============================================================================
/**
 * @brief Manages MIDI buffer overflow protection
 *
 * Features:
 * - Overflow detection and warning
 * - Dynamic buffer resizing
 * - Priority-based message dropping
 * - Buffer usage monitoring
 * - Overflow recovery strategies
 * - Statistics tracking
 */
class MidiBufferOverflowProtection {
public:
    //==========================================================================
    MidiBufferOverflowProtection();
    ~MidiBufferOverflowProtection();

    //==========================================================================
    /**
     * @brief Add message to buffer with overflow protection
     * @param buffer Buffer to add to
     * @param message Message to add
     * @param samplePosition Sample position
     * @return true if added, false if dropped
     */
    bool addMessage(juce::MidiBuffer& buffer,
                   const juce::MidiMessage& message,
                   int samplePosition);

    //==========================================================================
    /**
     * @brief Check buffer usage
     * @param buffer Buffer to check
     * @return Usage percentage (0-100)
     */
    double getBufferUsage(const juce::MidiBuffer& buffer) const;

    //==========================================================================
    /**
     * @brief Check for overflow condition
     * @param buffer Buffer to check
     * @param warningThreshold Threshold for warning (default 80%)
     * @return Overflow event if condition exists
     */
    MidiOverflowEvent checkOverflow(const juce::MidiBuffer& buffer,
                                    double warningThreshold = 80.0) const;

    //==========================================================================
    /**
     * @brief Drop low-priority messages from buffer
     * @param buffer Buffer to process
     * @param targetUsage Target usage percentage
     * @return Number of messages dropped
     */
    int dropLowPriorityMessages(juce::MidiBuffer& buffer,
                                double targetUsage = 70.0);

    //==========================================================================
    /**
     * @brief Resize buffer if needed
     * @param buffer Buffer to resize
     * @param targetSize Target size
     * @return true if resized
     */
    bool resizeBufferIfNeeded(juce::MidiBuffer& buffer, int targetSize);

    //==========================================================================
    /**
     * @brief Set maximum buffer size
     * @param maxSize Maximum number of messages
     */
    void setMaxBufferSize(int maxSize) {
        maxBufferSize_ = juce::jmax(100, maxSize);
    }

    /**
     * @brief Get maximum buffer size
     * @return Maximum size
     */
    int getMaxBufferSize() const { return maxBufferSize_; }

    //==========================================================================
    /**
     * @brief Set overflow warning threshold
     * @param threshold Usage percentage (0-100)
     */
    void setWarningThreshold(double threshold) {
        warningThreshold_ = juce::jlimit(0.0, 100.0, threshold);
    }

    //==========================================================================
    /**
     * @brief Get message priority
     * @param message MIDI message
     * @return Priority level
     */
    static MidiMessagePriority getMessagePriority(const juce::MidiMessage& message);

    //==========================================================================
    /**
     * @brief Get buffer statistics
     * @return Current statistics
     */
    MidiBufferStatistics getStatistics() const {
        return statistics_;
    }

    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        statistics_ = MidiBufferStatistics{};
    }

    //==========================================================================
    /**
     * @brief Register callback for overflow events
     * @param callback Function to call when overflow detected
     */
    void setOverflowCallback(std::function<void(const MidiOverflowEvent&)> callback) {
        overflowCallback_ = callback;
    }

private:
    //==========================================================================
    void updateStatistics(const juce::MidiBuffer& buffer);
    void reportOverflow(const MidiOverflowEvent& event);

    //==========================================================================
    // Settings
    int maxBufferSize_ = 1000;
    double warningThreshold_ = 80.0;

    // Callbacks
    std::function<void(const MidiOverflowEvent&)> overflowCallback_;

    // Statistics
    MidiBufferStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiBufferOverflowProtection)
};

//==============================================================================
/**
 * @brief Singleton accessor for MIDI buffer overflow protection
 */
class MidiBufferOverflowProtectionHolder {
public:
    static MidiBufferOverflowProtection& getInstance() {
        static MidiBufferOverflowProtection instance;
        return instance;
    }

    MidiBufferOverflowProtectionHolder(const MidiBufferOverflowProtectionHolder&) = delete;
    MidiBufferOverflowProtectionHolder& operator=(const MidiBufferOverflowProtectionHolder&) = delete;

private:
    MidiBufferOverflowProtectionHolder() = default;
    ~MidiBufferOverflowProtectionHolder() = default;
};

} // namespace zenith
