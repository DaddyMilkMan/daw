/*
  ==============================================================================

    MidiRecordingSafetyManager.cpp
    Implementation of MIDI recording safety

  ==============================================================================
*/

#include "MidiRecordingSafetyManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// MidiRecordingSafetyManager Implementation
//==============================================================================

MidiRecordingSafetyManager::MidiRecordingSafetyManager() {
    std::cout << "MidiRecordingSafetyManager: Initialized" << std::endl;
}

MidiRecordingSafetyManager::~MidiRecordingSafetyManager() {
    std::cout << "MidiRecordingSafetyManager: Shut down" << std::endl;
}

//==============================================================================
bool MidiRecordingSafetyManager::startRecording(const juce::String& trackId) {
    if (isRecording_) {
        std::cerr << "MidiRecordingSafetyManager: Already recording" << std::endl;
        return false;
    }

    currentTrackId_ = trackId;
    isRecording_ = true;
    recordedNotes_.clear();
    activeNotes_.clear();

    std::cout << "MidiRecordingSafetyManager: Started recording on '"
              << trackId << "'" << std::endl;

    return true;
}

//==============================================================================
std::vector<MidiRecordingIssue> MidiRecordingSafetyManager::processMessage(
    const juce::MidiMessage& message,
    double timestamp) {

    std::vector<MidiRecordingIssue> issues;

    if (!isRecording_) {
        return issues;
    }

    // Process note-on
    if (message.isNoteOn() && message.getVelocity() > 0) {
        int channel = message.getChannel();
        int noteNumber = message.getNoteNumber();
        int velocity = message.getVelocity();

        // Validate channel
        if (channel < 1 || channel > 16) {
            MidiRecordingIssue issue;
            issue.type = MidiRecordingIssue::ChannelInvalid;
            issue.description = "Channel " + juce::String(channel) +
                               " is invalid (must be 1-16)";
            issue.timestamp = timestamp;
            issue.severity = 8.0;
            issues.push_back(issue);
            statistics_.channelErrors++;
            return issues;
        }

        // Validate velocity
        if (velocity < 1 || velocity > 127) {
            MidiRecordingIssue issue;
            issue.type = MidiRecordingIssue::VelocityOutOfRange;
            issue.description = "Velocity " + juce::String(velocity) +
                               " is invalid (must be 1-127)";
            issue.timestamp = timestamp;
            issue.severity = 6.0;
            issues.push_back(issue);
            statistics_.velocityErrors++;
            // Don't skip, just warn
        }

        // Check if note is already active
        auto key = std::make_pair(channel, noteNumber);
        if (activeNotes_.find(key) != activeNotes_.end()) {
            // Note-on without note-off - mark previous as incomplete
            double prevStartTime = activeNotes_[key];

            // Create incomplete note record
            for (auto& note : recordedNotes_) {
                if (note.channel == channel &&
                    note.noteNumber == noteNumber &&
                    std::abs(note.startTime - prevStartTime) < 0.001) {

                    note.duration = timestamp - prevStartTime;
                    if (note.duration < 0.001) {
                        note.duration = 0.0;  // Too short, mark as incomplete
                        statistics_.incompleteNotes++;
                    }
                    break;
                }
            }

            // Remove from active notes
            activeNotes_.erase(key);
        }

        // Start new note
        activeNotes_[key] = timestamp;

        RecordedMidiNote recordedNote;
        recordedNote.startTime = timestamp;
        recordedNote.channel = channel;
        recordedNote.noteNumber = noteNumber;
        recordedNote.velocity = velocity;
        recordedNote.duration = 0.0;  // Will be set on note-off

        recordedNotes_.push_back(recordedNote);
        statistics_.notesRecorded++;
    }

    // Process note-off
    else if (message.isNoteOff() || (message.isNoteOn() && message.getVelocity() == 0)) {
        int channel = message.getChannel();
        int noteNumber = message.getNoteNumber();

        auto key = std::make_pair(channel, noteNumber);
        auto it = activeNotes_.find(key);

        if (it != activeNotes_.end()) {
            double startTime = it->second;
            double duration = timestamp - startTime;

            // Find and update the note
            for (auto& note : recordedNotes_) {
                if (note.channel == channel &&
                    note.noteNumber == noteNumber &&
                    std::abs(note.startTime - startTime) < 0.001) {

                    note.duration = duration;
                    updateStatistics(note);
                    break;
                }
            }

            // Remove from active notes
            activeNotes_.erase(it);
        }
    }

    return issues;
}

