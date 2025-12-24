# Save/Load Implementation Guide

## Overview

A production-grade file I/O system has been added to handle project saving, loading, recovery, and backups.

**Files Added:**
- `Source/engine/ProjectFileIO.h` - Header with full API
- `Source/engine/ProjectFileIO.cpp` - Implementation
- Updated `Source/engine/ProjectState.h` - Added methods for save/load support

## Features

### 1. Atomic Writes
- Writes to temporary file first
- Renames temp → target on success
- Prevents corrupted files if crash/power loss during save
- Industry-standard approach

### 2. Error Handling
- 9 specific error types (not just "it failed")
- Detailed error messages with context
- Graceful degradation for version mismatches
- Thread-safe error tracking

### 3. Auto-Save System
- Configurable interval (default: 5 minutes)
- Non-blocking
- Saves to dedicated recovery directory
- Can be enabled/disabled

### 4. Crash Recovery
- Auto-saves stored in `~/Documents/ZenithDAW/RecoveryFiles/`
- Automatically cleaned up after 7 days
- Dialog on startup to recover
- Browse/select which recovery file to restore

### 5. Backup Management
- Manual backups stored alongside project
- Automatic cleanup keeps last N backups (default: 10)
- Timestamped for easy identification
- Can restore from any backup

### 6. Metadata & Validation
- Read metadata without loading entire file
- Validate files before opening
- Version tracking for forward compatibility
- Sample rate, track count, duration info

## Integration Steps

### Step 1: Update CMakeLists.txt

Add to your target source files:

```cmake
target_sources(ZenithDAW PRIVATE
    Source/engine/ProjectFileIO.h
    Source/engine/ProjectFileIO.cpp
)
```

### Step 2: Add Required Methods to ProjectState

These methods need to be implemented in ProjectState.cpp:

```cpp
// Create empty default state (called when making new project)
void ProjectState::createDefaultState()
{
    state = juce::ValueTree(ID_PROJECT);
    state.setProperty(PROP_NAME, "Untitled");
    state.setProperty(PROP_TEMPO, 120.0);
    state.setProperty(PROP_TIME_SIG_NUM, 4);
    state.setProperty(PROP_TIME_SIG_DEN, 4);
    state.setProperty(PROP_SAMPLE_RATE, sampleRate_);
    
    // Create empty tracks container
    auto tracksNode = juce::ValueTree(ID_TRACKS);
    state.addChild(tracksNode, -1, nullptr);
    
    // Create mixer
    auto mixerNode = juce::ValueTree(ID_MIXER);
    state.addChild(mixerNode, -1, nullptr);
    
    // Create tempo map
    auto tempoNode = juce::ValueTree(ID_TEMPO_MAP);
    state.addChild(tempoNode, -1, nullptr);
}

// Rebuild ID counter after loading
void ProjectState::rebuildIdCounter()
{
    juce::int64 maxId = 0;
    auto tracksNode = state.getChildWithName(ID_TRACKS);
    
    if (tracksNode.isValid()) {
        for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
            auto track = tracksNode.getChild(i);
            int trackId = track.getProperty(PROP_ID, 0);
            maxId = juce::jmax(maxId, (juce::int64)trackId);
        }
    }
    
    idCounter.store(maxId + 1);
}

// Rebuild track lookup map
void ProjectState::rebuildTrackMap()
{
    trackIdMap_.clear();
    auto tracksNode = state.getChildWithName(ID_TRACKS);
    
    if (tracksNode.isValid()) {
        for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
            auto track = tracksNode.getChild(i);
            int trackId = track.getProperty(PROP_ID, 0);
            trackIdMap_[trackId] = i;
        }
    }
}

// Get number of tracks
int ProjectState::getNumTracks() const
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);
    return tracksNode.isValid() ? tracksNode.getNumChildren() : 0;
}
```

### Step 3: Update MainWindow

Create fileIO instance:

