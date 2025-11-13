/**
 * @file ProjectEditorState.cpp
 * @brief Implementation of ProjectEditorState
 */

#include <editor/ProjectEditorState.h>
#include <io/ProjectPersistence.h>
#include <render/OfflineRender.h>
#include <Engine.h>

namespace zenith
{

//==============================================================================
// Constructor
//==============================================================================

ProjectEditorState::ProjectEditorState(Engine& engine)
    : playback_(engine)
{
    // Start with empty project
    model_ = makeEmptyProject(48000.0, "Untitled");
}

//==============================================================================
// Project Management
//==============================================================================

void ProjectEditorState::setProject(const ProjectModel& project)
{
    // MESSAGE THREAD ONLY

    // Stop playback before replacing model
    stop();

    model_ = project;

    reloadPlayback();
}

void ProjectEditorState::reloadPlayback()
{
    // MESSAGE THREAD ONLY
    playback_.setProject(model_);
}

//==============================================================================
// Track Operations
//==============================================================================

int ProjectEditorState::getNumTracks() const
{
    return static_cast<int>(model_.tracks.size());
}

const TrackModel* ProjectEditorState::getTrack(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return nullptr;
    return &model_.tracks[trackIndex];
}

void ProjectEditorState::setTrackMuted(int trackIndex, bool muted)
{
    // MESSAGE THREAD ONLY

    // Validate track index
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return;

    // Update mute state
    model_.tracks[trackIndex].muted = muted;

    // Reload playback to apply changes
    reloadPlayback();
}

bool ProjectEditorState::isTrackMuted(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return false;
    return model_.tracks[trackIndex].muted;
}

void ProjectEditorState::setSelectedTrack(int trackIndex)
{
    // Clamp to valid range
    const int numTracks = static_cast<int>(model_.tracks.size());
    if (trackIndex < -1 || trackIndex >= numTracks)
        trackIndex = -1;

    selectedTrack_ = trackIndex;
}

int ProjectEditorState::addAudioTrack()
{
    // MESSAGE THREAD ONLY

    // Generate new track ID (max + 1)
    juce::int64 maxTrackId = -1;
    for (const auto& t : model_.tracks)
        maxTrackId = juce::jmax(maxTrackId, t.trackId);

    // Create new track
    TrackModel newTrack;
    newTrack.trackId = maxTrackId + 1;
    newTrack.name = juce::String("Track ") + juce::String(model_.tracks.size() + 1);
    newTrack.gain = 1.0f;
    newTrack.pan = 0.0f;
    newTrack.muted = false;

    // Add to project
    model_.tracks.push_back(newTrack);

    // Reload playback
    reloadPlayback();

    // Set as selected track
    const int newIndex = static_cast<int>(model_.tracks.size()) - 1;
    setSelectedTrack(newIndex);

    return newIndex;
}

void ProjectEditorState::removeTrack(int trackIndex)
{
    // MESSAGE THREAD ONLY

    // Validate track index
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return;

    // Remove track
    model_.tracks.erase(model_.tracks.begin() + trackIndex);

    // Adjust selected track
    const int numTracks = static_cast<int>(model_.tracks.size());
    if (numTracks == 0)
    {
        // No tracks left
        selectedTrack_ = -1;
    }
    else if (trackIndex >= numTracks)
    {
        // Deleted last track, select previous
        selectedTrack_ = numTracks - 1;
    }
    else
    {
        // Keep same index (next track shifted up)
        selectedTrack_ = trackIndex;
    }

    // Reload playback
    reloadPlayback();
}

//==============================================================================
// Clip Editing Operations
//==============================================================================

bool ProjectEditorState::deleteClip(int trackIndex, int clipIndex)
{
    // MESSAGE THREAD ONLY

    // Validate track index
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return false;

    auto& track = model_.tracks[trackIndex];

    // Validate clip index
    if (clipIndex < 0 || clipIndex >= static_cast<int>(track.clips.size()))
        return false;

    // Remove clip from model
    track.clips.erase(track.clips.begin() + clipIndex);

    // Reload playback to apply changes to engine
    reloadPlayback();

    return true;
}

bool ProjectEditorState::duplicateClip(int trackIndex, int clipIndex, juce::int64 offsetSamples)
{
    // MESSAGE THREAD ONLY

    // Validate track index
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return false;

    auto& track = model_.tracks[trackIndex];

    // Validate clip index
    if (clipIndex < 0 || clipIndex >= static_cast<int>(track.clips.size()))
        return false;

    // Get source clip
    const auto& srcClip = track.clips[clipIndex];

    // Create duplicate with new ID
    ClipModel dstClip = srcClip;

    // Generate new clip ID (simple: find max ID + 1)
    juce::int64 maxId = 0;
    for (const auto& t : model_.tracks)
    {
        for (const auto& c : t.clips)
            maxId = juce::jmax(maxId, c.id);
    }
    dstClip.id = maxId + 1;

    // Apply offset
    dstClip.startSample = srcClip.startSample + offsetSamples;

    // Clamp to non-negative
    dstClip.startSample = juce::jmax((juce::int64) 0, dstClip.startSample);

    // Add to track
    track.clips.push_back(dstClip);

    // Reload playback to apply changes to engine
    reloadPlayback();

    return true;
}

bool ProjectEditorState::insertClipFromFile(int trackIndex, juce::int64 startSample, const juce::File& audioFile)
{
    // MESSAGE THREAD ONLY

    // Validate audio file
    if (!audioFile.existsAsFile())
        return false;

    // Ensure track exists
    if (trackIndex < 0)
        return false;

    // If track doesn't exist and it's track 0, create it
    if (trackIndex >= static_cast<int>(model_.tracks.size()))
    {
        if (trackIndex == 0)
        {
            // Create track 0
            TrackModel newTrack;
            newTrack.id = 0;
            newTrack.name = "Track 1";
            model_.tracks.push_back(newTrack);
        }
        else
        {
            return false;  // Can't create tracks beyond track 0
        }
    }

    auto& track = model_.tracks[trackIndex];

    // Generate new clip ID
    juce::int64 maxId = 0;
    for (const auto& t : model_.tracks)
    {
        for (const auto& c : t.clips)
            maxId = juce::jmax(maxId, c.id);
    }

    // Create new clip
    ClipModel clip;
    clip.id = maxId + 1;
    clip.file = audioFile;
    clip.startSample = juce::jmax((juce::int64) 0, startSample);
    clip.lengthSamples = 0;  // 0 = use full file length
    clip.srcOffset = 0;
    clip.gain = 1.0f;
    clip.fadeInSamples = 0;
    clip.fadeOutSamples = 0;
    clip.muted = false;

    // Add to track
    track.clips.push_back(clip);

    // Reload playback to apply changes to engine
    reloadPlayback();

    return true;
}

bool ProjectEditorState::trimClipLeft(int trackIndex, int clipIndex, juce::int64 newStartSample)
{
    // MESSAGE THREAD ONLY

    // Validate track index
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return false;

    auto& track = model_.tracks[trackIndex];

    // Validate clip index
    if (clipIndex < 0 || clipIndex >= static_cast<int>(track.clips.size()))
        return false;

    auto& clip = track.clips[clipIndex];

    const auto oldStart = clip.startSample;
    const auto oldOffset = clip.srcOffset;
    auto oldLength = clip.lengthSamples;

    // For v0.1: disallow extending left beyond original start
    if (newStartSample <= oldStart)
        return true;  // No-op, but not an error

    // Compute delta
    const auto delta = newStartSample - oldStart;

    // Update start and source offset
    clip.startSample = oldStart + delta;
    clip.srcOffset = oldOffset + delta;

    // Length handling
    if (oldLength == 0)
    {
        // Keep as 0 (full length from new offset)
        // Playback will clamp to actual file length
    }
    else
    {
        // Reduce explicit length
        auto newLength = oldLength - delta;

        // Clamp to minimum
        if (newLength < kMinClipLengthSamples)
            newLength = kMinClipLengthSamples;

        clip.lengthSamples = newLength;
    }

    // Reload playback to apply changes to engine
    reloadPlayback();

    return true;
}

bool ProjectEditorState::trimClipRight(int trackIndex, int clipIndex, juce::int64 newEndSample)
{
    // MESSAGE THREAD ONLY

    // Validate track index
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
        return false;

    auto& track = model_.tracks[trackIndex];

    // Validate clip index
    if (clipIndex < 0 || clipIndex >= static_cast<int>(track.clips.size()))
        return false;

    auto& clip = track.clips[clipIndex];

    const auto start = clip.startSample;

    // Compute new length
    auto newLength = newEndSample - start;

    // Clamp to minimum
    if (newLength < kMinClipLengthSamples)
        newLength = kMinClipLengthSamples;

    // Set explicit length (no more "full file" mode after trim)
    clip.lengthSamples = newLength;

    // Reload playback to apply changes to engine
    reloadPlayback();

    return true;
}

//==============================================================================
// File I/O
//==============================================================================

const juce::File& ProjectEditorState::getCurrentProjectFile() const noexcept
{
    return currentProjectFile_;
}

bool ProjectEditorState::hasProjectFile() const noexcept
{
    return currentProjectFile_.existsAsFile();
}

juce::Result ProjectEditorState::saveToFile(const juce::File& file)
{
    // MESSAGE THREAD ONLY

    auto res = io::saveProjectToFile(model_, file);
    if (res.wasOk())
    {
        currentProjectFile_ = file;
        DBG("ProjectEditorState: Project saved to " + file.getFullPathName());
    }
    else
    {
        DBG("ProjectEditorState: Save failed - " + res.getErrorMessage());
    }

    return res;
}

juce::Result ProjectEditorState::save()
{
    // MESSAGE THREAD ONLY

    if (!currentProjectFile_.existsAsFile())
        return juce::Result::fail("No project file associated; use Save As first");

    return io::saveProjectToFile(model_, currentProjectFile_);
}

juce::Result ProjectEditorState::loadFromFile(const juce::File& file)
{
    // MESSAGE THREAD ONLY

    ProjectModel loaded;
    auto res = io::loadProjectFromFile(loaded, file);

    if (!res.wasOk())
    {
        DBG("ProjectEditorState: Load failed - " + res.getErrorMessage());
        return res;
    }

    // Stop playback before replacing
    stop();

    model_ = std::move(loaded);
    currentProjectFile_ = file;

    reloadPlayback();

    DBG("ProjectEditorState: Project loaded from " + file.getFullPathName());

    return res;
}

//==============================================================================
// Offline Export
//==============================================================================

juce::Result ProjectEditorState::renderCurrentProjectToWav(const juce::File& outputFile,
                                                           int blockSize,
                                                           double tailSeconds)
{
    // MESSAGE THREAD ONLY

    // Stop playback during export
    stop();

    // Use project's sample rate
    return renderProjectToWav(model_, outputFile, model_.sampleRate, blockSize, tailSeconds);
}

//==============================================================================
// Transport Controls
//==============================================================================

void ProjectEditorState::playFromStart()
{
    playback_.playFromStart();
}

void ProjectEditorState::playFromSamples(SamplePos startSample)
{
    playback_.playFromSamples(startSample);
}

void ProjectEditorState::stop()
{
    playback_.stop();
}

void ProjectEditorState::pause()
{
    playback_.pause();
}

ProjectEditorState::SamplePos ProjectEditorState::getTransportSamples() const
{
    return playback_.getTransportSamples();
}

bool ProjectEditorState::isPlaying() const
{
    return playback_.isPlaying();
}

void ProjectEditorState::setPlayheadSamples(SamplePos sample)
{
    // MESSAGE THREAD ONLY
    playback_.seekSamples(sample);
}

void ProjectEditorState::playFromPlayhead()
{
    // MESSAGE THREAD ONLY
    const SamplePos currentPos = getTransportSamples();
    playFromSamples(currentPos);
}

} // namespace zenith
