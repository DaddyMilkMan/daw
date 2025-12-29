/**
 * @file SAVE_LOAD_EXAMPLES.cpp
 * @brief Usage examples for ProjectFileIO
 * 
 * This file shows practical examples of how to use the save/load system.
 * Copy/adapt these patterns into your MainWindow implementation.
 */

#pragma once

#include "Source/engine/ProjectFileIO.h"
#include "Source/engine/ProjectState.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * EXAMPLE 1: Basic New/Open/Save Operations
 */
class BasicFileOperations {
public:
    void example_newProject(std::unique_ptr<ProjectFileIO>& fileIO)
    {
        // Create new blank project
        fileIO->newProject();
        
        // Project is now empty and dirty=false
        // User can start adding tracks, clips, etc.
    }

    void example_openProject(std::unique_ptr<ProjectFileIO>& fileIO)
    {
        // Prompt user for file
        juce::FileChooser chooser("Open Zenith Project",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.zth");
        
        if (!chooser.browseForFileToOpen()) {
            return;  // User cancelled
        }
        
        // Load project
        FileIOError error = fileIO->loadFromFile(chooser.getResult());
        
        if (error != FileIOError::Success) {
            // Handle error
            juce::String msg = ProjectFileIO::getErrorMessage(error);
            juce::String details = fileIO->getLastErrorDetails();
            
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Load Failed",
                msg + "\n\nDetails: " + details);
            return;
        }
        
        // Success! Project is loaded and dirty=false
        DBG("Loaded project: " << chooser.getResult().getFullPathName());
    }

    void example_saveProject(std::unique_ptr<ProjectFileIO>& fileIO)
    {
        juce::File projectFile = fileIO->getCurrentProjectFile();
        
        // If no file yet, use Save As
        if (!projectFile.existsAsFile()) {
            example_saveProjectAs(fileIO);
            return;
        }
        
        // Save to existing file
        FileIOError error = fileIO->saveToFile(projectFile);
        
        if (error != FileIOError::Success) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                ProjectFileIO::getErrorMessage(error));
            return;
        }
        
        // Success! dirty flag is now false
        DBG("Saved to: " << projectFile.getFullPathName());
    }

    void example_saveProjectAs(std::unique_ptr<ProjectFileIO>& fileIO)
    {
        // Prompt for new filename
        juce::FileChooser chooser("Save Zenith Project As",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.zth");
        
        if (!chooser.browseForFileToSave(true)) {
            return;  // User cancelled
        }
        
        juce::File targetFile = chooser.getResult();
        
        // Ensure .zth extension
        if (!targetFile.getFileExtension().equalsIgnoreCase(".zth")) {
            targetFile = targetFile.withFileExtension(".zth");
        }
        
        // Save with new filename
        FileIOError error = fileIO->saveToFileAs(targetFile);
        
        if (error != FileIOError::Success) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                ProjectFileIO::getErrorMessage(error));
            return;
        }
        
        DBG("Saved as: " << targetFile.getFullPathName());
    }
};

/**
 * EXAMPLE 2: Handling Unsaved Changes
 */
class UnsavedChangesHandling {
public:
    bool example_checkUnsavedBeforeClose(
        std::unique_ptr<ProjectFileIO>& fileIO,
        std::unique_ptr<ProjectState>& projectState)
    {
        // Check if project has unsaved changes
        if (!projectState->hasUnsavedChanges()) {
            return true;  // Safe to close
        }
        
        // Show save dialog
        int result = juce::NativeMessageBox::showYesNoCancelBox(
            juce::AlertWindow::WarningIcon,
            "Unsaved Changes",
            "Save changes before closing?");
        
        // JUCE return values:
        // 1 = Yes, 2 = No, 0 = Cancel
        
        if (result == 1) {  // Yes - save and close
            juce::File projectFile = fileIO->getCurrentProjectFile();
            if (!projectFile.existsAsFile()) {
                // Show Save As dialog
                // ... implement ...
                return false;  // Cancelled during Save As
            }
            
            FileIOError error = fileIO->saveToFile(projectFile);
            if (error != FileIOError::Success) {
                return false;  // Save failed
            }
            return true;  // Saved successfully, ok to close
        }
        else if (result == 2) {  // No - discard changes and close
            return true;
        }
        else {  // Cancel
            return false;
        }
    }

    void example_windowTitleWithDirtyIndicator(
        std::unique_ptr<ProjectFileIO>& fileIO,
        std::unique_ptr<ProjectState>& projectState,
        juce::DocumentWindow& window)
    {
        juce::File projectFile = fileIO->getCurrentProjectFile();
        juce::String title = "Zenith DAW";
        
        // Add project name if saved
        if (projectFile.existsAsFile()) {
            title += " - " + projectFile.getFileNameWithoutExtension();
        } else {
            title += " - [Untitled]";
        }
        
        // Add dirty indicator (asterisk = unsaved)
        if (projectState->hasUnsavedChanges()) {
            title += " *";
        }
        
        window.setName(title);
    }
};