```cpp
// In MainWindow.h
class MainWindow : public juce::DocumentWindow, private juce::Timer {
private:
    std::unique_ptr<ProjectFileIO> fileIO_;
    
    void menuItemSelected_NewProject();
    void menuItemSelected_OpenProject();
    void menuItemSelected_Save();
    void menuItemSelected_SaveAs();
    void checkForRecovery();
    void createManualBackup();
    void timerCallback() override;
    void closeButtonPressed() override;
    void updateWindowTitle();
};

// In MainWindow constructor
MainWindow::MainWindow(const juce::String& name)
    : juce::DocumentWindow(name, ...)
{
    projectState_ = std::make_unique<ProjectState>();
    fileIO_ = std::make_unique<ProjectFileIO>(*projectState_);
    
    // Configure auto-save
    fileIO_->setAutoSaveInterval(300);      // 5 minutes
    fileIO_->setAutoSaveEnabled(true);
    fileIO_->setMaxBackups(10);
    
    // Start auto-save timer (check every 30 seconds)
    startTimer(30000);
    
    // Check for recovery files on startup
    checkForRecovery();
}
```

### Step 4: Implement File Menu

```cpp
void MainWindow::menuItemSelected_NewProject()
{
    if (projectState_->hasUnsavedChanges()) {
        int result = juce::NativeMessageBox::showYesNoCancelBox(
            juce::AlertWindow::WarningIcon,
            "Unsaved Changes",
            "Save changes before creating a new project?");
        
        if (result == 1) {
            menuItemSelected_Save();
        } else if (result == 0) {
            return;  // Cancel
        }
    }
    
    fileIO_->newProject();
    updateWindowTitle();
    repaint();
}

void MainWindow::menuItemSelected_OpenProject()
{
    juce::FileChooser chooser("Open Zenith Project",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.zth");
    
    if (chooser.browseForFileToOpen()) {
        FileIOError error = fileIO_->loadFromFile(chooser.getResult());
        
        if (error != FileIOError::Success) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Load Failed",
                "Failed to load project: " + 
                ProjectFileIO::getErrorMessage(error) +
                "\n\nDetails: " + fileIO_->getLastErrorDetails());
            return;
        }
        
        updateWindowTitle();
        repaint();
    }
}

void MainWindow::menuItemSelected_Save()
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
            "Failed to save project: " + ProjectFileIO::getErrorMessage(error));
        return;
    }
    
    updateWindowTitle();
}

void MainWindow::menuItemSelected_SaveAs()
{
    juce::FileChooser chooser("Save Zenith Project As",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.zth");
    
    if (chooser.browseForFileToSave(true)) {
        juce::File targetFile = chooser.getResult();
        
        if (!targetFile.getFileExtension().equalsIgnoreCase(".zth")) {
            targetFile = targetFile.withFileExtension(".zth");
        }
        
        FileIOError error = fileIO_->saveToFileAs(targetFile);
        
        if (error != FileIOError::Success) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Failed to save project: " + ProjectFileIO::getErrorMessage(error));
            return;
        }
        
        updateWindowTitle();
    }
}

void MainWindow::timerCallback()
{
    fileIO_->autoSave();
}

void MainWindow::closeButtonPressed()
{
    if (projectState_->hasUnsavedChanges()) {
        int result = juce::NativeMessageBox::showYesNoCancelBox(
            juce::AlertWindow::WarningIcon,
            "Unsaved Changes",
            "Save changes before closing?");
        
        if (result == 1) {
            menuItemSelected_Save();
        } else if (result == 0) {
            return;
        }
    }
    
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::checkForRecovery()
{
    auto recoveries = fileIO_->getAvailableRecoveries();
    
    if (recoveries.empty()) {
        return;
    }
    
    juce::AlertWindow dialog("Project Recovery",
        "Zenith detected unsaved work from a previous session.",
        juce::AlertWindow::QuestionIcon);
    
    dialog.addButton("Recover Latest", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog.addButton("Discard", 2);
    
    int result = dialog.showDialog();
    
    if (result == 1) {
        FileIOError error = fileIO_->recoverFromFile(recoveries.back().recoveryFile);
        if (error == FileIOError::Success) {
            updateWindowTitle();
            repaint();
        }
    }
}

void MainWindow::createManualBackup()
{
    juce::File backupFile = fileIO_->createBackup();
    
    if (backupFile.existsAsFile()) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Backup Created",
            "Project backed up to:\n" + backupFile.getFullPathName());
    }
}

void MainWindow::updateWindowTitle()
{
    juce::File projectFile = fileIO_->getCurrentProjectFile();
    juce::String title = "Zenith DAW";
    
    if (projectFile.existsAsFile()) {
        title += " - " + projectFile.getFileNameWithoutExtension();
    } else {
        title += " - [Untitled]";
    }
    
    if (projectState_->hasUnsavedChanges()) {
        title += " *";
    }
    
    setName(title);
}
```

