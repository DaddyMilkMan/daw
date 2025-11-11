/*
  ==============================================================================

    ProjectManager.cpp

  ==============================================================================
*/

#include "ProjectManager.h"

ProjectManager::ProjectManager()
{
}

ProjectManager::~ProjectManager()
{
}

bool ProjectManager::saveProject(const juce::File& file, const juce::ValueTree& projectState)
{
    if (auto xml = projectState.createXml())
    {
        return xml->writeTo(file);
    }
    return false;
}

bool ProjectManager::loadProject(const juce::File& file, juce::ValueTree& projectState)
{
    if (file.existsAsFile())
    {
        if (auto xml = juce::parseXML(file))
        {
            projectState = juce::ValueTree::fromXml(*xml);
            return projectState.isValid();
        }
    }
    return false;
}
