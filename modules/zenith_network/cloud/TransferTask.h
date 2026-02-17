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
    struct TransferTask {
        juce::String localPath;
        juce::String remotePath;
        bool isUpload = true;
        size_t totalBytes = 0;
        size_t transferredBytes = 0;
        bool isPaused = false;
        bool hasError = false;
        juce::String errorMessage;
    };

    std::vector<TransferTask> transferQueue;
    std::mutex transferMutex;

    // Listeners
    std::vector<Listener*> listeners;

    // Internal methods
    void initializeProvider();
    void performSync();
    void syncProjectInternal(const juce::String& projectId);
    void scanProjectFiles(const juce::String& projectId);
    void uploadFileInternal(const juce::String& localPath, const juce::String& remotePath);
    void downloadFileInternal(const juce::String& remotePath, const juce::String& localPath);

    // File operations
    juce::String calculateChecksum(const juce::File& file);
    bool compareFiles(const juce::File& file1, const juce::File& file2);
    bool shouldSyncFile(const juce::File& file);
    juce::String getRemotePath(const juce::String& localPath, const juce::String& projectId);

    // Conflict resolution
    bool detectConflict(const juce::String& filePath);
    void resolveConflictInternal(const juce::String& filePath, SyncConfig::ConflictResolution resolution);
    bool mergeFilesInternal(const juce::File& localFile, const juce::File& remoteFile);

    // Backup operations
    juce::String createBackupInternal(const juce::String& projectId, const juce::String& name);
    bool restoreBackupInternal(const juce::String& backupId, const juce::String& targetPath);
    bool compressBackup(const juce::File& source, const juce::File& destination);
    bool encryptBackup(const juce::File& source, const juce::File& destination);

    // Version control
    bool createFileVersion(const juce::String& filePath);
    std::vector<juce::String> getFileVersionsInternal(const juce::String& filePath);

    // Network methods
    juce::String makeAPIRequest(const juce::String& endpoint, const juce::String& method = "GET", const juce::String& data = "");
    bool uploadChunk(const juce::String& uploadId, int chunkNumber, const juce::MemoryBlock& chunk);
    bool completeUpload(const juce::String& uploadId);

    // Storage management
    void updateStorageInfo();
    void cleanupOldBackups();
    void optimizeStorage();

    // Offline support
    void cacheFileForOffline(const juce::String& filePath);
    bool isFileCached(const juce::String& filePath);
    void syncCachedFiles();

    // Utility methods
    juce::String generateFileId();
    juce::String generateBackupId();
    juce::String formatFileSize(size_t bytes) const;
    juce::File getCacheDirectory();
    juce::File getTempDirectory();

    // Notification
    void notifySyncStatusChanged(SyncStatus status);
    void notifyProjectSynced(const juce::String& projectId);
    void notifyFileSynced(const juce::String& filePath);
    void notifySyncError(const juce::String& error);
    void notifyConflictDetected(const juce::String& filePath);
    void notifyBackupCreated(const juce::String& backupId);
    void notifyBackupRestored(const juce::String& backupId);
    void notifyStorageInfoUpdated(size_t used, size_t total);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudSyncSystem)
};

// Cloud sync UI

} // namespace
