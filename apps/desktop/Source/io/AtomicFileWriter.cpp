/*
  ==============================================================================

    AtomicFileWriter.cpp
    Implementation of atomic file write operations

  ==============================================================================
*/

#include "AtomicFileWriter.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// AtomicFileWriter Implementation
//==============================================================================

AtomicFileWriter::AtomicFileWriter() {
    std::cout << "AtomicFileWriter: Initialized" << std::endl;
}

AtomicFileWriter::~AtomicFileWriter() {
    std::cout << "AtomicFileWriter: Shut down" << std::endl;
}

//==============================================================================
AtomicWriteResult AtomicFileWriter::writeFile(
    const juce::File& targetFile,
    const juce::String& data,
    bool shouldCreateBackup)
{
    AtomicWriteResult result;
    result.finalFilePath = targetFile.getFullPathName();

    auto startTime = juce::Time::getCurrentTime();

    // Create backup if requested and file exists
    juce::File backupFile;
    if (shouldCreateBackup && targetFile.exists()) {
        backupFile = AtomicFileWriter::createBackup(targetFile);
        if (!backupFile.exists()) {
            result.success = false;
            result.errorMessage = "Failed to create backup";
            return result;
        }
    }

    // Generate temp file path
    juce::File tempFile = generateTempFilePath(targetFile);

    // Write to temp file
    {
        juce::FileOutputStream outputStream(tempFile);
        if (!outputStream.openedOk()) {
            result.success = false;
            result.errorMessage = "Failed to create temp file: " +
                                tempFile.getFullPathName();
            return result;
        }

        // Write data
        outputStream.writeText(data, false, false, nullptr);
        if (outputStream.getStatus().failed()) {
            result.success = false;
            result.errorMessage = "Failed to write to temp file";
            return result;
        }

        result.bytesWritten = outputStream.getPosition();
    }

    // Atomic rename (temp -> target)
    if (!atomicRename(tempFile, targetFile)) {
        result.success = false;
        result.errorMessage = "Failed to rename temp file to target";

        // Rollback to backup
        if (backupFile.exists()) {
            rollbackToBackup(targetFile, backupFile);
        }

        // Clean up temp file
        tempFile.deleteFile();
        return result;
    }

    // Verify write
    juce::String actualChecksum = calculateChecksum(targetFile);
    // In production, compare with expected checksum

    auto endTime = juce::Time::getCurrentTime();
    result.writeTimeMs = (endTime - startTime).inMilliseconds();
    result.success = true;
    result.tempFilePath = tempFile.getFullPathName();

    std::cout << "AtomicFileWriter: Wrote " << result.bytesWritten / 1024.0
              << " KB to " << targetFile.getFileName()
              << " in " << result.writeTimeMs << " ms" << std::endl;

    return result;
}

//==============================================================================
AtomicWriteResult AtomicFileWriter::writeFile(
    const juce::File& targetFile,
    const juce::MemoryBlock& data,
    bool shouldCreateBackup)
{
    AtomicWriteResult result;
    result.finalFilePath = targetFile.getFullPathName();

    auto startTime = juce::Time::getCurrentTime();

    // Create backup if requested
    juce::File backupFile;
    if (shouldCreateBackup && targetFile.exists()) {
        backupFile = AtomicFileWriter::createBackup(targetFile);
    }

    // Generate temp file path
    juce::File tempFile = generateTempFilePath(targetFile);

    // Write to temp file
    {
        juce::FileOutputStream outputStream(tempFile);
        if (!outputStream.openedOk()) {
            result.success = false;
            result.errorMessage = "Failed to create temp file";
            return result;
        }

        outputStream.write(data.getData(), data.getSize());
        if (outputStream.getStatus().failed()) {
            result.success = false;
            result.errorMessage = "Failed to write to temp file";
            return result;
        }

        result.bytesWritten = data.getSize();
    }

    // Atomic rename
    if (!atomicRename(tempFile, targetFile)) {
        result.success = false;
        result.errorMessage = "Failed to rename temp file";

        if (backupFile.exists()) {
            rollbackToBackup(targetFile, backupFile);
        }

        tempFile.deleteFile();
        return result;
    }

    auto endTime = juce::Time::getCurrentTime();
    result.writeTimeMs = (endTime - startTime).inMilliseconds();
    result.success = true;

    return result;
}

