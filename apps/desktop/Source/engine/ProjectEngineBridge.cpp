/*
  ==============================================================================

    ProjectEngineBridge.cpp
    Created: 2025-12-11
    Author:  Zenith DAW

    Consolidated synchronization between ProjectState and Engine.

  ==============================================================================
*/

#include "ProjectEngineBridge.h"
#include "ProjectState.h"
#include "Engine.h"
#include "Track.h"
#include "TempoMap.h"
#include "TransportController.h"

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

ProjectEngineBridge::ProjectEngineBridge(ProjectState& projectState, Engine& engine)
    : projectState_(projectState), engine_(engine)
{
    DBG("ProjectEngineBridge: Constructor");
}

ProjectEngineBridge::~ProjectEngineBridge()
{
    stop();
    DBG("ProjectEngineBridge: Destructor");
}

//==============================================================================
// Lifecycle
//==============================================================================

void ProjectEngineBridge::start(int automationUpdateRateHz)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(automationUpdateRateHz > 0 && automationUpdateRateHz <= 120);

    if (isActive_)
        return;

    DBG("ProjectEngineBridge: Starting with " + juce::String(automationUpdateRateHz) + " Hz automation update");

    // Listen to entire ProjectState tree
    projectState_.getState().addListener(this);

    // Build initial mappings
    rebuildTrackMapping();

    // Force initial sync
    forceFullSync();

    // Start timer for automation updates
    startTimer(1000 / automationUpdateRateHz);

    isActive_ = true;
}

void ProjectEngineBridge::stop()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!isActive_)
        return;

    DBG("ProjectEngineBridge: Stopping");

    stopTimer();
    projectState_.getState().removeListener(this);

    trackIdToIndex_.clear();
    pendingChanges_.clear();

    isActive_ = false;
}

void ProjectEngineBridge::forceFullSync()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Prevent re-entrancy
    if (isModifyingState_)
        return;

    juce::ScopedValueSetter<bool> guard(isModifyingState_, true);

    DBG("ProjectEngineBridge: Force full sync");

    // Sync tempo map
    syncTempoMap();

    // Rebuild track mapping
    rebuildTrackMapping();

    // Sync all tracks
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    for (auto trackNode : tracksNode)
    {
        if (!trackNode.hasType(ProjectState::ID_TRACK))
            continue;

        juce::String trackId = trackNode[ProjectState::PROP_ID].toString();
        int engineIndex = getEngineTrackIndex(trackId);

        if (engineIndex < 0)
            continue;

        // Sync all track properties
        syncTrackProperty(trackNode, ProjectState::PROP_VOLUME);
        syncTrackProperty(trackNode, ProjectState::PROP_PAN);
        syncTrackProperty(trackNode, ProjectState::PROP_MUTE);
        syncTrackProperty(trackNode, ProjectState::PROP_SOLO);
        syncTrackProperty(trackNode, ProjectState::PROP_ARMED);
    }
}

//==============================================================================
// Explicit Commits (Engine → ProjectState)
//==============================================================================

void ProjectEngineBridge::commitTransportPosition()
{
    // Currently a no-op; transport position is transient state
    // Future: Could persist loop region to ProjectState
}

void ProjectEngineBridge::commitRecordingResults()
{
    // Recording results are committed by RecordingManager directly
    // This is a hook for future centralized commit logic
}

//==============================================================================
// ValueTree::Listener Overrides
//==============================================================================

void ProjectEngineBridge::valueTreePropertyChanged(
    juce::ValueTree& tree, 
    const juce::Identifier& property)
{
    if (!isActive_ || isModifyingState_)
        return;

    // Track property changed
    if (tree.hasType(ProjectState::ID_TRACK))
    {
        syncTrackProperty(tree, property);
    }
    // Clip property changed
    else if (tree.hasType(ProjectState::ID_CLIP))
    {
        syncClipProperty(tree, property);
    }
    // Tempo point changed
    else if (tree.hasType(ProjectState::ID_TEMPO_POINT) ||
             tree.hasType(ProjectState::ID_TEMPO_MAP))
    {
        syncTempoMap();
    }
    // Project tempo changed
    else if (property == ProjectState::PROP_TEMPO)
    {
        syncTempoMap();
    }
}

