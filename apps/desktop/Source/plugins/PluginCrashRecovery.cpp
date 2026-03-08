/*
  ==============================================================================

    PluginCrashRecovery.cpp
    Implementation of plugin crash recovery system

  ==============================================================================
*/

#include "PluginCrashRecovery.h"
#include <juce_events/juce_events.h>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace zenith {

//==============================================================================
PluginCrashRecovery::PluginCrashRecovery(const CrashRecoveryConfig& config)
    : config_(config) {

    std::cout << "PluginCrashRecovery: Initialized" << std::endl;
    std::cout << "  Auto-backup: " << (config_.enableAutoBackup ? "enabled" : "disabled") << std::endl;
    std::cout << "  Auto-restart: " << (config_.enableAutoRestart ? "enabled" : "disabled") << std::endl;
    std::cout << "  Backup interval: " << config_.backupIntervalMs << " ms" << std::endl;

    if (config_.enableAutoBackup) {
        startBackupTimer();
    }
}

//==============================================================================
PluginCrashRecovery::~PluginCrashRecovery() {
    stopBackupTimer();

    std::cout << "PluginCrashRecovery: Shut down" << std::endl;

    std::lock_guard<std::mutex> lock(historyMutex_);
    if (!crashHistory_.empty()) {
        std::cout << "  Total crashes recorded: " << crashHistory_.size() << std::endl;
    }
}

//==============================================================================
void PluginCrashRecovery::registerPlugin(juce::AudioPluginInstance* plugin,
                                        const juce::String& pluginId) {
    if (plugin == nullptr || pluginId.isEmpty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(pluginsMutex_);

    PluginInfo info;
    info.instance = plugin;
    info.id = pluginId;
    info.name = plugin->getName();
    info.lastBackup = juce::Time();

    registeredPlugins_[pluginId] = info;

    std::cout << "PluginCrashRecovery: Registered plugin " << info.name
              << " (" << pluginId << ")" << std::endl;

    // Create initial backup
    if (config_.enableAutoBackup) {
        createBackup(pluginId);
    }
}

//==============================================================================
void PluginCrashRecovery::unregisterPlugin(const juce::String& pluginId) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = registeredPlugins_.find(pluginId);
    if (it != registeredPlugins_.end()) {
        std::cout << "PluginCrashRecovery: Unregistered plugin "
                  << it->second.name << " (" << pluginId << ")" << std::endl;
        registeredPlugins_.erase(it);
    }
}

//==============================================================================
bool PluginCrashRecovery::createBackup(const juce::String& pluginId) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    auto it = registeredPlugins_.find(pluginId);
    if (it == registeredPlugins_.end()) {
        return false;
    }

    PluginInfo& info = it->second;

    // Capture plugin state
    juce::MemoryBlock state;
    info.instance->getStateInformation(state);

    if (state.getSize() == 0) {
        std::cerr << "PluginCrashRecovery: Failed to capture state for "
                  << info.name << std::endl;
        return false;
    }

    // Create backup
    PluginStateBackup backup;
    backup.pluginId = pluginId;
    backup.state = state;
    backup.backupTime = juce::Time::getCurrentTime();
    backup.presetName = info.instance->getProgramName(info.instance->getCurrentProgram());
    backup.checksum = calculateChecksum(state);

    // Store backup
    {
        std::lock_guard<std::mutex> lock(backupsMutex_);

        if (backups_[pluginId].size() >= config_.maxBackupsPerPlugin) {
            // Remove oldest backup
            backups_[pluginId].erase(backups_[pluginId].begin());
        }

        backups_[pluginId].push_back(backup);
    }

    info.lastBackup = backup.backupTime;

    std::cout << "PluginCrashRecovery: Created backup for "
              << info.name << " (" << state.getSize() << " bytes)" << std::endl;

    return true;
}

//==============================================================================
void PluginCrashRecovery::createBackupForAll() {
    std::lock_guard<std::mutex> lock(pluginsMutex_);

    for (const auto& pair : registeredPlugins_) {
        createBackup(pair.first);
    }
}

