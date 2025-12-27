/**
 * @file ProjectFileIO.cpp
 * @brief Production-grade project file I/O implementation
 */

#include "ProjectFileIO.h"
#include "ProjectState.h"
#include "TempoMap.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace zenith {

// File format version - increment when changing XML structure
static constexpr const char *PROJECT_FORMAT_VERSION = "1.0.0";
static constexpr const char *ZENITH_VERSION = "0.1.0";
static constexpr const char *RECOVERY_DIR_NAME = "RecoveryFiles";
static constexpr const char *BACKUPS_DIR_NAME = "Backups";
static constexpr int DEFAULT_AUTO_SAVE_INTERVAL = 300; // 5 minutes
static constexpr int DEFAULT_MAX_BACKUPS = 10;
static constexpr int RECOVERY_RETENTION_DAYS = 7;

ProjectFileIO::ProjectFileIO(ProjectState &projectState)
    : projectState_(projectState), lastError_(FileIOError::Success),
      autoSaveIntervalSeconds_(DEFAULT_AUTO_SAVE_INTERVAL),
      autoSaveEnabled_(true), maxBackups_(DEFAULT_MAX_BACKUPS),
      lastAutoSaveTime_(0) {
  DBG("ProjectFileIO: Initialized");
  cleanupOldRecoveries();
}

ProjectFileIO::~ProjectFileIO() = default;

// ============================================================================
// Core I/O
// ============================================================================

void ProjectFileIO::newProject() {
  DBG("ProjectFileIO: Creating new blank project");

  projectState_.getUndoManager().clearUndoHistory();
  projectState_.createDefaultState();

  currentProjectFile_ = juce::File();
  lastError_ = FileIOError::Success;
  lastAutoSaveTime_ = 0;
  projectState_.isDirty.store(false);

  DBG("ProjectFileIO: New project created successfully");
}

FileIOError ProjectFileIO::loadFromFile(const juce::File &file) {
  DBG("ProjectFileIO: Loading from " << file.getFullPathName());

  // Validate file exists
  if (!file.existsAsFile()) {
    recordError(FileIOError::FileNotFound,
                "File does not exist: " + file.getFullPathName());
    return lastError_;
  }

  // Parse XML
  auto xml = juce::parseXML(file);
  if (xml == nullptr) {
    recordError(FileIOError::ParseError, "Failed to parse XML from file");
    return lastError_;
  }

  // Validate XML structure
  if (!validateXmlStructure(*xml)) {
    recordError(FileIOError::InvalidFormat,
                "XML structure is invalid or corrupted");
    return lastError_;
  }

  // Check version
  auto version = xml->getStringAttribute("formatVersion", "");
  if (version != PROJECT_FORMAT_VERSION) {
    DBG("ProjectFileIO: WARNING - Format version mismatch. Expected "
        << PROJECT_FORMAT_VERSION << ", got " << version);
    // Allow loading older formats with warning (graceful degradation)
  }

  // Create ValueTree from XML
  auto newState = juce::ValueTree::fromXml(*xml);
  if (!newState.isValid() || newState.getType() != ProjectState::ID_PROJECT) {
    recordError(FileIOError::InvalidFormat,
                "Invalid project ValueTree structure");
    return lastError_;
  }

  // Atomic replace: remove listener, update, re-add listener
  auto &state = projectState_.getState();
  state.removeListener(&projectState_);
  state = newState;
  state.addListener(&projectState_);

  // Rebuild internal caches
  projectState_.rebuildIdCounter();
  projectState_.rebuildTrackMap();
  projectState_.getUndoManager().clearUndoHistory();

  currentProjectFile_ = file;
  projectState_.setProjectFile(file);
  projectState_.isDirty.store(false);
  lastAutoSaveTime_ = juce::Time::currentTimeMillis();

  recordError(FileIOError::Success, "File loaded successfully");
  DBG("ProjectFileIO: File loaded successfully");
  return lastError_;
}

