#include <iostream>

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../include/ProjectState.h"

namespace
{
bool hasDuplicateTrackIds(const ProjectState& state)
{
    auto tracksNode = state.getState().getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
        return false;

    juce::StringArray ids;

    for (const auto& track : tracksNode)
        ids.add(track[ProjectState::PROP_ID].toString());

    for (int i = 0; i < ids.size(); ++i)
        for (int j = i + 1; j < ids.size(); ++j)
            if (ids[i] == ids[j])
                return true;

    return false;
}
}

int main()
{
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile("zenith_project_state_test", ".zth", false);

    {
        ProjectState original;
        original.addTrack("Track A", "audio");
        original.addTrack("Track B", "audio");

        if (!original.saveToFile(tempFile))
        {
            std::cerr << "Failed to save project state" << std::endl;
            return 1;
        }
    }

    ProjectState loaded;

    if (!loaded.loadFromFile(tempFile))
    {
        std::cerr << "Failed to load project state" << std::endl;
        tempFile.deleteFile();
        return 1;
    }

    auto newTrackId = loaded.addTrack("Track C", "audio");

    if (newTrackId == "track_0" || newTrackId == "track_1")
    {
        std::cerr << "Generated duplicate track ID: " << newTrackId << std::endl;
        tempFile.deleteFile();
        return 1;
    }

    if (hasDuplicateTrackIds(loaded))
    {
        std::cerr << "Duplicate track IDs detected after load/save cycle" << std::endl;
        tempFile.deleteFile();
        return 1;
    }

    tempFile.deleteFile();
    return 0;
}

