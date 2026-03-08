/*
  ==============================================================================

    AtomicFileWriter.h
    Created: 2026-02-19
    Author:  Zenith DAW - Month 8: File I/O Safety (Gap #1)

    Atomic file write operations to prevent data corruption.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <functional>

namespace zenith {

//==============================================================================
/**
 * @brief Atomic write operation result
 */
struct AtomicWriteResult {
    bool success = false;
    juce::String errorMessage;
    juce::String tempFilePath;
    juce::String finalFilePath;
    double bytesWritten = 0;
    double writeTimeMs = 0.0;

    juce::String toString() const {
        if (success) {
            return "Atomic write successful: " + finalFilePath +
                   " (" + juce::String(bytesWritten / 1024.0, 1) + " KB)";
        } else {
            return "Atomic write failed: " + errorMessage;
        }
    }
};

//==============================================================================
/**
 * @brief File backup information
 */
struct FileBackupInfo {
    juce::File backupFile;
    double timestamp = 0.0;
    int versionNumber = 0;
    juce::String checksum;

    juce::String toString() const {
        return "Backup v" + juce::String(versionNumber) + ": " +
               backupFile.getFileName();
    }
};

//==============================================================================
/**
 * @brief Atomic file writer
 *
 * Features:
 * - Atomic writes (write to temp, then rename)
 * - Automatic backup creation
 * - Rollback on failure
 * - Crash-safe operations
 * - Checksum verification
 * - Progress tracking
 */
class AtomicFileWriter {
public:
    //==========================================================================
    AtomicFileWriter();
    ~AtomicFileWriter();

    //==========================================================================
    /**
     * @brief Write data to file atomically
     * @param targetFile Destination file
     * @param data Data to write
     * @param shouldCreateBackup Create backup before overwriting
     * @return Write result
     */
    AtomicWriteResult writeFile(
        const juce::File& targetFile,
        const juce::String& data,
        bool shouldCreateBackup = true);

    //==========================================================================
    /**
     * @brief Write binary data to file atomically
     */
    AtomicWriteResult writeFile(
        const juce::File& targetFile,
        const juce::MemoryBlock& data,
        bool shouldCreateBackup = true);

    //==========================================================================
    /**
     * @brief Write from stream atomically
     * @param targetFile Destination file
     * @param streamCallback Function to write to stream
     * @param shouldCreateBackup Create backup before overwriting
     * @return Write result
     */
    AtomicWriteResult writeFileFromStream(
        const juce::File& targetFile,
        std::function<void(juce::OutputStream&)> streamCallback,
        bool shouldCreateBackup = true);

    //==========================================================================
    /**
     * @brief Create backup of file
     * @param fileToBackup File to backup
     * @return Backup file created
     */
    juce::File createBackup(const juce::File& fileToBackup);

    //==========================================================================
    /**
     * @brief Rollback to backup
     * @param targetFile Original file
     * @param backupFile Backup to restore from
     * @return true if restored successfully
     */
    bool rollbackToBackup(const juce::File& targetFile,
                         const juce::File& backupFile);

    //==========================================================================
    /**
     * @brief Get backup files for a given file
     * @param targetFile Original file
     * @return List of backups (sorted newest first)
     */
    std::vector<FileBackupInfo> getBackups(const juce::File& targetFile) const;

    //==========================================================================
    /**
     * @brief Clean up old backups
     * @param targetFile Original file
     * @param keepCount Number of backups to keep
     * @return Number of backups removed
     */
    int cleanupOldBackups(const juce::File& targetFile, int keepCount = 5);

    //==========================================================================
    /**
     * @brief Calculate checksum of file
     * @param file File to checksum
     * @return Checksum string (hex)
     */
    static juce::String calculateChecksum(const juce::File& file);

    //==========================================================================
    /**
     * @brief Verify file integrity with checksum
     * @param file File to verify
     * @param expectedChecksum Expected checksum
     * @return true if checksums match
     */
    static bool verifyChecksum(const juce::File& file,
                              const juce::String& expectedChecksum);

    //==========================================================================
    /**
     * @brief Check if atomic write is needed
     * (Always returns true for safety)
     */
    static bool needsAtomicWrite(const juce::File& file) {
        // Always use atomic writes for safety
        return true;
    }

private:
    //==========================================================================
    juce::File generateTempFilePath(const juce::File& targetFile) const;
    juce::File generateBackupPath(const juce::File& targetFile, int version) const;
    bool atomicRename(const juce::File& source, const juce::File& dest);

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AtomicFileWriter)
};

//==============================================================================
/**
 * @brief Singleton accessor for atomic file writer
 */
class AtomicFileWriterHolder {
public:
    static AtomicFileWriter& getInstance() {
        static AtomicFileWriter instance;
        return instance;
    }

    AtomicFileWriterHolder(const AtomicFileWriterHolder&) = delete;
    AtomicFileWriterHolder& operator=(const AtomicFileWriterHolder&) = delete;

private:
    AtomicFileWriterHolder() = default;
    ~AtomicFileWriterHolder() = default;
};

} // namespace zenith