void ProjectFileIO::loadFromFileAsync(
    const juce::File &file,
    std::function<void(bool success, juce::String error)> callback) {
  juce::Thread::launch([this, file, callback]() {
    DBG("ProjectFileIO: Starting async load...");

    if (!file.existsAsFile()) {
      juce::MessageManager::callAsync(
          [callback] { callback(false, "File does not exist"); });
      return;
    }

    juce::ValueTree newState;

    // Try XML first
    auto xml = juce::parseXML(file);
    if (xml != nullptr) {
      newState = juce::ValueTree::fromXml(*xml);
    } else {
      // Try binary
      juce::MemoryBlock mb;
      if (file.loadFileAsData(mb)) {
        juce::MemoryInputStream mi(mb, false);
        newState = juce::ValueTree::readFromStream(mi);
      }
    }

    if (!newState.isValid()) {
      juce::MessageManager::callAsync(
          [callback] { callback(false, "Invalid project file format"); });
      return;
    }

    juce::MessageManager::callAsync([this, file, newState, callback] {
      // Re-check type safely on message thread
      if (newState.getType() != ProjectState::ID_PROJECT) {
        callback(false, "File is not a Zenith project");
        return;
      }

      auto &state = projectState_.getState();
      state.removeListener(&projectState_);
      state = newState;
      state.addListener(&projectState_);

      projectState_.rebuildIdCounter();
      projectState_.rebuildTrackMap();
      projectState_.getUndoManager().clearUndoHistory();
      projectState_.setProjectFile(file);
      projectState_.isDirty.store(false);

      callback(true, "");
    });
  });
}

FileIOError ProjectFileIO::saveToFile(const juce::File &file) {
  DBG("ProjectFileIO: Saving to " << file.getFullPathName());

  // Check disk space (rough estimate: assume 5x the XML size)
  auto xml = projectState_.getState().createXml();
  if (xml == nullptr) {
    recordError(FileIOError::WriteError, "Failed to create XML from ValueTree");
    return lastError_;
  }

  // Add metadata
  ProjectMetadata meta = createMetadata();
  addMetadataToXml(*xml, meta);

  juce::String xmlString = xml->toString();
  size_t estimatedSize = xmlString.length() * 5;

  // Simple disk space check
  juce::File targetDir = file.getParentDirectory();
  if (targetDir.getBytesFreeOnVolume() < (juce::int64)estimatedSize) {
    recordError(FileIOError::InsufficientDiskSpace,
                "Not enough disk space to save file");
    return lastError_;
  }

  // Perform atomic write
  if (!atomicWrite(file, xmlString)) {
    return lastError_;
  }

  currentProjectFile_ = file;
  projectState_.setProjectFile(file);
  projectState_.isDirty.store(false);
  lastAutoSaveTime_ = juce::Time::currentTimeMillis();

  recordError(FileIOError::Success, "File saved successfully");
  DBG("ProjectFileIO: File saved successfully");
  return lastError_;
}

void ProjectFileIO::saveToFileAsync(
    const juce::File &file, IOSettings settings,
    std::function<void(bool success, juce::String error)> callback) {
  // Capture necessary state safely
  auto stateSnapshot = projectState_.getState().createCopy();

  juce::Thread::launch([this, file, settings, stateSnapshot,
                        callback]() mutable {
    DBG("ProjectFileIO: Starting async save...");

    std::unique_ptr<juce::XmlElement> xml;
    juce::MemoryBlock msgPackData;
    bool prepareSuccess = false;

    if (settings.format == SerializationFormat::Xml) {
      xml = stateSnapshot.createXml();
      if (xml != nullptr) {
        // Add metadata to snapshot
        ProjectMetadata meta = createMetadata();
        addMetadataToXml(*xml, meta);
        prepareSuccess = true;
      }
    } else {
      // MessagePack placeholder
      juce::MemoryOutputStream mo(msgPackData, false);
      stateSnapshot.writeToStream(mo);
      prepareSuccess = true;
    }

    if (!prepareSuccess) {
      juce::MessageManager::callAsync(
          [callback] { callback(false, "Failed to prepare data"); });
      return;
    }

    juce::File tempFile = file.getSiblingFile(file.getFileName() + ".savetmp");
    bool writeSuccess = false;

    if (settings.format == SerializationFormat::Xml)
      writeSuccess = xml->writeTo(tempFile);
    else
      writeSuccess = tempFile.replaceWithData(msgPackData.getData(),
                                              msgPackData.getSize());

    if (writeSuccess) {
      if (tempFile.moveFileTo(file)) {
        juce::MessageManager::callAsync([this, file, callback] {
          projectState_.setProjectFile(file);
          projectState_.isDirty.store(false);
          callback(true, "");
        });
      } else {
        tempFile.deleteFile();
        juce::MessageManager::callAsync([callback] {
          callback(false, "Failed to move file to destination");
        });
      }
    } else {
      juce::MessageManager::callAsync(
          [callback] { callback(false, "Failed to write data to disk"); });
    }
  });
}

