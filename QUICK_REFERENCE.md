# ProjectFileIO - Quick Reference Card

## Constructor
```cpp
fileIO_ = std::make_unique<ProjectFileIO>(*projectState_);
```

## New/Open/Save

| Operation | Method | Returns |
|-----------|--------|---------|
| New blank project | `newProject()` | void |
| Load from file | `loadFromFile(file)` | FileIOError |
| Save to file | `saveToFile(file)` | FileIOError |
| Save with new name | `saveToFileAs(file)` | FileIOError |

## Examples
```cpp
// New project
fileIO_->newProject();

// Load
FileIOError err = fileIO_->loadFromFile(file);
if (err == FileIOError::Success) { /* ok */ }

// Save
err = fileIO_->saveToFile(currentFile);
if (err != FileIOError::Success) {
    juce::String msg = ProjectFileIO::getErrorMessage(err);
}

// Save As
err = fileIO_->saveToFileAs(newFile);
```

## Auto-Save Configuration

| Method | Purpose |
|--------|---------|
| `setAutoSaveInterval(seconds)` | Set interval (default: 300 sec = 5 min) |
| `setAutoSaveEnabled(bool)` | Enable/disable auto-save |
| `autoSave()` | Manually trigger auto-save |

## Examples
```cpp
// Setup (in constructor)
fileIO_->setAutoSaveInterval(300);      // 5 minutes
fileIO_->setAutoSaveEnabled(true);
startTimer(30000);                       // Check every 30 sec

// In timerCallback
void timerCallback() override {
    fileIO_->autoSave();  // Check if time for auto-save
}
```

## Recovery & Backups

| Method | Returns |
|--------|---------|
| `getRecoveryFile()` | juce::File (latest) |
| `getAvailableRecoveries()` | vector<RecoveryInfo> |
| `recoverFromFile(file)` | FileIOError |
| `deleteRecoveryFile(file)` | void |
| `createBackup()` | juce::File |
| `getBackupFiles()` | vector<juce::File> |

## Examples
```cpp
// Check for recovery on startup
auto recoveries = fileIO_->getAvailableRecoveries();
if (!recoveries.empty()) {
    fileIO_->recoverFromFile(recoveries.back().recoveryFile);
}

// Create backup
juce::File backup = fileIO_->createBackup();

// List backups
auto backups = fileIO_->getBackupFiles();
for (const auto& file : backups) {
    DBG(file.getFileName());
}

// Set max backups to keep
fileIO_->setMaxBackups(10);
```

## Validation & Metadata

| Method | Returns |
|--------|---------|
| `validateFile(file)` | bool |
| `isValidZenithProject(file)` | bool |
| `readMetadata(file)` | ProjectMetadata |

## Examples
```cpp
// Validate before loading
if (fileIO_->validateFile(file)) {
    fileIO_->loadFromFile(file);
}

// Read metadata without loading
ProjectMetadata meta = fileIO_->readMetadata(file);
DBG("Created by: " << meta.createdBy);
DBG("Tracks: " << meta.trackCount);
DBG("Sample rate: " << meta.sampleRate);

// Check file type
if (fileIO_->isValidZenithProject(file)) {
    // It's a Zenith project
}
```

## Error Handling

| Error Type | Meaning |
|------------|---------|
| Success | Operation completed |
| FileNotFound | File doesn't exist |
| InvalidFormat | Not a valid project |
| CorruptedFile | File is damaged |
| InsufficientDiskSpace | Can't save |
| PermissionDenied | Access denied |
| WriteError | Write failed |
| ParseError | XML parsing failed |
| VersionMismatch | File version incompatible |
| Unknown | Unknown error |

## Examples
```cpp
FileIOError error = fileIO_->loadFromFile(file);

// Get error message
juce::String msg = ProjectFileIO::getErrorMessage(error);

// Get detailed info
juce::String details = fileIO_->getLastErrorDetails();

// Log it
if (error != FileIOError::Success) {
    DBG("Error: " << msg << " - " << details);
}

// Handle specific errors
switch (error) {
    case FileIOError::FileNotFound:
        // File doesn't exist
        break;
    case FileIOError::InvalidFormat:
        // Not a valid project
        break;
    case FileIOError::InsufficientDiskSpace:
        // Out of disk space
        break;
    default:
        // Other error
        break;
}
```

## Check for Unsaved Changes

