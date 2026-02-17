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
class CloudBackupManager {
public:
    CloudBackupManager(CloudSyncSystem& syncSystem);
    ~CloudBackupManager() = default;

    // Backup scheduling
    void scheduleBackup(const juce::String& projectId, const juce::Time& when);
    void scheduleRecurringBackup(const juce::String& projectId, int intervalHours);
    void cancelScheduledBackup(const juce::String& backupId);

    // Backup policies

} // namespace
