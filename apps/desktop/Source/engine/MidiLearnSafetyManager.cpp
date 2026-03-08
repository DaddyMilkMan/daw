/*
  ==============================================================================

    MidiLearnSafetyManager.cpp
    Implementation of MIDI learn safety

  ==============================================================================
*/

#include "MidiLearnSafetyManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// MidiLearnSafetyManager Implementation
//==============================================================================

MidiLearnSafetyManager::MidiLearnSafetyManager() {
    std::cout << "MidiLearnSafetyManager: Initialized" << std::endl;
}

MidiLearnSafetyManager::~MidiLearnSafetyManager() {
    std::cout << "MidiLearnSafetyManager: Shut down ("
              << statistics_.totalAssignments << " assignments)"
              << std::endl;
}

//==============================================================================
bool MidiLearnSafetyManager::startLearn(const juce::String& parameterId,
                                          double timeoutSeconds) {

    if (learnState_ == MidiLearnState::WaitingForMessage ||
        learnState_ == MidiLearnState::Learning) {
        std::cerr << "MidiLearnSafetyManager: Already learning" << std::endl;
        return false;
    }

    currentParameterId_ = parameterId;
    learnState_ = MidiLearnState::WaitingForMessage;
    learnStartTime_ = juce::Time::getCurrentTime();
    learnTimeout_ = timeoutSeconds;

    std::cout << "MidiLearnSafetyManager: Started learning '"
              << parameterId << "' (timeout: " << timeoutSeconds << "s)"
              << std::endl;

    return true;
}

//==============================================================================
MidiControllerAssignment MidiLearnSafetyManager::processLearnMessage(
    const juce::MidiMessage& message) {

    MidiControllerAssignment assignment;  // Invalid by default

    if (learnState_ != MidiLearnState::WaitingForMessage &&
        learnState_ != MidiLearnState::Learning) {
        return assignment;
    }

    // Check for timeout
    checkTimeout();
    if (learnState_ == MidiLearnState::Timeout) {
        return assignment;
    }

    // Only process note and CC messages
    if (!message.isNoteOnOrOff() && !message.isController()) {
        return assignment;  // Not learnable
    }

    // Create assignment
    assignment.id = currentParameterId_ + "_" + juce::String::toHexString(message.getTimeStamp());
    assignment.parameterId = currentParameterId_;
    assignment.channel = message.getChannel();
    assignment.learnedAt = juce::Time::getCurrentTime();

    if (message.isNoteOnOrOff()) {
        assignment.isNote = true;
        assignment.controllerNumber = message.getNoteNumber();
        assignment.name = "Note " + juce::String(message.getNoteNumber());
    } else if (message.isController()) {
        assignment.isNote = false;
        assignment.controllerNumber = message.getControllerNumber();
        assignment.name = "CC " + juce::String(message.getControllerNumber());
    }

    // Validate assignment
    std::vector<MidiLearnIssue> issues;
    bool isValid = validateAssignment(assignment, issues);

    if (!isValid) {
        // Check for conflicts
        for (const auto& issue : issues) {
            if (issue.type == MidiLearnIssue::AssignmentConflict) {
                statistics_.conflictsDetected++;

                if (conflictCallback_) {
                    conflictCallback_(issue);
                }
            }
        }

        learnState_ = MidiLearnState::Idle;
        return assignment;  // Invalid
    }

    // Add assignment
    assignments_[assignment.id] = assignment;
    learnState_ = MidiLearnState::Complete;

    statistics_.totalAssignments++;
    statistics_.successfulLearns++;

    juce::Time endTime = juce::Time::getCurrentTime();
    juce::RelativeTime duration = endTime - learnStartTime_;
    statistics_.averageLearnTime = (statistics_.averageLearnTime *
                                        (statistics_.totalAssignments - 1) +
                                        duration.inSeconds()) /
                                       statistics_.totalAssignments;

    std::cout << "MidiLearnSafetyManager: Learned "
              << assignment.toString() << std::endl;

    return assignment;
}

//==============================================================================
void MidiLearnSafetyManager::cancelLearn() {
    if (learnState_ == MidiLearnState::WaitingForMessage ||
        learnState_ == MidiLearnState::Learning) {

        learnState_ = MidiLearnState::Cancelled;

        std::cout << "MidiLearnSafetyManager: Learn cancelled for '"
                  << currentParameterId_ << "'" << std::endl;
    }
}

