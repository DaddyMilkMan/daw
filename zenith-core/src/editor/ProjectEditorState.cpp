/**
 * @file ProjectEditorState.cpp
 * @brief ProjectEditorState implementation
 */

#include "../../include/editor/ProjectEditorState.h"
#include "../../include/Engine.h"
#include "../../include/io/ProjectPersistence.h"
#include "../../include/render/OfflineRender.h"

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

    // Mark project as dirty
    isDirty_ = true;

    DBG("Moved clip " << clip.id << " to sample " << newStart);
}

//==============================================================================
// Project File Management
//==============================================================================

void ProjectEditorState::newProject(double sampleRate, const juce::String& name)
{
    // Stop playback if running
    if (isPlaying())
        stop();

    // Create empty project
    model_ = ProjectModel{};
    model_.name = name.toStdString();
    model_.sampleRate = sampleRate;
    model_.blockSize = 512;
    model_.nextClipId = 1;

    // Clear file tracking
    currentProjectFile_ = juce::File{};
    projectRoot_ = juce::File{};
    playhead_ = 0;

    // Mark as unsaved (dirty)
    isDirty_ = true;

    // Reload playback context
    reloadPlayback_();

    DBG("New project created: " << name << " @ " << sampleRate << " Hz");
}

bool ProjectEditorState::openProjectFromFile(const juce::File& file, juce::String* outError)
{
    // Load from disk
    ProjectModel loaded;
    if (!loadProjectFromFile(file, loaded, outError))
        return false;

    // Stop playback if running
    if (isPlaying())
        stop();

    // Replace model
    model_ = std::move(loaded);

    // Update file tracking
    currentProjectFile_ = file;
    projectRoot_ = file.getParentDirectory(); // v0.1: Use file directory as project root
    playhead_ = 0;

    // Mark as clean (just loaded)
    isDirty_ = false;

    // Reload playback context (sync model → engine)
    reloadPlayback_();

    DBG("Project opened: " << file.getFullPathName());
    return true;
}

bool ProjectEditorState::saveProjectToFile(const juce::File& file, juce::String* outError)
{
    // Save to disk
    if (!zenith::saveProjectToFile(model_, file, outError))
        return false;

    // Update file tracking
    currentProjectFile_ = file;
    projectRoot_ = file.getParentDirectory(); // v0.1: Use file directory as project root

    // Mark as clean (just saved)
    isDirty_ = false;

    DBG("Project saved: " << file.getFullPathName());
    return true;
}

bool ProjectEditorState::saveIfHasFile(juce::String* outError)
{
    // Check if we have a file to save to
    if (!currentProjectFile_.existsAsFile() && currentProjectFile_.getFullPathName().isEmpty())
    {
        if (outError)
            *outError = "No file path set (use Save As)";
        return false;
    }

    // Save to current file
    return saveProjectToFile(currentProjectFile_, outError);
}

//==============================================================================
// Offline Export
//==============================================================================

juce::Result ProjectEditorState::renderCurrentProjectToFile(const juce::File& outputFile,
                                                            const zenith::ExportOptions& options)
{
    // v0.1: Stop playback during export to avoid confusion
    if (isPlaying())
        stop();

    // Render via offline renderer
    return renderProjectToFile(model_,
                               outputFile,
                               model_.sampleRate,
                               options);
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
