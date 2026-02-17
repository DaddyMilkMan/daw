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
    struct Listener {
        virtual ~Listener() = default;
        virtual void syncStatusChanged(SyncStatus status) {}
        virtual void projectSynced(const juce::String& projectId) {}
        virtual void fileSynced(const juce::String& filePath) {}
        virtual void syncError(const juce::String& error) {}
        virtual void conflictDetected(const juce::String& filePath) {}
        virtual void backupCreated(const juce::String& backupId) {}
        virtual void backupRestored(const juce::String& backupId) {}
        virtual void storageInfoUpdated(size_t used, size_t total) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    // Configuration
    SyncConfig config;
    CloudProvider provider;

    // Authentication
    bool isAuthenticated = false;
    juce::Time lastTokenRefresh;

    // Projects
    std::unordered_map<juce::String, ProjectSyncInfo> projects;
    std::mutex projectsMutex;

    // Sync state
    std::atomic<bool> isCurrentlySyncing{false};
    std::atomic<bool> syncPaused{false};
    SyncStatus currentStatus = SyncStatus::Idle;
    std::atomic<bool> offlineMode{false};

    // File tracking
    std::unordered_map<juce::String, FileSyncInfo> trackedFiles;
    std::unordered_map<juce::String, juce::Time> lastModifiedTimes;
    std::mutex filesMutex;

    // Backups
    std::unordered_map<juce::String, BackupInfo> backups;
    std::mutex backupsMutex;

    // Network
    std::unique_ptr<juce::URL> apiClient;
    std::unique_ptr<juce::WebInputStream> webStream;

    // Upload/Download queues

} // namespace
