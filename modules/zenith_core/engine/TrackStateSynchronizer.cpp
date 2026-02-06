/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "TrackStateSynchronizer.h"
#include "ProjectState.h"
#include "Engine.h"
#include "Track.h"
#include "ZenithLogger.h"

namespace zenith {



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
        syncTrackProperty(track, ProjectState::PROP_INPUT_CHANNEL); // Sync audio input routing
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
    if (parent.hasType(zenith::ProjectState::ID_TRACKS) && child.hasType(zenith::ProjectState::ID_TRACK))
    {
        DBG("TrackStateSynchronizer: Track added - creating Engine track");
        
        // Create new Engine track
        juce::String name = child[zenith::ProjectState::PROP_NAME].toString();
        juce::String typeStr = child[zenith::ProjectState::PROP_TYPE].toString();
        juce::String id = child[zenith::ProjectState::PROP_ID].toString();
        
        zenith::Track::Type type = (typeStr == "midi") ? zenith::Track::Type::MIDI : zenith::Track::Type::Audio;
        
        auto track = zenith::Track::create(name, type);
        track->setTrackId(id);
        
        // Set initial properties
        track->setVolume(child[zenith::ProjectState::PROP_VOLUME]);
        track->setPan(child[zenith::ProjectState::PROP_PAN]);
        track->setMuted(child[zenith::ProjectState::PROP_MUTE]);
        track->setSolo(child[zenith::ProjectState::PROP_SOLO]);
        track->setInputChannel(child[zenith::ProjectState::PROP_INPUT_CHANNEL]); // Set audio input routing
        
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
    if (parent.hasType(zenith::ProjectState::ID_TRACKS) && child.hasType(zenith::ProjectState::ID_TRACK))
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
        
        // If Engine tracks are perfectly synced with zenith::ProjectState tracks, 
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

    if (!track.isValid() || !track.hasType(zenith::ProjectState::ID_TRACK))
        return;

    const int trackIndex = getEngineTrackIndex(track);

    if (trackIndex < 0)
    {
        // Track not found in engine - this is OK if tracks haven't been created yet
        return;
    }

    // Sync the property to engine
    if (property == zenith::ProjectState::PROP_VOLUME)
    {
        const float volume = track[zenith::ProjectState::PROP_VOLUME];
        engine.setTrackVolume(trackIndex, volume);
    }
    else if (property == zenith::ProjectState::PROP_PAN)
    {
        const float pan = track[zenith::ProjectState::PROP_PAN];
        engine.setTrackPan(trackIndex, pan);
    }
    else if (property == zenith::ProjectState::PROP_MUTE)
    {
        const bool muted = track[zenith::ProjectState::PROP_MUTE];
        engine.setTrackMute(trackIndex, muted);
    }
    else if (property == zenith::ProjectState::PROP_SOLO)
    {
        const bool solo = track[zenith::ProjectState::PROP_SOLO];
        engine.setTrackSolo(trackIndex, solo);
    }
    else if (property == zenith::ProjectState::PROP_ARMED)
    {
        const bool armed = track[zenith::ProjectState::PROP_ARMED];
        engine.setTrackArmed(trackIndex, armed);
    }
    else if (property == zenith::ProjectState::PROP_INPUT_CHANNEL)
    {
        const int channel = track[zenith::ProjectState::PROP_INPUT_CHANNEL];
        engine.setTrackInputChannel(trackIndex, channel);
    }
}

int TrackStateSynchronizer::getEngineTrackIndex(const juce::ValueTree& track) const
{
    if (!track.isValid())
        return -1;

    // Find the index of this track in the TRACKS node
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(zenith::ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
        return -1;

    // For Phase 11, we use simple index-based mapping
    // (zenith::ProjectState track index == Engine track index)
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
    if (mutableTrack.isValid() && mutableTrack.hasType(zenith::ProjectState::ID_TRACK))
    {
        mutableTrack.addListener(this);
    }
}

void TrackStateSynchronizer::removeTrackListener(const juce::ValueTree& track)
{
    // Create mutable copy since removeListener is non-const
    auto mutableTrack = track;
    if (mutableTrack.isValid() && mutableTrack.hasType(zenith::ProjectState::ID_TRACK))
    {
        mutableTrack.removeListener(this);
    }
}

} // namespace zenith

