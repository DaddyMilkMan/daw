/**
 * @file ProjectPersistence.cpp
 * @brief ProjectPersistence implementation
 */

#include "../../include/io/ProjectPersistence.h"
#include "../../include/model/ProjectIDs.h"

namespace zenith {

bool saveProjectToFile(const ProjectModel& model,
                       const juce::File& file,
                       juce::String* outError)
{
    // Convert model to ValueTree
    auto vt = projectToValueTree(model);

    // Create output stream
    juce::FileOutputStream out(file);
    if (!out.openedOk())
    {
        if (outError)
            *outError = "Failed to open file for writing: " + file.getFullPathName();
        return false;
    }

    // Write ValueTree to stream (binary format)
    vt.writeToStream(out);

    // Verify write completed
    if (out.getStatus().failed())
    {
        if (outError)
            *outError = "Failed to write project data: " + out.getStatus().getErrorMessage();
        return false;
    }

    DBG("Project saved to: " << file.getFullPathName());
    return true;
}

bool loadProjectFromFile(const juce::File& file,
                         ProjectModel& outProject,
                         juce::String* outError)
{
    // Check file exists
    if (!file.existsAsFile())
    {
        if (outError)
            *outError = "File does not exist: " + file.getFullPathName();
        return false;
    }

    // Create input stream
    juce::FileInputStream in(file);
    if (!in.openedOk())
    {
        if (outError)
            *outError = "Failed to open file for reading: " + file.getFullPathName();
        return false;
    }

    // Read ValueTree from stream
    auto vt = juce::ValueTree::readFromStream(in);
    if (!vt.isValid())
    {
        if (outError)
            *outError = "Corrupt project file (invalid ValueTree): " + file.getFullPathName();
        return false;
    }

    // Validate root type
    if (!vt.hasType(ProjectIDs::project))
    {
        if (outError)
            *outError = "Invalid project file format (wrong root type): " + file.getFullPathName();
        return false;
    }

    // Convert ValueTree to ProjectModel
    try
    {
        outProject = projectFromValueTree(vt);
    }
    catch (const std::exception& e)
    {
        if (outError)
            *outError = juce::String("Failed to parse project data: ") + e.what();
        return false;
    }
    catch (...)
    {
        if (outError)
            *outError = "Failed to parse project data (unknown exception)";
        return false;
    }

    DBG("Project loaded from: " << file.getFullPathName());
    return true;
}

} // namespace zenith
