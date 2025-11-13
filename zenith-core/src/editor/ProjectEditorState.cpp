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
