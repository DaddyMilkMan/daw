/*
  ==============================================================================

    MidiRecordingSafetyManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #9)

    Ensures safety during MIDI recording.

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
 * @brief Recorded MIDI note
 */
struct RecordedMidiNote {
    double startTime = 0.0;       // Seconds
    double duration = 0.0;        // Seconds
    int channel = 0;               // MIDI channel
    int noteNumber = 0;            // Note number
    int velocity = 0;               // Velocity

    juce::String toString() const {
        return "Note[ch=" + juce::String(channel) +
               ", note=" + juce::String(noteNumber) +
               ", vel=" + juce::String(velocity) +
               ", t=" + juce::String(startTime, 3) +
               ", dur=" + juce::String(duration, 3) + "]";
    }
};

//==============================================================================
/**
 * @brief MIDI recording issue
 */
struct MidiRecordingIssue {
    enum Type {
        TimestampInvalid,           // Timestamp out of sequence
        NoteIncomplete,             // Note without note-off
        VelocityOutOfRange,         // Velocity not 0-127
        ChannelInvalid,             // Channel not 1-16
    };

    Type type;
    juce::String description;
    double timestamp = 0.0;
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case TimestampInvalid: typeStr = "Invalid Timestamp"; break;
            case NoteIncomplete: typeStr = "Incomplete Note"; break;
            case VelocityOutOfRange: typeStr = "Velocity Range"; break;
            case ChannelInvalid: typeStr = "Channel Range"; break;
        }
        return "[" + typeStr + "] " + description +
               " (t=" + juce::String(timestamp, 3) + ")";
    }
};

//==============================================================================
/**
 * @brief MIDI recording statistics
 */
struct MidiRecordingStatistics {
    int notesRecorded = 0;
    int incompleteNotes = 0;
    int velocityErrors = 0;
    int channelErrors = 0;
    double averageNoteDuration = 0.0;

    juce::String toString() const {
        return "MIDI Recording: " + juce::String(notesRecorded) + " notes, " +
               juce::String(incompleteNotes) + " incomplete";
    }
};

//==============================================================================
/**
 * @brief Manages MIDI recording safety
 *
 * Features:
 * - Recording timestamp validation
 * - Note duration correction
 * - Velocity filtering
 * - Quantization error detection
 * - Buffer overflow detection during recording
 * - Multi-take recording safety
 */
class MidiRecordingSafetyManager {
public:
    //==========================================================================
    MidiRecordingSafetyManager();
    ~MidiRecordingSafetyManager();

    //==========================================================================
    /**
     * @brief Start MIDI recording
     * @param trackId Track identifier
     * @return true if started
     */
    bool startRecording(const juce::String& trackId);

    //==========================================================================
    /**
     * @brief Process MIDI message during recording
     * @param message MIDI message
     * @param timestamp Message timestamp (seconds)
     * @return List of issues detected
     */
    std::vector<MidiRecordingIssue> processMessage(
        const juce::MidiMessage& message,
        double timestamp);

    //==========================================================================
    /**
     * @brief Stop recording and get validation result
     * @return List of issues found
     */
    std::vector<MidiRecordingIssue> stopRecording();

    //==========================================================================
    /**
     * @brief Get recorded notes
     * @return List of recorded notes
     */
    std::vector<RecordedMidiNote> getRecordedNotes() const {
        return recordedNotes_;
    }

    //==========================================================================
    /**
     * @brief Validate and fix recorded notes
     * @param notes Notes to validate (will be modified)
     * @return List of issues found
     */
    std::vector<MidiRecordingIssue> validateAndFixNotes(
        std::vector<RecordedMidiNote>& notes) const;

    //==========================================================================
    /**
     * @brief Get recording statistics
     * @return Current statistics
     */
    MidiRecordingStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Get recording state
     * @return true if recording
     */
    bool isRecording() const { return isRecording_; }

    //==========================================================================
    /**
     * @brief Get current track ID
     * @return Track ID
     */
    juce::String getTrackId() const { return currentTrackId_; }

private:
    //==========================================================================
    void updateStatistics(const RecordedMidiNote& note);

    //==========================================================================
    // Recording state
    bool isRecording_ = false;
    juce::String currentTrackId_;

    // Recorded notes
    std::vector<RecordedMidiNote> recordedNotes_;

    // Active notes (channel, noteNumber) -> start time
    std::map<std::pair<int, int>, double> activeNotes_;

    // Statistics
    MidiRecordingStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRecordingSafetyManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for MIDI recording safety manager
 */
class MidiRecordingSafetyManagerHolder {
public:
    static MidiRecordingSafetyManager& getInstance() {
        static MidiRecordingSafetyManager instance;
        return instance;
    }

    MidiRecordingSafetyManagerHolder(const MidiRecordingSafetyManagerHolder&) = delete;
    MidiRecordingSafetyManagerHolder& operator=(const MidiRecordingSafetyManagerHolder&) = delete;

private:
    MidiRecordingSafetyManagerHolder() = default;
    ~MidiRecordingSafetyManagerHolder() = default;
};

} // namespace zenith
