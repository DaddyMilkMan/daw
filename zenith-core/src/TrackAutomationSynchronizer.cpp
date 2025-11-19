/**
 * @file TrackAutomationSynchronizer.cpp
 * @brief Automation synchronizer implementation
 */

#include "../include/TrackAutomationSynchronizer.h"

// Forward declare Track from namespace
#include "../Source/engine/Track.h"

//==============================================================================
TrackAutomationSynchronizer::TrackAutomationSynchronizer(ProjectState& ps, Engine& eng)
    : projectState(ps), engine(eng)
{
    DBG("TrackAutomationSynchronizer: Constructor");

    // Listen to the entire ProjectState tree for automation changes
    projectState.getState().addListener(this);

    // Build initial track mapping
    rebuildListeners();
}

TrackAutomationSynchronizer::~TrackAutomationSynchronizer()
{
    DBG("TrackAutomationSynchronizer: Destructor");

    // Stop timer
    stopTimer();

    // Remove listener
    projectState.getState().removeListener(this);
}

//==============================================================================
void TrackAutomationSynchronizer::start(int updateRateHz)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(updateRateHz > 0 && updateRateHz <= 1000);

    int intervalMs = 1000 / updateRateHz;
    startTimer(intervalMs);

    DBG("TrackAutomationSynchronizer: Started at " + juce::String(updateRateHz) + " Hz");
}

void TrackAutomationSynchronizer::stop()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    stopTimer();

    DBG("TrackAutomationSynchronizer: Stopped");
}

//==============================================================================
// Timer Callback (MESSAGE THREAD)
//==============================================================================

void TrackAutomationSynchronizer::timerCallback()
{
    // MESSAGE THREAD - Safe to access ValueTree and call Track setters

    // For now, use a simple frame counter approach
    // In a real implementation, Engine would expose playback position
    static juce::int64 frameCounter = 0;

    if (!engine.isPlaying())
    {
        frameCounter = 0;  // Reset on stop to ensure automation starts from beginning
        return;  // Only update during playback
    }

    // Get playback position in samples from engine
    // Note: Engine would need to expose this - for now we'll add a method
    // For MVP, we can use a simplified approach

    // Phase 15: Use TempoMap for samples → beats conversion (handles variable tempo)
    double sampleRate = engine.getSampleRate();
    double playbackBeats = engine.getTempoMap().samplesToBeats(frameCounter, sampleRate);

    // Update all tracks with automation
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int trackIndex = 0;
    for (auto trackNode : tracksNode)
    {
        if (!trackNode.hasType(ProjectState::ID_TRACK))
            continue;

        juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

        // Get corresponding engine track
        if (trackIndex < engine.getNumTracks())
        {
            auto& tracks = engine.tracks();
            if (trackIndex < static_cast<int>(tracks.size()))
            {
                auto* track = tracks[trackIndex].get();
                if (track != nullptr)
                {
                    updateTrackAutomation(trackId, track, playbackBeats);
                }
            }
        }

        ++trackIndex;
    }

    // Increment frame counter (rough estimate)
    // In real impl, this would come from Engine
    frameCounter += static_cast<juce::int64>(sampleRate / 60.0);  // ~1/60 sec
}

//==============================================================================
// ValueTree::Listener (MESSAGE THREAD)
//==============================================================================

void TrackAutomationSynchronizer::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // If a track property changed (mute, solo, armed, volume, pan), trigger immediate sync
    if (tree.hasType(ProjectState::ID_TRACK))
    {
        if (property == ProjectState::PROP_MUTE ||
            property == ProjectState::PROP_SOLO ||
            property == ProjectState::PROP_ARMED ||
            property == ProjectState::PROP_VOLUME ||
            property == ProjectState::PROP_PAN)
        {
            // Get track ID
            juce::String trackId = tree[ProjectState::PROP_ID].toString();

            // Find corresponding engine track
            auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
            if (!tracksNode.isValid())
                return;

            int trackIndex = 0;
            for (auto trackNode : tracksNode)
            {
                if (trackNode.hasType(ProjectState::ID_TRACK))
                {
                    if (trackNode[ProjectState::PROP_ID].toString() == trackId)
                    {
                        // Found the track, update it immediately
                        if (trackIndex < engine.getNumTracks())
                        {
                            auto& tracks = engine.tracks();
                            if (trackIndex < static_cast<int>(tracks.size()))
                            {
                                auto* track = tracks[trackIndex].get();
                                if (track != nullptr)
                                {
                                    // Sync this track immediately (don't wait for timer)
                                    updateTrackAutomation(trackId, track, 0.0);
                                }
                            }
                        }
                        break;
                    }
                    ++trackIndex;
                }
            }
        }
    }
}

void TrackAutomationSynchronizer::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(parent);

    // If automation node or envelope added, rebuild
    if (child.hasType(ProjectState::ID_AUTOMATION) ||
        child.hasType(ProjectState::ID_ENVELOPE) ||
        child.hasType(ProjectState::ID_POINT))
    {
        rebuildListeners();
    }
}

