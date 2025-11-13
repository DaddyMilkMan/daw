/**
 * @file ProjectEditorState.cpp
 * @brief ProjectEditorState implementation
 */

#include "../../include/editor/ProjectEditorState.h"
#include "../../include/Engine.h"

namespace zenith {

ProjectEditorState::ProjectEditorState(Engine& engine)
    : playback_(engine)
{
}

ProjectEditorState::~ProjectEditorState()
{
    stop();
}

//==============================================================================
// Project Model Access
//==============================================================================

void ProjectEditorState::setProject(const ProjectModel& newProject)
{
    // Stop playback if running
    if (isPlaying())
        stop();

    // Replace model
    model_ = newProject;

    // Reset playhead
    playhead_ = 0;

    // Reload playback context (sync model → engine)
    reloadPlayback_();

    DBG("Project set: " << model_.name << " (" << model_.tracks.size() << " tracks)");
}

//==============================================================================
// Playhead Control
//==============================================================================

void ProjectEditorState::setPlayheadSamples(SamplePos pos) noexcept
{
    playhead_ = juce::jmax(SamplePos(0), pos);
}

//==============================================================================
// Transport Control
//==============================================================================

void ProjectEditorState::play()
{
    // v0.1: Reload playback from model on every play
    // (ensures UI edits are synced to engine)
    reloadPlayback_();

    // Start playback from current playhead
    playback_.playFrom(playhead_);

    DBG("Playback started from sample " << playhead_);
}

void ProjectEditorState::stop()
{
    playback_.stop();
    DBG("Playback stopped");
}

void ProjectEditorState::playFromCursor()
{
    play(); // v0.1: play() already handles reload + playFrom
}

bool ProjectEditorState::isPlaying() const
{
    return playback_.isPlaying();
}

//==============================================================================
// Editing Operations
//==============================================================================

void ProjectEditorState::moveClipStart(int trackIndex, int clipIndex, SamplePos newStart)
{
    // v0.1: Only allow editing while NOT playing
    jassert(!isPlaying());
    if (isPlaying())
    {
        DBG("WARNING: Cannot edit clips while playing (v0.1 limitation)");
        return;
    }

    // Validate indices
    if (trackIndex < 0 || trackIndex >= static_cast<int>(model_.tracks.size()))
    {
        DBG("ERROR: Invalid track index: " << trackIndex);
        return;
    }

    auto& track = model_.tracks[static_cast<size_t>(trackIndex)];
    if (clipIndex < 0 || clipIndex >= static_cast<int>(track.clips.size()))
    {
        DBG("ERROR: Invalid clip index: " << clipIndex);
        return;
    }

    // Clamp to valid range
    newStart = juce::jmax(SamplePos(0), newStart);

    // Update model
    auto& clip = track.clips[static_cast<size_t>(clipIndex)];
    clip.startSample = newStart;

    DBG("Moved clip " << clip.id << " to sample " << newStart);
}

//==============================================================================
// Internal Helpers
//==============================================================================

void ProjectEditorState::reloadPlayback_()
{
    // Clear existing playback context
    playback_.clear();

    // Load from current model
    playback_.loadFromModel(model_, projectRoot_);

    DBG("Playback context reloaded");
}

} // namespace zenith
