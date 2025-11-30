/**
 * @file ClipSynchronizer.cpp
 * @brief ClipSynchronizer implementation (integration stub)
 */

#include "../include/ClipSynchronizer.h"
#include "engine/Track.h"
#include "engine/Clip.h"

//==============================================================================
ClipSynchronizer::ClipSynchronizer(ProjectState& ps, Engine& eng)
    : projectState(ps), engine(eng)
{
    DBG("ClipSynchronizer: Constructor");
}

ClipSynchronizer::~ClipSynchronizer()
{
    stop();
    DBG("ClipSynchronizer: Destructor");
}

//==============================================================================
void ClipSynchronizer::start(int updateRateHz)
{
    if (updateRateHz <= 0)
        updateRateHz = 30;

    startTimer(1000 / updateRateHz);
    DBG("ClipSynchronizer: Started at " + juce::String(updateRateHz) + " Hz");
}

void ClipSynchronizer::stop()
{
    stopTimer();
    DBG("ClipSynchronizer: Stopped");
}

//==============================================================================
juce::String ClipSynchronizer::createClip(const juce::String& trackId, double startBeats,
                                          double lengthBeats, const juce::String& clipType)
{
    // Integration stub: Would create clip in both Engine and ProjectState
    DBG("ClipSynchronizer: createClip(" + trackId + ", " +
        juce::String(startBeats) + ", " + juce::String(lengthBeats) + ", " + clipType + ")");

    // TODO (when U3 recording branch is merged):
    // 1. Find Engine track by trackId
    // 2. Create zenith::Clip in Engine
    // 3. Create CLIP node in ProjectState
    // 4. Link them together
    // 5. Return clip ID

    // For now, just create in ProjectState
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
        return {};

    // Find track (need mutable reference to appendChild)
    for (auto track : tracksNode)
    {
        if (track[ProjectState::PROP_ID].toString() == trackId)
        {
            auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
            if (!clipsNode.isValid())
            {
                clipsNode = juce::ValueTree(ProjectState::ID_CLIPS);
                track.appendChild(clipsNode, nullptr);
            }

            // Create clip
            juce::ValueTree clip(ProjectState::ID_CLIP);
            auto clipId = "clip_" + juce::Uuid().toString().substring(0, 8);
            clip.setProperty(ProjectState::PROP_ID, clipId, nullptr);
            clip.setProperty(ProjectState::PROP_TYPE, clipType, nullptr);
            clip.setProperty(ProjectState::PROP_START, startBeats, nullptr);
            clip.setProperty(ProjectState::PROP_LENGTH, lengthBeats, nullptr);

            clipsNode.appendChild(clip, &projectState.getUndoManager());

            DBG("ClipSynchronizer: Created clip " + clipId);
            return clipId;
        }
    }

    return {};
}

//==============================================================================
void ClipSynchronizer::timerCallback()
{
    // Sync Engine clips to ProjectState
    syncEngineToProjectState();
}

//==============================================================================
void ClipSynchronizer::syncEngineToProjectState()
{
    // Integration stub: Would check for new clips in Engine tracks
    // When RecordingEngine creates a clip, it appears here and we sync to ProjectState

    // TODO (when U3 recording branch is merged):
    // 1. Iterate Engine tracks
    // 2. Check if track has more clips than last time (engineClipCounts)
    // 3. For each new clip, create corresponding ProjectState CLIP node
    // 4. Update engineClipCounts

    // For now, this is a no-op since we don't have RecordingEngine yet
}

//==============================================================================
int64_t ClipSynchronizer::beatsToSamples(double beats, double tempo, double sampleRate) const
{
    // beats * (60 / tempo) * sampleRate = samples
    double seconds = beats * (60.0 / tempo);
    return static_cast<int64_t>(seconds * sampleRate);
}

double ClipSynchronizer::samplesToBeats(int64_t samples, double tempo, double sampleRate) const
{
    // samples / sampleRate / (60 / tempo) = beats
    double seconds = static_cast<double>(samples) / sampleRate;
    return seconds / (60.0 / tempo);
}