//==============================================================================
std::vector<MidiRecordingIssue> MidiRecordingSafetyManager::stopRecording() {
    std::vector<MidiRecordingIssue> issues;

    if (!isRecording_) {
        return issues;
    }

    // Check for incomplete notes (still active)
    for (const auto& entry : activeNotes_) {
        double startTime = entry.second;

        MidiRecordingIssue issue;
        issue.type = MidiRecordingIssue::NoteIncomplete;
        issue.description = "Note still active when recording stopped";
        issue.timestamp = startTime;
        issue.severity = 5.0;
        issues.push_back(issue);

        // Mark as incomplete
        for (auto& note : recordedNotes_) {
            if (note.channel == entry.first.first &&
                note.noteNumber == entry.first.second &&
                std::abs(note.startTime - startTime) < 0.001) {

                note.duration = 0.0;
                statistics_.incompleteNotes++;
                break;
            }
        }
    }

    isRecording_ = false;
    currentTrackId_.clear();

    std::cout << "MidiRecordingSafetyManager: Stopped recording ("
              << recordedNotes_.size() << " notes)" << std::endl;

    return issues;
}

//==============================================================================
std::vector<MidiRecordingIssue> MidiRecordingSafetyManager::validateAndFixNotes(
    std::vector<RecordedMidiNote>& notes) const {

    std::vector<MidiRecordingIssue> issues;

    for (auto& note : notes) {
        // Validate channel
        if (note.channel < 1 || note.channel > 16) {
            MidiRecordingIssue issue;
            issue.type = MidiRecordingIssue::ChannelInvalid;
            issue.description = "Channel " + juce::String(note.channel) +
                               " is invalid";
            issue.severity = 8.0;
            issues.push_back(issue);

            // Fix: set to channel 1
            note.channel = 1;
        }

        // Validate note number
        if (note.noteNumber < 0 || note.noteNumber > 127) {
            MidiRecordingIssue issue;
            issue.type = MidiRecordingIssue::TimestampInvalid;
            issue.description = "Note number " + juce::String(note.noteNumber) +
                               " is invalid";
            issue.timestamp = note.startTime;
            issue.severity = 9.0;
            issues.push_back(issue);

            // Fix: clamp to valid range
            note.noteNumber = juce::jlimit(0, 127, note.noteNumber);
        }

        // Validate velocity
        if (note.velocity < 1 || note.velocity > 127) {
            MidiRecordingIssue issue;
            issue.type = MidiRecordingIssue::VelocityOutOfRange;
            issue.description = "Velocity " + juce::String(note.velocity) +
                               " is invalid";
            issue.severity = 6.0;
            issues.push_back(issue);

            // Fix: clamp to valid range
            note.velocity = juce::jlimit(1, 127, note.velocity);
        }

        // Validate timestamp
        if (note.startTime < 0.0) {
            MidiRecordingIssue issue;
            issue.type = MidiRecordingIssue::TimestampInvalid;
            issue.description = "Start time " + juce::String(note.startTime) +
                               " is invalid";
            issue.timestamp = note.startTime;
            issue.severity = 7.0;
            issues.push_back(issue);

            // Fix: set to 0
            note.startTime = 0.0;
        }

        // Validate duration
        if (note.duration < 0.0) {
            MidiRecordingIssue issue;
            issue.type = MidiRecordingIssue::NoteIncomplete;
            issue.description = "Negative duration: " + juce::String(note.duration);
            issue.severity = 5.0;
            issues.push_back(issue);

            // Fix: set to 0 (incomplete)
            note.duration = 0.0;
        }
    }

    return issues;
}

//==============================================================================
// Private Methods
//==============================================================================
void MidiRecordingSafetyManager::updateStatistics(const RecordedMidiNote& note) {
    // Update average note duration
    if (statistics_.notesRecorded > 0) {
        statistics_.averageNoteDuration =
            (statistics_.averageNoteDuration * (statistics_.notesRecorded - 1) +
             note.duration) / statistics_.notesRecorded;
    }
}

} // namespace zenith
