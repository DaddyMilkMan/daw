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
struct FileSyncInfo {
    juce::String localPath;
    juce::String remotePath;
    juce::String fileId;
    juce::String versionId;
    juce::Time lastModified;
    size_t fileSize = 0;
    juce::String checksum;
    bool isDirectory = false;
    bool isSynced = false;
    SyncStatus status = SyncStatus::Idle;
    juce::String errorMessage;
};

// Project sync information
struct ProjectSyncInfo {
    juce::String projectId;
    juce::String projectName;
    juce::String localPath;
    juce::String remotePath;
    juce::Time lastSync;
    juce::Time lastModified;
    bool autoSyncEnabled = true;
    bool hasConflicts = false;
    std::vector<FileSyncInfo> files;
    std::vector<juce::String> conflictedFiles;
};

// Backup information
struct BackupInfo {
    juce::String backupId;
    juce::String projectId;
    juce::String name;
    juce::String description;
    juce::Time created;
    juce::Time expires;
    size_t totalSize = 0;
    int fileCount = 0;
    bool isAutomatic = false;
    bool isEncrypted = true;
    juce::String compressionLevel;  // "none", "fast", "normal", "maximum"
    std::vector<juce::String> tags;
};

// Cloud provider configuration
struct CloudProvider {
    enum class Type {
        GoogleDrive,
        Dropbox,
        OneDrive,
        AWS,
        Custom,
        Local
    };
    
    Type type = Type::GoogleDrive;
    juce::String name;
    juce::String apiEndpoint;
    juce::String authEndpoint;
    juce::String clientId;
    juce::String clientSecret;
    juce::String accessToken;
    juce::String refreshToken;
    juce::Time tokenExpiry;
    size_t maxFileSize = 5ULL * 1024 * 1024 * 1024;  // 5GB
    size_t totalStorage = 15ULL * 1024 * 1024 * 1024;  // 15GB
    size_t usedStorage = 0;
    bool isConnected = false;
};

