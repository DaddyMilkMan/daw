/**
 * @file ProjectPersistence.h
 * @brief Project save/load I/O functions (v0.1)
 *
 * Pure functions for persisting ProjectModel to/from disk.
 * No engine or UI dependencies - just ProjectModel ↔ file.
 *
 * File format: Binary ValueTree (JUCE native format)
 * Thread safety: MESSAGE THREAD only (all file I/O)
 */

#pragma once

#include <JuceHeader.h>
#include "../model/ProjectModel.h"

namespace zenith {

/**
 * @brief Save project to file
 * @param model Project data to save
 * @param file Target file path (.zenithproj)
 * @param outError Optional error message output
 * @return true on success, false on failure
 *
 * Writes ProjectModel as binary ValueTree to disk.
 * On failure, writes error message to outError if non-null.
 *
 * MESSAGE THREAD only
 */
bool saveProjectToFile(const ProjectModel& model,
                       const juce::File& file,
                       juce::String* outError = nullptr);

/**
 * @brief Load project from file
 * @param file Source file path (.zenithproj)
 * @param outProject Output parameter for loaded project
 * @param outError Optional error message output
 * @return true on success, false on failure
 *
 * Reads binary ValueTree from disk and converts to ProjectModel.
 * On failure, outProject is untouched and error message written to outError.
 *
 * MESSAGE THREAD only
 */
bool loadProjectFromFile(const juce::File& file,
                         ProjectModel& outProject,
                         juce::String* outError = nullptr);

} // namespace zenith
