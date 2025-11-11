/*
  ==============================================================================

    ProjectState.cpp

  ==============================================================================
*/

#include "ProjectState.h"

ProjectState::ProjectState()
    : state("Project")
{
    state.addListener(this);
}

ProjectState::~ProjectState()
{
    state.removeListener(this);
}

void ProjectState::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                           const juce::Identifier& property)
{
    // TODO: Handle property changes
}