//==============================================================================
void PluginCrashRecovery::reportCrash(juce::AudioPluginInstance* plugin,
                                     const juce::String& reason,
                                     double position,
                                     bool wasPlaying) {
    if (plugin == nullptr) {
        return;
    }

    juce::String pluginId = getPluginId(plugin);

    // Create crash event
    PluginCrashEvent crash;
    crash.pluginName = plugin->getName();
    crash.pluginId = pluginId;
    crash.crashTime = juce::Time::getCurrentTime();
    crash.crashReason = reason;
    crash.wasPlaying = wasPlaying;
    crash.currentPosition = position;

    // Capture state before crash
    crash.stateSnapshot = "[State captured in backup system]";

    // Add to history
    {
        std::lock_guard<std::mutex> lock(historyMutex_);
        crashHistory_.push_back(crash);
    }

    // Create emergency backup
    createBackup(pluginId);

    std::cerr << "PluginCrashRecovery: CRASH - " << crash.toString() << std::endl;

    if (config_.saveCrashDumps) {
        saveCrashDump(crash);
    }

    // Notify callback
    if (crashCallback_) {
        crashCallback_(crash);
    }

    if (config_.enableNotification) {
        // TODO: Show user notification
        std::cerr << "PluginCrashRecovery: User should be notified of crash" << std::endl;
    }
}

//==============================================================================
bool PluginCrashRecovery::recoverPlugin(const juce::String& pluginId,
                                       juce::AudioPluginInstance* newPluginInstance) {
    if (newPluginInstance == nullptr) {
        std::cerr << "PluginCrashRecovery: Cannot recover - null plugin instance" << std::endl;
        return false;
    }

    // Get latest backup
    PluginStateBackup backup = getLatestBackup(pluginId);
    if (!backup.isValid()) {
        std::cerr << "PluginCrashRecovery: No backup found for " << pluginId << std::endl;
        return false;
    }

    // Verify checksum
    juce::uint64 calculatedChecksum = calculateChecksum(backup.state);
    if (calculatedChecksum != backup.checksum) {
        std::cerr << "PluginCrashRecovery: Backup checksum mismatch - data corrupted" << std::endl;
        return false;
    }

    // Restore state
    bool restored = restorePluginState(newPluginInstance, backup);

    if (restored) {
        std::cout << "PluginCrashRecovery: Successfully recovered "
                  << newPluginInstance->getName() << " from backup" << std::endl;

        // Re-register the plugin
        registerPlugin(newPluginInstance, pluginId);

        if (recoveryCallback_) {
            recoveryCallback_(pluginId, true);
        }

        return true;
    } else {
        std::cerr << "PluginCrashRecovery: Failed to restore state" << std::endl;

        if (recoveryCallback_) {
            recoveryCallback_(pluginId, false);
        }

        return false;
    }
}

//==============================================================================
PluginStateBackup PluginCrashRecovery::getLatestBackup(const juce::String& pluginId) const {
    std::lock_guard<std::mutex> lock(backupsMutex_);

    auto it = backups_.find(pluginId);
    if (it != backups_.end() && !it->second.empty()) {
        return it->second.back();
    }

    return PluginStateBackup{};
}

//==============================================================================
std::vector<PluginCrashEvent> PluginCrashRecovery::getCrashHistory() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    return crashHistory_;
}

//==============================================================================
juce::uint32 PluginCrashRecovery::getCrashCount(const juce::String& pluginId) const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    juce::uint32 count = 0;
    for (const auto& crash : crashHistory_) {
        if (crash.pluginId == pluginId) {
            count++;
        }
    }

    return count;
}

//==============================================================================
void PluginCrashRecovery::clearCrashHistory() {
    std::lock_guard<std::mutex> lock(historyMutex_);
    crashHistory_.clear();

    std::cout << "PluginCrashRecovery: Crash history cleared" << std::endl;
}

//==============================================================================
std::vector<PluginStateBackup> PluginCrashRecovery::getBackups(const juce::String& pluginId) const {
    std::lock_guard<std::mutex> lock(backupsMutex_);

    auto it = backups_.find(pluginId);
    if (it != backups_.end()) {
        return it->second;
    }

    return {};
}

//==============================================================================
void PluginCrashRecovery::setCrashCallback(CrashCallback callback) {
    crashCallback_ = std::move(callback);
}

//==============================================================================
void PluginCrashRecovery::setRecoveryCallback(RecoveryCallback callback) {
    recoveryCallback_ = std::move(callback);
}

//==============================================================================
void PluginCrashRecovery::setAutoBackupEnabled(bool enabled) {
    autoBackupEnabled_.store(enabled);

    if (enabled) {
        startBackupTimer();
    } else {
        stopBackupTimer();
    }

    std::cout << "PluginCrashRecovery: Auto-backup "
              << (enabled ? "enabled" : "disabled") << std::endl;
}

