/**
 * @file ProjectManager.h
 * @brief High-level project management with change tracking
 *
 * Wraps ProjectState with FileBasedDocument functionality to provide:
 * - Automatic change tracking (hasUnsavedChanges)
 * - Save/Load with user dialogs
 * - New project creation
 * - Recent files management
 *
 * This class bridges the gap between the UI and ProjectState,
 * providing a clean API for file operations.
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @class ProjectManager
 * @brief Manages project file operations with automatic change tracking
 *
 * Uses JUCE's FileBasedDocument pattern to provide:
 * - hasUnsavedChanges() - check if project needs saving
 * - saveProject() - save with dialog if needed
 * - loadProject() - load with file chooser
 * - newProject() - create new project (prompts to save current)
 *
 * Example usage:
 * @code
 * ProjectManager manager(projectState);
 *
 * if (manager.hasUnsavedChanges()) {
 *     manager.saveProject();  // Shows save dialog if needed
 * }
 *
 * manager.newProject();  // Prompts to save current project first
 * @endcode
 */
class ProjectManager : public juce::FileBasedDocument
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to the ProjectState to manage
     * @param undoManager Reference to the UndoManager for change tracking
     */
    ProjectManager(ProjectState& projectState, juce::UndoManager& undoManager);

    /**
     * @brief Destructor
     */
    ~ProjectManager() override;

    //==========================================================================
    // High-level operations
    //==========================================================================

    /**
     * @brief Check if project has unsaved changes
     * @return true if changes need to be saved
     */
    bool hasUnsavedChanges() const;

    /**
     * @brief Save project (shows dialog if no current file)
     * @param askUserForFileIfNotSpecified If true, shows save dialog when needed
     * @param showMessageOnFailure If true, shows error message on failure
     * @return true if saved successfully
     */
    bool saveProject(bool askUserForFileIfNotSpecified = true,
                     bool showMessageOnFailure = true);

    /**
     * @brief Save project as (always shows save dialog)
     * @param showMessageOnFailure If true, shows error message on failure
     * @return true if saved successfully
     */
    bool saveProjectAs(bool showMessageOnFailure = true);

    /**
     * @brief Load project from file
     * @param fileToLoad Optional file to load (shows dialog if not provided)
     * @return true if loaded successfully
     */
    bool loadProject(const juce::File& fileToLoad = juce::File());

    /**
     * @brief Create new project
     * @return true if new project was created (may be false if user cancelled save)
     */
    bool newProject();

    /**
     * @brief Get current project file
     * @return File object for current project (may be non-existent)
     */
    juce::File getCurrentProjectFile() const;

    /**
     * @brief Get project name from file
     * @return Project name (file name without extension)
     */
    juce::String getCurrentProjectName() const;

    /**
     * @brief Mark project as changed (needs saving)
     */
    void markAsChanged();

    //==========================================================================
    // FileBasedDocument overrides (protected implementation)
    //==========================================================================

protected:
    /**
     * @brief Load document from file
     * @param file File to load from
     * @return Result indicating success or error message
     */
    juce::Result loadDocument(const juce::File& file) override;

    /**
     * @brief Save document to file
     * @param file File to save to
     * @return Result indicating success or error message
     */
    juce::Result saveDocument(const juce::File& file) override;

    /**
     * @brief Get last location for file chooser
     * @return Last used directory
     */
    juce::File getLastDocumentOpened() override;

    /**
     * @brief Save last location for file chooser
     * @param file Last used file
     */
    void setLastDocumentOpened(const juce::File& file) override;

private:
    //==========================================================================
    // Member variables
    //==========================================================================

    ProjectState& projectState;
    juce::UndoManager& undoManager;

    // Track undo manager state to detect changes
    int lastSavedUndoIndex{0};

    //==========================================================================
    // Helper methods
    //==========================================================================

    /**
     * @brief Update change flag based on undo manager state
     */
    void updateChangeFlag();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};