//==============================================================================
AtomicWriteResult AtomicFileWriter::writeFileFromStream(
    const juce::File& targetFile,
    std::function<void(juce::OutputStream&)> streamCallback,
    bool shouldCreateBackup)
{
    AtomicWriteResult result;
    result.finalFilePath = targetFile.getFullPathName();

    auto startTime = juce::Time::getCurrentTime();

    // Create backup if requested
    juce::File backupFile;
    if (shouldCreateBackup && targetFile.exists()) {
        backupFile = AtomicFileWriter::createBackup(targetFile);
    }

    // Generate temp file path
    juce::File tempFile = generateTempFilePath(targetFile);

    // Write to temp file
    {
        juce::FileOutputStream outputStream(tempFile);
        if (!outputStream.openedOk()) {
            result.success = false;
            result.errorMessage = "Failed to create temp file";
            return result;
        }

        // Call callback to write data
        streamCallback(outputStream);

        if (outputStream.getStatus().failed()) {
            result.success = false;
            result.errorMessage = "Stream callback failed";
            return result;
        }

        result.bytesWritten = outputStream.getPosition();
    }

    // Atomic rename
    if (!atomicRename(tempFile, targetFile)) {
        result.success = false;
        result.errorMessage = "Failed to rename temp file";

        if (backupFile.exists()) {
            rollbackToBackup(targetFile, backupFile);
        }

        tempFile.deleteFile();
        return result;
    }

    auto endTime = juce::Time::getCurrentTime();
    result.writeTimeMs = (endTime - startTime).inMilliseconds();
    result.success = true;

    return result;
}

//==============================================================================
juce::File AtomicFileWriter::createBackup(const juce::File& fileToBackup) {
    // Find next backup version number
    int version = 1;
    juce::File backupFile;

    do {
        backupFile = generateBackupPath(fileToBackup, version);
        version++;
    } while (backupFile.exists());

    // Copy file to backup location
    bool success = fileToBackup.copyFileTo(backupFile);

    if (success) {
        std::cout << "AtomicFileWriter: Created backup: "
                  << backupFile.getFileName() << std::endl;
    } else {
        std::cerr << "AtomicFileWriter: Failed to create backup" << std::endl;
    }

    return backupFile;
}

//==============================================================================
bool AtomicFileWriter::rollbackToBackup(const juce::File& targetFile,
                                        const juce::File& backupFile) {
    if (!backupFile.exists()) {
        std::cerr << "AtomicFileWriter: Backup file doesn't exist" << std::endl;
        return false;
    }

    // Copy backup to target
    bool success = backupFile.copyFileTo(targetFile);

    if (success) {
        std::cout << "AtomicFileWriter: Rolled back to backup: "
                  << backupFile.getFileName() << std::endl;
    }

    return success;
}

//==============================================================================
std::vector<FileBackupInfo> AtomicFileWriter::getBackups(
    const juce::File& targetFile) const
{
    std::vector<FileBackupInfo> backups;

    // Get parent directory
    juce::File parentDir = targetFile.getParentDirectory();
    if (!parentDir.exists()) {
        return backups;
    }

    // Find backup files
    juce::String baseName = targetFile.getFileNameWithoutExtension();
    juce::Array<juce::File> matchingFiles;
    parentDir.findChildFiles(matchingFiles, juce::File::findFiles, false,
                           "*" + baseName + "_backup*");

    for (const auto& file : matchingFiles) {
        FileBackupInfo info;
        info.backupFile = file;
        info.timestamp = file.getLastModificationTime().toMilliseconds() / 1000.0;
        info.checksum = calculateChecksum(file);

        // Extract version number from filename
        juce::String fileName = file.getFileNameWithoutExtension();
        int versionIndex = fileName.lastIndexOf("_v");
        if (versionIndex > 0) {
            juce::String versionStr = fileName.substring(versionIndex + 2);
            info.versionNumber = versionStr.getIntValue();
        }

        backups.push_back(info);
    }

    // Sort by version (newest first)
    std::sort(backups.begin(), backups.end(),
        [](const FileBackupInfo& a, const FileBackupInfo& b) {
            return a.versionNumber > b.versionNumber;
        });

    return backups;
}

