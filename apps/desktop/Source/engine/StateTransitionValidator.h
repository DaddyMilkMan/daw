/*
  ==============================================================================

    StateTransitionValidator.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #11)

    Validates audio engine state transitions for safety.

  ==============================================================================
*/

#pragma once

#include "EngineStateManager.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Transition validation issue
 */
struct TransitionValidationIssue {
    enum Type {
        InvalidTransition,        // Transition not allowed
        UnsafeTransition,         // Transition might cause problems
        MissingPrerequisite,     // Required condition not met
        ResourceConflict,        // Resource would conflict
        StateLock,               // State is locked
        Timeout,                 // Transition would timeout
        Unknown
    };

    Type type;
    juce::String description;
    EngineState fromState;
    EngineState toState;
    int severity = 0;  // 0-10

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case InvalidTransition: typeStr = "Invalid"; break;
            case UnsafeTransition: typeStr = "Unsafe"; break;
            case MissingPrerequisite: typeStr = "Missing Prereq"; break;
            case ResourceConflict: typeStr = "Resource Conflict"; break;
            case StateLock: typeStr = "State Locked"; break;
            case Timeout: typeStr = "Timeout"; break;
            case Unknown: typeStr = "Unknown"; break;
        }
        return "[" + typeStr + "] " +
               StateTransitionResult::getStateString(fromState) +
               " -> " +
               StateTransitionResult::getStateString(toState) +
               ": " + description;
    }
};

//==============================================================================
/**
 * @brief Prerequisite check function
 * Returns true if prerequisite is met
 */
using PrerequisiteCheck = std::function<bool()>;

//==============================================================================
/**
 * @brief Prerequisite definition
 */
struct TransitionPrerequisite {
    juce::String name;
    PrerequisiteCheck check;
    bool required = true;
    juce::String failureMessage;

    bool isMet() const {
        return check();
    }
};

//==============================================================================
/**
 * @brief State transition validator
 *
 * Features:
 * - Validates all state transitions
 * - Checks prerequisites
 * - Custom validation rules
 * - Transition locking
 * - Resource conflict detection
 */
class StateTransitionValidator {
public:
    //==========================================================================
    StateTransitionValidator();
    ~StateTransitionValidator();

    //==========================================================================
    /**
     * @brief Validate a state transition
     * @param from Current state
     * @param to Desired state
     * @return List of issues (empty if valid)
     */
    std::vector<TransitionValidationIssue> validateTransition(
        EngineState from,
        EngineState to) const;

    //==========================================================================
    /**
     * @brief Check if transition is valid
     * @return true if no issues
     */
    bool isValidTransition(EngineState from, EngineState to) const;

    //==========================================================================
    /**
     * @brief Add prerequisite for a transition
     * @param from Source state
     * @param to Destination state
     * @param prereq Prerequisite to check
     */
    void addPrerequisite(EngineState from, EngineState to,
                        const TransitionPrerequisite& prereq);

    //==========================================================================
    /**
     * @brief Remove all prerequisites for a transition
     */
    void removePrerequisites(EngineState from, EngineState to);

    //==========================================================================
    /**
     * @brief Get all prerequisites for a transition
     */
    std::vector<TransitionPrerequisite> getPrerequisites(
        EngineState from,
        EngineState to) const;

    //==========================================================================
    /**
     * @brief Lock a state (prevent leaving)
     * @param state State to lock
     * @param reason Reason for lock
     */
    void lockState(EngineState state, const juce::String& reason = "");

    //==========================================================================
    /**
     * @brief Unlock a state
     * @param state State to unlock
     */
    void unlockState(EngineState state);

    //==========================================================================
    /**
     * @brief Check if state is locked
     */
    bool isStateLocked(EngineState state) const;

    //==========================================================================
    /**
     * @brief Get lock reason
     */
    juce::String getLockReason(EngineState state) const;

    //==========================================================================
    /**
     * @brief Add custom validation rule
     * @param from Source state (Use EngineState::Error for all states)
     * @param to Destination state
     * @param validator Validation function (returns true if valid)
     * @param errorMessage Message to return if validation fails
     */
    void addValidationRule(EngineState from, EngineState to,
                          std::function<bool()> validator,
                          const juce::String& errorMessage);

    //==========================================================================
    /**
     * @brief Remove validation rules for a transition
     */
    void removeValidationRules(EngineState from, EngineState to);

    //==========================================================================
    /**
     * @brief Set strict mode
     * In strict mode, all warnings become errors
     */
    void setStrictMode(bool strict) {
        strictMode_ = strict;
    }

    //==========================================================================
    /**
     * @brief Get strict mode
     */
    bool isStrictMode() const {
        return strictMode_;
    }

private:
    //==========================================================================
    std::vector<TransitionValidationIssue> checkPrerequisites(
        EngineState from,
        EngineState to) const;

    std::vector<TransitionValidationIssue> runValidationRules(
        EngineState from,
        EngineState to) const;

    std::vector<TransitionValidationIssue> checkLocks(
        EngineState from,
        EngineState to) const;

    //==========================================================================
    // Prerequisites: (from, to) -> {prerequisites}
    std::map<std::pair<EngineState, EngineState>,
            std::vector<TransitionPrerequisite>> prerequisites_;

    // Validation rules: (from, to) -> {validator, errorMessage}
    std::map<std::pair<EngineState, EngineState>,
            std::vector<std::pair<std::function<bool()>, juce::String>>> validationRules_;

    // State locks: state -> lockReason
    std::map<EngineState, juce::String> stateLocks_;

    // Settings
    bool strictMode_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateTransitionValidator)
};

//==============================================================================
/**
 * @brief Singleton accessor for state transition validator
 */
class StateTransitionValidatorHolder {
public:
    static StateTransitionValidator& getInstance() {
        static StateTransitionValidator instance;
        return instance;
    }

    StateTransitionValidatorHolder(const StateTransitionValidatorHolder&) = delete;
    StateTransitionValidatorHolder& operator=(const StateTransitionValidatorHolder&) = delete;

private:
    StateTransitionValidatorHolder() = default;
    ~StateTransitionValidatorHolder() = default;
};

} // namespace zenith
