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

#include "ClipStateManager.h"
#include "ProjectState.h"
#include "ZenithLogger.h"

namespace zenith {

//==============================================================================
ClipStateManager::ClipStateManager(ProjectState& projectState)
    : projectState_(projectState)
{
}

juce::String ClipStateManager::addClip(const juce::String& trackId,
                                        const juce::String& clipType,
                                        double startBeats, double lengthBeats,
                                        int laneIndex)
{
    auto clipsNode = getClipsContainer(trackId);
    if (!clipsNode.isValid()) {
        ZENITH_LOG_ERROR("ClipStateManager: Failed to add clip - Track not found: " + trackId);
        return {};
    }

    juce::String clipId = generateClipId();

    // Create clip node
    juce::ValueTree clip(ProjectState::ID_CLIP);
    clip.setProperty(ProjectState::PROP_ID, clipId, nullptr);
    clip.setProperty(ProjectState::PROP_TYPE, clipType, nullptr);
    clip.setProperty(ProjectState::PROP_START_BEATS, startBeats, nullptr);
    clip.setProperty(ProjectState::PROP_LENGTH_BEATS, lengthBeats, nullptr);
    clip.setProperty(ProjectState::PROP_MUTE, false, nullptr);
    clip.setProperty("lane", laneIndex, nullptr);

    // Add notes container for MIDI clips
    if (clipType == "midi")
    {
        juce::ValueTree notesNode(ProjectState::ID_NOTES);
        clip.addChild(notesNode, -1, nullptr);
    }

    // Add with undo
    clipsNode.addChild(clip, -1, &projectState_.getUndoManager());

    ZENITH_LOG_INFO("ClipStateManager: Added " + clipType + " clip (ID: " + clipId + 
        ") at " + juce::String(startBeats) + " beats");

    return clipId;
}

juce::String ClipStateManager::createEmptyClip(const juce::String& trackId,
                                                double startBeats, double lengthBeats,
                                                bool isMidi, const juce::String& name,
                                                const juce::String& actionName)
{
    auto clipsNode = getClipsContainer(trackId);
    if (!clipsNode.isValid()) {
         ZENITH_LOG_ERROR("ClipStateManager: Failed to create empty clip - Track not found: " + trackId);
        return {};
    }

    projectState_.getUndoManager().beginNewTransaction(actionName);

    juce::String clipId = generateClipId();

    juce::ValueTree clip(ProjectState::ID_CLIP);
    clip.setProperty(ProjectState::PROP_ID, clipId, nullptr);
    clip.setProperty(ProjectState::PROP_TYPE, isMidi ? "midi" : "audio", nullptr);
    clip.setProperty(ProjectState::PROP_NAME, name, nullptr);
    clip.setProperty(ProjectState::PROP_START_BEATS, startBeats, nullptr);
    clip.setProperty(ProjectState::PROP_LENGTH_BEATS, lengthBeats, nullptr);
    clip.setProperty(ProjectState::PROP_MUTE, false, nullptr);

    if (isMidi)
    {
        juce::ValueTree notesNode(ProjectState::ID_NOTES);
        clip.addChild(notesNode, -1, nullptr);
    }

    clipsNode.addChild(clip, -1, &projectState_.getUndoManager());
    
    ZENITH_LOG_INFO("ClipStateManager: Created empty clip (ID: " + clipId + ")");

    return clipId;
}

bool ClipStateManager::removeClip(const juce::String& trackId, const juce::String& clipId,
                                   const juce::String& actionName)
{
    auto clipsNode = getClipsContainer(trackId);
    if (!clipsNode.isValid()) {
        ZENITH_LOG_ERROR("ClipStateManager: Failed to remove clip - Track not found: " + trackId);
        return false;
    }

    for (int i = 0; i < clipsNode.getNumChildren(); ++i)
    {
        auto clip = clipsNode.getChild(i);
        if (clip[ProjectState::PROP_ID].toString() == clipId)
        {
            projectState_.getUndoManager().beginNewTransaction(actionName);
            clipsNode.removeChild(i, &projectState_.getUndoManager());
            ZENITH_LOG_INFO("ClipStateManager: Removed clip " + clipId);
            return true;
        }
    }
    
    ZENITH_LOG_WARNING("ClipStateManager: Failed to remove clip - Clip ID not found: " + clipId);
    return false;
}

void ClipStateManager::deleteClip(const juce::String& clipId, const juce::String& actionName)
{
    auto [trackTree, clipTree] = findClip(clipId);
    if (!clipTree.isValid())
        return;

    auto clipsNode = clipTree.getParent();
    if (clipsNode.isValid())
    {
        projectState_.getUndoManager().beginNewTransaction(actionName);
        clipsNode.removeChild(clipTree, &projectState_.getUndoManager());
    }
}

//==============================================================================
// Clip Queries
//==============================================================================

juce::ValueTree ClipStateManager::getClip(const juce::String& trackId, 
                                           const juce::String& clipId) const
{
    auto clipsNode = getClipsContainer(trackId);
    if (!clipsNode.isValid())
        return {};

    for (auto clip : clipsNode)
    {
        if (clip[ProjectState::PROP_ID].toString() == clipId)
            return clip;
    }
    return {};
}

std::pair<juce::ValueTree, juce::ValueTree> ClipStateManager::findClip(const juce::String& clipId) const
{
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return {{}, {}};

    for (auto track : tracksNode)
    {
        auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
        if (!clipsNode.isValid())
            continue;

        for (auto clip : clipsNode)
        {
            if (clip[ProjectState::PROP_ID].toString() == clipId)
                return {track, clip};
        }
    }
    return {{}, {}};
}

juce::String ClipStateManager::getClipAudioFile(const juce::String& trackId,
                                                  const juce::String& clipId) const
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
        return {};

