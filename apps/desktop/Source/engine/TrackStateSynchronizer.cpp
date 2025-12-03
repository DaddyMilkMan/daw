/**
 * @file TrackStateSynchronizer.cpp
 * @brief Implementation of TrackStateSynchronizer
 */

#include "../include/TrackStateSynchronizer.h"
#include "engine/Track.h"

//==============================================================================
TrackStateSynchronizer::TrackStateSynchronizer(ProjectState& ps, Engine& eng)
    : projectState(ps), engine(eng)
{
    DBG("TrackStateSynchronizer: Constructor");
}

TrackStateSynchronizer::~TrackStateSynchronizer()
{
    shutdown();
    DBG("TrackStateSynchronizer: Destructor");
}

//==============================================================================
void TrackStateSynchronizer::initialize()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (initialized)
        return;

    DBG("TrackStateSynchronizer: Initializing...");

    // Listen to TRACKS node for track add/remove
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (tracksNode.isValid())
    {
        tracksNode.addListener(this);

        // Add listeners to all existing tracks
        for (const auto& track : tracksNode)
        {
            addTrackListener(track);
        }
    }

    // Perform initial sync
    syncAll();

    initialized = true;
    DBG("TrackStateSynchronizer: Initialized");
}

void TrackStateSynchronizer::shutdown()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!initialized)
        return;

    DBG("TrackStateSynchronizer: Shutting down...");

    // Remove listeners from all tracks
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (tracksNode.isValid())
    {
        for (const auto& track : tracksNode)
        {
            removeTrackListener(track);
        }

        tracksNode.removeListener(this);
    }

    initialized = false;
    DBG("TrackStateSynchronizer: Shut down");
}

void TrackStateSynchronizer::syncAll()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    DBG("TrackStateSynchronizer: Syncing all tracks...");

    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
    {
        DBG("TrackStateSynchronizer: No tracks node found");
        return;
    }

    const int numProjectTracks = tracksNode.getNumChildren();
    const int numEngineTracks = engine.getNumTracks();

    DBG("TrackStateSynchronizer: ProjectState has " + juce::String(numProjectTracks) + " tracks");
    DBG("TrackStateSynchronizer: Engine has " + juce::String(numEngineTracks) + " tracks");

    // For now, we assume tracks are in sync (same count and order)
    // In a future phase, we could add track ID mapping
    const int numTracksToSync = juce::jmin(numProjectTracks, numEngineTracks);

    for (int i = 0; i < numTracksToSync; ++i)
    {
        auto track = tracksNode.getChild(i);

        // Sync all mixer properties
        syncTrackProperty(track, ProjectState::PROP_VOLUME);
        syncTrackProperty(track, ProjectState::PROP_PAN);
        syncTrackProperty(track, ProjectState::PROP_MUTE);
        syncTrackProperty(track, ProjectState::PROP_SOLO);
        syncTrackProperty(track, ProjectState::PROP_ARMED);
    }

    DBG("TrackStateSynchronizer: Sync complete");
}

//==============================================================================
// ValueTree::Listener Implementation
//==============================================================================

void TrackStateSynchronizer::valueTreePropertyChanged(
    juce::ValueTree& tree,
    const juce::Identifier& property)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Check if this is a TRACK node
    if (tree.hasType(ProjectState::ID_TRACK))
    {
        syncTrackProperty(tree, property);
    }
}

void TrackStateSynchronizer::valueTreeChildAdded(
    juce::ValueTree& parent,
    juce::ValueTree& child)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Check if a track was added to TRACKS node
    if (parent.hasType(ProjectState::ID_TRACKS) && child.hasType(ProjectState::ID_TRACK))
    {
        DBG("TrackStateSynchronizer: Track added - creating Engine track");
        
        // Create new Engine track
        juce::String name = child[ProjectState::PROP_NAME].toString();
        juce::String typeStr = child[ProjectState::PROP_TYPE].toString();
        juce::String id = child[ProjectState::PROP_ID].toString();
        
        zenith::Track::Type type = (typeStr == "midi") ? zenith::Track::Type::MIDI : zenith::Track::Type::Audio;
        
        auto track = std::make_unique<zenith::Track>(name, type);
        track->setTrackId(id);
        
        // Set initial properties
        track->setVolume(child[ProjectState::PROP_VOLUME]);
        track->setPan(child[ProjectState::PROP_PAN]);
        track->setMuted(child[ProjectState::PROP_MUTE]);
        track->setSolo(child[ProjectState::PROP_SOLO]);
        
        // Add to Engine
        engine.addTrack(std::move(track));

        // Add listener to new track
        addTrackListener(child);
    }
}

