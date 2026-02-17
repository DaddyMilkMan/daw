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
    struct BackupPolicy {
        int maxBackups = 10;
        int retentionDays = 30;
        bool compress = true;
        bool encrypt = true;
        bool includeAudio = true;
        bool includeSettings = true;
        std::vector<juce::String> excludePatterns;
    };

    void setBackupPolicy(const BackupPolicy& policy);
    BackupPolicy getBackupPolicy() const;

    // Automatic backups
    void enableAutomaticBackups(bool enabled);
    bool areAutomaticBackupsEnabled() const;

    // Backup verification
    bool verifyBackup(const juce::String& backupId);
    bool testRestore(const juce::String& backupId);

private:
    CloudSyncSystem& syncSystem;
    BackupPolicy policy;
    bool autoBackupsEnabled = true;

    std::unordered_map<juce::String, juce::Time> scheduledBackups;
    std::mutex scheduledMutex;

    void performScheduledBackup(const juce::String& projectId);
    void cleanupOldBackups(const juce::String& projectId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudBackupManager)
};

} // namespace cloud
} // namespace zenith