    return clip[ProjectState::PROP_AUDIO_FILE].toString();
}

//==============================================================================
// Clip Modification
//==============================================================================

bool ClipStateManager::moveClip(const juce::String& trackId, const juce::String& clipId,
                                 double newStartBeats, const juce::String& actionName)
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    clip.setProperty(ProjectState::PROP_START_BEATS, newStartBeats, &projectState_.getUndoManager());
    return true;
}

void ClipStateManager::moveClipToTrack(const juce::String& clipId,
                                        const juce::String& newTrackId,
                                        double newStartBeats,
                                        const juce::String& actionName)
{
    auto [oldTrack, clip] = findClip(clipId);
    if (!clip.isValid())
        return;

    auto newClipsNode = getClipsContainer(newTrackId);
    if (!newClipsNode.isValid())
        return;

    projectState_.getUndoManager().beginNewTransaction(actionName);

    // Copy the clip
    juce::ValueTree clipCopy = clip.createCopy();
    clipCopy.setProperty(ProjectState::PROP_START_BEATS, newStartBeats, nullptr);

    // Remove from old location
    auto oldClipsNode = clip.getParent();
    oldClipsNode.removeChild(clip, &projectState_.getUndoManager());

    // Add to new location
    newClipsNode.addChild(clipCopy, -1, &projectState_.getUndoManager());
}

bool ClipStateManager::resizeClip(const juce::String& trackId, const juce::String& clipId,
                                   double newLengthBeats, const juce::String& actionName)
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    clip.setProperty(ProjectState::PROP_LENGTH_BEATS, newLengthBeats, &projectState_.getUndoManager());
    return true;
}

void ClipStateManager::setClipRange(const juce::String& clipId,
                                     double newStartBeats, double newLengthBeats,
                                     const juce::String& actionName)
{
    auto [track, clip] = findClip(clipId);
    if (!clip.isValid())
        return;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    clip.setProperty(ProjectState::PROP_START_BEATS, newStartBeats, &projectState_.getUndoManager());
    clip.setProperty(ProjectState::PROP_LENGTH_BEATS, newLengthBeats, &projectState_.getUndoManager());
}

