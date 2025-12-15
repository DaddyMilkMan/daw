/**
 * @file ProjectFileIO.h
 * @brief Production-grade project file I/O with error handling and recovery
 *
 * Features:
 * - Atomic writes (save to temp, rename on success)
 * - Crash recovery with auto-saves
 * - File validation and version checking
 * - Detailed error reporting
 * - Async save/load capability
 * - Backup management
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <vector>

namespace zenith {

class ProjectState;

/**
 * @brief File I/O errors for proper error handling
 */
enum class FileIOError {
    Success = 0,
    FileNotFound,
    InvalidFormat,
    CorruptedFile,
    InsufficientDiskSpace,
    PermissionDenied,
    WriteError,
    ParseError,
    VersionMismatch,
    Unknown
};

/**
 * @brief Metadata about a saved project
 */
struct ProjectMetadata {
    juce::String version;           // Format version (e.g., "1.0.0")
    juce::String zenithVersion;     // Zenith DAW version it was created in
    juce::int64 savedTimestamp;     // Unix timestamp
    double sampleRate;              // Sample rate when saved
    int trackCount;                 // Number of tracks
    double durationSeconds;         // Total project duration
    juce::String createdBy;         // User/system identifier
};

/**
 * @brief Recovery information for crash recovery
 */
struct RecoveryInfo {
    juce::File originalFile;        // Original project path
    juce::File recoveryFile;        // Temporary recovery file
    juce::int64 recoveryTimestamp;  // When recovery was saved
    bool isAutoSave;                // Is this an auto-save?
};

/**
 * @class ProjectFileIO
 * @brief Production-grade project file I/O with error handling and recovery
 */
class ProjectFileIO {
public:
    explicit ProjectFileIO(ProjectState& projectState);
    ~ProjectFileIO();

    // ========================================================================
    // Core I/O Operations
    // ========================================================================

    /**
     * Create a new blank project
     */
    void newProject();

    /**
     * Load project from file with full error handling
     * @param file Project file to load
     * @return FileIOError status
     */
    FileIOError loadFromFile(const juce::File& file);

    /**
     * Save project to file (atomic write)
     * @param file File to save to
     * @return FileIOError status
     */
    FileIOError saveToFile(const juce::File& file);

    /**
     * Save project with a new filename
     */
    FileIOError saveToFileAs(const juce::File& newFile);

    // ========================================================================
    // Recovery & Backup
    // ========================================================================

    /**
     * Auto-save the current project (for crash recovery)
     * Creates a backup in a system recovery directory
     * @return true if successful
     */
    bool autoSave();

    /**
     * Get the auto-save/recovery file if it exists
     */
    juce::File getRecoveryFile() const;

    /**
     * List all available recovery files
     */
    std::vector<RecoveryInfo> getAvailableRecoveries() const;

    /**
     * Recover from a crash recovery file
     */
    FileIOError recoverFromFile(const juce::File& recoveryFile);

    /**
     * Delete a recovery file
     */
    void deleteRecoveryFile(const juce::File& recoveryFile);

    /**
     * Create a manual backup of the current project
     * Saves to ProjectName_YYYYMMDD_HHMMSS.zth in a backups folder
     */
    juce::File createBackup();

    /**
     * List all backup files for the current project
     */
    std::vector<juce::File> getBackupFiles() const;

    // ========================================================================
    // Metadata & Validation
    // ========================================================================

    /**
     * Get metadata about a project file without loading it
     */
    ProjectMetadata readMetadata(const juce::File& file) const;

    /**
     * Validate a project file without loading it
     * @return true if file is valid and loadable
     */
    bool validateFile(const juce::File& file) const;

    /**
     * Check if file is a valid Zenith project
     */
    bool isValidZenithProject(const juce::File& file) const;

    /**
     * Get human-readable error message
     */
    static juce::String getErrorMessage(FileIOError error);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Set auto-save interval in seconds (0 to disable)
     */
    void setAutoSaveInterval(int intervalSeconds);

    /**
     * Set maximum number of backups to keep
     */
    void setMaxBackups(int count) { maxBackups_ = count; }

    /**
     * Get the file currently loaded/saved
     */
    juce::File getCurrentProjectFile() const { return currentProjectFile_; }

    /**
     * Enable/disable auto-save feature
     */
    void setAutoSaveEnabled(bool enabled) { autoSaveEnabled_ = enabled; }

    // ========================================================================
    // Error Information
    // ========================================================================

    /**
     * Get the last error that occurred
     */
    FileIOError getLastError() const { return lastError_; }

    /**
     * Get detailed error description
     */
    juce::String getLastErrorDetails() const { return lastErrorDetails_; }

private:
    ProjectState& projectState_;
    juce::File currentProjectFile_;
    FileIOError lastError_;
    juce::String lastErrorDetails_;

    int autoSaveIntervalSeconds_;
    bool autoSaveEnabled_;
    int maxBackups_;
    juce::int64 lastAutoSaveTime_;

    // ========================================================================
    // Private Implementation
    // ========================================================================

    /**
     * Internal: Create metadata from current state
     */
    ProjectMetadata createMetadata() const;

    /**
     * Internal: Add metadata to XML root
     */
    void addMetadataToXml(juce::XmlElement& root, const ProjectMetadata& meta) const;

    /**
     * Internal: Read metadata from XML
     */
    bool readMetadataFromXml(const juce::XmlElement& root, ProjectMetadata& outMeta) const;

    /**
     * Internal: Validate XML structure
     */
    bool validateXmlStructure(const juce::XmlElement& root) const;

    /**
     * Internal: Perform atomic write (temp -> target)
     */
    bool atomicWrite(const juce::File& targetFile, const juce::String& xmlString);

    /**
     * Internal: Get recovery directory
     */
    juce::File getRecoveryDirectory() const;

    /**
     * Internal: Get backups directory
     */
    juce::File getBackupsDirectory() const;

    /**
     * Internal: Clean up old recovery files
     */
    void cleanupOldRecoveries();

    /**
     * Internal: Record an error
     */
    void recordError(FileIOError err, const juce::String& details);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectFileIO)
};

} // namespace zenith
