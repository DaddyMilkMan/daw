/*
  ==============================================================================

    ProjectManager.h
    Created: 2025-11-11
    Author:  Vexel DAW

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ProjectManager
{
public:
    ProjectManager();
    ~ProjectManager();

    bool saveProject(const juce::File& file, const juce::ValueTree& projectState);
    bool loadProject(const juce::File& file, juce::ValueTree& projectState);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};
