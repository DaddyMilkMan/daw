/**
 * @file ClipSynchronizer.cpp
 * @brief Clip synchronizer implementation
 */

#include "../include/ClipSynchronizer.h"

// Forward declare Track from namespace
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"

//==============================================================================
ClipSynchronizer::ClipSynchronizer(ProjectState& ps, Engine& eng)
    : projectState(ps), engine(eng)
{
    DBG("ClipSynchronizer: Constructor");

    // Listen to the entire ProjectState tree for clip changes
    projectState.getState().addListener(this);

    // Build initial state
    rebuildState();
}

ClipSynchronizer::~ClipSynchronizer()
{
    DBG("ClipSynchronizer: Destructor");

    // Stop timer
    stopTimer();

    // Remove listener
    projectState.getState().removeListener(this);
}

//==============================================================================
void ClipSynchronizer::start(int updateRateHz)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(updateRateHz > 0 && updateRateHz <= 1000);

    int intervalMs = 1000 / updateRateHz;
    startTimer(intervalMs);

    DBG("ClipSynchronizer: Started at " + juce::String(updateRateHz) + " Hz");
}

void ClipSynchronizer::stop()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    stopTimer();

    DBG("ClipSynchronizer: Stopped");
}

//==============================================================================
// Timer Callback (MESSAGE THREAD)
//==============================================================================

void ClipSynchronizer::timerCallback()
{
    // MESSAGE THREAD - Safe to access ValueTree and Engine tracks

    // Poll engine for new clips
    syncNewRecordedClipsFromEngine();
}

//==============================================================================
// ValueTree::Listener (MESSAGE THREAD)
//==============================================================================

void ClipSynchronizer::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    // Check if a CLIP node's position properties changed
    if (tree.hasType(ProjectState::ID_CLIP))
    {
        if (property == ProjectState::PROP_START || property == ProjectState::PROP_LENGTH)
        {
            // Clip position changed in ProjectState - sync to engine
            applyClipMoveToEngine(tree);
        }
    }
}

void ClipSynchronizer::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(parent);

    // If a CLIP node was added manually (not by us), ensure we track it
    if (child.hasType(ProjectState::ID_CLIP))
    {
        juce::String clipId = child[ProjectState::PROP_ID].toString();
        if (clipId.isNotEmpty())
        {
            knownClipIds.insert(clipId);
        }
    }

    // If tracks were added/removed, rebuild state
    if (child.hasType(ProjectState::ID_TRACK))
    {
        rebuildState();
    }
}

void ClipSynchronizer::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(parent, index);

    // If a CLIP node was removed, remove from known IDs
    if (child.hasType(ProjectState::ID_CLIP))
    {
        juce::String clipId = child[ProjectState::PROP_ID].toString();
        if (clipId.isNotEmpty())
        {
            knownClipIds.erase(clipId);
        }
    }

    // If tracks were added/removed, rebuild state
    if (child.hasType(ProjectState::ID_TRACK))
    {
        rebuildState();
    }
}

void ClipSynchronizer::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    juce::ignoreUnused(parent, oldIndex, newIndex);
}

void ClipSynchronizer::valueTreeParentChanged(juce::ValueTree& tree)
{
    juce::ignoreUnused(tree);
}

//==============================================================================
// Helper Methods
//==============================================================================

