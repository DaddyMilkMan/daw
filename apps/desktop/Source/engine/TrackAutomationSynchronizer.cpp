/**
 * @file TrackAutomationSynchronizer.cpp
 * @brief Automation synchronizer implementation
 */

#include "TrackAutomationSynchronizer.h"
#include "ProjectState.h"
#include "Engine.h"
#include "TempoMap.h"

// Forward declare Track from namespace
#include "../Source/engine/Track.h"

//==============================================================================
namespace zenith {

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

    if (!engine.isPlaying())
    {
        return;  // Only update during playback
    }

    // Get actual playback position from engine
    juce::int64 playheadSamples = engine.getPlayheadSamples();
    double sampleRate = engine.getSampleRate();
    
    // Phase 15: Use TempoMap for samples → beats conversion (handles variable tempo)
    double playbackBeats = engine.getTempoMap().samplesToBeats(playheadSamples, sampleRate);

    // Update all tracks with automation
    auto tracksNode = projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int trackIndex = 0;
    for (const auto& trackNode : tracksNode)
    {
        if (!trackNode.hasType(zenith::ProjectState::ID_TRACK))
            continue;

        juce::String trackId = trackNode[zenith::ProjectState::PROP_ID].toString();

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
}

//==============================================================================
// ValueTree::Listener (MESSAGE THREAD)
//==============================================================================

void TrackAutomationSynchronizer::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // If a track property changed (mute, solo, armed, volume, pan), trigger immediate sync
    if (tree.hasType(zenith::ProjectState::ID_TRACK))
    {
        if (property == zenith::ProjectState::PROP_MUTE ||
            property == zenith::ProjectState::PROP_SOLO ||
            property == zenith::ProjectState::PROP_ARMED ||
            property == zenith::ProjectState::PROP_VOLUME ||
            property == zenith::ProjectState::PROP_PAN)
        {
            // Get track ID
            juce::String trackId = tree[zenith::ProjectState::PROP_ID].toString();

            // Find corresponding engine track
            auto tracksNode = projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
            if (!tracksNode.isValid())
                return;

            int trackIndex = 0;
            for (const auto& trackNode : tracksNode)
            {
                if (trackNode.hasType(zenith::ProjectState::ID_TRACK))
                {
                    if (trackNode[zenith::ProjectState::PROP_ID].toString() == trackId)
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
    if (child.hasType(zenith::ProjectState::ID_AUTOMATION) ||
        child.hasType(zenith::ProjectState::ID_ENVELOPE) ||
        child.hasType(zenith::ProjectState::ID_POINT))
    {
        rebuildListeners();
    }
}

void TrackAutomationSynchronizer::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, index);

    // If automation node or envelope removed, rebuild
    if (child.hasType(zenith::ProjectState::ID_AUTOMATION) ||
        child.hasType(zenith::ProjectState::ID_ENVELOPE) ||
        child.hasType(zenith::ProjectState::ID_POINT))
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
        if (!point.hasType(zenith::ProjectState::ID_POINT))
            continue;

        double pointTime = point[zenith::ProjectState::PROP_TIME_BEATS];

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
        return nextPoint[zenith::ProjectState::PROP_VALUE];

    // No points after current time - hold last value
    if (prevPoint.isValid() && !nextPoint.isValid())
        return prevPoint[zenith::ProjectState::PROP_VALUE];

    // No points at all
    if (!prevPoint.isValid() && !nextPoint.isValid())
        return -1.0;

    // Interpolate between two points
    double prevTime = prevPoint[zenith::ProjectState::PROP_TIME_BEATS];
    double prevValue = prevPoint[zenith::ProjectState::PROP_VALUE];
    double nextTime = nextPoint[zenith::ProjectState::PROP_TIME_BEATS];
    double nextValue = nextPoint[zenith::ProjectState::PROP_VALUE];

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

    // Get track node from zenith::ProjectState
    auto trackNode = projectState.getTrack(trackId);
    if (!trackNode.isValid())
        return;

    // First, sync basic track properties (always apply these)
    // These come from the track header controls, not automation
    bool hasMuteAutomation = projectState.hasAutomation(trackId, "mute");

    // Only apply static mute/solo/armed if there's no automation for them
    if (!hasMuteAutomation)
    {
        bool muted = trackNode[zenith::ProjectState::PROP_MUTE];
        if (track->isMuted() != muted)
            track->setMuted(muted);
    }

    // Solo and armed don't have automation, always sync them
    bool soloed = trackNode[zenith::ProjectState::PROP_SOLO];
    if (track->isSolo() != soloed)
        track->setSolo(soloed);

    bool armed = trackNode[zenith::ProjectState::PROP_ARMED];
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
        // No automation, sync static volume from zenith::ProjectState
        float volume = trackNode[zenith::ProjectState::PROP_VOLUME];
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
        // No automation, sync static pan from zenith::ProjectState
        float pan = trackNode[zenith::ProjectState::PROP_PAN];
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

    // Rebuild mapping from zenith::ProjectState track IDs to Engine track indices
    auto tracksNode = projectState.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    int index = 0;
    for (const auto& trackNode : tracksNode)
    {
        if (trackNode.hasType(zenith::ProjectState::ID_TRACK))
        {
            juce::String trackId = trackNode[zenith::ProjectState::PROP_ID].toString();
            trackIdToIndex[trackId] = index;
            ++index;
        }
    }

    DBG("TrackAutomationSynchronizer: Rebuilt listeners for " + juce::String(trackIdToIndex.size()) + " tracks");
}

} // namespace zenith