void ProjectEngineBridge::valueTreeChildAdded(
    juce::ValueTree& parent, 
    juce::ValueTree& child)
{
    if (!isActive_ || isModifyingState_)
        return;

    // Track added
    if (child.hasType(ProjectState::ID_TRACK))
    {
        DBG("ProjectEngineBridge: Track added, rebuilding mapping");
        rebuildTrackMapping();
        
        // Note: Engine::syncWithProjectState() handles adding new tracks
        // This is triggered by the synchronizer, not duplicated here
    }
    // Clip added
    else if (child.hasType(ProjectState::ID_CLIP))
    {
        DBG("ProjectEngineBridge: Clip added");
        // Clips are synced by ClipSynchronizer logic (now internal)
    }
    // Tempo point added
    else if (child.hasType(ProjectState::ID_TEMPO_POINT))
    {
        syncTempoMap();
    }
}

void ProjectEngineBridge::valueTreeChildRemoved(
    juce::ValueTree& parent, 
    juce::ValueTree& child, 
    int index)
{
    juce::ignoreUnused(index);

    if (!isActive_ || isModifyingState_)
        return;

    // Track removed
    if (child.hasType(ProjectState::ID_TRACK))
    {
        DBG("ProjectEngineBridge: Track removed, rebuilding mapping");
        rebuildTrackMapping();
    }
    // Tempo point removed
    else if (child.hasType(ProjectState::ID_TEMPO_POINT))
    {
        syncTempoMap();
    }
}

void ProjectEngineBridge::valueTreeChildOrderChanged(
    juce::ValueTree& parent, 
    int oldIndex, 
    int newIndex)
{
    juce::ignoreUnused(parent, oldIndex, newIndex);

    if (!isActive_ || isModifyingState_)
        return;

    // Track order changed - rebuild mapping
    if (parent.hasType(ProjectState::ID_TRACKS))
    {
        DBG("ProjectEngineBridge: Track order changed, rebuilding mapping");
        rebuildTrackMapping();
    }
}

void ProjectEngineBridge::valueTreeParentChanged(juce::ValueTree& tree)
{
    juce::ignoreUnused(tree);
    // Not relevant for our synchronization
}

//==============================================================================
// Timer Override
//==============================================================================

void ProjectEngineBridge::timerCallback()
{
    // Update automation during playback
    if (engine_.isPlaying())
    {
        updatePlaybackAutomation();
    }

    // Process any batched pending changes
    processPendingChanges();
}

//==============================================================================
// Sync Helpers
//==============================================================================

void ProjectEngineBridge::syncTrackProperty(
    const juce::ValueTree& track, 
    const juce::Identifier& property)
{
    if (!track.isValid())
        return;

    juce::String trackId = track[ProjectState::PROP_ID].toString();
    int engineIndex = getEngineTrackIndex(trackId);

    if (engineIndex < 0)
        return;

    // Sync specific property
    if (property == ProjectState::PROP_VOLUME)
    {
        float volume = track[ProjectState::PROP_VOLUME];
        engine_.setTrackVolume(engineIndex, volume);
    }
    else if (property == ProjectState::PROP_PAN)
    {
        float pan = track[ProjectState::PROP_PAN];
        engine_.setTrackPan(engineIndex, pan);
    }
    else if (property == ProjectState::PROP_MUTE)
    {
        bool muted = track[ProjectState::PROP_MUTE];
        engine_.setTrackMute(engineIndex, muted);
    }
    else if (property == ProjectState::PROP_SOLO)
    {
        bool soloed = track[ProjectState::PROP_SOLO];
        engine_.setTrackSolo(engineIndex, soloed);
    }
    else if (property == ProjectState::PROP_ARMED)
    {
        bool armed = track[ProjectState::PROP_ARMED];
        engine_.setTrackArmed(engineIndex, armed);
    }
}

void ProjectEngineBridge::syncClipProperty(
    const juce::ValueTree& clip,
    const juce::Identifier& property)
{
    if (!clip.isValid())
        return;

    // Clip syncing is handled by the engine's syncWithProjectState call
    // For real-time updates, we would need more fine-grained control
    
    juce::ignoreUnused(property);
    
    // Future: Direct clip manipulation in Engine
    // For now, clips are re-synced when needed via full sync
}

void ProjectEngineBridge::syncTempoMap()
{
    DBG("ProjectEngineBridge: Syncing tempo map");
    engine_.syncTempoMap();
}

