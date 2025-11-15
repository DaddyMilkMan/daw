/**
 * @file TrackAutomationSynchronizer.cpp
 * @brief Implementation of RT-safe automation synchronization
 */

#include "../include/TrackAutomationSynchronizer.h"
#include "../include/ProjectState.h"
#include "../include/Engine.h"
#include "../Source/engine/Track.h"

//==============================================================================
TrackAutomationSynchronizer::TrackAutomationSynchronizer(ProjectState& ps, Engine& eng)
    : projectState(ps)
    , engine(eng)
{
    DBG("TrackAutomationSynchronizer: Created");
}

TrackAutomationSynchronizer::~TrackAutomationSynchronizer()
{
    stop();
    DBG("TrackAutomationSynchronizer: Destroyed");
}

//==============================================================================
// Control
//==============================================================================

void TrackAutomationSynchronizer::start(int updateRateHz)
{
    if (isTimerRunning())
        return;

    rebuildTrackIdMap();

    int intervalMs = juce::jmax(1, 1000 / updateRateHz);
    startTimer(intervalMs);

    DBG("TrackAutomationSynchronizer: Started at " + juce::String(updateRateHz) + " Hz");
}

void TrackAutomationSynchronizer::stop()
{
    if (!isTimerRunning())
        return;

    stopTimer();
    DBG("TrackAutomationSynchronizer: Stopped");
}

//==============================================================================
// Timer Callback
//==============================================================================

void TrackAutomationSynchronizer::timerCallback()
{
    // MESSAGE THREAD ONLY

    // Check if track count changed (need to rebuild ID map)
    int currentTrackCount = engine.getNumTracks();
    if (currentTrackCount != lastTrackCount)
    {
        rebuildTrackIdMap();
        lastTrackCount = currentTrackCount;
    }

    // Get current playback position in beats
    // For now, we'll use a simple conversion from samples to beats
    // In a full implementation, this would come from the Engine's transport
    double sampleRate = engine.getSampleRate();
    double tempo = projectState.getTempo();

    // Calculate beats per second
    double beatsPerSecond = tempo / 60.0;

    // For now, always start from beat 0 if not playing
    // TODO: Wire this to actual Engine playback position
    double timeBeats = 0.0;

    if (engine.isPlaying())
    {
        // Simplified: in a real implementation, Engine would expose playbackPosition in beats
        // For now, we'll just increment based on time
        // This is a placeholder - actual implementation should read from Engine
        static double lastTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        double deltaTime = currentTime - lastTime;

        static double accumulatedBeats = 0.0;
        accumulatedBeats += deltaTime * beatsPerSecond;
        timeBeats = accumulatedBeats;

        lastTime = currentTime;
    }
    else
    {
        // Reset when stopped
        static double lastTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        lastTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        static double& accumulatedBeats = *new double(0.0);
        accumulatedBeats = 0.0;
    }

    // Sample automation for each track
    const auto& tracks = engine.tracks();
    for (size_t i = 0; i < tracks.size() && i < trackIdMap.size(); ++i)
    {
        if (tracks[i] != nullptr)
        {
            sampleTrackAutomation(tracks[i].get(), trackIdMap[i], timeBeats);
        }
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

void TrackAutomationSynchronizer::sampleTrackAutomation(zenith::Track* track,
                                                        const juce::String& trackId,
                                                        double timeBeats)
{
    if (track == nullptr)
        return;

    // Sample volume automation
    if (projectState.hasAutomation(trackId, "volume"))
    {
        double value = sampleAutomationValue(trackId, "volume", timeBeats);
        track->setVolume(static_cast<float>(value));
    }

    // Sample pan automation
    if (projectState.hasAutomation(trackId, "pan"))
    {
        double value = sampleAutomationValue(trackId, "pan", timeBeats);
        track->setPan(static_cast<float>(value));
    }

    // Sample mute automation
    if (projectState.hasAutomation(trackId, "mute"))
    {
        double value = sampleAutomationValue(trackId, "mute", timeBeats);
        track->setMuted(value > 0.5); // 0 = unmuted, 1 = muted
    }
}

double TrackAutomationSynchronizer::sampleAutomationValue(const juce::String& trackId,
                                                          const juce::String& paramId,
                                                          double timeBeats)
{
    auto envelope = projectState.getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return 0.0;

    auto pointsNode = envelope.getChildWithName(ProjectState::ID_POINTS);
    if (!pointsNode.isValid() || pointsNode.getNumChildren() == 0)
        return 0.0;

    // Find the two points surrounding the current time
    juce::ValueTree prevPoint;
    juce::ValueTree nextPoint;

    for (int i = 0; i < pointsNode.getNumChildren(); ++i)
    {
        auto point = pointsNode.getChild(i);
        double pointTime = point[ProjectState::PROP_TIME_BEATS];

        if (pointTime <= timeBeats)
        {
            prevPoint = point;
        }
        else
        {
            nextPoint = point;
            break;
        }
    }

    // If we're before the first point, use first point's value
    if (!prevPoint.isValid() && nextPoint.isValid())
    {
        return nextPoint[ProjectState::PROP_VALUE];
    }

    // If we're after the last point, use last point's value
    if (prevPoint.isValid() && !nextPoint.isValid())
    {
        return prevPoint[ProjectState::PROP_VALUE];
    }

    // If we have both points, interpolate
    if (prevPoint.isValid() && nextPoint.isValid())
    {
        double time1 = prevPoint[ProjectState::PROP_TIME_BEATS];
        double value1 = prevPoint[ProjectState::PROP_VALUE];
        double time2 = nextPoint[ProjectState::PROP_TIME_BEATS];
        double value2 = nextPoint[ProjectState::PROP_VALUE];

        return interpolate(time1, value1, time2, value2, timeBeats);
    }

    return 0.0;
}

double TrackAutomationSynchronizer::interpolate(double time1, double value1,
                                                double time2, double value2,
                                                double currentTime)
{
    if (time2 <= time1)
        return value1;

    double t = (currentTime - time1) / (time2 - time1);
    t = juce::jlimit(0.0, 1.0, t);

    return value1 + t * (value2 - value1);
}

void TrackAutomationSynchronizer::rebuildTrackIdMap()
{
    trackIdMap.clear();

    // Get tracks from ProjectState
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return;

    for (auto track : tracksNode)
    {
        juce::String trackId = track[ProjectState::PROP_ID].toString();
        trackIdMap.push_back(trackId);
    }

    DBG("TrackAutomationSynchronizer: Rebuilt track ID map with " +
        juce::String(trackIdMap.size()) + " tracks");
}