//==============================================================================
PluginCrashRecovery& PluginCrashRecovery::getInstance() {
    static PluginCrashRecovery instance;
    return instance;
}

//==============================================================================
void PluginCrashRecovery::startBackupTimer() {
    backupTimer_ = std::make_unique<BackupTimer>(*this);
    backupTimer_->startTimer(config_.backupIntervalMs);

    std::cout << "PluginCrashRecovery: Auto-backup timer started ("
              << config_.backupIntervalMs << " ms)" << std::endl;
}

//==============================================================================
void PluginCrashRecovery::stopBackupTimer() {
    if (backupTimer_) {
        backupTimer_->stopTimer();
        backupTimer_.reset();
    }

    std::cout << "PluginCrashRecovery: Auto-backup timer stopped" << std::endl;
}

//==============================================================================
void PluginCrashRecovery::onBackupTimer() {
    if (!autoBackupEnabled_.load()) {
        return;
    }

    createBackupForAll();
}

//==============================================================================
juce::String PluginCrashRecovery::getPluginId(juce::AudioPluginInstance* plugin) const {
    // Find plugin ID from registered plugins
    for (const auto& pair : registeredPlugins_) {
        if (pair.second.instance == plugin) {
            return pair.first;
        }
    }

    // Generate ID from plugin name if not registered
    if (plugin != nullptr) {
        return juce::String::toHexString(
            reinterpret_cast<juce::uint64>(plugin)
        ).substring(0, 8);
    }

    return "unknown";
}

//==============================================================================
juce::MemoryBlock PluginCrashRecovery::capturePluginState(juce::AudioPluginInstance* plugin) const {
    if (plugin == nullptr) {
        return juce::MemoryBlock();
    }

    juce::MemoryBlock state;
    plugin->getStateInformation(state);

    return state;
}

//==============================================================================
bool PluginCrashRecovery::restorePluginState(juce::AudioPluginInstance* plugin,
                                            const PluginStateBackup& backup) const {
    if (plugin == nullptr || !backup.isValid()) {
        return false;
    }

    try {
        plugin->setStateInformation(backup.state.getData(), backup.state.getSize());
        return true;

    } catch (const std::exception& e) {
        std::cerr << "PluginCrashRecovery: Exception during state restore: "
                  << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "PluginCrashRecovery: Unknown exception during state restore" << std::endl;
        return false;
    }
}

//==============================================================================
void PluginCrashRecovery::saveCrashDump(const PluginCrashEvent& crash) {
    // Create crash dump file
    juce::File crashDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("ZenithDAW")
                             .getChildFile("CrashDumps");

    if (!crashDir.exists()) {
        crashDir.createDirectory();
    }

    juce::String filename = juce::String::formatted(
        "%s_crash_%s.txt",
        crash.pluginId.replaceCharacters(" :", "__"),
        crash.crashTime.toString(true, true).replaceCharacters(" :+-", "_")
    );

    juce::File crashFile = crashDir.getChildFile(filename);

    juce::FileOutputStream stream(crashFile);
    if (stream.openedOk()) {
        stream.writeString("=== Plugin Crash Report ===\n\n");
        stream.writeString("Plugin: " + crash.pluginName + "\n");
        stream.writeString("Plugin ID: " + crash.pluginId + "\n");
        stream.writeString("Time: " + crash.crashTime.toString(true, true) + "\n");
        stream.writeString("Reason: " + crash.crashReason + "\n");
        stream.writeString("Position: " + juce::String(crash.currentPosition, 2) + "\n");
        stream.writeString("Was Playing: " + juce::String(crash.wasPlaying ? "Yes" : "No") + "\n");
        stream.writeString("\nState snapshot: " + crash.stateSnapshot + "\n");

        std::cout << "PluginCrashRecovery: Crash dump saved to "
                  << crashFile.getFullPathName() << std::endl;
    }
}

//==============================================================================
juce::uint64 PluginCrashRecovery::calculateChecksum(const juce::MemoryBlock& data) const {
    if (data.getSize() == 0) {
        return 0;
    }

    // Simple checksum - XOR all bytes
    const juce::uint8* bytes = static_cast<const juce::uint8*>(data.getData());
    juce::uint64 checksum = 0;

    for (size_t i = 0; i < data.getSize(); ++i) {
        checksum = (checksum << 8) ^ bytes[i];
    }

    return checksum;
}

} // namespace zenith
