/**
 * @file ProjectManager.cpp
 * @brief ProjectManager implementation
 */

#include "../include/ProjectManager.h"

//==============================================================================
// Constructor / Destructor
//==============================================================================

ProjectManager::ProjectManager(ProjectState& ps, juce::UndoManager& um)
    : FileBasedDocument(
          ".zth",                           // File extension
          "*.zth",                          // File wildcard for dialogs
          "Open Project",                   // Open dialog title
          "Save Project"                    // Save dialog title
      ),
      projectState(ps),
      undoManager(um),
      lastSavedUndoIndex(0)
{
    // Start with no unsaved changes
    setChangedFlag(false);
}

ProjectManager::~ProjectManager()
{
}

//==============================================================================
// High-level operations
//==============================================================================

bool ProjectManager::hasUnsavedChanges() const
{
    // Check if document has changed since last save
    return hasChangedSinceSave();
}

bool ProjectManager::saveProject(bool askUserForFileIfNotSpecified,
                                  bool showMessageOnFailure)
{
    // Use FileBasedDocument's save method
    auto result = save(askUserForFileIfNotSpecified, showMessageOnFailure);

    if (result == FileBasedDocument::savedOk)
    {
        // Update undo manager tracking
        lastSavedUndoIndex = undoManager.getNumActionsInCurrentTransaction();
        return true;
    }

    return false;
}

bool ProjectManager::saveProjectAs(bool showMessageOnFailure)
{
    // Use FileBasedDocument's saveAs method
    auto result = saveAs(juce::File(), true, true, showMessageOnFailure);

    if (result == FileBasedDocument::savedOk)
    {
        // Update undo manager tracking
        lastSavedUndoIndex = undoManager.getNumActionsInCurrentTransaction();
        return true;
    }

    return false;
}

bool ProjectManager::loadProject(const juce::File& fileToLoad)
{
    juce::File file = fileToLoad;

    // Show file chooser if no file provided
    if (!file.existsAsFile())
    {
        juce::FileChooser chooser("Open Project",
                                  getLastDocumentOpened(),
                                  "*.zth");

        if (!chooser.browseForFileToOpen())
            return false;  // User cancelled

        file = chooser.getResult();
    }

    // Load using FileBasedDocument
    auto result = loadFrom(file, true);

    if (result == FileBasedDocument::loadedOk)
    {
        // Reset change tracking
        lastSavedUndoIndex = 0;
        undoManager.clearUndoHistory();
        setChangedFlag(false);

        DBG("Loaded project: " + file.getFullPathName());
        return true;
    }

    DBG("Failed to load project: " + file.getFullPathName());
    return false;
}

bool ProjectManager::newProject()
{
    // Check if current project needs saving
    if (hasUnsavedChanges())
    {
        auto result = saveIfNeededAndUserAgrees();

        if (result == FileBasedDocument::failedToWriteToFile)
            return false;  // Save failed

        if (result == FileBasedDocument::userCancelledSave)
            return false;  // User cancelled
    }

    // Create new project
    projectState.newProject();

    // Clear file association
    setFile(juce::File());

    // Reset change tracking
    lastSavedUndoIndex = 0;
    undoManager.clearUndoHistory();
    setChangedFlag(false);

    DBG("Created new project");
    return true;
}

juce::File ProjectManager::getCurrentProjectFile() const
{
    return getFile();
}

juce::String ProjectManager::getCurrentProjectName() const
{
    auto file = getFile();

    if (file.existsAsFile())
        return file.getFileNameWithoutExtension();

    return projectState.getProjectName();
}

void ProjectManager::markAsChanged()
{
    changed();
}

//==============================================================================
// FileBasedDocument overrides
//==============================================================================

juce::Result ProjectManager::loadDocument(const juce::File& file)
{
    // Delegate to ProjectState
    if (projectState.loadFromFile(file))
    {
        DBG("ProjectManager: Successfully loaded " + file.getFullPathName());
        return juce::Result::ok();
    }

    return juce::Result::fail("Failed to load project file: " + file.getFullPathName());
}

juce::Result ProjectManager::saveDocument(const juce::File& file)
{
    // Delegate to ProjectState
    if (projectState.saveToFile(file))
    {
        DBG("ProjectManager: Successfully saved " + file.getFullPathName());
        return juce::Result::ok();
    }

    return juce::Result::fail("Failed to save project file: " + file.getFullPathName());
}

juce::File ProjectManager::getLastDocumentOpened()
{
    // Load from application properties
    auto& properties = juce::PropertiesFile::Options().getDefaultFile();

    if (properties.existsAsFile())
    {
        auto props = std::make_unique<juce::PropertiesFile>(properties);
        auto lastFile = props->getValue("lastProjectFile");

        if (lastFile.isNotEmpty())
            return juce::File(lastFile);
    }

    // Default to documents directory
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
}

void ProjectManager::setLastDocumentOpened(const juce::File& file)
{
    // Save to application properties
    auto& properties = juce::PropertiesFile::Options().getDefaultFile();

    auto props = std::make_unique<juce::PropertiesFile>(properties);
    props->setValue("lastProjectFile", file.getFullPathName());
    props->saveIfNeeded();
}

//==============================================================================
// Helper methods
//==============================================================================

void ProjectManager::updateChangeFlag()
{
    // Check if undo manager state has changed since last save
    int currentUndoIndex = undoManager.getNumActionsInCurrentTransaction();

    if (currentUndoIndex != lastSavedUndoIndex)
    {
        changed();
    }
}