### Step 5: Track Changes with isDirty Flag

In ProjectState, whenever state changes:

```cpp
void ProjectState::addTrack(const juce::String& name)
{
    // ... add track logic ...
    isDirty.store(true);
}

void ProjectState::deleteTrack(int trackIndex)
{
    // ... delete track logic ...
    isDirty.store(true);
}

void ProjectState::setTrackVolume(int trackIndex, float volume)
{
    // ... set volume logic ...
    isDirty.store(true);
}

// For every mutation that should trigger "unsaved changes"
```

## API Quick Reference

### Core Operations

```cpp
// New project
fileIO_->newProject();

// Load
FileIOError error = fileIO_->loadFromFile(file);

// Save
error = fileIO_->saveToFile(file);

// Save As
error = fileIO_->saveToFileAs(newFile);
```

### Recovery & Backups

```cpp
// Auto-save configuration
fileIO_->setAutoSaveInterval(300);      // seconds
fileIO_->setAutoSaveEnabled(true);

// Manual trigger
fileIO_->autoSave();

// Recovery
auto recoveries = fileIO_->getAvailableRecoveries();
fileIO_->recoverFromFile(recoveries[0].recoveryFile);
fileIO_->deleteRecoveryFile(file);

// Backups
juce::File backup = fileIO_->createBackup();
auto backups = fileIO_->getBackupFiles();
```

### Metadata & Validation

```cpp
// Read metadata without loading
ProjectMetadata meta = fileIO_->readMetadata(file);

// Validate
if (fileIO_->validateFile(file)) {
    fileIO_->loadFromFile(file);
}

// Check if valid Zenith project
bool isZenith = fileIO_->isValidZenithProject(file);
```

### Error Handling

```cpp
FileIOError error = fileIO_->loadFromFile(file);
if (error != FileIOError::Success) {
    juce::String msg = ProjectFileIO::getErrorMessage(error);
    juce::String details = fileIO_->getLastErrorDetails();
}
```

## File Format

Projects are saved as `.zth` files (XML-based):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ZenithProject formatVersion="1.0.0" zenithVersion="0.1.0" 
               savedTimestamp="1702584000000" sampleRate="48000.0" 
               trackCount="2" duration="120.5" createdBy="CompName">
    <Tracks>
        <Track id="1" name="Lead" volume="0.8" pan="0" mute="0" solo="0" armed="0">
            <!-- Track clips and data -->
        </Track>
    </Tracks>
    <Mixer>
        <!-- Mixer state -->
    </Mixer>
    <TempoMap>
        <!-- Tempo changes -->
    </TempoMap>
</ZenithProject>
```

## Directories

**Recovery Files:**
```
~/Documents/ZenithDAW/RecoveryFiles/
├── autosave_20251214_143022.zth
└── autosave_20251214_143522.zth
```

**Project Backups:**
```
~/Projects/MyProject.zth
~/Projects/Backups/
├── MyProject_20251214_120000.zth
└── MyProject_20251210_093000.zth
```

## Testing

To test the system:

1. Create a new project: `fileIO_->newProject()`
2. Add some tracks
3. Save: `fileIO_->saveToFile(file)`
4. Load: `fileIO_->loadFromFile(file)`
5. Check recovery files exist in Documents/ZenithDAW/RecoveryFiles/
6. Verify backups created when saving

## Notes

- The system is thread-safe for metadata operations
- Auto-saves run on the message thread (non-blocking)
- Recovery files older than 7 days are automatically deleted
- Maximum of 10 backup files are kept (configurable)
- All errors are logged to debug output