void TrackStateSynchronizer::valueTreeChildRemoved(
    juce::ValueTree& parent,
    juce::ValueTree& child,
    int index)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::ignoreUnused(index);

    // Check if a track was removed from TRACKS node
    if (parent.hasType(ProjectState::ID_TRACKS) && child.hasType(ProjectState::ID_TRACK))
    {
        DBG("TrackStateSynchronizer: Track removed - removing from Engine");
        
        // Find track index in Engine
        // Since we maintain 1:1 mapping and order, we can use the child index
        // But we need to be careful if the Engine has extra tracks (e.g. test tracks)
        // For now, we assume strict sync
        
        // We can't use getEngineTrackIndex because the child is already removed from parent?
        // Wait, valueTreeChildRemoved is called AFTER removal from parent.
        // So 'parent' no longer contains 'child'.
        // But we have 'index' which is where it WAS.
        
        // If Engine tracks are perfectly synced with ProjectState tracks, 
        // then the Engine track at 'index' is the one to remove.
        
        engine.removeTrack(index);

        // Remove listener from removed track
        removeTrackListener(child);
    }
}

void TrackStateSynchronizer::valueTreeChildOrderChanged(
    juce::ValueTree& parent,
    int oldIndex,
    int newIndex)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::ignoreUnused(parent, oldIndex, newIndex);

    // Track order changed - would need to reorder Engine tracks
    // For Phase 11, we don't implement track reordering
}

void TrackStateSynchronizer::valueTreeParentChanged(juce::ValueTree& tree)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::ignoreUnused(tree);
    // Not used for our purposes
}

//==============================================================================
// Helper Methods
//==============================================================================

void TrackStateSynchronizer::syncTrackProperty(
    const juce::ValueTree& track,
    const juce::Identifier& property)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!track.isValid() || !track.hasType(ProjectState::ID_TRACK))
        return;

    const int trackIndex = getEngineTrackIndex(track);

    if (trackIndex < 0)
    {
        // Track not found in engine - this is OK if tracks haven't been created yet
        return;
    }

    // Sync the property to engine
    if (property == ProjectState::PROP_VOLUME)
    {
        const float volume = track[ProjectState::PROP_VOLUME];
        engine.setTrackVolume(trackIndex, volume);
    }
    else if (property == ProjectState::PROP_PAN)
    {
        const float pan = track[ProjectState::PROP_PAN];
        engine.setTrackPan(trackIndex, pan);
    }
    else if (property == ProjectState::PROP_MUTE)
    {
        const bool muted = track[ProjectState::PROP_MUTE];
        engine.setTrackMute(trackIndex, muted);
    }
    else if (property == ProjectState::PROP_SOLO)
    {
        const bool solo = track[ProjectState::PROP_SOLO];
        engine.setTrackSolo(trackIndex, solo);
    }
    else if (property == ProjectState::PROP_ARMED)
    {
        const bool armed = track[ProjectState::PROP_ARMED];
        engine.setTrackArmed(trackIndex, armed);
    }
}

int TrackStateSynchronizer::getEngineTrackIndex(const juce::ValueTree& track) const
{
    if (!track.isValid())
        return -1;

    // Find the index of this track in the TRACKS node
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
        return -1;

    // For Phase 11, we use simple index-based mapping
    // (ProjectState track index == Engine track index)
    for (int i = 0; i < tracksNode.getNumChildren(); ++i)
    {
        if (tracksNode.getChild(i) == track)
            return i;
    }

    return -1;
}

void TrackStateSynchronizer::addTrackListener(const juce::ValueTree& track)
{
    // Create mutable copy since addListener is non-const
    auto mutableTrack = track;
    if (mutableTrack.isValid() && mutableTrack.hasType(ProjectState::ID_TRACK))
    {
        mutableTrack.addListener(this);
    }
}

void TrackStateSynchronizer::removeTrackListener(const juce::ValueTree& track)
{
    // Create mutable copy since removeListener is non-const
    auto mutableTrack = track;
    if (mutableTrack.isValid() && mutableTrack.hasType(ProjectState::ID_TRACK))
    {
        mutableTrack.removeListener(this);
    }
}

