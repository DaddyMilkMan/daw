/*
  ==============================================================================

    EngineStateManager.cpp
    Implementation of audio engine state management

  ==============================================================================
*/

#include "EngineStateManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// EngineStateManager Implementation
//==============================================================================

EngineStateManager::EngineStateManager() {
    std::cout << "EngineStateManager: Initialized" << std::endl;
}

EngineStateManager::~EngineStateManager() {
    // Ensure we're shut down
    if (isInitialized()) {
        std::cerr << "EngineStateManager: Warning - engine not properly shut down" << std::endl;
        shutdown();
    }

    std::cout << "EngineStateManager: Shut down" << std::endl;
}

//==============================================================================
StateTransitionResult EngineStateManager::initialize() {
    StateTransitionResult result = performTransition(EngineState::Initializing);

    if (result.success) {
        // Initialization complete
        result = performTransition(EngineState::Idle);

        if (!result.success) {
            // Rollback
            performTransition(EngineState::Uninitialized);
        }
    }

    return result;
}

//==============================================================================
StateTransitionResult EngineStateManager::shutdown() {
    EngineState current = getCurrentState();

    // Can't shutdown if already shut down
    if (current == EngineState::Uninitialized ||
        current == EngineState::ShutDown) {
        StateTransitionResult result;
        result.success = false;
        result.fromState = current;
        result.toState = EngineState::ShutDown;
        result.errorMessage = "Engine is already shut down or never initialized";
        return result;
    }

    // First, stop whatever we're doing
    if (current == EngineState::Playing ||
        current == EngineState::Recording ||
        current == EngineState::Paused) {

        StateTransitionResult stopResult = stopPlayback();
        if (!stopResult.success) {
            // Couldn't stop - force shutdown anyway
            std::cerr << "EngineStateManager: Warning - forcing shutdown without stopping" << std::endl;
        }
    }

    // Transition to shutting down
    StateTransitionResult result = performTransition(EngineState::ShuttingDown);

    if (result.success) {
        // Shutdown complete
        result = performTransition(EngineState::ShutDown);
    }

    return result;
}

//==============================================================================
StateTransitionResult EngineStateManager::startPlayback() {
    return performTransition(EngineState::Playing);
}

//==============================================================================
StateTransitionResult EngineStateManager::stopPlayback() {
    EngineState current = getCurrentState();

    // From playing or recording, go to stopping first
    if (current == EngineState::Playing ||
        current == EngineState::Recording ||
        current == EngineState::Paused) {

        StateTransitionResult result = performTransition(EngineState::Stopping);

        if (result.success) {
            // Then go to idle
            return performTransition(EngineState::Idle);
        }

        return result;
    }

    // Already stopped
    if (current == EngineState::Idle) {
        StateTransitionResult result;
        result.success = true;
        result.fromState = current;
        result.toState = current;
        return result;
    }

    // Invalid transition
    StateTransitionResult result;
    result.success = false;
    result.fromState = current;
    result.toState = EngineState::Idle;
    result.errorMessage = "Cannot stop from current state";
    return result;
}

//==============================================================================
StateTransitionResult EngineStateManager::pause() {
    return performTransition(EngineState::Paused);
}

//==============================================================================
StateTransitionResult EngineStateManager::resume() {
    return performTransition(EngineState::Playing);
}

//==============================================================================
StateTransitionResult EngineStateManager::startRecording() {
    return performTransition(EngineState::Recording);
}

//==============================================================================
StateTransitionResult EngineStateManager::stopRecording() {
    return stopPlayback();  // Same as stopping playback
}

//==============================================================================
StateTransitionResult EngineStateManager::requestTransition(EngineState toState) {
    return performTransition(toState);
}

//==============================================================================
std::vector<StateTransitionEvent> EngineStateManager::getTransitionHistory() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    return transitionHistory_;
}

//==============================================================================
void EngineStateManager::clearHistory() {
    std::lock_guard<std::mutex> lock(historyMutex_);
    transitionHistory_.clear();
}

//==============================================================================
// Private Methods
//==============================================================================

StateTransitionResult EngineStateManager::performTransition(EngineState toState) {
    StateTransitionResult result;
    result.fromState = getCurrentState();
    result.toState = toState;

    // Validate transition
    if (config_.enableStateValidation && !isValidTransition(result.fromState, toState)) {
        result.success = false;
        result.errorMessage = "Invalid state transition";
        return result;
    }

    // Call leave callback for current state
    callLeaveCallback(result.fromState);

    // Perform transition
    previousState_ = result.fromState;
    currentState_.store(toState);

    // Call enter callback for new state
    callEnterCallback(toState);

    // Call state change callback
    if (stateChangeCallback_) {
        stateChangeCallback_(result.fromState, toState);
    }

    result.success = true;

    // Record transition
    StateTransitionEvent event;
    event.fromState = result.fromState;
    event.toState = toState;
    event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
    event.success = true;
    recordTransition(event);

    if (config_.enableStateLogging) {
        std::cout << "EngineStateManager: " << event.toString() << std::endl;
    }

    return result;
}

bool EngineStateManager::isValidTransition(EngineState from, EngineState to) const {
    // Allow direct transitions if configured
    if (config_.allowDirectTransitions) {
        return true;
    }

    // Define valid transitions
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

void EngineStateManager::recordTransition(const StateTransitionEvent& event) {
    std::lock_guard<std::mutex> lock(historyMutex_);

    transitionHistory_.push_back(event);

    // Limit history size
    if (static_cast<int>(transitionHistory_.size()) > maxHistorySize) {
        transitionHistory_.erase(transitionHistory_.begin());
    }
}

void EngineStateManager::callEnterCallback(EngineState state) {
    auto it = enterStateCallbacks_.find(state);
    if (it != enterStateCallbacks_.end() && it->second) {
        it->second();
    }
}

void EngineStateManager::callLeaveCallback(EngineState state) {
    auto it = leaveStateCallbacks_.find(state);
    if (it != leaveStateCallbacks_.end() && it->second) {
        it->second();
    }
}

} // namespace zenith
