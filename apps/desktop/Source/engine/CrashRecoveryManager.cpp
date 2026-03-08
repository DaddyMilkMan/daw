/*
  ==============================================================================

    CrashRecoveryManager.cpp
    Implementation of crash detection and recovery

  ==============================================================================
*/

#include "CrashRecoveryManager.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// CrashRecoveryManager Implementation
//==============================================================================

CrashRecoveryManager::CrashRecoveryManager() {
    startTime_ = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

    // Set default auto-save directory
    juce::File defaultDir = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("AutoSaves");

    config_.autoSaveDirectory = defaultDir;

    std::cout << "CrashRecoveryManager: Initialized" << std::endl;
}

CrashRecoveryManager::~CrashRecoveryManager() {
    // Clean shutdown
    markCleanShutdown();

    std::cout << "CrashRecoveryManager: Shut down ("
              << statistics_.autoSavesTriggered << " auto-saves, "
              << statistics_.statesRestored << " restorals)" << std::endl;
}

//==============================================================================
bool CrashRecoveryManager::initialize(const CrashRecoveryConfig& config) {
    config_ = config;

    // Create auto-save directory if it doesn't exist
    if (!config_.autoSaveDirectory.exists()) {
        if (!config_.autoSaveDirectory.createDirectory()) {
            std::cerr << "CrashRecoveryManager: Failed to create auto-save directory: "
                      << config_.autoSaveDirectory.getFullPathName() << std::endl;
            return false;
        }
    }

    // Check for previous crash
    if (detectPreviousCrash()) {
        CrashEvent event;
        event.type = CrashEventType::CrashDetected;
        event.description = "Previous application crash detected";
        event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
        recordEvent(event);
        statistics_.crashesDetected++;

        std::cerr << "CrashRecoveryManager: Previous crash detected!" << std::endl;
    }

    // Mark clean startup
    markCleanStartup();

    return true;
}

//==============================================================================
bool CrashRecoveryManager::triggerSave(const juce::String& reason) {
    if (!saveFunction_) {
        std::cerr << "CrashRecoveryManager: No save function registered" << std::endl;
        return false;
    }

    // Call save function
    juce::String stateData = saveFunction_();
    if (stateData.isEmpty()) {
        std::cerr << "CrashRecoveryManager: Save function returned empty data" << std::endl;
        return false;
    }

    // Generate file path
    juce::File saveFile = generateAutoSavePath();

    // Save to file
    juce::FileOutputStream outputStream(saveFile);
    if (outputStream.openedOk()) {
        outputStream.write(stateData.toUTF8(), stateData.length());
        outputStream.flush();

        CrashEvent event;
        event.type = CrashEventType::StateSaved;
        event.description = "State saved: " + reason;
        event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
        event.filePath = saveFile.getFullPathName();
        recordEvent(event);

        statistics_.statesSaved++;
        statistics_.lastSaveTime = event.timestamp;

        std::cout << "CrashRecoveryManager: State saved to "
                  << saveFile.getFileName() << std::endl;

        return true;
    }

    std::cerr << "CrashRecoveryManager: Failed to write to file: "
              << saveFile.getFullPathName() << std::endl;
    return false;
}

//==============================================================================
bool CrashRecoveryManager::triggerAutoSave() {
    double currentTime = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

    // Check if enough time has passed since last save
    if (currentTime - lastAutoSaveTime_ < config_.autoSaveIntervalSeconds) {
        return true;  // Not time yet, but not a failure
    }

    bool success = triggerSave("Auto-save");

    if (success) {
        CrashEvent event;
        event.type = CrashEventType::AutoSaveTriggered;
        event.description = "Auto-save triggered";
        event.timestamp = currentTime;
        recordEvent(event);

        statistics_.autoSavesTriggered++;
        lastAutoSaveTime_ = currentTime;

        // Cleanup old auto-saves
        cleanupOldAutoSaves();
    }

    return success;
}

//==============================================================================
std::vector<juce::File> CrashRecoveryManager::findRecoveryFiles() const {
    std::vector<juce::File> recoveryFiles;

    if (!config_.autoSaveDirectory.exists()) {
        return recoveryFiles;
    }

    // Find all autosave files
    juce::Array<juce::File> matches;
    config_.autoSaveDirectory.findChildFiles(matches, juce::File::findFiles, false, "*.autosave");

    for (const auto& file : matches) {
        recoveryFiles.push_back(file);
    }

    // Sort by modification time (newest first)
    std::sort(recoveryFiles.begin(), recoveryFiles.end(),
        [](const juce::File& a, const juce::File& b) {
            return a.getLastModificationTime() > b.getLastModificationTime();
        });

    return recoveryFiles;
}