void TrackAutomationSynchronizer::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, index);

    // If automation node or envelope removed, rebuild
    if (child.hasType(ProjectState::ID_AUTOMATION) ||
        child.hasType(ProjectState::ID_ENVELOPE) ||
        child.hasType(ProjectState::ID_POINT))
    {
        rebuildListeners();
    }
}

void TrackAutomationSynchronizer::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    juce::ignoreUnused(parent, oldIndex, newIndex);
}

void TrackAutomationSynchronizer::valueTreeParentChanged(juce::ValueTree& tree)
{
    juce::ignoreUnused(tree);
}

//==============================================================================
// Helper Methods
//==============================================================================

double TrackAutomationSynchronizer::sampleEnvelope(const juce::ValueTree& envelope, double timeBeats) const
{
    if (!envelope.isValid() || envelope.getNumChildren() == 0)
        return -1.0;  // No automation

    // Get all points sorted by time (they should already be sorted)
    int numPoints = envelope.getNumChildren();

    // Find the two points that bracket the current time
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

    // No points before current time - use first point value
    if (!prevPoint.isValid() && nextPoint.isValid())
        return nextPoint[ProjectState::PROP_VALUE];

    // No points after current time - hold last value
    if (prevPoint.isValid() && !nextPoint.isValid())
        return prevPoint[ProjectState::PROP_VALUE];

    // No points at all
    if (!prevPoint.isValid() && !nextPoint.isValid())
        return -1.0;

    // Interpolate between two points
    double prevTime = prevPoint[ProjectState::PROP_TIME_BEATS];
    double prevValue = prevPoint[ProjectState::PROP_VALUE];
    double nextTime = nextPoint[ProjectState::PROP_TIME_BEATS];
    double nextValue = nextPoint[ProjectState::PROP_VALUE];

    // Linear interpolation
    double t = (timeBeats - prevTime) / (nextTime - prevTime);
    t = juce::jlimit(0.0, 1.0, t);

    return prevValue + t * (nextValue - prevValue);
}

void TrackAutomationSynchronizer::updateTrackAutomation(const juce::String& trackId,
                                                         zenith::Track* track,
                                                         double playbackBeats)
{
    if (track == nullptr)
        return;

    // Get track node from ProjectState
    auto trackNode = projectState.getTrack(trackId);
    if (!trackNode.isValid())
        return;

    // First, sync basic track properties (always apply these)
    // These come from the track header controls, not automation
    bool hasMuteAutomation = projectState.hasAutomation(trackId, "mute");

    // Only apply static mute/solo/armed if there's no automation for them
    if (!hasMuteAutomation)
    {
        bool muted = trackNode[ProjectState::PROP_MUTE];
        if (track->isMuted() != muted)
            track->setMuted(muted);
    }

    // Solo and armed don't have automation, always sync them
    bool soloed = trackNode[ProjectState::PROP_SOLO];
    if (track->isSolo() != soloed)
        track->setSolo(soloed);

    bool armed = trackNode[ProjectState::PROP_ARMED];
    if (track->isArmed() != armed)
        track->setArmed(armed);

    // Sample and apply volume automation
    if (projectState.hasAutomation(trackId, "volume"))
    {
        auto envelope = projectState.getAutomationEnvelope(trackId, "volume");
        double value = sampleEnvelope(envelope, playbackBeats);
        if (value >= 0.0)
        {
            track->setVolume(static_cast<float>(value));
        }
    }
    else
    {
        // No automation, sync static volume from ProjectState
        float volume = trackNode[ProjectState::PROP_VOLUME];
        if (std::abs(track->getVolume() - volume) > 0.001f)
            track->setVolume(volume);
    }

    // Sample and apply pan automation
    if (projectState.hasAutomation(trackId, "pan"))
    {
        auto envelope = projectState.getAutomationEnvelope(trackId, "pan");
        double value = sampleEnvelope(envelope, playbackBeats);
        if (value >= -1.0)  // Valid pan values are -1 to 1
        {
            track->setPan(static_cast<float>(value));
        }
    }
    else
    {
        // No automation, sync static pan from ProjectState
        float pan = trackNode[ProjectState::PROP_PAN];
        if (std::abs(track->getPan() - pan) > 0.001f)
            track->setPan(pan);
    }

    // Sample and apply mute automation (if present)
    if (hasMuteAutomation)
    {
        auto envelope = projectState.getAutomationEnvelope(trackId, "mute");
        double value = sampleEnvelope(envelope, playbackBeats);
        if (value >= 0.0)
        {
            track->setMuted(value >= 0.5);  // Binary: 0 or 1
        }
    }
}

void TrackAutomationSynchronizer::rebuildListeners()
{
    // Clear existing mapping
    trackIdToIndex.clear();

    // Rebuild mapping from ProjectState track IDs to Engine track indices
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int index = 0;
    for (auto trackNode : tracksNode)
    {
        if (trackNode.hasType(ProjectState::ID_TRACK))
        {
            juce::String trackId = trackNode[ProjectState::PROP_ID].toString();
            trackIdToIndex[trackId] = index;
            ++index;
        }
    }

    DBG("TrackAutomationSynchronizer: Rebuilt listeners for " + juce::String(trackIdToIndex.size()) + " tracks");
}
