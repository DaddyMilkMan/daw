/*
  ==============================================================================

    CrashRecoveryManager.h
    Created: 2026-02-18
    Author:  Zenith DAW - Month 7: Audio Engine Safety (Gap #12)

    Crash detection, auto-save, and state recovery for audio applications.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Crash event type
 */
enum class CrashEventType {
    CrashDetected,         // Application crash detected
    AutoSaveTriggered,     // Auto-save triggered
    StateSaved,            // State saved successfully
    StateRestored,         // State restored successfully
    RecoveryFailed,        // Recovery failed
    Unknown
};

//==============================================================================
/**
 * @brief Crash event
 */
struct CrashEvent {
    CrashEventType type;
    juce::String description;
    double timestamp = 0.0;
    juce::String filePath;  // For save/restore operations

    juce::String toString() const {
        juce::String typeStr;
        switch (type) {
            case CrashEventType::CrashDetected: typeStr = "Crash Detected"; break;
            case CrashEventType::AutoSaveTriggered: typeStr = "Auto-Save"; break;
            case CrashEventType::StateSaved: typeStr = "State Saved"; break;
            case CrashEventType::StateRestored: typeStr = "State Restored"; break;
            case CrashEventType::RecoveryFailed: typeStr = "Recovery Failed"; break;
            case CrashEventType::Unknown: typeStr = "Unknown"; break;
        }
        return "[" + typeStr + "] " + description;
    }
};

//==============================================================================
/**
 * @brief Crash recovery statistics
 */
struct CrashRecoveryStatistics {
    int crashesDetected = 0;
    int autoSavesTriggered = 0;
    int statesSaved = 0;
    int statesRestored = 0;
    int recoveriesFailed = 0;
    double lastSaveTime = 0.0;

    juce::String toString() const {
        return "Crash Recovery: " +
               juce::String(crashesDetected) + " crashes, " +
               juce::String(autoSavesTriggered) + " auto-saves, " +
               juce::String(statesRestored) + " restored";
    }
};

//==============================================================================
/**
 * @brief Crash recovery configuration
 */
struct CrashRecoveryConfig {
    double autoSaveIntervalSeconds = 60.0;      // Auto-save every minute
    int maxAutoSaveFiles = 10;                  // Keep last 10 auto-saves
    juce::File autoSaveDirectory;               // Directory for auto-saves
    bool enableAutoSave = true;
    bool enableCrashDetection = true;
    bool promptUserOnRecovery = true;           // Ask user before restoring
};

//==============================================================================
/**
 * @brief Crash recovery manager
 *
 * Features:
 * - Auto-save before risky operations
 * - Crash detection and logging
 * - Graceful recovery from crashes
 * - State restoration
 * - Audio buffer recovery
 */
class CrashRecoveryManager {
public:
    //==========================================================================
    CrashRecoveryManager();
    ~CrashRecoveryManager();

    //==========================================================================
    /**
     * @brief Initialize crash recovery
     * @param config Configuration
     * @return true if initialized successfully
     */
    bool initialize(const CrashRecoveryConfig& config);

    //==========================================================================
    /**
     * @brief Trigger manual save
     * @param reason Reason for save
     * @return true if saved successfully
     */
    bool triggerSave(const juce::String& reason = "Manual");

    //==========================================================================
    /**
     * @brief Trigger auto-save
     * @return true if saved successfully
     */
    bool triggerAutoSave();

    //==========================================================================
    /**
     * @brief Check for crash recovery files
     * @return List of recovery files available
     */
    std::vector<juce::File> findRecoveryFiles() const;

    //==========================================================================
    /**
     * @brief Get latest recovery file
     * @return Latest recovery file (or invalid if none)
     */
    juce::File getLatestRecoveryFile() const;

    //==========================================================================
    /**
     * @brief Restore state from file
     * @param file File to restore from
     * @return true if restored successfully
     */
    bool restoreState(const juce::File& file);

    //==========================================================================
    /**
     * @brief Restore from latest crash
     * @return true if restored successfully
     */
    bool restoreFromLatestCrash();

    //==========================================================================
    /**
     * @brief Detect if previous crash occurred
     * @return true if crash detected
     */
    bool detectPreviousCrash() const;

    //==========================================================================
    /**
     * @brief Mark application as running cleanly
     * Call this at startup to clear crash flag
     */
    void markCleanStartup();

    //==========================================================================
    /**
     * @brief Mark application as shutting down cleanly
     * Call this before exit
     */
    void markCleanShutdown();

    //==========================================================================
    /**
     * @brief Update (call periodically)
     * Handles auto-save timer
     */
    void update();

    //==========================================================================
    /**
     * @brief Get statistics
     */
    CrashRecoveryStatistics getStatistics() const {
        return statistics_;
    }

    //==========================================================================
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

    //==========================================================================
    /**
     * @brief Get configuration
     */
    CrashRecoveryConfig getConfig() const {
        return config_;
    }

    //==========================================================================
    /**
     * @brief Set configuration
     */
    void setConfig(const CrashRecoveryConfig& config);

    //==========================================================================
    /**
     * @brief Register callback for crash events
     * @param callback Function to call when crash event occurs
     */
    void setEventCallback(std::function<void(const CrashEvent&)> callback) {
        eventCallback_ = callback;
    }

    //==========================================================================
    /**
     * @brief Register state save function
     * @param saveFunction Function to call to save state
     */
    void setSaveFunction(std::function<juce::String()> saveFunction) {
        saveFunction_ = saveFunction;
    }

    //==========================================================================
    /**
     * @brief Register state restore function
     * @param restoreFunction Function to call to restore state
     */
    void setRestoreFunction(std::function<bool(const juce::String&)> restoreFunction) {
        restoreFunction_ = restoreFunction;
    }

private:
    //==========================================================================
    juce::File generateAutoSavePath() const;
    void cleanupOldAutoSaves();
    void recordEvent(const CrashEvent& event);
    juce::File getCrashFlagFile() const;

    //==========================================================================
    // Configuration
    CrashRecoveryConfig config_;

    // Statistics
    CrashRecoveryStatistics statistics_;

    // Timing
    double lastAutoSaveTime_ = 0.0;
    double startTime_ = 0.0;

    // Callbacks
    std::function<void(const CrashEvent&)> eventCallback_;
    std::function<juce::String()> saveFunction_;
    std::function<bool(const juce::String&)> restoreFunction_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrashRecoveryManager)
};

//==============================================================================
/**
 * @brief Singleton accessor for crash recovery manager
 */
class CrashRecoveryManagerHolder {
public:
    static CrashRecoveryManager& getInstance() {
        static CrashRecoveryManager instance;
        return instance;
    }

    CrashRecoveryManagerHolder(const CrashRecoveryManagerHolder&) = delete;
    CrashRecoveryManagerHolder& operator=(const CrashRecoveryManagerHolder&) = delete;

private:
    CrashRecoveryManagerHolder() = default;
    ~CrashRecoveryManagerHolder() = default;
};

} // namespace zenith