//==============================================================================
juce::File CrashRecoveryManager::getLatestRecoveryFile() const {
    auto recoveryFiles = findRecoveryFiles();
    return recoveryFiles.empty() ? juce::File() : recoveryFiles[0];
}

//==============================================================================
bool CrashRecoveryManager::restoreState(const juce::File& file) {
    if (!file.existsAsFile()) {
        std::cerr << "CrashRecoveryManager: Recovery file doesn't exist: "
                  << file.getFullPathName() << std::endl;
        return false;
    }

    // Read file
    juce::FileInputStream inputStream(file);
    if (!inputStream.openedOk()) {
        std::cerr << "CrashRecoveryManager: Failed to open recovery file" << std::endl;
        return false;
    }

    juce::String stateData = inputStream.readEntireStreamAsString();

    // Call restore function
    if (!restoreFunction_) {
        std::cerr << "CrashRecoveryManager: No restore function registered" << std::endl;
        return false;
    }

    bool success = restoreFunction_(stateData);

    if (success) {
        CrashEvent event;
        event.type = CrashEventType::StateRestored;
        event.description = "State restored from: " + file.getFileName();
        event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
        event.filePath = file.getFullPathName();
        recordEvent(event);

        statistics_.statesRestored++;

        std::cout << "CrashRecoveryManager: State restored from "
                  << file.getFileName() << std::endl;
    } else {
        CrashEvent event;
        event.type = CrashEventType::RecoveryFailed;
        event.description = "Failed to restore state from: " + file.getFileName();
        event.timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;
        recordEvent(event);

        statistics_.recoveriesFailed++;
    }

    return success;
}

//==============================================================================
bool CrashRecoveryManager::restoreFromLatestCrash() {
    juce::File latestFile = getLatestRecoveryFile();
    if (latestFile == juce::File()) {
        std::cerr << "CrashRecoveryManager: No recovery files found" << std::endl;
        return false;
    }

    return restoreState(latestFile);
}

//==============================================================================
bool CrashRecoveryManager::detectPreviousCrash() const {
    // Check for crash flag file
    juce::File crashFlag = getCrashFlagFile();
    return crashFlag.exists();
}

//==============================================================================
void CrashRecoveryManager::markCleanStartup() {
    // Remove crash flag if it exists
    juce::File crashFlag = getCrashFlagFile();
    if (crashFlag.exists()) {
        crashFlag.deleteFile();
    }
}

//==============================================================================
void CrashRecoveryManager::markCleanShutdown() {
    // Remove crash flag
    markCleanStartup();

    // Create shutdown flag
    juce::File shutdownFlag = getCrashFlagFile().withFileExtension(".shutdown");
    shutdownFlag.create();
}

//==============================================================================
void CrashRecoveryManager::update() {
    if (!config_.enableAutoSave) {
        return;
    }

    // Check if auto-save is needed
    triggerAutoSave();
}

//==============================================================================
void CrashRecoveryManager::resetStatistics() {
    statistics_.crashesDetected = 0;
    statistics_.autoSavesTriggered = 0;
    statistics_.statesSaved = 0;
    statistics_.statesRestored = 0;
    statistics_.recoveriesFailed = 0;
    statistics_.lastSaveTime = 0.0;

    std::cout << "CrashRecoveryManager: Statistics reset" << std::endl;
}

//==============================================================================
void CrashRecoveryManager::setConfig(const CrashRecoveryConfig& config) {
    config_ = config;
}

//==============================================================================
// Private Methods
//==============================================================================

juce::File CrashRecoveryManager::generateAutoSavePath() const {
    double timestamp = juce::Time::getCurrentTime().currentTimeMillis() / 1000.0;

    juce::String filename = "autosave_" +
                           juce::String(timestamp, 0).replaceCharacter('.', '_') +
                           ".autosave";

    return config_.autoSaveDirectory.getChildFile(filename);
}

void CrashRecoveryManager::cleanupOldAutoSaves() {
    auto recoveryFiles = findRecoveryFiles();

    // Keep only the most recent files
    while (static_cast<int>(recoveryFiles.size()) > config_.maxAutoSaveFiles) {
        // Delete oldest file
        juce::File oldestFile = recoveryFiles.back();
        oldestFile.deleteFile();
        recoveryFiles.pop_back();
    }
}

void CrashRecoveryManager::recordEvent(const CrashEvent& event) {
    if (eventCallback_) {
        eventCallback_(event);
    }

    // Log important events
    if (event.type == CrashEventType::CrashDetected ||
        event.type == CrashEventType::RecoveryFailed) {
        std::cerr << "CrashRecoveryManager: " << event.toString() << std::endl;
    }
}

juce::File CrashRecoveryManager::getCrashFlagFile() const {
    return juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("zenith_daw_crash.flag");
}

} // namespace zenith
