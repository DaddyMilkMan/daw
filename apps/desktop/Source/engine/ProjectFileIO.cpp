/*
  ==============================================================================

    ProjectFileIO.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    File I/O implementation.

  ==============================================================================
*/

#include "ProjectFileIO.h"
#include "ProjectState.h"

namespace zenith {

ProjectFileIO::ProjectFileIO(ProjectState& projectState)
    : projectState_(projectState)
{
}

ProjectFileIO::~ProjectFileIO()
{
    // Signal to async callbacks that this object is being destroyed
    // Callbacks check this flag before accessing member variables
    isShuttingDown_->store(true);
}

void ProjectFileIO::newProject()
{
    DBG("ProjectFileIO: Creating new project");

    // Clear undo history first
    projectState_.getUndoManager().clearUndoHistory();

    // Create default state
    projectState_.createDefaultState();

    // Reset cache and ID counter
    projectState_.trackIdMap_.clear();
    projectState_.idCounter.store(0);

    // Reset project file and dirty flag
    projectState_.projectFile = juce::File();
    projectState_.isDirty = false;

    DBG("ProjectFileIO: New project created");
}

bool ProjectFileIO::loadFromFile(const juce::File& file)
{
    DBG("ProjectFileIO: Loading from " + file.getFullPathName());

    if (!file.existsAsFile())
    {
        DBG("ProjectFileIO: File does not exist");
        return false;
    }

    // Parse XML
    auto xml = juce::parseXML(file);

    if (xml == nullptr)
    {
        DBG("ProjectFileIO: Failed to parse XML");
        return false;
    }

    // Create ValueTree from XML
    auto newState = juce::ValueTree::fromXml(*xml);

    if (!newState.isValid() || newState.getType() != ProjectState::ID_PROJECT)
    {
        DBG("ProjectFileIO: Invalid project file - wrong type or invalid state");
        return false;
    }
    
    // Validate and repair project structure
    // Ensure required child nodes exist (for backward compatibility with older projects)
    if (!newState.getChildWithName(ProjectState::ID_TRACKS).isValid()) {
        DBG("ProjectFileIO: Project missing TRACKS node, creating empty one");
        newState.appendChild(juce::ValueTree(ProjectState::ID_TRACKS), nullptr);
    }
    
    // Validate required properties exist, use defaults if missing
    if (!newState.hasProperty(ProjectState::PROP_TEMPO)) {
        DBG("ProjectFileIO: Project missing tempo, setting default 120 BPM");
        newState.setProperty(ProjectState::PROP_TEMPO, 120.0, nullptr);
    }
    
    if (!newState.hasProperty(ProjectState::PROP_TIME_SIG_NUM)) {
        newState.setProperty(ProjectState::PROP_TIME_SIG_NUM, 4, nullptr);
    }
    
    if (!newState.hasProperty(ProjectState::PROP_TIME_SIG_DEN)) {
        newState.setProperty(ProjectState::PROP_TIME_SIG_DEN, 4, nullptr);
    }

    // Replace current state with loaded state
    auto& state = projectState_.getState();
    state.removeListener(&projectState_);
    state = newState;
    state.addListener(&projectState_);

    // Ensure future IDs do not clash with those loaded from disk
    projectState_.rebuildIdCounter();

    // Rebuild O(1) lookup map
    projectState_.rebuildTrackMap();

    // Clear undo history (fresh start)
    projectState_.getUndoManager().clearUndoHistory();

    projectState_.setProjectFile(file);
    
    // Reset isDirty flag - project was just loaded, no unsaved changes
    // ProjectFileIO is declared as friend in ProjectState.h so we can access isDirty
    projectState_.isDirty = false;
    
    DBG("ProjectFileIO: Loaded successfully");
    return true;
}

bool ProjectFileIO::saveToFile(const juce::File& file, IOSettings settings)
{
    DBG("ProjectFileIO: Saving to " + file.getFullPathName() + (settings.format == SerializationFormat::MessagePack ? " (MessagePack)" : " (XML)"));

    std::unique_ptr<juce::XmlElement> xml;
    juce::MemoryBlock msgPackData;

    if (settings.format == SerializationFormat::Xml)
    {
        xml = projectState_.getState().createXml();
        if (xml == nullptr)
        {
            DBG("ProjectFileIO: Failed to create XML from ValueTree");
            return false;
        }

        // Add metadata
        auto now = juce::Time::getCurrentTime();
        xml->setAttribute("appVersion", "0.1.0-alpha");
        xml->setAttribute("savedAt", now.formatted("%Y-%m-%d %H:%M:%S"));
        xml->setAttribute("timestamp", static_cast<double>(now.toMilliseconds()));
        xml->setAttribute("isCrashDump", "0");
        xml->setAttribute("platform", juce::SystemStats::getOperatingSystemName());
        xml->setAttribute("format", "xml");
    }
    else
    {
        // MessagePack serialization (To be implemented in detail, using ValueTree::writeToStream as a fallback for now)
        juce::MemoryOutputStream mo(msgPackData, false);
        projectState_.getState().writeToStream(mo);
        // Note: In a real implementation, we'd use a proper MessagePack encoder here.
    }

    // Atomic write approach: save to temp file then rename
    juce::File targetFile = file;
    juce::File tempFile = file.getSiblingFile(file.getFileName() + ".tmp");

    bool success = false;
    if (settings.format == SerializationFormat::Xml)
    {
        success = xml->writeTo(tempFile);
    }
    else
    {
        success = tempFile.replaceWithData(msgPackData.getData(), msgPackData.getSize());
    }

    if (!success)
    {
        DBG("ProjectFileIO: Failed to write to temporary file");
        return false;
    }

    // Attempt to swap temp file with target file
    if (settings.useAtomicWrite)
    {
        if (!tempFile.moveFileTo(targetFile))
        {
            DBG("ProjectFileIO: Failed to move temporary file to destination");
            tempFile.deleteFile();
            return false;
        }
    }
    else
    {
        tempFile.deleteFile(); // This shouldn't happen if we use settings.useAtomicWrite=true
    }

    DBG("ProjectFileIO: Saved successfully");
    projectState_.setProjectFile(file);
    projectState_.isDirty = false;

    return true;
}

void ProjectFileIO::saveToFileAsync(const juce::File& file, IOSettings settings, std::function<void(bool success, juce::String error)> callback)
{
    // Capture necessary state safely (ValueTree is ref-counted, but the projectState reference needs to be handled)
    // For simplicity and safety, we take a copy of the ValueTree snapshot
    auto stateSnapshot = projectState_.getState().createCopy();
    
    // Capture shutdown flag by value (shared_ptr) for safe async access
    auto shutdownFlag = isShuttingDown_;
    
    juce::Thread::launch([this, file, settings, stateSnapshot, callback, shutdownFlag]() mutable {
        DBG("ProjectFileIO: Starting async save...");
        
        std::unique_ptr<juce::XmlElement> xml;
        juce::MemoryBlock msgPackData;
        bool prepareSuccess = false;

        if (settings.format == SerializationFormat::Xml)
        {
            xml = stateSnapshot.createXml();
            if (xml != nullptr)
            {
                auto now = juce::Time::getCurrentTime();
                xml->setAttribute("appVersion", "0.1.0-alpha");
                xml->setAttribute("savedAt", now.formatted("%Y-%m-%d %H:%M:%S"));
                xml->setAttribute("format", "xml");
                prepareSuccess = true;
            }
        }
        else
        {
            juce::MemoryOutputStream mo(msgPackData, false);
            stateSnapshot.writeToStream(mo);
            prepareSuccess = true;
        }

        if (!prepareSuccess) {
            juce::MessageManager::callAsync([callback] { callback(false, "Failed to prepare data"); });
            return;
        }

        juce::File tempFile = file.getSiblingFile(file.getFileName() + ".savetmp");
        bool writeSuccess = false;
        
        if (settings.format == SerializationFormat::Xml)
            writeSuccess = xml->writeTo(tempFile);
        else
            writeSuccess = tempFile.replaceWithData(msgPackData.getData(), msgPackData.getSize());

        if (writeSuccess) {
            if (tempFile.moveFileTo(file)) {
                juce::MessageManager::callAsync([this, file, callback, shutdownFlag] {
                    // Check if object was destroyed before accessing this->
                    if (shutdownFlag->load()) {
                        DBG("ProjectFileIO: Skipping save completion - object destroyed");
                        callback(false, "Operation cancelled - project closed");
                        return;
                    }
                    projectState_.setProjectFile(file);
                    projectState_.isDirty = false;
                    callback(true, "");
                });
            } else {
                tempFile.deleteFile();
                juce::MessageManager::callAsync([callback] { callback(false, "Failed to move file to destination"); });
            }
        } else {
            juce::MessageManager::callAsync([callback] { callback(false, "Failed to write data to disk"); });
        }
    });
}

void ProjectFileIO::loadFromFileAsync(const juce::File& file, std::function<void(bool success, juce::String error)> callback)
{
    // Capture shutdown flag by value (shared_ptr) for safe async access
    auto shutdownFlag = isShuttingDown_;
    
    juce::Thread::launch([this, file, callback, shutdownFlag]() {
        DBG("ProjectFileIO: Starting async load...");
        
        if (!file.existsAsFile()) {
            juce::MessageManager::callAsync([callback] { callback(false, "File does not exist"); });
            return;
        }

        juce::ValueTree newState;
        
        // Try XML first
        auto xml = juce::parseXML(file);
        if (xml != nullptr) {
            newState = juce::ValueTree::fromXml(*xml);
        } else {
            // Try binary
            juce::MemoryBlock mb;
            if (file.loadFileAsData(mb)) {
                juce::MemoryInputStream mi(mb, false);
                newState = juce::ValueTree::readFromStream(mi);
            }
        }

        if (!newState.isValid()) {
            juce::MessageManager::callAsync([callback] { callback(false, "Invalid project file format"); });
            return;
        }

        juce::MessageManager::callAsync([this, file, newState, callback, shutdownFlag] {
            // Check if object was destroyed before accessing this->
            if (shutdownFlag->load()) {
                DBG("ProjectFileIO: Skipping load completion - object destroyed");
                callback(false, "Operation cancelled - project closed");
                return;
            }
            
            // Re-check type safely on message thread
            if (newState.getType() != ProjectState::ID_PROJECT) {
                callback(false, "File is not a Zenith project");
                return;
            }

            auto& state = projectState_.getState();
            state.removeListener(&projectState_);
            state = newState;
            state.addListener(&projectState_);

            projectState_.rebuildIdCounter();
            projectState_.rebuildTrackMap();
            projectState_.getUndoManager().clearUndoHistory();
            projectState_.setProjectFile(file);
            projectState_.isDirty = false;

            callback(true, "");
        });
    });
}

juce::File ProjectFileIO::saveCrashDump()
{
    auto documentsDir =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto crashDir =
        documentsDir.getChildFile("ZenithDAW").getChildFile("CrashDumps");

    if (!crashDir.exists())
        crashDir.createDirectory();

    auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    auto dumpFile = crashDir.getChildFile("crash_recovery_" + timestamp + ".zth");

    DBG("ProjectFileIO: Saving crash dump to " + dumpFile.getFullPathName());

    if (saveToFile(dumpFile))
        return dumpFile;

    return juce::File();
}

} // namespace zenith