FileIOError ProjectFileIO::saveToFileAs(const juce::File &newFile) {
  FileIOError result = saveToFile(newFile);
  if (result == FileIOError::Success) {
    currentProjectFile_ = newFile;
  }
  return result;
}

juce::File ProjectFileIO::saveCrashDump() {
  auto documentsDir =
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
  auto crashDir =
      documentsDir.getChildFile("ZenithDAW").getChildFile("CrashDumps");

  if (!crashDir.exists())
    crashDir.createDirectory();

  auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
  auto dumpFile = crashDir.getChildFile("crash_recovery_" + timestamp + ".zth");

  DBG("ProjectFileIO: Saving crash dump to " + dumpFile.getFullPathName());

  if (saveToFile(dumpFile) == FileIOError::Success)
    return dumpFile;

  return juce::File();
}

// ============================================================================
// Recovery & Backup
// ============================================================================

bool ProjectFileIO::autoSave() {
  if (!autoSaveEnabled_) {
    return true;
  }

  juce::int64 currentTime = juce::Time::currentTimeMillis();
  if (currentTime - lastAutoSaveTime_ <
      (juce::int64)autoSaveIntervalSeconds_ * 1000) {
    return true; // Not time yet
  }

  juce::File recoveryDir = getRecoveryDirectory();
  if (!recoveryDir.exists()) {
    recoveryDir.createDirectory();
  }

  // Generate timestamp-based recovery file
  auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
  juce::File recoveryFile =
      recoveryDir.getChildFile("autosave_" + timestamp + ".zth");

  DBG("ProjectFileIO: Auto-saving to " << recoveryFile.getFullPathName());

  if (saveToFile(recoveryFile) == FileIOError::Success) {
    lastAutoSaveTime_ = currentTime;
    return true;
  }

  return false;
}

juce::File ProjectFileIO::getRecoveryFile() const {
  juce::File recoveryDir = getRecoveryDirectory();
  if (!recoveryDir.exists()) {
    return juce::File();
  }

  // Return most recent recovery file
  juce::Array<juce::File> files;
  recoveryDir.findChildFiles(files, juce::File::findFiles, false, "*.zth");

  if (files.isEmpty()) {
    return juce::File();
  }

  // Sort by modification time (most recent last)
  std::sort(files.begin(), files.end(),
            [](const juce::File &a, const juce::File &b) {
              return a.getLastModificationTime() < b.getLastModificationTime();
            });

  return files.getLast();
}

std::vector<RecoveryInfo> ProjectFileIO::getAvailableRecoveries() const {
  std::vector<RecoveryInfo> recoveries;
  juce::File recoveryDir = getRecoveryDirectory();

  if (!recoveryDir.exists()) {
    return recoveries;
  }

  juce::Array<juce::File> files;
  recoveryDir.findChildFiles(files, juce::File::findFiles, false, "*.zth");

  for (const auto &file : files) {
    RecoveryInfo info;
    info.recoveryFile = file;
    info.recoveryTimestamp = file.getLastModificationTime().toMilliseconds();
    info.isAutoSave = file.getFileName().startsWith("autosave_");
    recoveries.push_back(info);
  }

  return recoveries;
}

FileIOError ProjectFileIO::recoverFromFile(const juce::File &recoveryFile) {
  DBG("ProjectFileIO: Recovering from " << recoveryFile.getFullPathName());
  return loadFromFile(recoveryFile);
}

void ProjectFileIO::deleteRecoveryFile(const juce::File &recoveryFile) {
  if (recoveryFile.existsAsFile()) {
    recoveryFile.deleteFile();
    DBG("ProjectFileIO: Deleted recovery file "
        << recoveryFile.getFullPathName());
  }
}

