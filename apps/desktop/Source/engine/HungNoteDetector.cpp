/*
  ==============================================================================

    HungNoteDetector.cpp
    Implementation of hung note detection and resolution

  ==============================================================================
*/

#include "HungNoteDetector.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// HungNoteDetector Implementation
//==============================================================================

HungNoteDetector::HungNoteDetector() {
    std::cout << "HungNoteDetector: Initialized (timeout: "
              << hungNoteTimeout_ << "s)" << std::endl;
}

HungNoteDetector::~HungNoteDetector() {
    // Clear any remaining active notes
    if (!activeNotes_.empty()) {
        std::cerr << "HungNoteDetector: Warning - "
                  << activeNotes_.size() << " notes still active at shutdown"
                  << std::endl;
    }

    std::cout << "HungNoteDetector: Shut down ("
              << statistics_.stuckNotesResolved << " stuck notes resolved)"
              << std::endl;
}

//==============================================================================
std::vector<HungNoteEvent> HungNoteDetector::processMessage(
    const juce::MidiMessage& message) {

    std::vector<HungNoteEvent> hungNotes;

    // Update durations for all active notes
    updateNoteDurations();

    // Check for note-on
    if (message.isNoteOn()) {
        int channel = message.getChannel();
        int noteNumber = message.getNoteNumber();
        int velocity = message.getVelocity();

        // Ignore zero-velocity note-on (some devices use as note-off)
        if (velocity == 0) {
            // Treat as note-off
            removeActiveNote(channel, noteNumber);
            statistics_.totalNotesOff++;
        } else {
            ActiveNote note;
            note.channel = channel;
            note.noteNumber = noteNumber;
            note.velocity = velocity;
            note.startTime = juce::Time::getCurrentTime();
            note.duration = 0.0;

            // Check if note is already active (repeated note-on)
            if (isNoteActive(channel, noteNumber)) {
                // Remove old note and add new one
                removeActiveNote(channel, noteNumber);
            }

            addActiveNote(note);
            statistics_.totalNotesOn++;
        }
    }
    // Check for note-off
    else if (message.isNoteOff()) {
        int channel = message.getChannel();
        int noteNumber = message.getNoteNumber();

        removeActiveNote(channel, noteNumber);
        statistics_.totalNotesOff++;
    }

    // Check for all-notes-off
    else if (message.isAllNotesOff()) {
        int channel = message.getChannel();
        clearActiveNotes();
        std::cout << "HungNoteDetector: All-notes-off received for channel "
                  << channel << std::endl;
    }

    // Check for reset
    else if (message.isReset()) {
        clearActiveNotes();
        std::cout << "HungNoteDetector: MIDI reset received" << std::endl;
    }

    // Check for hung notes
    if (autoResolve_) {
        hungNotes = checkForHungNotes(hungNoteTimeout_);

        // Auto-resolve if any found
        if (!hungNotes.empty()) {
            auto panicBuffer = sendPanic();
            std::cout << "HungNoteDetector: Auto-resolved "
                      << hungNotes.size() << " stuck notes" << std::endl;
        }
    }

    statistics_.currentActiveNotes = static_cast<int>(activeNotes_.size());

    return hungNotes;
}

//==============================================================================
std::vector<HungNoteEvent> HungNoteDetector::processBuffer(
    const juce::MidiBuffer& buffer) {

    std::vector<HungNoteEvent> allHungNotes;

    for (const auto& metadata : buffer) {
        const auto& message = metadata.getMessage();
        auto hungNotes = processMessage(message);
        allHungNotes.insert(allHungNotes.end(),
                           hungNotes.begin(),
                           hungNotes.end());
    }

    return allHungNotes;
}

//==============================================================================
std::vector<HungNoteEvent> HungNoteDetector::checkForHungNotes(
    double timeoutSeconds) const {

    std::vector<HungNoteEvent> hungNotes;
    juce::Time currentTime = juce::Time::getCurrentTime();

    for (const auto& entry : activeNotes_) {
        const auto& note = entry.second;

        if (note.duration >= timeoutSeconds) {
            HungNoteEvent event;
            event.note = note;
            event.detectedTime = currentTime;
            event.timeout = timeoutSeconds;
            hungNotes.push_back(event);
        }
    }

    return hungNotes;
}

