/**
 * @file ProjectPersistence.h
 * @brief Project file save/load (v0.1)
 *
 * Handles serialization of ProjectModel to/from .zenithproj files.
 *
 * File Format:
 * - Extension: *.zenithproj
 * - Format: XML ValueTree
 * - Top-level node: IDs::ID_PROJECT
 * - Schema version: stored in projSchemaVersion property (v0.1 = 1)
 *
 * Path Handling:
 * - Clip file paths stored as-is (absolute or relative)
 * - When loading: relative paths resolved from project file directory
 * - Resolution happens in playback context, not here
 *
 * Thread Safety:
 * - MESSAGE THREAD ONLY
 * - All file I/O is blocking
 */

#pragma once

#include <model/ProjectModel.h>
#include <JuceHeader.h>

namespace zenith::io
{
    /**
     * @brief Save a ProjectModel to disk as a .zenithproj file (XML ValueTree)
     * @param model Project to save
     * @param file Target file path
     * @return Result indicating success or failure
     *
     * @note MESSAGE THREAD ONLY
     *
     * Behavior:
     * - Overwrites existing file atomically using juce::TemporaryFile
     * - Creates parent directory if necessary
     * - Adds projSchemaVersion = 1 to root
     * - Uses projectToValueTree() for conversion
     */
    juce::Result saveProjectToFile(const ProjectModel& model,
                                   const juce::File& file);

    /**
     * @brief Load a ProjectModel from a .zenithproj file
     * @param outModel Output parameter - filled on success
     * @param file Project file to load
     * @return Result indicating success or failure
     *
     * @note MESSAGE THREAD ONLY
     *
     * Behavior:
     * - Reads XML, validates root, converts via projectFromValueTree()
     * - Checks projSchemaVersion (v0.1: accepts 1, warns if >1)
     * - On success, outModel is filled with loaded project
     * - On failure, outModel is unchanged
     */
    juce::Result loadProjectFromFile(ProjectModel& outModel,
                                     const juce::File& file);

} // namespace zenith::io