juce::File ProjectFileIO::createBackup() {
  if (!currentProjectFile_.existsAsFile()) {
    return juce::File();
  }

  juce::File backupDir = getBackupsDirectory();
  if (!backupDir.exists()) {
    backupDir.createDirectory();
  }

  auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
  juce::String baseName = currentProjectFile_.getFileNameWithoutExtension();
  juce::File backupFile =
      backupDir.getChildFile(baseName + "_" + timestamp + ".zth");

  if (currentProjectFile_.copyFileTo(backupFile)) {
    DBG("ProjectFileIO: Created backup at " << backupFile.getFullPathName());

    // Clean up old backups
    auto backups = getBackupFiles();
    if ((int)backups.size() > maxBackups_) {
      std::sort(backups.begin(), backups.end(),
                [](const juce::File &a, const juce::File &b) {
                  return a.getLastModificationTime() <
                         b.getLastModificationTime();
                });

      for (int i = 0; i < (int)backups.size() - maxBackups_; ++i) {
        backups[i].deleteFile();
        DBG("ProjectFileIO: Deleted old backup "
            << backups[i].getFullPathName());
      }
    }

    return backupFile;
  }

  return juce::File();
}

std::vector<juce::File> ProjectFileIO::getBackupFiles() const {
  std::vector<juce::File> backups;
  juce::File backupDir = getBackupsDirectory();

  if (!backupDir.exists()) {
    return backups;
  }

  juce::Array<juce::File> files;
  backupDir.findChildFiles(files, juce::File::findFiles, false, "*.zth");

  for (const auto &file : files) {
    backups.push_back(file);
  }

  return backups;
}

// ============================================================================
// Metadata & Validation
// ============================================================================

ProjectMetadata ProjectFileIO::readMetadata(const juce::File &file) const {
  ProjectMetadata meta;
  meta.version = "unknown";
  meta.zenithVersion = "unknown";
  meta.savedTimestamp = 0;
  meta.sampleRate = 48000.0;
  meta.trackCount = 0;
  meta.durationSeconds = 0.0;

  auto xml = juce::parseXML(file);
  if (xml != nullptr) {
    readMetadataFromXml(*xml, meta);
  }

  return meta;
}

bool ProjectFileIO::validateFile(const juce::File &file) const {
  if (!file.existsAsFile()) {
    return false;
  }

  auto xml = juce::parseXML(file);
  if (xml == nullptr) {
    return false;
  }

  return validateXmlStructure(*xml);
}

bool ProjectFileIO::isValidZenithProject(const juce::File &file) const {
  return validateFile(file);
}

juce::String ProjectFileIO::getErrorMessage(FileIOError error) {
  switch (error) {
  case FileIOError::Success:
    return "Success";
  case FileIOError::FileNotFound:
    return "File not found";
  case FileIOError::InvalidFormat:
    return "Invalid project format";
  case FileIOError::CorruptedFile:
    return "File is corrupted";
  case FileIOError::InsufficientDiskSpace:
    return "Insufficient disk space";
  case FileIOError::PermissionDenied:
    return "Permission denied";
  case FileIOError::WriteError:
    return "Write error";
  case FileIOError::ParseError:
    return "Failed to parse file";
  case FileIOError::VersionMismatch:
    return "Version mismatch";
  case FileIOError::Unknown:
  default:
    return "Unknown error";
  }
}

// ============================================================================
// Configuration
// ============================================================================

void ProjectFileIO::setAutoSaveInterval(int intervalSeconds) {
  autoSaveIntervalSeconds_ = juce::jmax(0, intervalSeconds);
  DBG("ProjectFileIO: Auto-save interval set to " << autoSaveIntervalSeconds_
                                                  << " seconds");
}

// ============================================================================
// Private Implementation
// ============================================================================

ProjectMetadata ProjectFileIO::createMetadata() const {
  ProjectMetadata meta;
  meta.version = PROJECT_FORMAT_VERSION;
  meta.zenithVersion = ZENITH_VERSION;
  meta.savedTimestamp = juce::Time::currentTimeMillis();
  meta.sampleRate = projectState_.getSampleRate();
  meta.trackCount = projectState_.getNumTracks();

  // Calculate project duration from clips
  double maxBeats = 0.0;
  auto tracksNode =
      projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);

  if (tracksNode.isValid()) {
    for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
      auto track = tracksNode.getChild(i);
      auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);

      if (clipsNode.isValid()) {
        for (int j = 0; j < clipsNode.getNumChildren(); ++j) {
          auto clip = clipsNode.getChild(j);
          double start = clip.getProperty(ProjectState::PROP_START_BEATS, 0.0);
          double length =
              clip.getProperty(ProjectState::PROP_LENGTH_BEATS, 0.0);
          maxBeats = std::max(maxBeats, start + length);
        }
      }
    }
  }

  TempoMap tempoMap;
  tempoMap.updateFromValueTree(projectState_.getTempoMap());
  meta.durationSeconds = tempoMap.beatsToSeconds(maxBeats, meta.sampleRate);

  meta.createdBy = juce::SystemStats::getComputerName();
  return meta;
}

