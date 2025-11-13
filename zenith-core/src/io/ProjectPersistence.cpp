/**
 * @file ProjectPersistence.cpp
 * @brief Implementation of project file save/load
 */

#include <io/ProjectPersistence.h>
#include <model/ProjectIDs.h>

namespace zenith::io
{

using namespace zenith::model;

//==============================================================================
// Save Project to File
//==============================================================================

juce::Result saveProjectToFile(const ProjectModel& model,
                               const juce::File& file)
{
    // MESSAGE THREAD ONLY

    // 1. Validate target
    if (file.isDirectory())
        return juce::Result::fail("Target is a directory, not a file");

    auto parent = file.getParentDirectory();
    if (!parent.exists())
    {
        if (!parent.createDirectory())
            return juce::Result::fail("Could not create project directory");
    }

    // 2. Build root ValueTree
    juce::File projectDir = file.getParentDirectory();
    auto vt = projectToValueTree(model, projectDir);
    vt.setProperty(ids::projSchemaVersion, 1, nullptr);

    // 3. Convert to XML
    std::unique_ptr<juce::XmlElement> xml(vt.createXml());
    if (xml == nullptr)
        return juce::Result::fail("Failed to create XML from project");

    // 4. Use juce::TemporaryFile for atomic write
    juce::TemporaryFile tempFile(file);
    std::unique_ptr<juce::FileOutputStream> out(tempFile.getFile().createOutputStream());

    if (out == nullptr || !out->openedOk())
        return juce::Result::fail("Could not open temporary file for writing");

    out->setPosition(0);
    xml->writeTo(*out, juce::XmlElement::TextFormat());  // default formatting

    if (!out->flush())
        return juce::Result::fail("Failed to flush project file to disk");

    out.reset();  // close the stream before moving temp file

    if (!tempFile.overwriteTargetFileWithTemporary())
        return juce::Result::fail("Failed to replace project file with temporary file");

    // 5. Success
    DBG("ProjectPersistence: Saved project to " + file.getFullPathName());
    return juce::Result::ok();
}

//==============================================================================
// Load Project from File
//==============================================================================

juce::Result loadProjectFromFile(ProjectModel& outModel,
                                 const juce::File& file)
{
    // MESSAGE THREAD ONLY

    // 1. Basic checks
    if (!file.existsAsFile())
        return juce::Result::fail("Project file does not exist");

    // 2. Read XML
    juce::XmlDocument doc(file);
    std::unique_ptr<juce::XmlElement> xml(doc.getDocumentElement());

    if (xml == nullptr)
        return juce::Result::fail("Failed to parse project XML: " + doc.getLastParseError());

    // 3. Convert to ValueTree
    juce::ValueTree root = juce::ValueTree::fromXml(*xml);
    if (!root.isValid())
        return juce::Result::fail("Invalid project ValueTree");

    // 4. Validate root type
    if (root.getType() != ids::ID_PROJECT)
        return juce::Result::fail("Root node is not a project");

    // 5. Check projSchemaVersion
    const int schemaVersion = root.hasProperty(ids::projSchemaVersion)
        ? static_cast<int>(root[ids::projSchemaVersion])
        : 1;

    if (schemaVersion > 1)
    {
        DBG("ProjectPersistence: WARNING - Loading project with future schema version " +
            juce::String(schemaVersion));
        // v0.1: still attempt to load, just warn
    }

    // 6. Convert to ProjectModel
    juce::File projectDir = file.getParentDirectory();
    ProjectModel loaded = projectFromValueTree(root, projectDir);

    outModel = std::move(loaded);

    DBG("ProjectPersistence: Loaded project from " + file.getFullPathName());
    DBG("  Name: " + outModel.name);
    DBG("  Sample Rate: " + juce::String(outModel.sampleRate));
    DBG("  Tracks: " + juce::String(outModel.tracks.size()));

    return juce::Result::ok();
}

} // namespace zenith::io
