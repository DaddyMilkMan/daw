/**
 * @file TempoMapSynchronizer.cpp
 * @brief Tempo map synchronizer implementation
 */

#include "../include/TempoMapSynchronizer.h"

//==============================================================================
TempoMapSynchronizer::TempoMapSynchronizer(ProjectState& state, TempoMap& map)
    : projectState(state), tempoMap(map)
{
    DBG("TempoMapSynchronizer: Constructor");
}

TempoMapSynchronizer::~TempoMapSynchronizer()
{
    stop();
    DBG("TempoMapSynchronizer: Destructor");
}

void TempoMapSynchronizer::start()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (isActive)
        return;

    DBG("TempoMapSynchronizer: Starting");

    // Listen to ProjectState changes
    projectState.getState().addListener(this);

    // Initial update
    forceUpdate();

    isActive = true;
}

void TempoMapSynchronizer::stop()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!isActive)
        return;

    DBG("TempoMapSynchronizer: Stopping");

    // Stop listening
    projectState.getState().removeListener(this);

    isActive = false;
}

void TempoMapSynchronizer::forceUpdate()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    updateTempoMap();
}

//==============================================================================
// ValueTree::Listener Implementation
//==============================================================================

void TempoMapSynchronizer::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // If a tempo point property changed, update
    if (isTempoMapNode(tree))
    {
        DBG("TempoMapSynchronizer: Tempo map property changed");
        updateTempoMap();
    }
}

void TempoMapSynchronizer::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    // If a tempo point was added, update
    if (isTempoMapNode(parent) || isTempoMapNode(child))
    {
        DBG("TempoMapSynchronizer: Tempo point added");
        updateTempoMap();
    }
}

void TempoMapSynchronizer::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int /* index */)
{
    // If a tempo point was removed, update
    if (isTempoMapNode(parent) || isTempoMapNode(child))
    {
        DBG("TempoMapSynchronizer: Tempo point removed");
        updateTempoMap();
    }
}

void TempoMapSynchronizer::valueTreeChildOrderChanged(juce::ValueTree& parent, int /* oldIndex */, int /* newIndex */)
{
    // If tempo points were reordered, update
    if (isTempoMapNode(parent))
    {
        DBG("TempoMapSynchronizer: Tempo points reordered");
        updateTempoMap();
    }
}

void TempoMapSynchronizer::valueTreeParentChanged(juce::ValueTree& /* tree */)
{
    // Not relevant for tempo map
}

//==============================================================================
// Helper Methods
//==============================================================================

void TempoMapSynchronizer::updateTempoMap()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempoMapTree = projectState.getTempoMap();

    if (tempoMapTree.isValid())
    {
        tempoMap.updateFromValueTree(tempoMapTree);
    }
    else
    {
        // No tempo map: use default tempo from project
        double defaultTempo = projectState.getTempo();
        tempoMap.setSingleTempo(defaultTempo);
    }
}

bool TempoMapSynchronizer::isTempoMapNode(const juce::ValueTree& tree) const
{
    if (!tree.isValid())
        return false;

    // Check if this is the tempo map itself or a tempo point
    if (tree.hasType(ProjectState::ID_TEMPO_MAP) ||
        tree.hasType(ProjectState::ID_TEMPO_POINT))
    {
        return true;
    }

    // Check if parent is tempo map
    auto parent = tree.getParent();
    if (parent.isValid() && parent.hasType(ProjectState::ID_TEMPO_MAP))
    {
        return true;
    }

    return false;
}
