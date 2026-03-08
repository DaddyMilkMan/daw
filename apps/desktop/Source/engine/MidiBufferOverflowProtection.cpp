/*
  ==============================================================================

    MidiBufferOverflowProtection.cpp
    Implementation of MIDI buffer overflow protection

  ==============================================================================
*/

#include "MidiBufferOverflowProtection.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// MidiBufferOverflowProtection Implementation
//==============================================================================

MidiBufferOverflowProtection::MidiBufferOverflowProtection() {
    std::cout << "MidiBufferOverflowProtection: Initialized (max size: "
              << maxBufferSize_ << ")" << std::endl;
}

MidiBufferOverflowProtection::~MidiBufferOverflowProtection() {
    std::cout << "MidiBufferOverflowProtection: Shut down ("
              << statistics_.totalMessagesDropped << " messages dropped)"
              << std::endl;
}

//==============================================================================
bool MidiBufferOverflowProtection::addMessage(juce::MidiBuffer& buffer,
                                              const juce::MidiMessage& message,
                                              int samplePosition) {

    statistics_.totalMessagesReceived++;

    // Check current usage
    double usage = getBufferUsage(buffer);

    if (usage >= 100.0) {
        // Buffer is full
        MidiOverflowEvent event;
        event.type = MidiOverflowEvent::BufferFull;
        event.description = "Buffer full - message dropped";
        event.bufferSize = maxBufferSize_;
        event.messagesInBuffer = maxBufferSize_;
        event.messagesDropped = 1;
        event.usagePercentage = 100.0;
        event.severity = 9.0;

        statistics_.totalMessagesDropped++;
        statistics_.overflowsDetected++;

        reportOverflow(event);
        return false;
    }

    if (usage >= warningThreshold_) {
        // High usage warning
        MidiOverflowEvent event;
        event.type = MidiOverflowEvent::HighUsage;
        event.description = "Buffer usage at " + juce::String(usage, 1) + "%";
        event.bufferSize = maxBufferSize_;
        event.messagesInBuffer = static_cast<int>(usage * maxBufferSize_ / 100.0);
        event.usagePercentage = usage;
        event.severity = juce::jmin(10.0, (usage - warningThreshold_) / 5.0);

        reportOverflow(event);

        // Try to make space by dropping low-priority messages
        dropLowPriorityMessages(buffer, warningThreshold_ * 0.8);
    }

    // Add message to buffer
    buffer.addEvent(message, samplePosition);

    return true;
}

//==============================================================================
double MidiBufferOverflowProtection::getBufferUsage(const juce::MidiBuffer& buffer) const {
    int numMessages = buffer.getNumMessages();
    double usage = static_cast<double>(numMessages) / maxBufferSize_ * 100.0;
    return juce::jmin(100.0, usage);
}

//==============================================================================
MidiOverflowEvent MidiBufferOverflowProtection::checkOverflow(
    const juce::MidiBuffer& buffer,
    double warningThreshold) const {

    MidiOverflowEvent event;
    event.type = MidiOverflowEvent::HighUsage;

    double usage = getBufferUsage(buffer);
    event.bufferSize = maxBufferSize_;
    event.messagesInBuffer = buffer.getNumMessages();
    event.usagePercentage = usage;

    if (usage >= 100.0) {
        event.type = MidiOverflowEvent::BufferFull;
        event.description = "Buffer completely full";
        event.severity = 10.0;
    } else if (usage >= warningThreshold) {
        event.type = MidiOverflowEvent::HighUsage;
        event.description = "High buffer usage";
        event.severity = juce::jmin(10.0, (usage - warningThreshold) / 5.0);
    }

    return event;
}