void ProjectFileIO::addMetadataToXml(juce::XmlElement &root,
                                     const ProjectMetadata &meta) const {
  root.setAttribute("formatVersion", meta.version);
  root.setAttribute("zenithVersion", meta.zenithVersion);
  root.setAttribute("savedTimestamp", juce::String(meta.savedTimestamp));
  root.setAttribute("sampleRate", meta.sampleRate);
  root.setAttribute("trackCount", meta.trackCount);
  root.setAttribute("duration", meta.durationSeconds);
  root.setAttribute("createdBy", meta.createdBy);
}

bool ProjectFileIO::readMetadataFromXml(const juce::XmlElement &root,
                                        ProjectMetadata &outMeta) const {
  outMeta.version = root.getStringAttribute("formatVersion", "unknown");
  outMeta.zenithVersion = root.getStringAttribute("zenithVersion", "unknown");
  outMeta.savedTimestamp =
      root.getStringAttribute("savedTimestamp", "0").getLargeIntValue();
  outMeta.sampleRate = root.getDoubleAttribute("sampleRate", 48000.0);
  outMeta.trackCount = root.getIntAttribute("trackCount", 0);
  outMeta.durationSeconds = root.getDoubleAttribute("duration", 0.0);
  outMeta.createdBy = root.getStringAttribute("createdBy", "unknown");
  return true;
}

bool ProjectFileIO::validateXmlStructure(const juce::XmlElement &root) const {
  if (root.getTagName() != "ZenithProject") {
    return false;
  }

  // Must have tracks node
  if (root.getChildByName("Tracks") == nullptr) {
    return false;
  }

  return true;
}

bool ProjectFileIO::atomicWrite(const juce::File &targetFile,
                                const juce::String &xmlString) {
  // Write to temporary file first
  juce::File tempFile = targetFile.getParentDirectory().getChildFile(
      targetFile.getFileName() + ".tmp");

  // Remove stale temp file if it exists
  if (tempFile.existsAsFile()) {
    tempFile.deleteFile();
  }

  // Write to temp
  if (!tempFile.replaceWithText(xmlString)) {
    recordError(FileIOError::WriteError, "Failed to write temporary file");
    return false;
  }

  // Atomic rename: temp -> target
  if (!tempFile.moveFileTo(targetFile)) {
    tempFile.deleteFile();
    recordError(FileIOError::WriteError, "Failed to rename file atomically");
    return false;
  }

  return true;
}

juce::File ProjectFileIO::getRecoveryDirectory() const {
  auto docsDir =
      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
  return docsDir.getChildFile("ZenithDAW").getChildFile(RECOVERY_DIR_NAME);
}

juce::File ProjectFileIO::getBackupsDirectory() const {
  if (!currentProjectFile_.existsAsFile()) {
    return juce::File();
  }

  return currentProjectFile_.getParentDirectory().getChildFile(
      BACKUPS_DIR_NAME);
}

void ProjectFileIO::cleanupOldRecoveries() {
  juce::File recoveryDir = getRecoveryDirectory();
  if (!recoveryDir.exists()) {
    return;
  }

  juce::Array<juce::File> files;
  recoveryDir.findChildFiles(files, juce::File::findFiles, false, "*.zth");

  juce::int64 cutoffTime = juce::Time::currentTimeMillis() -
                           (RECOVERY_RETENTION_DAYS * 24 * 60 * 60 * 1000);

  for (const auto &file : files) {
    if (file.getLastModificationTime().toMilliseconds() < cutoffTime) {
      file.deleteFile();
      DBG("ProjectFileIO: Cleaned up old recovery file "
          << file.getFullPathName());
    }
  }
}

void ProjectFileIO::recordError(FileIOError err, const juce::String &details) {
  lastError_ = err;
  lastErrorDetails_ = details;

  if (err != FileIOError::Success) {
    DBG("ProjectFileIO ERROR: " << details);
  }
}

} // namespace zenith