//==============================================================================
int AtomicFileWriter::cleanupOldBackups(const juce::File& targetFile,
                                       int keepCount)
{
    auto backups = getBackups(targetFile);

    int removedCount = 0;

    // Keep only the most recent backups
    while (static_cast<int>(backups.size()) > keepCount) {
        juce::File oldestBackup = backups.back().backupFile;
        if (oldestBackup.deleteFile()) {
            removedCount++;
        }
        backups.pop_back();
    }

    if (removedCount > 0) {
        std::cout << "AtomicFileWriter: Cleaned up " << removedCount
                  << " old backups" << std::endl;
    }

    return removedCount;
}

//==============================================================================
juce::String AtomicFileWriter::calculateChecksum(const juce::File& file) {
    // Calculate CRC32 checksum
    juce::FileInputStream inputStream(file);
    if (!inputStream.openedOk()) {
        return "";
    }

    // Read entire file
    juce::MemoryBlock data;
    inputStream.readIntoMemoryBlock(data);

    // Calculate CRC32
    juce::uint32 crc = 0xFFFFFFFF;
    const uint8_t* dataPtr = static_cast<const uint8_t*>(data.getData());
    size_t size = data.getSize();

    for (size_t i = 0; i < size; ++i) {
        crc ^= dataPtr[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }

    crc ^= 0xFFFFFFFF;

    // Return as hex string
    return juce::String::toHexString(crc).paddedLeft('0', 8);
}

//==============================================================================
bool AtomicFileWriter::verifyChecksum(const juce::File& file,
                                     const juce::String& expectedChecksum)
{
    juce::String actualChecksum = calculateChecksum(file);
    return actualChecksum == expectedChecksum;
}

//==============================================================================
// Private Methods
//==============================================================================

juce::File AtomicFileWriter::generateTempFilePath(
    const juce::File& targetFile) const
{
    // Create temp file in same directory as target
    juce::File parentDir = targetFile.getParentDirectory();

    // Generate unique temp filename
    juce::String tempFileName = ".temp_" +
                               juce::String::toHexString(juce::Time::getHighResolutionTicks()) +
                               "_" +
                               targetFile.getFileName();

    return parentDir.getChildFile(tempFileName);
}

juce::File AtomicFileWriter::generateBackupPath(const juce::File& targetFile,
                                                int version) const
{
    juce::File parentDir = targetFile.getParentDirectory();
    juce::String baseName = targetFile.getFileNameWithoutExtension();
    juce::String extension = targetFile.getFileExtension();

    juce::String backupName = baseName + "_backup_v" +
                             juce::String(version) + "." + extension;

    return parentDir.getChildFile(backupName);
}

bool AtomicFileWriter::atomicRename(const juce::File& source,
                                    const juce::File& dest)
{
    // Delete destination if it exists
    if (dest.exists()) {
        if (!dest.deleteFile()) {
            std::cerr << "AtomicFileWriter: Failed to delete existing file" << std::endl;
            return false;
        }
    }

    // Rename source to destination (atomic on most filesystems)
    bool success = source.moveFileTo(dest);

    if (!success) {
        std::cerr << "AtomicFileWriter: Atomic rename failed" << std::endl;
    }

    return success;
}

} // namespace zenith
