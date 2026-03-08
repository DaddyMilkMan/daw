/*
  ==============================================================================

    StateTransitionValidator.cpp
    Implementation of state transition validation

  ==============================================================================
*/

#include "StateTransitionValidator.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// StateTransitionValidator Implementation
//==============================================================================

StateTransitionValidator::StateTransitionValidator() {
    std::cout << "StateTransitionValidator: Initialized" << std::endl;
}

StateTransitionValidator::~StateTransitionValidator() {
    std::cout << "StateTransitionValidator: Shut down" << std::endl;
}

//==============================================================================
std::vector<TransitionValidationIssue> StateTransitionValidator::validateTransition(
    EngineState from,
    EngineState to) const
{
    std::vector<TransitionValidationIssue> allIssues;

    // Check basic validity
    if (!isValidTransition(from, to)) {
        TransitionValidationIssue issue;
        issue.type = TransitionValidationIssue::InvalidTransition;
        issue.description = "This transition is not allowed";
        issue.fromState = from;
        issue.toState = to;
        issue.severity = 10;
        allIssues.push_back(issue);
        return allIssues;  // Don't continue if invalid
    }

    // Check prerequisites
    auto prereqIssues = checkPrerequisites(from, to);
    allIssues.insert(allIssues.end(), prereqIssues.begin(), prereqIssues.end());

    // Check locks
    auto lockIssues = checkLocks(from, to);
    allIssues.insert(allIssues.end(), lockIssues.begin(), lockIssues.end());

    // Run custom validation rules
    auto ruleIssues = runValidationRules(from, to);
    allIssues.insert(allIssues.end(), ruleIssues.begin(), ruleIssues.end());

    return allIssues;
}

//==============================================================================
bool StateTransitionValidator::isValidTransition(EngineState from, EngineState to) const {
    // Define valid transitions (same as EngineStateManager)
    switch (from) {
        case EngineState::Uninitialized:
            return to == EngineState::Initializing;

        case EngineState::Initializing:
            return to == EngineState::Idle ||
                   to == EngineState::Error ||
                   to == EngineState::ShuttingDown;

        case EngineState::Idle:
            return to == EngineState::Playing ||
                   to == EngineState::Recording ||
                   to == EngineState::ShuttingDown ||
                   to == EngineState::Error;

        case EngineState::Playing:
            return to == EngineState::Idle ||
                   to == EngineState::Paused ||
                   to == EngineState::Stopping ||
                   to == EngineState::Error;

        case EngineState::Paused:
            return to == EngineState::Playing ||
                   to == EngineState::Idle ||
                   to == EngineState::Stopping ||
                   to == EngineState::Error;

        case EngineState::Recording:
            return to == EngineState::Idle ||
                   to == EngineState::Stopping ||
                   to == EngineState::Error;

        case EngineState::Stopping:
            return to == EngineState::Idle ||
                   to == EngineState::Error;

        case EngineState::ShuttingDown:
            return to == EngineState::ShutDown ||
                   to == EngineState::Error;

        case EngineState::ShutDown:
            return false;  // Can't transition from shut down

        case EngineState::Error:
            return to == EngineState::Idle ||
                   to == EngineState::ShuttingDown;
    }

    return false;
}

//==============================================================================
void StateTransitionValidator::addPrerequisite(
    EngineState from,
    EngineState to,
    const TransitionPrerequisite& prereq)
{
    auto key = std::make_pair(from, to);
    prerequisites_[key].push_back(prereq);
}

//==============================================================================
void StateTransitionValidator::removePrerequisites(EngineState from, EngineState to) {
    auto key = std::make_pair(from, to);
    prerequisites_.erase(key);
}

//==============================================================================
std::vector<TransitionPrerequisite> StateTransitionValidator::getPrerequisites(
    EngineState from,
    EngineState to) const
{
    auto key = std::make_pair(from, to);
    auto it = prerequisites_.find(key);
    if (it != prerequisites_.end()) {
        return it->second;
    }
    return {};
}

//==============================================================================
void StateTransitionValidator::lockState(EngineState state, const juce::String& reason) {
    stateLocks_[state] = reason;
}