void ProjectEngineBridge::updatePlaybackAutomation()
{
    // Get current playback position in beats
    double playbackBeats = engine_.getPlaybackPositionBeats();

    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    const auto& engineTracks = engine_.tracks();

    int trackIndex = 0;
    for (auto trackNode : tracksNode)
    {
        if (!trackNode.hasType(ProjectState::ID_TRACK))
            continue;

        if (trackIndex >= static_cast<int>(engineTracks.size()))
            break;

        auto* track = engineTracks[trackIndex].get();
        if (track == nullptr)
        {
            ++trackIndex;
            continue;
        }

        juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

        // Check for volume automation
        if (projectState_.hasAutomation(trackId, "volume"))
        {
            auto envelope = projectState_.getAutomationEnvelope(trackId, "volume");
            double value = sampleEnvelope(envelope, playbackBeats);
            if (value >= 0.0)
            {
                track->setVolume(static_cast<float>(value));
            }
        }

        // Check for pan automation
        if (projectState_.hasAutomation(trackId, "pan"))
        {
            auto envelope = projectState_.getAutomationEnvelope(trackId, "pan");
            double value = sampleEnvelope(envelope, playbackBeats);
            if (value >= -1.0)
            {
                track->setPan(static_cast<float>(value));
            }
        }

        // Check for mute automation
        if (projectState_.hasAutomation(trackId, "mute"))
        {
            auto envelope = projectState_.getAutomationEnvelope(trackId, "mute");
            double value = sampleEnvelope(envelope, playbackBeats);
            if (value >= 0.0)
            {
                track->setMuted(value >= 0.5);
            }
        }

        ++trackIndex;
    }
}

int ProjectEngineBridge::getEngineTrackIndex(const juce::String& trackId) const
{
    auto it = trackIdToIndex_.find(trackId);
    if (it != trackIdToIndex_.end())
        return it->second;
    return -1;
}

double ProjectEngineBridge::sampleEnvelope(const juce::ValueTree& envelope, double timeBeats) const
{
    if (!envelope.isValid() || envelope.getNumChildren() == 0)
        return -1.0;  // No automation

    int numPoints = envelope.getNumChildren();

    // Find bracketing points
    juce::ValueTree prevPoint, nextPoint;

    for (int i = 0; i < numPoints; ++i)
    {
        auto point = envelope.getChild(i);
        if (!point.hasType(ProjectState::ID_POINT))
            continue;

        double pointTime = point[ProjectState::PROP_TIME_BEATS];

        if (pointTime <= timeBeats)
        {
            prevPoint = point;
        }
        else if (!nextPoint.isValid())
        {
            nextPoint = point;
            break;
        }
    }

    // No points before current time - use first point
    if (!prevPoint.isValid() && nextPoint.isValid())
        return static_cast<double>(nextPoint[ProjectState::PROP_VALUE]);

    // No points after current time - hold last value
    if (prevPoint.isValid() && !nextPoint.isValid())
        return static_cast<double>(prevPoint[ProjectState::PROP_VALUE]);

    // No points at all
    if (!prevPoint.isValid() && !nextPoint.isValid())
        return -1.0;

    // Linear interpolation between points
    double prevTime = prevPoint[ProjectState::PROP_TIME_BEATS];
    double prevValue = prevPoint[ProjectState::PROP_VALUE];
    double nextTime = nextPoint[ProjectState::PROP_TIME_BEATS];
    double nextValue = nextPoint[ProjectState::PROP_VALUE];

    double t = (timeBeats - prevTime) / (nextTime - prevTime);
    t = juce::jlimit(0.0, 1.0, t);

    return prevValue + t * (nextValue - prevValue);
}

void ProjectEngineBridge::processPendingChanges()
{
    // Currently all changes are processed immediately via listener callbacks
    // This method is reserved for future batched update optimization
    
    juce::ScopedLock lock(pendingLock_);
    pendingChanges_.clear();
}

void ProjectEngineBridge::rebuildTrackMapping()
{
    trackIdToIndex_.clear();

    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int index = 0;
    for (auto trackNode : tracksNode)
    {
        if (trackNode.hasType(ProjectState::ID_TRACK))
        {
            juce::String trackId = trackNode[ProjectState::PROP_ID].toString();
            trackIdToIndex_[trackId] = index;
            ++index;
        }
    }

    DBG("ProjectEngineBridge: Rebuilt mapping for " + juce::String(trackIdToIndex_.size()) + " tracks");
}

} // namespace zenith
