/*
  ==============================================================================

    MidiLearnSafetyManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 6: MIDI Safety (Gap #6)

    Ensures safe MIDI learn functionality.

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
 * @brief MIDI controller assignment
 */
struct MidiControllerAssignment {
    juce::String id;               // Unique assignment ID
    juce::String parameterId;       // Parameter being controlled
    int channel = 0;                // MIDI channel (1-16)
    int controllerNumber = 0;       // CC number or note number
    bool isNote = false;            // true = note, false = CC
    int minValue = 0;               // Minimum input value
    int maxValue = 127;             // Maximum input value
    juce::String name;              // Assignment name
    juce::Time learnedAt;           // When learned

    juce::String toString() const {
        return (isNote ? "Note" : "CC") +
               juce::String(controllerNumber) +
               " [ch" + juce::String(channel) + "] -> " + parameterId;
    }
};

//==============================================================================
/**
 * @brief MIDI learn issue
 */
struct MidiLearnIssue {
    enum Type {
        AssignmentConflict,         // Assignment conflicts with existing
        InvalidController,          // Controller not valid for learn
        ChannelOutOfRange,          // Channel out of range
        LearnTimeout,               // Learn operation timed out
        DuplicateAssignment,        // Same assignment exists
        ParameterNotLearnable,      // Parameter can't be learned
        InvalidRange,               // Value range invalid
        LearnInterrupted             // Learn was cancelled
    };

    Type type;
    juce::String description;
    juce::String parameterId;
    juce::String conflictingAssignmentId;
    double severity = 0.0;         // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case AssignmentConflict: typeStr = "Conflict"; break;
            case InvalidController: typeStr = "Invalid Controller"; break;
            case ChannelOutOfRange: typeStr = "Channel Range"; break;
            case LearnTimeout: typeStr = "Timeout"; break;
            case DuplicateAssignment: typeStr = "Duplicate"; break;
            case ParameterNotLearnable: typeStr = "Not Learnable"; break;
            case InvalidRange: typeStr = "Invalid Range"; break;
            case LearnInterrupted: typeStr = "Interrupted"; break;
        }
        return "[" + typeStr + "] " + description +
               (parameterId.isNotEmpty() ? " (" + parameterId + ")" : "");
    }
};

//==============================================================================
/**
 * @brief MIDI learn state
 */
enum class MidiLearnState {
    Idle,                        // Not learning
    WaitingForMessage,           // Waiting for MIDI input
    Learning,                    // Currently learning
    Complete,                    // Learn complete
    Timeout,                     // Learn timed out
    Cancelled                    // Learn cancelled
};

//==============================================================================
/**
 * @brief MIDI learn statistics
 */
struct MidiLearnStatistics {
    int totalAssignments = 0;
    int successfulLearns = 0;
    int conflictsDetected = 0;
    int timeouts = 0;
    double averageLearnTime = 0.0;

    juce::String toString() const {
        return "MIDI Learn: " + juce::String(totalAssignments) + " assignments, " +
               juce::String(conflictsDetected) + " conflicts";
    }
};

//==============================================================================
/**
 * @brief Manages MIDI learn safety
 *
 * Features:
 * - Assignment validation
 * - Conflict detection
 * - Learn timeout
 * - Duplicate prevention
 * - Range validation
 * - Zone filtering
 */
class MidiLearnSafetyManager {
public:
    //==========================================================================
    MidiLearnSafetyManager();
    ~MidiLearnSafetyManager();

    //==========================================================================
    /**
     * @brief Start MIDI learn for parameter
     * @param parameterId Parameter to learn
     * @param timeoutSeconds Timeout (0 = no timeout)
     * @return true if started
     */
    bool startLearn(const juce::String& parameterId,
                   double timeoutSeconds = 10.0);

    //==========================================================================
    /**
     * @brief Process MIDI message during learn
     * @param message MIDI message
     * @return Assignment if learn complete, invalid otherwise
     */
    MidiControllerAssignment processLearnMessage(const juce::MidiMessage& message);

    //==========================================================================
    /**
     * @brief Cancel active learn
     */
    void cancelLearn();

    //==========================================================================
    /**
     * @brief Get current learn state
     * @return Current state
     */
    MidiLearnState getLearnState() const { return learnState_; }

    //==========================================================================
    /**
     * @brief Add assignment manually
     * @param assignment Assignment to add
     * @return Validation result
     */
    std::vector<MidiLearnIssue> addAssignment(const MidiControllerAssignment& assignment);

    //==========================================================================
    /**
     * @brief Remove assignment
     * @param assignmentId Assignment to remove
     * @return true if removed
     */
    bool removeAssignment(const juce::String& assignmentId);

    //==========================================================================
    /**
     * @brief Get assignment for parameter
     * @param parameterId Parameter ID
     * @return Assignment (invalid if not found)
     */
    MidiControllerAssignment getAssignmentForParameter(const juce::String& parameterId) const;

    //==========================================================================
    /**
     * @brief Get all assignments
     * @return List of all assignments
     */
    std::vector<MidiControllerAssignment> getAllAssignments() const;

    //==========================================================================
    /**
     * @brief Check for assignment conflicts
     * @param assignment Assignment to check
     * @return true if conflict exists
     */
    bool hasConflict(const MidiControllerAssignment& assignment) const;

    //==========================================================================
    /**
     * @brief Set learn timeout
     * @param timeoutSeconds Timeout in seconds
     */
    void setLearnTimeout(double timeoutSeconds) {
        learnTimeout_ = juce::jmax(1.0, timeoutSeconds);
    }

    //==========================================================================
    /**
     * @brief Get learn statistics
     * @return Current statistics
     */
    MidiLearnStatistics getStatistics() const { return statistics_; }

    //==========================================================================
    /**
     * @brief Register callback for conflicts
     * @param callback Function to call when conflict detected
     */
    void setConflictCallback(std::function<void(const MidiLearnIssue&)> callback) {
        conflictCallback_ = callback;
    }

private:
    //==========================================================================
    void checkTimeout();
    bool validateAssignment(const MidiControllerAssignment& assignment,
                           std::vector<MidiLearnIssue>& issues) const;

    //==========================================================================
    // Learn state
    MidiLearnState learnState_ = MidiLearnState::Idle;
    juce::String currentParameterId_;
    juce::Time learnStartTime_;

    // Settings
    double learnTimeout_ = 10.0;

    // Assignments
    std::map<juce::String, MidiControllerAssignment> assignments_;

    // Statistics
    MidiLearnStatistics statistics_;

    // Callbacks
    std::function<void(const MidiLearnIssue&)> conflictCallback_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnSafetyManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for MIDI learn safety manager
 */
class MidiLearnSafetyManagerHolder {
public:
    static MidiLearnSafetyManager& getInstance() {
        static MidiLearnSafetyManager instance;
        return instance;
    }

    MidiLearnSafetyManagerHolder(const MidiLearnSafetyManagerHolder&) = delete;
    MidiLearnSafetyManagerHolder& operator=(const MidiLearnSafetyManagerHolder&) = delete;

private:
    MidiLearnSafetyManagerHolder() = default;
    ~MidiLearnSafetyManagerHolder() = default;
};

} // namespace zenith
