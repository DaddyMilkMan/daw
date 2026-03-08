/*
  ==============================================================================

    HungNoteDetector.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #2)

    Detects and resolves stuck MIDI notes.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <chrono>

namespace zenith {

//==============================================================================
/**
 * @brief Active MIDI note tracking
 */
struct ActiveNote {
    int channel = 0;               // MIDI channel (1-16)
    int noteNumber = 0;            // Note number (0-127)
    int velocity = 0;              // Note-on velocity
    juce::Time startTime;          // When note was pressed
    double duration = 0.0;         // Current duration in seconds

    juce::String toString() const {
        return "Note[ch=" + juce::String(channel) +
               ", note=" + juce::String(noteNumber) +
               ", vel=" + juce::String(velocity) +
               ", dur=" + juce::String(duration, 1) + "s]";
    }
};

//==============================================================================
/**
 * @brief Hung note event
 */
struct HungNoteEvent {
    ActiveNote note;               // The stuck note
    juce::Time detectedTime;       // When it was detected
    double timeout = 0.0;          // How long before considered stuck

    juce::String toString() const {
        return "Hung Note: " + note.toString() +
               " (timeout: " + juce::String(timeout, 1) + "s)";
    }
};

//==============================================================================
/**
 * @brief Note tracking statistics
 */
struct NoteTrackingStatistics {
    int totalNotesOn = 0;
    int totalNotesOff = 0;
    int stuckNotesDetected = 0;
    int stuckNotesResolved = 0;
    double averageNoteDuration = 0.0;
    int currentActiveNotes = 0;

    juce::String toString() const {
        return "Note Tracking: " +
               juce::String(totalNotesOn) + " note-ons, " +
               juce::String(totalNotesOff) + " note-offs, " +
               juce::String(stuckNotesDetected) + " stuck, " +
               juce::String(currentActiveNotes) + " active";
    }
};

//==============================================================================
/**
 * @brief Detects and resolves hung MIDI notes
 *
 * Features:
 * - Note-on/off tracking per channel
 * - Stuck note detection (timeout-based)
 * - Automatic all-notes-off
 * - Per-voice note tracking
 * - Note duration monitoring
 * - Panic button functionality
 */
class HungNoteDetector {
public:
    //==========================================================================
    HungNoteDetector();
    ~HungNoteDetector();

    //==========================================================================
    /**
     * @brief Process MIDI message for note tracking
     * @param message MIDI message to process
     * @return List of hung notes detected (if any)
     */
    std::vector<HungNoteEvent> processMessage(const juce::MidiMessage& message);

    //==========================================================================
    /**
     * @brief Process MIDI buffer
     * @param buffer Buffer to process
     * @return List of hung notes detected
     */
    std::vector<HungNoteEvent> processBuffer(const juce::MidiBuffer& buffer);

    //==========================================================================
    /**
     * @brief Check for hung notes (manual check)
     * @param timeoutSeconds Timeout threshold
     * @return List of hung notes found
     */
    std::vector<HungNoteEvent> checkForHungNotes(double timeoutSeconds = 30.0) const;

    //==========================================================================
    /**
     * @brief Get all currently active notes
     * @return List of active notes
     */
    std::vector<ActiveNote> getActiveNotes() const;

    //==========================================================================
    /**
     * @brief Send all-notes-off for stuck notes
     * @param channel Channel to send to (0 = all channels)
     * @return MIDI buffer containing all-notes-off messages
     */
    juce::MidiBuffer sendAllNotesOff(int channel = 0);

    //==========================================================================
    /**
     * @brief Send panic (all-sound-off + reset all controllers)
     * @param channel Channel to send to (0 = all channels)
     * @return MIDI buffer containing panic messages
     */
    juce::MidiBuffer sendPanic(int channel = 0);

    //==========================================================================
    /**
     * @brief Clear all active notes (reset state)
     */
    void clearActiveNotes();

    //==========================================================================
    /**
     * @brief Set hung note timeout
     * @param timeoutSeconds Timeout in seconds
     */
    void setHungNoteTimeout(double timeoutSeconds) {
        hungNoteTimeout_ = juce::jmax(1.0, timeoutSeconds);
    }

    /**
     * @brief Get hung note timeout
     * @return Timeout in seconds
     */
    double getHungNoteTimeout() const { return hungNoteTimeout_; }

    //==========================================================================
    /**
     * @brief Enable/disable automatic hung note resolution
     * @param enable true to automatically resolve hung notes
     */
    void setAutoResolutionEnabled(bool enable) {
        autoResolve_ = enable;
    }

    /**
     * @brief Check if auto-resolution is enabled
     * @return true if enabled
     */
    bool isAutoResolutionEnabled() const { return autoResolve_; }

    //==========================================================================
    /**
     * @brief Get tracking statistics
     * @return Current statistics
     */
    NoteTrackingStatistics getStatistics() const {
        return statistics_;
    }

    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        statistics_ = NoteTrackingStatistics{};
    }

    //==========================================================================
    /**
     * @brief Register callback for hung note detection
     * @param callback Function to call when hung note detected
     */
    void setHungNoteCallback(std::function<void(const HungNoteEvent&)> callback) {
        hungNoteCallback_ = callback;
    }

private:
    //==========================================================================
    void updateNoteDurations();
    bool isNoteActive(int channel, int noteNumber) const;
    void addActiveNote(const ActiveNote& note);
    bool removeActiveNote(int channel, int noteNumber);

    //==========================================================================
    // Active notes per channel (key: channel_noteNumber)
    std::map<std::pair<int, int>, ActiveNote> activeNotes_;

    // Settings
    double hungNoteTimeout_ = 30.0;  // seconds
    bool autoResolve_ = true;

    // Callbacks
    std::function<void(const HungNoteEvent&)> hungNoteCallback_;

    // Statistics
    NoteTrackingStatistics statistics_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HungNoteDetector)
};

//==============================================================================
/**
 * @brief Singleton accessor for hung note detector
 */
class HungNoteDetectorHolder {
public:
    static HungNoteDetector& getInstance() {
        static HungNoteDetector instance;
        return instance;
    }

    HungNoteDetectorHolder(const HungNoteDetectorHolder&) = delete;
    HungNoteDetectorHolder& operator=(const HungNoteDetectorHolder&) = delete;

private:
    HungNoteDetectorHolder() = default;
    ~HungNoteDetectorHolder() = default;
};

} // namespace zenith
