/*
  ==============================================================================
    CloudSyncSystem.h
    Cloud synchronization and backup system with version control
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace zenith {
namespace cloud {

// Sync status
enum class SyncStatus {
    Idle,
    Syncing,
    Uploading,
    Downloading,
    Conflict,
    Error,
    Offline
};

// File sync information
struct SyncConfig {
    // General settings
    bool enableAutoSync = true;
    int syncInterval = 300;  // seconds
    bool syncOnStartup = true;
    bool syncOnShutdown = true;
    bool syncOnlyOnWiFi = false;

    // File settings
    std::vector<juce::String> includePatterns;
    std::vector<juce::String> excludePatterns;
    bool syncAudioFiles = true;
    bool syncProjectFiles = true;
    bool syncSettings = true;
    bool syncPresets = true;

    // Backup settings
    bool enableAutoBackup = true;
    int backupInterval = 24;  // hours
    int maxBackups = 10;
    bool compressBackups = true;
    bool encryptBackups = true;
    juce::String encryptionKey;

    // Conflict resolution
    enum class ConflictResolution {
        AskUser,
        LocalWins,
        RemoteWins,
        KeepBoth,
        Merge
    } conflictResolution = ConflictResolution::AskUser;

    // Performance
    int maxConcurrentUploads = 3;
    int maxConcurrentDownloads = 3;
    size_t chunkSize = 1024 * 1024;  // 1MB
    bool enableDeltaSync = true;
    bool enableCompression = true;
};

// Cloud sync system

} // namespace
