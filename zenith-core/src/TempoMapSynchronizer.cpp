/**
 * @file TempoMapSynchronizer.cpp
 * @brief Tempo map synchronizer implementation
 */

#include "../include/TempoMapSynchronizer.h"

//==============================================================================
TempoMapSynchronizer::TempoMapSynchronizer(ProjectState& ps, Engine& eng)
    : projectState(ps), engine(eng)
{
    DBG("TempoMapSynchronizer: Constructor");
}

TempoMapSynchronizer::~TempoMapSynchronizer()
{
    DBG("TempoMapSynchronizer: Destructor");

    // Remove listener
    projectState.getState().removeListener(this);
}

//==============================================================================
void TempoMapSynchronizer::initialize()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Listen to the entire ProjectState tree for tempo map changes
    projectState.getState().addListener(this);

    // Do initial update
    forceUpdate();

    DBG("TempoMapSynchronizer: Initialized");
}

void TempoMapSynchronizer::forceUpdate()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    updateEngineTempoMap();
}

//==============================================================================
// ValueTree::Listener (MESSAGE THREAD)
//==============================================================================

void TempoMapSynchronizer::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // Check if this is a tempo map property change
    if (tree.hasType(ProjectState::ID_TEMPO_CHANGE))
    {
        if (property == ProjectState::PROP_BEAT_POSITION ||
            property == ProjectState::PROP_BPM ||
            property == ProjectState::PROP_TIME_SIG_NUM_CHANGE ||
            property == ProjectState::PROP_TIME_SIG_DEN_CHANGE)
        {
            updateEngineTempoMap();
        }
    }
}

void TempoMapSynchronizer::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(parent);

    // If tempo map node or tempo change added, update
    if (child.hasType(ProjectState::ID_TEMPO_MAP) ||
        child.hasType(ProjectState::ID_TEMPO_CHANGE))
    {
        updateEngineTempoMap();
    }
}

void TempoMapSynchronizer::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, index);

    // If tempo change removed, update
    if (child.hasType(ProjectState::ID_TEMPO_CHANGE))
    {
        updateEngineTempoMap();
    }
}

void TempoMapSynchronizer::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    juce::ignoreUnused(oldIndex, newIndex);

    // If tempo map children reordered, update
    if (parent.hasType(ProjectState::ID_TEMPO_MAP))
    {
        updateEngineTempoMap();
    }
}

void TempoMapSynchronizer::valueTreeParentChanged(juce::ValueTree& tree)
{
    juce::ignoreUnused(tree);
}

//==============================================================================
// Helper Methods
//==============================================================================

void TempoMapSynchronizer::updateEngineTempoMap()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Get tempo changes from ProjectState
    auto tempoChanges = projectState.getTempoChanges();

    // Update Engine tempo map
    engine.setTempoMap(tempoChanges);

    DBG("TempoMapSynchronizer: Updated Engine tempo map with " +
        juce::String(tempoChanges.size()) + " tempo changes");
}
