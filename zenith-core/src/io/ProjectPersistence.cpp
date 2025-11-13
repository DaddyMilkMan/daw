/**
 * @file ProjectPersistence.cpp
 * @brief Implementation of project persistence functions
 */

#include "../../include/io/ProjectPersistence.h"
#include "../../include/model/ProjectModel.h"
#include "../../include/model/ProjectIDs.h"

namespace zenith
{
    //==========================================================================
    // Save Project
    //==========================================================================

    ProjectSaveResult saveProjectToFile(const ProjectModel& project,
                                        const juce::File& file)
    {
        // MESSAGE THREAD ONLY
        DBG("ProjectPersistence: Saving project '" + project.name + "' to " +
            file.getFullPathName());

        ProjectSaveResult result;
        result.ok = false;

        // 1) Convert ProjectModel to ValueTree
        juce::ValueTree projectVT = projectToValueTree(project);

        if (!projectVT.isValid())
        {
            result.errorMessage = "Failed to convert project to ValueTree";
            DBG("  ERROR: " + result.errorMessage);
            return result;
        }

        // 2) Convert ValueTree to XML
        std::unique_ptr<juce::XmlElement> xml = projectVT.createXml();

        if (xml == nullptr)
        {
            result.errorMessage = "Failed to create XML from ValueTree";
            DBG("  ERROR: " + result.errorMessage);
            return result;
        }

        // 3) Ensure parent directory exists
        juce::File parentDir = file.getParentDirectory();

        if (!parentDir.exists())
        {
            DBG("  Creating parent directory: " + parentDir.getFullPathName());

            if (!parentDir.createDirectory())
            {
                result.errorMessage = "Failed to create parent directory: " +
                                      parentDir.getFullPathName();
                DBG("  ERROR: " + result.errorMessage);
                return result;
            }
        }

        // 4) Write XML to file
        if (!xml->writeToFile(file, juce::String()))
        {
            result.errorMessage = "Failed to write XML to file: " +
                                  file.getFullPathName();
            DBG("  ERROR: " + result.errorMessage);
            return result;
        }

        // Success!
        result.ok = true;
        DBG("  SUCCESS: Project saved to " + file.getFullPathName());

        return result;
    }

    //==========================================================================
    // Load Project
    //==========================================================================

    ProjectLoadResult loadProjectFromFile(const juce::File& file,
                                          double defaultSampleRateIfMissing)
    {
        // MESSAGE THREAD ONLY
        DBG("ProjectPersistence: Loading project from " + file.getFullPathName());

        ProjectLoadResult result;
        result.ok = false;

        // 1) Check file exists
        if (!file.existsAsFile())
        {
            result.errorMessage = "File does not exist: " + file.getFullPathName();
            DBG("  ERROR: " + result.errorMessage);
            return result;
        }

        // 2) Parse XML
        std::unique_ptr<juce::XmlElement> xml = juce::parseXML(file);

        if (xml == nullptr)
        {
            result.errorMessage = "Failed to parse XML from file: " +
                                  file.getFullPathName();
            DBG("  ERROR: " + result.errorMessage);
            return result;
        }

        // 3) Convert XML to ValueTree
        juce::ValueTree rootVT = juce::ValueTree::fromXml(*xml);

        if (!rootVT.isValid())
        {
            result.errorMessage = "Failed to create ValueTree from XML";
            DBG("  ERROR: " + result.errorMessage);
            return result;
        }

        // 4) Validate root type
        if (rootVT.getType() != ids::project)
        {
            result.errorMessage = "Invalid project file: root element is not 'Project'";
            DBG("  ERROR: " + result.errorMessage);
            DBG("  Expected type: " + ids::project.toString());
            DBG("  Actual type: " + rootVT.getType().toString());
            return result;
        }

        // 5) Convert ValueTree to ProjectModel
        result.project = projectFromValueTree(rootVT);

        // 6) Validate and fix sample rate if needed
        if (result.project.sampleRate <= 0.0)
        {
            DBG("  WARNING: Invalid sample rate " +
                juce::String(result.project.sampleRate) +
                ", using default " + juce::String(defaultSampleRateIfMissing));

            result.project.sampleRate = defaultSampleRateIfMissing;
        }

        // Success!
        result.ok = true;
        DBG("  SUCCESS: Loaded project '" + result.project.name + "'" +
            " (" + juce::String(result.project.tracks.size()) + " tracks)");

        return result;
    }
}