void ClipSynchronizer::syncNewRecordedClipsFromEngine()
{
    // MESSAGE THREAD - Safe to iterate engine tracks

    auto& tracks = engine.tracks();
    auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);

    if (!tracksNode.isValid())
        return;

    int trackIndex = 0;

    // Iterate all engine tracks
    for (auto& trackPtr : tracks)
    {
        if (!trackPtr)
        {
            ++trackIndex;
            continue;
        }

        auto* track = trackPtr.get();

        // Get corresponding ProjectState track
        if (trackIndex >= tracksNode.getNumChildren())
            break;

        auto trackNode = tracksNode.getChild(trackIndex);
        if (!trackNode.hasType(ProjectState::ID_TRACK))
        {
            ++trackIndex;
            continue;
        }

        juce::String trackId = trackNode[ProjectState::PROP_ID].toString();

        // Iterate all clips in this track
        int numClips = track->getNumClips();
        for (int clipIndex = 0; clipIndex < numClips; ++clipIndex)
        {
            auto* clip = track->getClip(clipIndex);
            if (!clip)
                continue;

            juce::String clipId = clip->getClipId();

            // If clip doesn't have an ID, generate one
            if (clipId.isEmpty())
            {
                clipId = projectState.generateUniqueId("clip");
                clip->setClipId(clipId);
                DBG("ClipSynchronizer: Assigned ID " + clipId + " to new clip");
            }

            // Check if this is a new clip we haven't seen before
            if (knownClipIds.find(clipId) == knownClipIds.end())
            {
                // New clip detected! Create ProjectState CLIP node

                // Get clip properties
                int64_t startSamples = clip->getStartPosition();
                int64_t lengthSamples = clip->getLength();

                // Convert to beats
                double startBeats = samplesToBeats(startSamples);
                double lengthBeats = samplesToBeats(lengthSamples);

                // Determine clip type
                juce::String clipType = (clip->getType() == zenith::Track::Clip::Type::Audio) ? "audio" : "midi";

                // Add to ProjectState
                if (projectState.addClip(trackId, clipId, clipType, startBeats, lengthBeats,
                                          startSamples, lengthSamples))
                {
                    knownClipIds.insert(clipId);
                    DBG("ClipSynchronizer: Created ProjectState node for clip " + clipId);
                }
            }
        }

        ++trackIndex;
    }
}

void ClipSynchronizer::applyClipMoveToEngine(const juce::ValueTree& clipNode)
{
    // MESSAGE THREAD - Safe to access engine

    if (!clipNode.isValid() || !clipNode.hasType(ProjectState::ID_CLIP))
        return;

    juce::String clipId = clipNode[ProjectState::PROP_ID].toString();
    if (clipId.isEmpty())
        return;

    // Get new position from ProjectState (in beats)
    double startBeats = clipNode[ProjectState::PROP_START];
    double lengthBeats = clipNode[ProjectState::PROP_LENGTH];

    // Convert to samples
    int64_t startSamples = beatsToSamples(startBeats);
    int64_t lengthSamples = beatsToSamples(lengthBeats);

    // Find the corresponding engine clip
    auto& tracks = engine.tracks();

    for (auto& trackPtr : tracks)
    {
        if (!trackPtr)
            continue;

        auto* track = trackPtr.get();
        int numClips = track->getNumClips();

        for (int clipIndex = 0; clipIndex < numClips; ++clipIndex)
        {
            auto* clip = track->getClip(clipIndex);
            if (!clip)
                continue;

            if (clip->getClipId() == clipId)
            {
                // Found it! Update position
                clip->setStartPosition(startSamples);
                clip->setLength(lengthSamples);

                DBG("ClipSynchronizer: Updated engine clip " + clipId +
                    " to position " + juce::String(startSamples) + " samples");
                return;
            }
        }
    }
}

double ClipSynchronizer::samplesToBeats(int64_t samples) const
{
    // Get tempo and sample rate
    double tempo = projectState.getTempo();
    double sampleRate = engine.getSampleRate();

    if (sampleRate <= 0.0)
        return 0.0;

    // beats = (samples / sampleRate) * (tempo / 60.0)
    double seconds = static_cast<double>(samples) / sampleRate;
    double beats = seconds * (tempo / 60.0);

    return beats;
}

int64_t ClipSynchronizer::beatsToSamples(double beats) const
{
    // Get tempo and sample rate
    double tempo = projectState.getTempo();
    double sampleRate = engine.getSampleRate();

    if (tempo <= 0.0)
        return 0;

    // samples = beats * (60.0 / tempo) * sampleRate
    double seconds = beats * (60.0 / tempo);
    int64_t samples = static_cast<int64_t>(seconds * sampleRate);

    return samples;
}

void ClipSynchronizer::rebuildState()
{
    // Clear known clips
    knownClipIds.clear();

    // Clear track mapping
    trackIdToIndex.clear();

    // Rebuild track mapping from ProjectState track IDs to Engine track indices
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

            // Also collect all existing clip IDs
            auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
            if (clipsNode.isValid())
            {
                for (auto clipNode : clipsNode)
                {
                    if (clipNode.hasType(ProjectState::ID_CLIP))
                    {
                        juce::String clipId = clipNode[ProjectState::PROP_ID].toString();
                        if (clipId.isNotEmpty())
                        {
                            knownClipIds.insert(clipId);
                        }
                    }
                }
            }

            ++index;
        }
    }

    DBG("ClipSynchronizer: Rebuilt state - " +
        juce::String(trackIdToIndex.size()) + " tracks, " +
        juce::String(knownClipIds.size()) + " known clips");
}
