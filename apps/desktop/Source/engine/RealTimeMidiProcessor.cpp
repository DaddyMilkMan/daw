/*
  ==============================================================================

    RealTimeMidiProcessor.cpp
    Implementation of real-time MIDI processing

  ==============================================================================
*/

#include "RealTimeMidiProcessor.h"
#include "MidiMessageValidator.h"
#include <iostream>
#include <chrono>

namespace zenith {

//==============================================================================
// RealTimeMidiProcessor Implementation
//==============================================================================

RealTimeMidiProcessor::RealTimeMidiProcessor() {
    std::cout << "RealTimeMidiProcessor: Initialized (max queue: "
              << maxQueueSize_ << ")" << std::endl;
}

RealTimeMidiProcessor::~RealTimeMidiProcessor() {
    std::cout << "RealTimeMidiProcessor: Shut down ("
              << statistics_.totalMessagesProcessed << " processed, "
              << statistics_.messagesDropped << " dropped)"
              << std::endl;
}

//==============================================================================
bool RealTimeMidiProcessor::submitMessage(const juce::MidiMessage& message,
                                               int priority) {

    // Validate message first
    auto& validator = MidiMessageValidatorHolder::getInstance();
    auto validationResult = validator.validateMessage(message);

    if (!validationResult.isValid && validationResult.getErrorCount() > 0) {
        statistics_.messagesDropped++;
        return false;
    }

    // Create prioritized message
    PrioritizedMidiMessage prioMsg;
    prioMsg.message = message;
    prioMsg.priority = priorityProcessingEnabled_ ? priority : calculatePriority(message);
    prioMsg.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

    // Lock and add to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);

        if (static_cast<int>(messageQueue_.size()) >= maxQueueSize_) {
            // Queue full
            statistics_.queueOverflows++;

            RealTimeMidiIssue issue;
            issue.type = RealTimeMidiIssue::QueueOverflow;
            issue.description = "MIDI queue full - message dropped";
            issue.severity = 8.0;
            reportIssue(issue);

            return false;
        }

        messageQueue_.push(prioMsg);
    }

    return true;
}

//==============================================================================
int RealTimeMidiProcessor::processMessages(int maxMessages,
                                           double sampleRate) {
    int processed = 0;

    while (processed < maxMessages) {
        // Get message from queue
        PrioritizedMidiMessage prioMsg;

        {
            std::lock_guard<std::mutex> lock(queueMutex_);

            if (messageQueue_.empty()) {
                break;  // No more messages
            }

            prioMsg = messageQueue_.top();
            messageQueue_.pop();
        }

        // Process message (measure time)
        auto startTime = std::chrono::high_resolution_clock::now();

        // In a real implementation, this would:
        // 1. Route message to appropriate plugin/track
        // 2. Apply any transformations
        // 3. Update synthesizer state

        auto endTime = std::chrono::high_resolution_clock::now();
        double processingTimeUs =
            std::chrono::duration<double, std::micro>(endTime - startTime).count();

        // Update statistics
        statistics_.totalMessagesProcessed++;
        statistics_.averageProcessingTime =
            (statistics_.averageProcessingTime.load() * 0.99 +
             processingTimeUs * 0.01);

        // Check for processing stall (processing too slow)
        if (processingTimeUs > 1000.0) {  // 1ms threshold
            RealTimeMidiIssue issue;
            issue.type = RealTimeMidiIssue::DeadlineMissed;
            issue.description = "Processing took " +
                               juce::String(processingTimeUs / 1000.0, 2) +
                               "ms (too slow)";
            issue.severity = juce::jmin(10.0, processingTimeUs / 200.0);
            reportIssue(issue);
        }

        processed++;
    }

    return processed;
}

//==============================================================================
// Private Methods
//==============================================================================
int RealTimeMidiProcessor::calculatePriority(
    const juce::MidiMessage& message) const {

    // Timing messages - highest priority
    if (message.isMidiClock() ||
        message.isSongPositionPointer() ||
        message.isMidiStart() ||
        message.isMidiStop() ||
        message.isMidiContinue()) {
        return 0;
    }

    // Note messages - high priority
    if (message.isNoteOn() || message.isNoteOff()) {
        return 1;
    }

    // Pitch bend - high priority
    if (message.isPitchWheel()) {
        return 1;
    }

    // Important CCs - medium-high priority
    if (message.isController()) {
        int cc = message.getControllerNumber();
        if (cc == 0 ||    // Bank select MSB
            cc == 32 ||   // Bank select LSB
            cc == 64 ||   // Sustain
            cc == 66 ||   // Sostenuto
            cc == 67) {   // Una corda
            return 2;
        }
        return 4;  // Other CCs
    }

    // Program change - medium priority
    if (message.isProgramChange()) {
        return 4;
    }

    // Channel pressure - low priority
    if (message.isChannelPressure()) {
        return 6;
    }

    // Aftertouch - low priority
    if (message.isAftertouch()) {
        return 7;
    }

    // Default
    return 5;
}

void RealTimeMidiProcessor::reportIssue(const RealTimeMidiIssue& issue) {
    // Update statistics
    if (issue.type == RealTimeMidiIssue::ThreadSafetyViolation) {
        statistics_.threadSafetyViolations++;
    }

    // Call callback if set
    if (issueCallback_) {
        issueCallback_(issue);
    }

    // Log high-severity issues
    if (issue.severity >= 7.0) {
        std::cerr << "RealTimeMidiProcessor: " << issue.toString() << std::endl;
    }
}

} // namespace zenith
