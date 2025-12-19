/*
  ==============================================================================

    ProjectFileIO.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Handles project file loading, saving, and crash dumping.
    
    Extracted from ProjectState.cpp.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

class ProjectState;

/**
    Handles I/O operations for ProjectState.
*/
class ProjectFileIO {
public:
    enum class SerializationFormat {
        Xml,
        MessagePack
    };

    struct IOSettings {
        SerializationFormat format = SerializationFormat::Xml;
        bool useAtomicWrite = true;
    };
    explicit ProjectFileIO(ProjectState& projectState);
    ~ProjectFileIO() = default;

    /**
     * @brief Create a new empty project
     */
    void newProject();

    /**
     * @brief Load project from file
     * @return true if successful
     */
    bool loadFromFile(const juce::File& file);

    /**
     * @brief Load project from file asynchronously
     */
    void loadFromFileAsync(const juce::File& file,
                           std::function<void(bool success, juce::String error)> callback);

    /**
     * @brief Save project to file
     * @return true if successful
     */
    bool saveToFile(const juce::File& file, IOSettings settings = {});

    /**
     * @brief Save project to file asynchronously
     */
    void saveToFileAsync(const juce::File& file,
                         IOSettings settings,
                         std::function<void(bool success, juce::String error)> callback);

    /**
     * @brief Save crash dump
     * @return File where dump was saved
     */
    juce::File saveCrashDump();

private:
    ProjectState& projectState_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectFileIO)
};

} // namespace zenith