//==============================================================================
void StateTransitionValidator::unlockState(EngineState state) {
    stateLocks_.erase(state);
}

//==============================================================================
bool StateTransitionValidator::isStateLocked(EngineState state) const {
    return stateLocks_.find(state) != stateLocks_.end();
}

//==============================================================================
juce::String StateTransitionValidator::getLockReason(EngineState state) const {
    auto it = stateLocks_.find(state);
    if (it != stateLocks_.end()) {
        return it->second;
    }
    return "";
}

//==============================================================================
void StateTransitionValidator::addValidationRule(
    EngineState from,
    EngineState to,
    std::function<bool()> validator,
    const juce::String& errorMessage)
{
    auto key = std::make_pair(from, to);
    validationRules_[key].push_back({validator, errorMessage});
}

//==============================================================================
void StateTransitionValidator::removeValidationRules(EngineState from, EngineState to) {
    auto key = std::make_pair(from, to);
    validationRules_.erase(key);
}

//==============================================================================
// Private Methods
//==============================================================================

std::vector<TransitionValidationIssue> StateTransitionValidator::checkPrerequisites(
    EngineState from,
    EngineState to) const
{
    std::vector<TransitionValidationIssue> issues;

    auto key = std::make_pair(from, to);
    auto it = prerequisites_.find(key);
    if (it == prerequisites_.end()) {
        return issues;  // No prerequisites
    }

    for (const auto& prereq : it->second) {
        if (!prereq.isMet()) {
            TransitionValidationIssue issue;
            issue.type = TransitionValidationIssue::MissingPrerequisite;
            issue.description = prereq.failureMessage.isNotEmpty() ?
                               prereq.failureMessage :
                               "Prerequisite not met: " + prereq.name;
            issue.fromState = from;
            issue.toState = to;
            issue.severity = prereq.required ? 8 : 4;
            issues.push_back(issue);

            if (prereq.required && strictMode_) {
                // In strict mode, this is a hard failure
                // But we still continue checking to report all issues
            }
        }
    }

    return issues;
}

std::vector<TransitionValidationIssue> StateTransitionValidator::runValidationRules(
    EngineState from,
    EngineState to) const
{
    std::vector<TransitionValidationIssue> issues;

    // Check specific rules
    auto key = std::make_pair(from, to);
    auto it = validationRules_.find(key);
    if (it != validationRules_.end()) {
        for (const auto& rule : it->second) {
            if (!rule.first()) {
                TransitionValidationIssue issue;
                issue.type = TransitionValidationIssue::UnsafeTransition;
                issue.description = rule.second;
                issue.fromState = from;
                issue.toState = to;
                issue.severity = strictMode_ ? 8 : 5;
                issues.push_back(issue);
            }
        }
    }

    // Check wildcard rules (from = Error means all states)
    auto wildcardKey = std::make_pair(EngineState::Error, to);
    auto wildcardIt = validationRules_.find(wildcardKey);
    if (wildcardIt != validationRules_.end()) {
        for (const auto& rule : wildcardIt->second) {
            if (!rule.first()) {
                TransitionValidationIssue issue;
                issue.type = TransitionValidationIssue::UnsafeTransition;
                issue.description = rule.second;
                issue.fromState = from;
                issue.toState = to;
                issue.severity = strictMode_ ? 8 : 5;
                issues.push_back(issue);
            }
        }
    }

    return issues;
}

std::vector<TransitionValidationIssue> StateTransitionValidator::checkLocks(
    EngineState from,
    EngineState to) const
{
    std::vector<TransitionValidationIssue> issues;

    // Check if source state is locked
    auto it = stateLocks_.find(from);
    if (it != stateLocks_.end()) {
        TransitionValidationIssue issue;
        issue.type = TransitionValidationIssue::StateLock;
        issue.description = "Cannot leave state - locked: " + it->second;
        issue.fromState = from;
        issue.toState = to;
        issue.severity = 9;
        issues.push_back(issue);
    }

    return issues;
}

} // namespace zenith