bool ClipStateManager::setClipAudioFile(const juce::String& trackId, const juce::String& clipId,
                                         const juce::File& audioFile, const juce::String& actionName)
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
        return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    clip.setProperty(ProjectState::PROP_AUDIO_FILE, audioFile.getFullPathName(), 
                     &projectState_.getUndoManager());
    clip.setProperty(ProjectState::PROP_TYPE, "audio", &projectState_.getUndoManager());
    return true;
}

//==============================================================================
// Clip Editing
//==============================================================================

std::pair<juce::String, juce::String> ClipStateManager::splitClip(
    const juce::String& trackId, const juce::String& clipId,
    double splitBeats, const juce::String& actionName)
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
        return {{}, {}};

    double startBeats = clip.getProperty(ProjectState::PROP_START_BEATS);
    double lengthBeats = clip.getProperty(ProjectState::PROP_LENGTH_BEATS);
    double endBeats = startBeats + lengthBeats;

    // Validate split position
    if (splitBeats <= startBeats || splitBeats >= endBeats)
        return {{}, {}};

    projectState_.getUndoManager().beginNewTransaction(actionName);

    // Calculate new lengths
    double leftLength = splitBeats - startBeats;
    double rightLength = endBeats - splitBeats;

    // Resize original clip (becomes left part)
    clip.setProperty(ProjectState::PROP_LENGTH_BEATS, leftLength, &projectState_.getUndoManager());

    // Create right part
    juce::String clipType = clip[ProjectState::PROP_TYPE].toString();
    juce::String rightId = addClip(trackId, clipType, splitBeats, rightLength, 0);

    // Copy audio file reference if exists
    juce::String audioFile = clip[ProjectState::PROP_AUDIO_FILE].toString();
    if (audioFile.isNotEmpty())
    {
        auto rightClip = getClip(trackId, rightId);
        if (rightClip.isValid())
        {
            rightClip.setProperty(ProjectState::PROP_AUDIO_FILE, audioFile, 
                                  &projectState_.getUndoManager());
            // Set offset for the right clip
            double currentOffset = clip.getProperty(ProjectState::PROP_OFFSET, 0.0);
            rightClip.setProperty(ProjectState::PROP_OFFSET, currentOffset + leftLength,
                                  &projectState_.getUndoManager());
        }
    }

    return {clipId, rightId};
}

juce::String ClipStateManager::duplicateClip(const juce::String& trackId, const juce::String& clipId,
                                              double offsetBeats, const juce::String& actionName)
{
    auto clip = getClip(trackId, clipId);
    if (!clip.isValid())
        return {};

    auto clipsNode = getClipsContainer(trackId);
    if (!clipsNode.isValid())
        return {};

    projectState_.getUndoManager().beginNewTransaction(actionName);

    // Deep copy
    juce::ValueTree copy = clip.createCopy();

    // New ID
    juce::String newId = generateClipId();
    copy.setProperty(ProjectState::PROP_ID, newId, nullptr);

    // Adjust position
    double start = copy.getProperty(ProjectState::PROP_START_BEATS);
    double length = copy.getProperty(ProjectState::PROP_LENGTH_BEATS);
    double newStart = (offsetBeats != 0.0) ? start + offsetBeats : start + length;
    copy.setProperty(ProjectState::PROP_START_BEATS, newStart, nullptr);

    // Add copy
    clipsNode.addChild(copy, -1, &projectState_.getUndoManager());

    return newId;
}

//==============================================================================
// Internal Helpers
//==============================================================================

juce::ValueTree ClipStateManager::getClipsContainer(const juce::String& trackId) const
{
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    if (!tracksNode.isValid())
        return {};

    for (auto track : tracksNode)
    {
        if (track[ProjectState::PROP_ID].toString() == trackId)
            return track.getChildWithName(ProjectState::ID_CLIPS);
    }
    return {};
}

juce::String ClipStateManager::generateClipId() const
{
    return "clip_" + juce::Uuid().toString().substring(0, 8);
}

} // namespace zenith