//==============================================================================
std::vector<ActiveNote> HungNoteDetector::getActiveNotes() const {
    std::vector<ActiveNote> notes;

    for (const auto& entry : activeNotes_) {
        notes.push_back(entry.second);
    }

    return notes;
}

//==============================================================================
juce::MidiBuffer HungNoteDetector::sendAllNotesOff(int channel) {
    juce::MidiBuffer buffer;

    if (channel == 0) {
        // Send to all channels
        for (int ch = 1; ch <= 16; ++ch) {
            auto message = juce::MidiMessage::allNotesOff(ch);
            buffer.addEvent(message, 0);
        }
    } else {
        // Send to specific channel
        auto message = juce::MidiMessage::allNotesOff(channel);
        buffer.addEvent(message, 0);
    }

    std::cout << "HungNoteDetector: Sent all-notes-off for "
              << (channel == 0 ? "all channels" : juce::String(channel))
              << std::endl;

    return buffer;
}

//==============================================================================
juce::MidiBuffer HungNoteDetector::sendPanic(int channel) {
    juce::MidiBuffer buffer;

    if (channel == 0) {
        // Send to all channels
        for (int ch = 1; ch <= 16; ++ch) {
            // All-sound-off
            auto allSoundOff = juce::MidiMessage::allSoundOff(ch);
            buffer.addEvent(allSoundOff, 0);

            // Reset all controllers
            auto resetControllers = juce::MidiMessage::resetAllControllers(ch);
            buffer.addEvent(resetControllers, 0);

            // All-notes-off
            auto allNotesOff = juce::MidiMessage::allNotesOff(ch);
            buffer.addEvent(allNotesOff, 0);
        }
    } else {
        // Send to specific channel
        auto allSoundOff = juce::MidiMessage::allSoundOff(channel);
        buffer.addEvent(allSoundOff, 0);

        auto resetControllers = juce::MidiMessage::resetAllControllers(channel);
        buffer.addEvent(resetControllers, 0);

        auto allNotesOff = juce::MidiMessage::allNotesOff(channel);
        buffer.addEvent(allNotesOff, 0);
    }

    std::cout << "HungNoteDetector: Sent panic for "
              << (channel == 0 ? "all channels" : juce::String(channel))
              << std::endl;

    return buffer;
}

//==============================================================================
void HungNoteDetector::clearActiveNotes() {
    int count = static_cast<int>(activeNotes_.size());
    activeNotes_.clear();
    statistics_.currentActiveNotes = 0;

    if (count > 0) {
        std::cout << "HungNoteDetector: Cleared " << count
                  << " active notes" << std::endl;
    }
}

//==============================================================================
// Private Methods
//==============================================================================
void HungNoteDetector::updateNoteDurations() {
    juce::Time currentTime = juce::Time::getCurrentTime();

    for (auto& entry : activeNotes_) {
        ActiveNote& note = entry.second;
        RelativeTime elapsed = currentTime - note.startTime;
        note.duration = elapsed.inSeconds();
    }

    // Update average note duration
    if (!activeNotes_.empty()) {
        double totalDuration = 0.0;
        for (const auto& entry : activeNotes_) {
            totalDuration += entry.second.duration;
        }
        statistics_.averageNoteDuration = totalDuration / activeNotes_.size();
    }
}

bool HungNoteDetector::isNoteActive(int channel, int noteNumber) const {
    auto key = std::make_pair(channel, noteNumber);
    return activeNotes_.find(key) != activeNotes_.end();
}

void HungNoteDetector::addActiveNote(const ActiveNote& note) {
    auto key = std::make_pair(note.channel, note.noteNumber);
    activeNotes_[key] = note;
}

bool HungNoteDetector::removeActiveNote(int channel, int noteNumber) {
    auto key = std::make_pair(channel, noteNumber);
    auto it = activeNotes_.find(key);

    if (it != activeNotes_.end()) {
        activeNotes_.erase(it);
        return true;
    }

    return false;
}

} // namespace zenith
