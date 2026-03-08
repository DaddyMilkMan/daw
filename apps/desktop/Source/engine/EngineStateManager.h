/*
  ==============================================================================

    EngineStateManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #11)

    Manages audio engine state lifecycle with safe transitions.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

namespace zenith {

//==============================================================================
/**
 * @brief Audio engine state
 */
enum class EngineState {
    Uninitialized,     // Engine not initialized
    Initializing,      // Engine is initializing
    Idle,              // Engine ready, not playing
    Playing,           // Engine is playing
    Paused,            // Engine is paused
    Recording,         // Engine is recording
    Stopping,          // Engine is stopping
    ShuttingDown,      // Engine is shutting down
    ShutDown,          // Engine has shut down
    Error              // Engine in error state
};

//==============================================================================
/**
 * @brief State transition result
 */
struct StateTransitionResult {
    bool success = false;
    EngineState fromState = EngineState::Uninitialized;
    EngineState toState = EngineState::Uninitialized;
    juce::String errorMessage;
    juce::String warningMessage;

    juce::String toString() const {
        if (success) {
            return "State transition successful: " +
                   getStateString(fromState) + " -> " +
                   getStateString(toState);
        } else {
            return "State transition failed: " +
                   getStateString(fromState) + " -> " +
                   getStateString(toState) +
                   (errorMessage.isNotEmpty() ? " (" + errorMessage + ")" : "");
        }
    }

    static juce::String getStateString(EngineState state) {
        switch (state) {
            case EngineState::Uninitialized: return "Uninitialized";
            case EngineState::Initializing: return "Initializing";
            case EngineState::Idle: return "Idle";
            case EngineState::Playing: return "Playing";
            case EngineState::Paused: return "Paused";
            case EngineState::Recording: return "Recording";
            case EngineState::Stopping: return "Stopping";
            case EngineState::ShuttingDown: return "ShuttingDown";
            case EngineState::ShutDown: return "ShutDown";
            case EngineState::Error: return "Error";
        }
        return "Unknown";
    }
};

//==============================================================================
/**
 * @brief State transition event
 */
struct StateTransitionEvent {
    EngineState fromState = EngineState::Uninitialized;
    EngineState toState = EngineState::Uninitialized;
    double timestamp = 0.0;
    bool success = false;
    juce::String details;

    juce::String toString() const {
        return StateTransitionResult::getStateString(fromState) +
               " -> " +
               StateTransitionResult::getStateString(toState) +
               (success ? " [SUCCESS]" : " [FAILED]") +
               (details.isNotEmpty() ? " " + details : "");
    }
};

//==============================================================================
/**
 * @brief Engine state configuration
 */
struct EngineStateConfig {
    double initializationTimeoutSeconds = 10.0;
    double shutdownTimeoutSeconds = 5.0;
    bool enableStateValidation = true;
    bool enableStateLogging = true;
    bool allowDirectTransitions = false;  // If true, allow any transition
};

//==============================================================================
/**
 * @brief Engine state manager
 *
 * Features:
 * - State lifecycle management
 * - Safe state transitions
 * - State validation
 * - Transition history
 * - Rollback on failure
 * - Thread-safe state access
 */
class EngineStateManager {
public:
    //==========================================================================
    EngineStateManager();
    ~EngineStateManager();

    //==========================================================================
    /**
     * @brief Initialize the engine
     * @return Transition result
     */
    StateTransitionResult initialize();

    //==========================================================================
    /**
     * @brief Shutdown the engine
     * @return Transition result
     */
    StateTransitionResult shutdown();

    //==========================================================================
    /**
     * @brief Start playback
     * @return Transition result
     */
    StateTransitionResult startPlayback();

    //==========================================================================
    /**
     * @brief Stop playback
     * @return Transition result
     */
    StateTransitionResult stopPlayback();

    //==========================================================================
    /**
     * @brief Pause playback
     * @return Transition result
     */
    StateTransitionResult pause();

    //==========================================================================
    /**
     * @brief Resume from pause
     * @return Transition result
     */
    StateTransitionResult resume();

