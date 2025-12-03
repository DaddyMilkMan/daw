/**
 * @file ClipSynchronizer.cpp
 * @brief ClipSynchronizer implementation (integration stub)
 */

#include "../include/ClipSynchronizer.h"
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"

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
    DBG("ClipSynchronizer: createClip(" + trackId + ", " +
        juce::String(startBeats) + ", " + juce::String(lengthBeats) + ", " + clipType + ")");

    // 1. Create in ProjectState first to generate ID
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
        return {};

    juce::String newClipId;

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
            newClipId = "clip_" + juce::Uuid().toString().substring(0, 8);
            clip.setProperty(ProjectState::PROP_ID, newClipId, nullptr);
            clip.setProperty(ProjectState::PROP_TYPE, clipType, nullptr);
            clip.setProperty(ProjectState::PROP_START, startBeats, nullptr);
            clip.setProperty(ProjectState::PROP_LENGTH, lengthBeats, nullptr);

            clipsNode.appendChild(clip, &projectState.getUndoManager());
            break;
        }
    }

    if (newClipId.isEmpty())
    {
        DBG("ClipSynchronizer: Failed to find track in ProjectState: " + trackId);
        return {};
    }

    // 2. Create in Engine (Real Implementation)
    // Iterate engine tracks to find the matching one
    // Note: This assumes Engine tracks are synced with ProjectState tracks.
    // Since we don't have a map, we might need to rely on index or name, but let's try to find by ID if Track has it.
    // Track.h has getTrackId().

    bool engineTrackFound = false;
    for (const auto& trackPtr : engine.tracks())
    {
        if (trackPtr->getTrackId() == trackId)
        {
            auto newClip = std::make_unique<zenith::Track::Clip>();
            
            // Convert beats to samples
            double tempo = projectState.getTempo();
            double sampleRate = engine.getSampleRate();
            int64_t startSamples = beatsToSamples(startBeats, tempo, sampleRate);
            int64_t lengthSamples = beatsToSamples(lengthBeats, tempo, sampleRate);

            newClip->setStartPosition(startSamples);
            newClip->setLength(lengthSamples);
            newClip->setName(newClipId); // Use ID as name for now
            newClip->setType(clipType == "midi" ? zenith::Track::Clip::Type::MIDI : zenith::Track::Clip::Type::Audio);

            trackPtr->addClip(std::move(newClip));
            engineTrackFound = true;
            DBG("ClipSynchronizer: Added clip to Engine track " + trackId);
            break;
        }
    }

    if (!engineTrackFound)
    {
        DBG("ClipSynchronizer: Warning - Track not found in Engine: " + trackId);
    }

    DBG("ClipSynchronizer: Created clip " + newClipId);
    return newClipId;
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
    // [STUB AUDIT] - This method is intended for RECORDING sync (Engine -> ProjectState).
    // Since RecordingEngine is not yet merged, this remains a partial stub.
    // However, we log it to ensure visibility.
    
    // In the future, this will:
    // 1. Iterate Engine tracks
    // 2. Check for new recorded clips
    // 3. Create ProjectState nodes for them
    
    // DBG("ClipSynchronizer: syncEngineToProjectState (No-op until RecordingEngine)");
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

