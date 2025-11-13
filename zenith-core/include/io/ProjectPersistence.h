/**
 * @file ProjectPersistence.h
 * @brief Project save/load functionality using XML serialization
 *
 * Provides simple, non-UI file I/O for ProjectModel using ValueTree XML format.
 *
 * Thread Safety:
 * - All functions are MESSAGE THREAD ONLY
 * - Do NOT call from audio thread
 *
 * Error Handling:
 * - All functions return result structs with ok flag and errorMessage
 * - Never throws exceptions
 * - Always checks file/directory existence
 */

#pragma once

#include <JuceHeader.h>
#include "../model/ProjectModel.h"

namespace zenith
{
    /**
     * @struct ProjectSaveResult
     * @brief Result of a save operation
     */
    struct ProjectSaveResult
    {
        bool ok = false;                ///< true if save succeeded
        juce::String errorMessage;      ///< Human-readable error (empty if ok)
    };

    /**
     * @struct ProjectLoadResult
     * @brief Result of a load operation
     */
    struct ProjectLoadResult
    {
        bool ok = false;                ///< true if load succeeded
        ProjectModel project;           ///< Loaded project (only valid if ok==true)
        juce::String errorMessage;      ///< Human-readable error (empty if ok)
    };

    /**
     * @brief Save project to XML file
     * @param project Project model to save
     * @param file Target file path
     * @return Result with ok flag and error message
     *
     * @note MESSAGE THREAD ONLY
     *
     * This function:
     * 1. Converts ProjectModel to ValueTree using projectToValueTree()
     * 2. Converts ValueTree to XML using ValueTree::createXml()
     * 3. Writes XML to file using FileOutputStream
     * 4. Creates parent directory if needed
     * 5. Overwrites existing file
     *
     * Returns ok=false if:
     * - Cannot create parent directory
     * - Cannot open file for writing
     * - Write operation fails
     */
    ProjectSaveResult saveProjectToFile(const ProjectModel& project,
                                        const juce::File& file);

    /**
     * @brief Load project from XML file
     * @param file Source file path
     * @param defaultSampleRateIfMissing Fallback sample rate if file has invalid value
     * @return Result with ok flag, loaded project, and error message
     *
     * @note MESSAGE THREAD ONLY
     *
     * This function:
     * 1. Checks file exists and is readable
     * 2. Parses XML using XmlDocument
     * 3. Converts XML to ValueTree using ValueTree::fromXml()
     * 4. Converts ValueTree to ProjectModel using projectFromValueTree()
     * 5. Validates and fixes sample rate if needed
     *
     * Returns ok=false if:
     * - File doesn't exist or isn't a file
     * - XML parsing fails
     * - Root element isn't a valid project ValueTree
     *
     * If project.sampleRate <= 0 after loading, sets it to defaultSampleRateIfMissing.
     */
    ProjectLoadResult loadProjectFromFile(const juce::File& file,
                                          double defaultSampleRateIfMissing = 48000.0);
}