// Sync configuration
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
class CloudSyncSystem : public juce::Timer,
                       public juce::Thread,
                       public juce::AsyncUpdater {
public:
    CloudSyncSystem();
    ~CloudSyncSystem() override;
    
    // Configuration
    void setConfig(const SyncConfig& config);
    SyncConfig getConfig() const;
    
    // Provider management
    void setProvider(const CloudProvider& provider);
    CloudProvider getProvider() const;
    bool connectProvider();
    bool disconnectProvider();
    bool isProviderConnected() const;
    
    // Authentication
    bool authenticate(const juce::String& username, const juce::String& password);
    bool refreshToken();
    bool isAuthenticated() const;
    
    // Project synchronization
    bool addProject(const juce::String& localPath, const juce::String& remotePath = "");
    bool removeProject(const juce::String& projectId);
    std::vector<ProjectSyncInfo> getProjects() const;
    ProjectSyncInfo getProjectInfo(const juce::String& projectId) const;
    
    // Sync operations
    void syncAll();
    void syncProject(const juce::String& projectId);
    void syncFile(const juce::String& localPath);
    void pauseSync();
    void resumeSync();
    bool isSyncing() const;
    SyncStatus getSyncStatus() const;
    
    // File operations
    bool uploadFile(const juce::String& localPath, const juce::String& remotePath = "");
    bool downloadFile(const juce::String& remotePath, const juce::String& localPath = "");
    bool deleteFile(const juce::String& remotePath);
    bool moveFile(const juce::String& oldPath, const juce::String& newPath);
    bool createFolder(const juce::String& path);
    
    // Backup operations
    juce::String createBackup(const juce::String& projectId, const juce::String& name = "");
    bool restoreBackup(const juce::String& backupId, const juce::String& targetPath);
    bool deleteBackup(const juce::String& backupId);
    std::vector<BackupInfo> getBackups(const juce::String& projectId = "");
    BackupInfo getBackupInfo(const juce::String& backupId) const;
    
    // Conflict resolution
    std::vector<juce::String> getConflictedFiles(const juce::String& projectId = "");
    bool resolveConflict(const juce::String& filePath, SyncConfig::ConflictResolution resolution);
    bool mergeFiles(const juce::String& localPath, const juce::String& remotePath);
    
    // Version control
    std::vector<juce::String> getFileVersions(const juce::String& filePath);
    bool restoreFileVersion(const juce::String& filePath, const juce::String& versionId);
    juce::String getCurrentFileVersion(const juce::String& filePath) const;
    
    // Storage information
    size_t getTotalStorage() const;
    size_t getUsedStorage() const;
    size_t getAvailableStorage() const;
    std::vector<std::pair<juce::String, size_t>> getStorageUsage() const;
    
    // Offline mode
    void enableOfflineMode(bool enabled);
    bool isOfflineModeEnabled() const;
    void syncWhenOnline();
    
    // Thread and timer callbacks
    void run() override;
    void timerCallback() override;
    void handleAsyncUpdate() override;
    
    // Listeners
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
class CloudSyncUI : public juce::Component,
                   public CloudSyncSystem::Listener,
                   public juce::Button::Listener,
                   public juce::Timer {
public:
    CloudSyncUI();
    ~CloudSyncUI() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Sync system access
    void setSyncSystem(CloudSyncSystem* syncSystem);
    CloudSyncSystem* getSyncSystem() const { return syncSystem; }
    
    // UI control
    void showSyncStatus(bool show);
    void showProjects(bool show);
    void showBackups(bool show);
    void showSettings(bool show);
    
    // CloudSyncSystem::Listener
    void syncStatusChanged(SyncStatus status) override;
    void projectSynced(const juce::String& projectId) override;
    void fileSynced(const juce::String& filePath) override;
    void syncError(const juce::String& error) override;
    void conflictDetected(const juce::String& filePath) override;
    void storageInfoUpdated(size_t used, size_t total) override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // Timer callback for UI updates
    void timerCallback() override;
    
private:
    CloudSyncSystem* syncSystem = nullptr;
    
    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // Status panel
    std::unique_ptr<juce::Component> statusPanel;
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::ProgressBar> progressBar;
    std::unique_ptr<juce::TextButton> syncButton;
    std::unique_ptr<juce::TextButton> pauseButton;
    
    // Storage info
    std::unique_ptr<juce::Label> storageLabel;
    std::unique_ptr<juce::ProgressBar> storageBar;
    
    // Projects list
    std::unique_ptr<juce::ListBox> projectListBox;
    std::unique_ptr<juce::TextButton> addProjectButton;
    std::unique_ptr<juce::TextButton> removeProjectButton;
    
    // Backups list
    std::unique_ptr<juce::ListBox> backupListBox;
    std::unique_ptr<juce::TextButton> createBackupButton;
    std::unique_ptr<juce::TextButton> restoreBackupButton;
    
    // Settings
    std::unique_ptr<juce::TextButton> settingsButton;
    std::unique_ptr<juce::ToggleButton> autoSyncToggle;
    std::unique_ptr<juce::Slider> syncIntervalSlider;
    
    // Conflict dialog
    std::unique_ptr<juce::DialogWindow> conflictDialog;
    
    // UI creation
    void createStatusPanel();
    void createProjectsList();
    void createBackupsList();
    void createSettingsPanel();
    
    // Updates
    void updateStatusDisplay();
    void updateStorageDisplay();
    void updateProjectsList();
    void updateBackupsList();
    
    // Dialogs
    void showAddProjectDialog();
    void showConflictDialog(const juce::String& filePath);
    void showSettingsDialog();
    void showBackupDialog();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudSyncUI)
};

// Cloud backup manager
class CloudBackupManager {
public:
    CloudBackupManager(CloudSyncSystem& syncSystem);
    ~CloudBackupManager() = default;
    
    // Backup scheduling
    void scheduleBackup(const juce::String& projectId, const juce::Time& when);
    void scheduleRecurringBackup(const juce::String& projectId, int intervalHours);
    void cancelScheduledBackup(const juce::String& backupId);
    
    // Backup policies
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