    //==========================================================================
    /**
     * @brief Start recording
     * @return Transition result
     */
    StateTransitionResult startRecording();

    //==========================================================================
    /**
     * @brief Stop recording
     * @return Transition result
     */
    StateTransitionResult stopRecording();

    //==========================================================================
    /**
     * @brief Request state transition
     * Use this for custom transitions
     * @param toState Desired state
     * @return Transition result
     */
    StateTransitionResult requestTransition(EngineState toState);

    //==========================================================================
    /**
     * @brief Get current state
     */
    EngineState getCurrentState() const {
        return currentState_.load();
    }

    //==========================================================================
    /**
     * @brief Check if engine is initialized
     */
    bool isInitialized() const {
        EngineState state = getCurrentState();
        return state != EngineState::Uninitialized &&
               state != EngineState::ShutDown &&
               state != EngineState::Error;
    }

    //==========================================================================
    /**
     * @brief Check if engine is playing
     */
    bool isPlaying() const {
        return getCurrentState() == EngineState::Playing;
    }

    //==========================================================================
    /**
     * @brief Check if engine is recording
     */
    bool isRecording() const {
        return getCurrentState() == EngineState::Recording;
    }

    //==========================================================================
    /**
     * @brief Check if engine is idle
     */
    bool isIdle() const {
        return getCurrentState() == EngineState::Idle;
    }

    //==========================================================================
    /**
     * @brief Check if engine is in error state
     */
    bool isError() const {
        return getCurrentState() == EngineState::Error;
    }

    //==========================================================================
    /**
     * @brief Get transition history
     */
    std::vector<StateTransitionEvent> getTransitionHistory() const;

    //==========================================================================
    /**
     * @brief Clear transition history
     */
    void clearHistory();

    //==========================================================================
    /**
     * @brief Get configuration
     */
    EngineStateConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const EngineStateConfig& config) {
        config_ = config;
    }

    //==========================================================================
    /**
     * @brief Register callback for state changes
     * @param callback Function to call when state changes
     */
    void setStateChangeCallback(std::function<void(EngineState, EngineState)> callback) {
        stateChangeCallback_ = callback;
    }

    //==========================================================================
    /**
     * @brief Register callback for entering a state
     * @param state State to watch
     * @param callback Function to call when entering state
     */
    void setEnterStateCallback(EngineState state,
                              std::function<void()> callback) {
        enterStateCallbacks_[state] = callback;
    }

    //==========================================================================
    /**
     * @brief Register callback for leaving a state
     * @param state State to watch
     * @param callback Function to call when leaving state
     */
    void setLeaveStateCallback(EngineState state,
                              std::function<void()> callback) {
        leaveStateCallbacks_[state] = callback;
    }

private:
    //==========================================================================
    StateTransitionResult performTransition(EngineState toState);
    bool isValidTransition(EngineState from, EngineState to) const;
    void recordTransition(const StateTransitionEvent& event);
    void callEnterCallback(EngineState state);
    void callLeaveCallback(EngineState state);

    //==========================================================================
    // Current state
    std::atomic<EngineState> currentState_{EngineState::Uninitialized};

    // Previous state (for rollback)
    EngineState previousState_ = EngineState::Uninitialized;

    // Configuration
    EngineStateConfig config_;

    // History
    std::vector<StateTransitionEvent> transitionHistory_;
    static constexpr int maxHistorySize = 100;
    mutable std::mutex historyMutex_;

    // Callbacks
    std::function<void(EngineState, EngineState)> stateChangeCallback_;
    std::map<EngineState, std::function<void()>> enterStateCallbacks_;
    std::map<EngineState, std::function<void()>> leaveStateCallbacks_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineStateManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for engine state manager
 */
class EngineStateManagerHolder {
public:
    static EngineStateManager& getInstance() {
        static EngineStateManager instance;
        return instance;
    }

    EngineStateManagerHolder(const EngineStateManagerHolder&) = delete;
    EngineStateManagerHolder& operator=(const EngineStateManagerHolder&) = delete;

private:
    EngineStateManagerHolder() = default;
    ~EngineStateManagerHolder() = default;
};

} // namespace zenith