```cpp
if (projectState_->hasUnsavedChanges()) {
    // Show save dialog
}

// Mark as changed
projectState_->markDirty();

// Or directly
projectState_->isDirty.store(true);
```

## Current Project File

```cpp
// Get current file
juce::File current = fileIO_->getCurrentProjectFile();

// Check if saved
if (current.existsAsFile()) {
    // Saved to disk
} else {
    // Unsaved (untitled)
}
```

## Typical MainWindow Integration

```cpp
class MainWindow : public juce::DocumentWindow, private juce::Timer {
private:
    std::unique_ptr<ProjectFileIO> fileIO_;
    std::unique_ptr<ProjectState> projectState_;

public:
    MainWindow(const juce::String& name) 
        : juce::DocumentWindow(name, ...) {
        // Create state and FileIO
        projectState_ = std::make_unique<ProjectState>();
        fileIO_ = std::make_unique<ProjectFileIO>(*projectState_);
        
        // Configure
        fileIO_->setAutoSaveInterval(300);
        fileIO_->setAutoSaveEnabled(true);
        startTimer(30000);  // Check every 30 sec
        
        // Check for recovery
        checkForRecovery();
    }

    void timerCallback() override {
        fileIO_->autoSave();
    }

    void closeButtonPressed() override {
        if (projectState_->hasUnsavedChanges()) {
            // Ask to save
        }
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    void menuFile_Save() {
        FileIOError err = fileIO_->saveToFile(
            fileIO_->getCurrentProjectFile());
        if (err != FileIOError::Success) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                ProjectFileIO::getErrorMessage(err));
        }
    }

    void checkForRecovery() {
        auto recoveries = fileIO_->getAvailableRecoveries();
        if (!recoveries.empty()) {
            // Show recovery dialog
        }
    }
};
```

## File Locations

| Type | Location |
|------|----------|
| Projects | User chooses (save dialog) |
| Recovery files | ~/Documents/ZenithDAW/RecoveryFiles/ |
| Backups | ~/Projects/MyProject/Backups/ |
| Temp files | Same dir as target (*.tmp) |

## Configuration (Defaults)

| Setting | Default | Change With |
|---------|---------|-------------|
| Auto-save interval | 300 sec (5 min) | `setAutoSaveInterval()` |
| Auto-save enabled | true | `setAutoSaveEnabled()` |
| Max backups | 10 | `setMaxBackups()` |
| Recovery retention | 7 days | (automatic) |

## File Format

Extension: `.zth`
Format: XML
Size: ~5% of audio data

Example structure:
```xml
<ZenithProject formatVersion="1.0.0" 
               zenithVersion="0.1.0"
               savedTimestamp="1702584000000"
               sampleRate="48000.0"
               trackCount="2">
    <Tracks>
        <!-- All tracks here -->
    </Tracks>
    <Mixer>
        <!-- Mixer state -->
    </Mixer>
</ZenithProject>
```

## Common Patterns

### Pattern 1: Safe Close
```cpp
void closeWindow() {
    if (projectState_->hasUnsavedChanges()) {
        int result = juce::NativeMessageBox::showYesNoCancelBox(
            juce::AlertWindow::WarningIcon,
            "Save?", "Save changes before closing?");
        if (result == 1) saveProject();
        else if (result == 0) return;  // Cancel
    }
    quit();
}
```

### Pattern 2: Unsaved Indicator
```cpp
void updateTitle() {
    juce::String title = "MyDAW";
    if (fileIO_->getCurrentProjectFile().existsAsFile())
        title += " - " + fileName;
    if (projectState_->hasUnsavedChanges())
        title += " *";
    setName(title);
}
```

### Pattern 3: Error Dialog
```cpp
FileIOError err = fileIO_->loadFromFile(file);
if (err != FileIOError::Success) {
    juce::NativeMessageBox::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Load Failed",
        ProjectFileIO::getErrorMessage(err));
}
```

### Pattern 4: Recovery Check
```cpp
auto recoveries = fileIO_->getAvailableRecoveries();
if (!recoveries.empty()) {
    fileIO_->recoverFromFile(recoveries.back().recoveryFile);
}
```

---

**For full integration steps, see: `docs/SAVE_LOAD_IMPLEMENTATION.md`**
**For working examples, see: `docs/SAVE_LOAD_EXAMPLES.cpp`**
**For checklist, see: `SAVE_LOAD_CHECKLIST.md`**