/**
 * EXAMPLE 3: Auto-Save and Recovery
 */
class AutoSaveAndRecovery {
public:
    void example_setupAutoSave(std::unique_ptr<ProjectFileIO>& fileIO)
    {
        // Configure auto-save to happen every 5 minutes
        fileIO->setAutoSaveInterval(300);
        
        // Enable auto-save feature
        fileIO->setAutoSaveEnabled(true);
        
        // Start a timer that checks every 30 seconds
        // In MainWindow, inherit from juce::Timer
        // startTimer(30000);  // 30 seconds in milliseconds
    }

    void example_timerCallback(std::unique_ptr<ProjectFileIO>& fileIO)
    {
        // This is called from MainWindow::timerCallback()
        // Every 30 seconds, check if it's time for auto-save
        
        if (fileIO->autoSave()) {
            DBG("Auto-saved successfully");
        }
        // Returns false if save failed, but doesn't throw
    }

    void example_checkForRecoveryOnStartup(
        std::unique_ptr<ProjectFileIO>& fileIO,
        std::unique_ptr<ProjectState>& projectState)
    {
        // Call this in MainWindow constructor after creating fileIO_
        
        auto recoveries = fileIO->getAvailableRecoveries();
        
        if (recoveries.empty()) {
            return;  // No recovery files
        }
        
        // Show recovery dialog
        juce::AlertWindow dialog("Project Recovery",
            "Zenith detected unsaved work from a previous session.",
            juce::AlertWindow::QuestionIcon);
        
        dialog.addButton("Recover Latest", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog.addButton("Recover from List", 2);
        dialog.addButton("Discard", 3);
        
        int result = dialog.showDialog();
        
        if (result == 1) {
            // Recover latest
            auto latest = recoveries.back();
            FileIOError error = fileIO->recoverFromFile(latest.recoveryFile);
            
            if (error != FileIOError::Success) {
                DBG("Recovery failed: " << ProjectFileIO::getErrorMessage(error));
                return;
            }
            
            DBG("Recovered project successfully");
            
            // Optionally clean up the recovery file since we've recovered
            // fileIO->deleteRecoveryFile(latest.recoveryFile);
        }
        else if (result == 2) {
            // Show list of recoveries (implement recovery browser UI)
            // TODO: Implement recovery file browser
        }
        // result == 3 means discard - do nothing
    }

    void example_listAndBrowseRecoveries(
        std::unique_ptr<ProjectFileIO>& fileIO)
    {
        auto recoveries = fileIO->getAvailableRecoveries();
        
        for (const auto& recovery : recoveries) {
            juce::Time saveTime(recovery.recoveryTimestamp);
            juce::String timeStr = saveTime.formatted("%Y-%m-%d %H:%M:%S");
            
            DBG("Recovery: " << recovery.recoveryFile.getFileName() 
                << " - Saved at " << timeStr);
            
            if (recovery.isAutoSave) {
                DBG("  (Auto-save)");
            }
        }
    }
};

/**
 * EXAMPLE 4: Backups
 */
class BackupManagement {
public:
    void example_createManualBackup(
        std::unique_ptr<ProjectFileIO>& fileIO)
    {
        // Save project first
        juce::File projectFile = fileIO->getCurrentProjectFile();
        if (!projectFile.existsAsFile()) {
            DBG("No project file to backup");
            return;
        }
        
        // Create backup
        juce::File backupFile = fileIO->createBackup();
        
        if (backupFile.existsAsFile()) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Backup Created",
                "Project backed up to:\n" + backupFile.getFullPathName());
        } else {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Backup Failed",
                "Could not create backup");
        }
    }

    void example_listAndManageBackups(
        std::unique_ptr<ProjectFileIO>& fileIO)
    {
        auto backups = fileIO->getBackupFiles();
        
        if (backups.empty()) {
            DBG("No backups found");
            return;
        }
        
        DBG("=== Available Backups ===");
        for (const auto& backup : backups) {
            juce::Time backupTime(backup.getLastModificationTime());
            juce::String timeStr = backupTime.formatted("%Y-%m-%d %H:%M:%S");
            
            DBG("- " << backup.getFileName() 
                << " (" << backup.getSize() << " bytes)"
                << " - " << timeStr);
        }
        
        // To restore a backup, load it:
        // fileIO->loadFromFile(backups[0]);
    }

    void example_setBackupRetention(
        std::unique_ptr<ProjectFileIO>& fileIO)
    {
        // Keep only the last 5 backups instead of default 10
        fileIO->setMaxBackups(5);
        
        // Next time a backup is created, old ones will be cleaned up
        fileIO->createBackup();  // Old backups beyond 5 will be deleted
    }
};

/**
 * EXAMPLE 5: Validation and Metadata
 */
class ValidationAndMetadata {
public:
    void example_validateFileBeforeOpening(
        std::unique_ptr<ProjectFileIO>& fileIO)
    {
        juce::FileChooser chooser("Open Zenith Project",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.zth");
        
        if (!chooser.browseForFileToOpen()) {
            return;
        }
        
        juce::File file = chooser.getResult();
        
        // Validate before loading
        if (!fileIO->validateFile(file)) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Invalid File",
                "File is not a valid Zenith project or is corrupted");
            return;
        }
        