//==============================================================================
std::vector<MidiLearnIssue> MidiLearnSafetyManager::addAssignment(
    const MidiControllerAssignment& assignment) {

    std::vector<MidiLearnIssue> issues;

    // Validate assignment
    bool isValid = validateAssignment(assignment, issues);

    if (!isValid) {
        for (const auto& issue : issues) {
            if (issue.severity >= 7.0) {
                return issues;  // Don't add invalid assignment
            }
        }
    }

    // Check for duplicates
    for (const auto& entry : assignments_) {
        const auto& existing = entry.second;

        if (existing.channel == assignment.channel &&
            existing.controllerNumber == assignment.controllerNumber &&
            existing.isNote == assignment.isNote) {

            MidiLearnIssue issue;
            issue.type = MidiLearnIssue::DuplicateAssignment;
            issue.description = "Controller already assigned to " +
                               existing.parameterId;
            issue.parameterId = assignment.parameterId;
            issue.conflictingAssignmentId = existing.id;
            issue.severity = 6.0;
            issues.push_back(issue);

            return issues;
        }
    }

    // Add assignment
    assignments_[assignment.id] = assignment;
    statistics_.totalAssignments++;

    std::cout << "MidiLearnSafetyManager: Added assignment "
              << assignment.toString() << std::endl;

    return issues;
}

//==============================================================================
bool MidiLearnSafetyManager::removeAssignment(const juce::String& assignmentId) {
    auto it = assignments_.find(assignmentId);
    if (it != assignments_.end()) {
        std::cout << "MidiLearnSafetyManager: Removed assignment "
                  << it->second.toString() << std::endl;
        assignments_.erase(it);
        return true;
    }
    return false;
}

//==============================================================================
MidiControllerAssignment MidiLearnSafetyManager::getAssignmentForParameter(
    const juce::String& parameterId) const {

    for (const auto& entry : assignments_) {
        if (entry.second.parameterId == parameterId) {
            return entry.second;
        }
    }

    return MidiControllerAssignment{};  // Invalid
}

//==============================================================================
std::vector<MidiControllerAssignment> MidiLearnSafetyManager::getAllAssignments() const {
    std::vector<MidiControllerAssignment> all;

    for (const auto& entry : assignments_) {
        all.push_back(entry.second);
    }

    return all;
}

//==============================================================================
bool MidiLearnSafetyManager::hasConflict(
    const MidiControllerAssignment& assignment) const {

    for (const auto& entry : assignments_) {
        const auto& existing = entry.second;

        // Same controller and channel
        if (existing.channel == assignment.channel &&
            existing.controllerNumber == assignment.controllerNumber &&
            existing.isNote == assignment.isNote) {

            // Different parameter = conflict
            if (existing.parameterId != assignment.parameterId) {
                return true;
            }
        }
    }

    return false;
}

//==============================================================================
// Private Methods
//==============================================================================
void MidiLearnSafetyManager::checkTimeout() {
    if (learnState_ != MidiLearnState::WaitingForMessage &&
        learnState_ != MidiLearnState::Learning) {
        return;
    }

    juce::Time currentTime = juce::Time::getCurrentTime();
    juce::RelativeTime elapsed = currentTime - learnStartTime_;
    double elapsedSeconds = elapsed.inSeconds();

    if (elapsedSeconds > learnTimeout_) {
        learnState_ = MidiLearnState::Timeout;
        statistics_.timeouts++;

        std::cout << "MidiLearnSafetyManager: Learn timed out for '"
                  << currentParameterId_ << "'" << std::endl;
    }
}

bool MidiLearnSafetyManager::validateAssignment(
    const MidiControllerAssignment& assignment,
    std::vector<MidiLearnIssue>& issues) const {

    // Check channel range
    if (assignment.channel < 1 || assignment.channel > 16) {
        MidiLearnIssue issue;
        issue.type = MidiLearnIssue::ChannelOutOfRange;
        issue.description = "Channel " + juce::String(assignment.channel) +
                           " is out of range (must be 1-16)";
        issue.parameterId = assignment.parameterId;
        issue.severity = 8.0;
        issues.push_back(issue);
        return false;
    }

    // Check controller/note range
    if (assignment.controllerNumber < 0 || assignment.controllerNumber > 127) {
        MidiLearnIssue issue;
        issue.type = MidiLearnIssue::InvalidController;
        issue.description = "Controller number " +
                           juce::String(assignment.controllerNumber) +
                           " is out of range (must be 0-127)";
        issue.parameterId = assignment.parameterId;
        issue.severity = 8.0;
        issues.push_back(issue);
        return false;
    }

    // Check value range
    if (assignment.minValue < 0 || assignment.minValue > 127 ||
        assignment.maxValue < 0 || assignment.maxValue > 127) {

        MidiLearnIssue issue;
        issue.type = MidiLearnIssue::InvalidRange;
        issue.description = "Value range invalid: [" +
                           juce::String(assignment.minValue) +
                           ", " + juce::String(assignment.maxValue) + "]";
        issue.parameterId = assignment.parameterId;
        issue.severity = 7.0;
        issues.push_back(issue);
    }

    // Check for conflicts
    if (hasConflict(assignment)) {
        MidiLearnIssue issue;
        issue.type = MidiLearnIssue::AssignmentConflict;
        issue.description = "Controller already assigned to another parameter";
        issue.parameterId = assignment.parameterId;
        issue.severity = 7.0;
        issues.push_back(issue);
    }

    return issues.empty();
}

} // namespace zenith
