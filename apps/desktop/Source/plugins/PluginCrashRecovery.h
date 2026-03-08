/*
  ==============================================================================

    PluginCrashRecovery.h
    Created: 2026-02-19
    Month 10, Gap #2 - Plugin Crash Recovery

    Automatic crash detection, state preservation, and recovery.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

namespace zenith {

//==============================================================================
/**
 * @brief Crash event information
 */
struct PluginCrashEvent {
    juce::String pluginName;
    juce::String pluginId;
    juce::Time crashTime;
    juce::String crashReason;
    bool wasPlaying = false;
    double currentPosition = 0.0;
    juce::String stateSnapshot;  // Serialized plugin state

    juce::String toString() const {
        return juce::String::formatted(
            "Crash: %s (%s) at %s - Reason: %s, Position: %.2f",
            pluginName,
            pluginId,
            crashTime.toString(true, true),
            crashReason,
            currentPosition
        );
    }
};

//==============================================================================
/**
 * @brief Plugin state backup
 */
struct PluginStateBackup {
    juce::String pluginId;
    juce::MemoryBlock state;
    juce::Time backupTime;
    juce::String presetName;
    juce::uint64 checksum = 0;  // For integrity verification

    bool isValid() const {
        return !pluginId.isEmpty() && state.getSize() > 0;
    }

    juce::String toString() const {
        return juce::String::formatted(
            "Backup: %s, Size: %llu bytes, Time: %s",
            pluginId,
            state.getSize(),
            backupTime.toString(true, true)
        );
    }
};

//==============================================================================
/**
 * @brief Crash recovery configuration
 */
struct CrashRecoveryConfig {
    bool enableAutoBackup = true;           // Auto-backup state periodically
    juce::uint32 backupIntervalMs = 30000;  // Backup every 30 seconds
    bool enableAutoRestart = true;          // Auto-restart crashed plugins
    juce::uint32 maxRestartAttempts = 3;    // Max restart attempts
    juce::uint32 restartDelayMs = 1000;     // Delay between restarts
    bool enableNotification = true;         // Notify user of crashes
    bool saveCrashDumps = true;             // Save crash information
    juce::uint32 maxBackupsPerPlugin = 5;   // Keep last N backups
};

//==============================================================================
/**
 * @brief Plugin crash recovery manager
 *
 * Features:
 * - Automatic state backup before processing
 * - Crash detection and logging
 * - State restoration after crash
 * - Automatic plugin restart
 * - Crash history tracking
 * - User notification
 */
class PluginCrashRecovery {
public:
    //==========================================================================
    explicit PluginCrashRecovery(const CrashRecoveryConfig& config = {});
    ~PluginCrashRecovery();

    //==========================================================================
    /**
     * @brief Register plugin for crash recovery
     */
    void registerPlugin(juce::AudioPluginInstance* plugin,
                       const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Unregister plugin from crash recovery
     */
    void unregisterPlugin(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Create state backup for plugin
     */
    bool createBackup(const juce::String& pluginId);

    //==========================================================================
    /**
     * @brief Create state backup for all registered plugins
     */
    void createBackupForAll();

    //==========================================================================
    /**
     * @brief Report plugin crash
     */
    void reportCrash(juce::AudioPluginInstance* plugin,
                    const juce::String& reason,
                    double position = 0.0,
                    bool wasPlaying = false);

    //==========================================================================
    /**
     * @brief Attempt to recover crashed plugin
     */
    bool recoverPlugin(const juce::String& pluginId,
                      juce::AudioPluginInstance* newPluginInstance);

    //==========================================================================
    /**
     * @brief Get latest backup for plugin
     */
    PluginStateBackup getLatestBackup(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Get crash history
     */
    std::vector<PluginCrashEvent> getCrashHistory() const;

    //==========================================================================
    /**
     * @brief Get crash count for plugin
     */
    juce::uint32 getCrashCount(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Clear crash history
     */
    void clearCrashHistory();

    //==========================================================================
    /**
     * @brief Get all backups for plugin
     */
    std::vector<PluginStateBackup> getBackups(const juce::String& pluginId) const;

    //==========================================================================
    /**
     * @brief Set crash callback
     */
    using CrashCallback = std::function<void(const PluginCrashEvent&)>;
    void setCrashCallback(CrashCallback callback);

    //==========================================================================
    /**
     * @brief Set recovery callback
     */
    using RecoveryCallback = std::function<void(const juce::String&, bool)>;
    void setRecoveryCallback(RecoveryCallback callback);

    //==========================================================================
    /**
     * @brief Enable/disable auto-backup
     */
    void setAutoBackupEnabled(bool enabled);

    //==========================================================================
    /**
     * @brief Get singleton instance
     */
    static PluginCrashRecovery& getInstance();

private:
    //==========================================================================
    CrashRecoveryConfig config_;
    std::atomic<bool> autoBackupEnabled_{true};

    // Registered plugins
    struct PluginInfo {
        juce::AudioPluginInstance* instance;
        juce::String id;
        juce::String name;
        juce::Time lastBackup;
    };

    std::map<juce::String, PluginInfo> registeredPlugins_;
    mutable std::mutex pluginsMutex_;

    // State backups
    std::map<juce::String, std::vector<PluginStateBackup>> backups_;
    mutable std::mutex backupsMutex_;

    // Crash history
    std::vector<PluginCrashEvent> crashHistory_;
    mutable std::mutex historyMutex_;

    // Callbacks
    CrashCallback crashCallback_;
    RecoveryCallback recoveryCallback_;

    // Auto-backup timer
    class BackupTimer : public juce::Timer {
    public:
        BackupTimer(PluginCrashRecovery& owner) : owner_(owner) {}
        void timerCallback() override { owner_.onBackupTimer(); }
    private:
        PluginCrashRecovery& owner_;
    };
    std::unique_ptr<BackupTimer> backupTimer_;

    //==========================================================================
    void startBackupTimer();
    void stopBackupTimer();
    void onBackupTimer();

    juce::String getPluginId(juce::AudioPluginInstance* plugin) const;
    juce::MemoryBlock capturePluginState(juce::AudioPluginInstance* plugin) const;
    bool restorePluginState(juce::AudioPluginInstance* plugin,
                           const PluginStateBackup& backup) const;

    void saveCrashDump(const PluginCrashEvent& crash);
    juce::uint64 calculateChecksum(const juce::MemoryBlock& data) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCrashRecovery)
};

//==============================================================================
/**
 * @brief RAII crash recovery scope
 *
 * Automatically creates backup on scope entry and
 * handles crashes during scope.
 */
class CrashRecoveryScope {
public:
    CrashRecoveryScope(juce::AudioPluginInstance* plugin,
                      const juce::String& pluginId,
                      double position = 0.0,
                      bool playing = false)
        : plugin_(plugin)
        , pluginId_(pluginId)
        , position_(position)
        , playing_(playing)
        , recovery_(PluginCrashRecovery::getInstance()) {

        recovery_.createBackup(pluginId_);
    }

    ~CrashRecoveryScope() {
        // Scope completed successfully
    }

    void reportCrash(const juce::String& reason) {
        recovery_.reportCrash(plugin_, reason, position_, playing_);
    }

private:
    juce::AudioPluginInstance* plugin_;
    juce::String pluginId_;
    double position_;
    bool playing_;
    PluginCrashRecovery& recovery_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrashRecoveryScope)
};

} // namespace zenith