        // File is valid, safe to load
        FileIOError error = fileIO->loadFromFile(file);
        // ... handle result ...
    }

    void example_readMetadataWithoutLoading(
        std::unique_ptr<ProjectFileIO>& fileIO)
    {
        juce::File projectFile("C:/Projects/MyProject.zth");
        
        // Read metadata WITHOUT loading entire project
        ProjectMetadata meta = fileIO->readMetadata(projectFile);
        
        juce::String info;
        info << "Project: " << projectFile.getFileName() << "\n"
             << "Format Version: " << meta.version << "\n"
             << "Zenith Version: " << meta.zenithVersion << "\n"
             << "Sample Rate: " << (int)meta.sampleRate << " Hz\n"
             << "Tracks: " << meta.trackCount << "\n"
             << "Duration: " << meta.durationSeconds << " seconds\n";
        
        DBG(info);
    }

    void example_checkFileType(
        std::unique_ptr<ProjectFileIO>& fileIO,
        const juce::File& file)
    {
        if (!fileIO->isValidZenithProject(file)) {
            DBG("Not a Zenith project file");
            return;
        }
        
        DBG("Valid Zenith project");
    }
};

/**
 * EXAMPLE 6: Error Handling
 */
class ErrorHandling {
public:
    void example_detailedErrorHandling(
        std::unique_ptr<ProjectFileIO>& fileIO,
        const juce::File& file)
    {
        FileIOError error = fileIO->loadFromFile(file);
        
        if (error == FileIOError::Success) {
            DBG("File loaded successfully");
            return;
        }
        
        // Get error information
        juce::String errorMsg = ProjectFileIO::getErrorMessage(error);
        juce::String details = fileIO->getLastErrorDetails();
        
        // Log for debugging
        DBG("Load failed: " << errorMsg);
        DBG("Details: " << details);
        
        // Show user-friendly message
        juce::String userMessage;
        
        switch (error) {
            case FileIOError::FileNotFound:
                userMessage = "The file could not be found.";
                break;
            case FileIOError::InvalidFormat:
                userMessage = "The file is not a valid Zenith project.";
                break;
            case FileIOError::CorruptedFile:
                userMessage = "The file appears to be corrupted.";
                break;
            case FileIOError::InsufficientDiskSpace:
                userMessage = "Not enough disk space to save the file.";
                break;
            case FileIOError::PermissionDenied:
                userMessage = "Permission denied. Check file permissions.";
                break;
            case FileIOError::ParseError:
                userMessage = "Failed to parse the file.";
                break;
            case FileIOError::VersionMismatch:
                userMessage = "File version is not compatible.";
                break;
            default:
                userMessage = "An unknown error occurred.";
                break;
        }
        
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Error",
            userMessage);
    }
};

/**
 * EXAMPLE 7: Integration into MainWindow
 */
class MainWindowIntegration : public juce::DocumentWindow {
public:
    MainWindowIntegration()
        : juce::DocumentWindow("Zenith DAW", 
                              juce::Colours::darkgrey,
                              juce::DocumentWindow::allButtons, true)
    {
        // Create FileIO after ProjectState
        projectState_ = std::make_unique<ProjectState>();
        fileIO_ = std::make_unique<ProjectFileIO>(*projectState_);
        
        // Configure auto-save
        fileIO_->setAutoSaveInterval(300);      // 5 minutes
        fileIO_->setAutoSaveEnabled(true);
        fileIO_->setMaxBackups(10);
        
        // Start auto-save timer (check every 30 seconds)
        startTimer(30000);
        
        // Check for recovery files from previous crash
        checkForRecovery();
    }

    void timerCallback() override
    {
        // Called every 30 seconds
        fileIO_->autoSave();
    }

    void closeButtonPressed() override
    {
        // User clicked X button
        if (projectState_->hasUnsavedChanges()) {
            int result = juce::NativeMessageBox::showYesNoCancelBox(
                juce::AlertWindow::WarningIcon,
                "Unsaved Changes",
                "Save changes before closing?");
            
            if (result == 1) {  // Yes
                menuItemSelected_Save();
            } else if (result == 0) {  // Cancel
                return;
            }
        }
        
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    std::unique_ptr<ProjectState> projectState_;
    std::unique_ptr<ProjectFileIO> fileIO_;

    void menuItemSelected_Save()
    {
        juce::File projectFile = fileIO_->getCurrentProjectFile();
        
        if (!projectFile.existsAsFile()) {
            menuItemSelected_SaveAs();
            return;
        }
        
        FileIOError error = fileIO_->saveToFile(projectFile);
        if (error != FileIOError::Success) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                ProjectFileIO::getErrorMessage(error));
        }
    }

    void menuItemSelected_SaveAs()
    {
        // ... implement ...
    }

    void checkForRecovery()
    {
        // ... implement ...
    }
};

} // namespace zenith