//==============================================================================
int MidiBufferOverflowProtection::dropLowPriorityMessages(juce::MidiBuffer& buffer,
                                                          double targetUsage) {
    juce::MidiBuffer filteredBuffer;
    std::vector<std::pair<int, MidiMessagePriority>> messages;

    // Get all messages with priorities
    for (const auto& metadata : buffer) {
        auto priority = getMessagePriority(metadata.getMessage());
        messages.push_back({metadata.samplePosition, priority});
    }

    // Sort by priority (low priority first)
    std::sort(messages.begin(), messages.end(),
        [](const auto& a, const auto& b) {
            return static_cast<int>(a.second) > static_cast<int>(b.second);
        });

    // Calculate how many to keep to reach target usage
    int targetCount = static_cast<int>(maxBufferSize_ * targetUsage / 100.0);
    int toKeep = juce::min(targetCount, static_cast<int>(messages.size()));
    int dropped = 0;

    // Keep high-priority messages
    for (int i = static_cast<int>(messages.size()) - 1; i >= static_cast<int>(messages.size()) - toKeep; --i) {
        // Find and copy this message
        for (const auto& metadata : buffer) {
            if (metadata.samplePosition == messages[i].first) {
                filteredBuffer.addEvent(metadata.getMessage(), metadata.samplePosition);
                break;
            }
        }
    }

    dropped = static_cast<int>(messages.size()) - toKeep;
    buffer = filteredBuffer;

    if (dropped > 0) {
        std::cout << "MidiBufferOverflowProtection: Dropped " << dropped
                  << " low-priority messages" << std::endl;
    }

    statistics_.totalMessagesDropped += dropped;

    return dropped;
}

//==============================================================================
bool MidiBufferOverflowProtection::resizeBufferIfNeeded(juce::MidiBuffer& buffer,
                                                         int targetSize) {
    // JUCE's MidiBuffer doesn't support resizing directly
    // It's dynamically sized, so we just ensure we're not exceeding our limit
    return true;
}

//==============================================================================
MidiMessagePriority MidiBufferOverflowProtection::getMessagePriority(
    const juce::MidiMessage& message) {

    // Timing messages - highest priority
    if (message.isMidiClock() ||
        message.isSongPositionPointer() ||
        message.isMidiStart() ||
        message.isMidiStop() ||
        message.isMidiContinue()) {
        return MidiMessagePriority::Critical;
    }

    // Note messages - high priority
    if (message.isNoteOn() || message.isNoteOff()) {
        return MidiMessagePriority::High;
    }

    // Pitch bend - high priority
    if (message.isPitchWheel()) {
        return MidiMessagePriority::High;
    }

    // Control change - normal priority
    if (message.isController()) {
        // Some CCs are more important
        int ccNumber = message.getControllerNumber();
        if (ccNumber == 0 ||    // Bank select MSB
            ccNumber == 32 ||   // Bank select LSB
            ccNumber == 64 ||   // Sustain pedal
            ccNumber == 66 ||   // Sostenuto pedal
            ccNumber == 67) {   // Una corda pedal
            return MidiMessagePriority::High;
        }
        return MidiMessagePriority::Normal;
    }

    // Program change - normal priority
    if (message.isProgramChange()) {
        return MidiMessagePriority::Normal;
    }

    // Channel pressure - low priority
    if (message.isChannelPressure()) {
        return MidiMessagePriority::Low;
    }

    // Aftertouch - low priority
    if (message.isAftertouch()) {
        return MidiMessagePriority::Low;
    }

    // Default - normal priority
    return MidiMessagePriority::Normal;
}

//==============================================================================
// Private Methods
//==============================================================================
void MidiBufferOverflowProtection::updateStatistics(const juce::MidiBuffer& buffer) {
    int numMessages = buffer.getNumMessages();
    double usage = static_cast<double>(numMessages) / maxBufferSize_ * 100.0;

    statistics_.averageUsage = statistics_.averageUsage * 0.9 + usage * 0.1;
    statistics_.peakUsage = juce::jmax(statistics_.peakUsage, usage);
    statistics_.currentBufferSize = numMessages;
}

void MidiBufferOverflowProtection::reportOverflow(const MidiOverflowEvent& event) {
    // Update statistics
    if (event.type == MidiOverflowEvent::BufferFull ||
        event.type == MidiOverflowEvent::MessagesDropped ||
        event.type == MidiOverflowEvent::PriorityDrop) {
        statistics_.overflowsDetected++;
    }

    // Call callback if set
    if (overflowCallback_) {
        overflowCallback_(event);
    }

    // Log high-severity events
    if (event.severity >= 7.0) {
        std::cerr << "MidiBufferOverflowProtection: " << event.toString() << std::endl;
    }
}

} // namespace zenith
