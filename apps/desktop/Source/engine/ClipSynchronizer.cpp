/**
 * @file ClipSynchronizer.cpp
 * @brief ClipSynchronizer implementation (integration stub)
 */

#include "../include/ClipSynchronizer.h"
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"

//==============================================================================
ClipSynchronizer::ClipSynchronizer(zenith::ProjectState& ps, zenith::Engine& eng)
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

    // 1. Create in zenith::ProjectState first to generate ID
    auto& state = projectState.getState();
    auto tracksNode = state.getChildWithName(zenith::ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
        return {};

    juce::String newClipId;

    // Find track (need mutable reference to appendChild)
    for (auto track : tracksNode)
    {
        if (track[zenith::ProjectState::PROP_ID].toString() == trackId)
        {
            auto clipsNode = track.getChildWithName(zenith::ProjectState::ID_CLIPS);
            if (!clipsNode.isValid())
            {
                clipsNode = juce::ValueTree(zenith::ProjectState::ID_CLIPS);
                track.appendChild(clipsNode, nullptr);
            }

            // Create clip
            juce::ValueTree clip(zenith::ProjectState::ID_CLIP);
            newClipId = "clip_" + juce::Uuid().toString().substring(0, 8);
            clip.setProperty(zenith::ProjectState::PROP_ID, newClipId, nullptr);
            clip.setProperty(zenith::ProjectState::PROP_TYPE, clipType, nullptr);
            clip.setProperty(zenith::ProjectState::PROP_START, startBeats, nullptr);
            clip.setProperty(zenith::ProjectState::PROP_LENGTH, lengthBeats, nullptr);

            clipsNode.appendChild(clip, &projectState.getUndoManager());
            break;
        }
    }

    if (newClipId.isEmpty())
    {
        DBG("ClipSynchronizer: Failed to find track in zenith::ProjectState: " + trackId);
        return {};
    }

    // 2. Create in Engine (Real Implementation)
    // Iterate engine tracks to find the matching one
    // Note: This assumes Engine tracks are synced with zenith::ProjectState tracks.
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
    // Sync Engine clips to zenith::ProjectState
    syncEngineToProjectState();
}

//==============================================================================
void ClipSynchronizer::syncEngineToProjectState()
{
    // This method implements Engine→zenith::ProjectState sync for recorded clips.
    // Called from timer (Message Thread), so safe to modify zenith::ProjectState.
    
    // Thread safety note: This runs on Message Thread (via timer),
    // Engine access must be done carefully to avoid blocking audio thread.
    
    // Get all Engine tracks (read-only access, should be lock-free)
    const auto& engineTracks = engine.tracks();
    
    // Iterate Engine tracks to find new/modified clips
    for (const auto& trackPtr : engineTracks)
    {
        if (!trackPtr) continue;
        
        juce::String trackId = trackPtr->getTrackId();
        
        // Get corresponding zenith::ProjectState track
        auto projectTrack = projectState.getTrack(trackId);
        if (!projectTrack.isValid())
        {
            DBG("ClipSynchronizer: Track " + trackId + " not found in zenith::ProjectState - skipping sync");
            continue;
        }
        
        // Get Engine's clip list (thread-safe read via accessor)
        const auto& engineClips = trackPtr->getClips();
        
        // Get zenith::ProjectState clips container
        auto clipsNode = projectTrack.getChildWithName(zenith::ProjectState::ID_CLIPS);
        if (!clipsNode.isValid())
        {
            clipsNode = juce::ValueTree(zenith::ProjectState::ID_CLIPS);
            projectTrack.appendChild(clipsNode, nullptr);
        }
        
        // Sync each Engine clip to zenith::ProjectState
        for (size_t i = 0; i < engineClips.size(); ++i)
        {
            const auto& engineClip = engineClips[i];
            
            // Check if this clip exists in zenith::ProjectState
            juce::String clipName = engineClip->getName();
            bool foundInProjectState = false;
            
            for (auto clipNode : clipsNode)
            {
                if (clipNode[zenith::ProjectState::PROP_NAME].toString() == clipName)
                {
                    foundInProjectState = true;
                    
                    // Update clip properties if changed
                    int64_t engineStart = engineClip->getStartPosition();
                    int64_t engineLength = engineClip->getLength();
                    
                    // Convert samples to beats
                    double tempo = projectState.getTempo();
                    double sampleRate = engine.getSampleRate();
                    
                    double startBeats = samplesToBeats(engineStart, tempo, sampleRate);
                    double lengthBeats = samplesToBeats(engineLength, tempo, sampleRate);
                    
                    // Update if different (with small tolerance for float precision)
                    double currentStart = clipNode[zenith::ProjectState::PROP_START];
                    double currentLength = clipNode[zenith::ProjectState::PROP_LENGTH];
                    
                    const double tolerance = 0.001; // ~1ms at 120bpm
                    if (std::abs(currentStart - startBeats) > tolerance || 
                        std::abs(currentLength - lengthBeats) > tolerance)
                    {
                        clipNode.setProperty(zenith::ProjectState::PROP_START, startBeats, &projectState.getUndoManager());
                        clipNode.setProperty(zenith::ProjectState::PROP_LENGTH, lengthBeats, &projectState.getUndoManager());
                        
                        DBG("ClipSynchronizer: Updated clip " + clipName + " in track " + trackId);
                    }
                    break;
                }
            }
            
            // If clip not found in zenith::ProjectState, it was just recorded - add it
            if (!foundInProjectState)
            {
                // This would typically only happen for newly recorded clips
                juce::ValueTree newClip(zenith::ProjectState::ID_CLIP);
                
                juce::String newClipId = "clip_" + juce::Uuid().toString().substring(0, 8);
                newClip.setProperty(zenith::ProjectState::PROP_ID, juce::var(newClipId), nullptr);
                newClip.setProperty(zenith::ProjectState::PROP_NAME, juce::var(clipName), nullptr);
                newClip.setProperty(zenith::ProjectState::PROP_TYPE, 
                    juce::var(engineClip->getType() == zenith::Track::Clip::Type::MIDI ? "midi" : "audio"), 
                    nullptr);
                
                // Convert samples to beats
                double tempo = projectState.getTempo();
                double sampleRate = engine.getSampleRate();
                
                double startBeats = samplesToBeats(engineClip->getStartPosition(), tempo, sampleRate);
                double lengthBeats = samplesToBeats(engineClip->getLength(), tempo, sampleRate);
                
                newClip.setProperty(zenith::ProjectState::PROP_START, startBeats, nullptr);
                newClip.setProperty(zenith::ProjectState::PROP_LENGTH, lengthBeats, nullptr);
                
                clipsNode.appendChild(newClip, &projectState.getUndoManager());
                
                DBG("ClipSynchronizer: Added new recorded clip " + clipName + " to track " + trackId);
            }
        }
    }
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

