/**
 * @file ProjectPlaybackContext.cpp
 * @brief ProjectPlaybackContext implementation
 */

#include "../../include/model/ProjectPlaybackContext.h"
#include "../../include/Engine.h"
#include "../../Source/engine/Track.h"
#include "../../Source/engine/Clip.h"
#include "../../Source/engine/nodes/GainPanNode.h"

namespace zenith {

ProjectPlaybackContext::ProjectPlaybackContext(Engine& engine)
    : engine_(engine)
{
}

ProjectPlaybackContext::~ProjectPlaybackContext()
{
    clear();
}

//==============================================================================
// Project Loading (MESSAGE THREAD)
//==============================================================================

void ProjectPlaybackContext::loadFromModel(const ProjectModel& model,
                                           const juce::File& projectRoot)
{
    // CRITICAL: Must be called when playback is stopped
    jassert(!engine_.isPlaying());
    if (engine_.isPlaying())
    {
        DBG("ERROR: Cannot load project while playback is active");
        return;
    }

    // Clear any existing arrangement
    clear();

    // Store model and project root
    model_ = model;
    projectRoot_ = projectRoot;

#if ZENITH_ENABLE_PHASE1_AUDIO
    auto& mixer = engine_.getMixer();

    // Create tracks
    mixer.setNumTracks(static_cast<int>(model.tracks.size()));

    // Load each track
    for (size_t trackIdx = 0; trackIdx < model.tracks.size(); ++trackIdx)
    {
        const auto& trackModel = model.tracks[trackIdx];
        auto* track = mixer.getTrack(static_cast<int>(trackIdx));
        if (!track)
        {
            DBG("ERROR: Failed to get track " << trackIdx);
            continue;
        }

        // Configure track FX: Gain/Pan in slot 0 (v0.1 only)
        auto gainPanNode = std::make_unique<GainPanNode>();
        gainPanNode->setGain(trackModel.gain);
        gainPanNode->setPan(trackModel.pan);
        track->setFxNode(0, std::move(gainPanNode));

        // Load all clips for this track
        for (const auto& clipModel : trackModel.clips)
        {
            // Skip muted clips (don't load into track at all for v0.1)
            // v0.2+: Load but mark as inactive
            if (clipModel.muted)
                continue;

            // Skip clips on muted tracks (don't load for v0.1)
            // v0.2+: Load but track mute will prevent scheduling
            if (trackModel.muted)
                continue;

            bool loaded = loadClipIntoTrack(static_cast<int>(trackIdx),
                                            clipModel,
                                            projectRoot);
            if (!loaded)
            {
                DBG("WARNING: Failed to load clip " << clipModel.filePath
                    << " on track " << trackIdx);
            }
        }
    }

    isLoaded_ = true;
    DBG("Project loaded: " << model.name << " (" << model.tracks.size() << " tracks)");
#else
    juce::ignoreUnused(model, projectRoot);
    DBG("ERROR: Phase 1 audio not enabled (ZENITH_ENABLE_PHASE1_AUDIO=OFF)");
#endif
}

void ProjectPlaybackContext::clear()
{
    // Stop playback if running
    if (engine_.isPlaying())
        engine_.stop();

#if ZENITH_ENABLE_PHASE1_AUDIO
    // Clear all tracks and clips
    auto& mixer = engine_.getMixer();
    mixer.clearTracks();
#endif

    // Clear model
    model_ = ProjectModel{};
    projectRoot_ = juce::File{};
    isLoaded_ = false;

    DBG("Project cleared");
}

//==============================================================================
// Transport Control (MESSAGE THREAD)
//==============================================================================

void ProjectPlaybackContext::playFrom(SamplePos startSample)
{
    if (!isLoaded_)
    {
        DBG("ERROR: No project loaded, cannot play");
        return;
    }

    // Seek to start position
    seek(startSample);

    // Schedule all active clips from this position
    scheduleEventsFrom(startSample);

    // Start playback
    engine_.play();

    DBG("Playback started from sample " << startSample);
}

void ProjectPlaybackContext::stop()
{
    engine_.stop();
    DBG("Playback stopped");
}

void ProjectPlaybackContext::seek(SamplePos targetSample)
{
    // CRITICAL: Must stop playback before seeking
    // (seekSamples violates SPSC contract if audio thread is consuming events)
    const bool wasPlaying = engine_.isPlaying();
    if (wasPlaying)
        engine_.stop();

#if ZENITH_ENABLE_PHASE1_AUDIO
    // Update transport position
    engine_.seekSamples(targetSample);

    // If we were playing, reschedule events from new position and resume
    if (wasPlaying)
    {
        scheduleEventsFrom(targetSample);
        engine_.play();
    }
#else
    juce::ignoreUnused(targetSample);
#endif

    DBG("Seeked to sample " << targetSample);
}

//==============================================================================
// State Query (MESSAGE THREAD)
//==============================================================================

SamplePos ProjectPlaybackContext::getTransportSamples() const
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    return engine_.getTransportSamples();
#else
    return 0;
#endif
}

bool ProjectPlaybackContext::isPlaying() const
{
    return engine_.isPlaying();
}

//==============================================================================
// Internal Helpers
//==============================================================================

void ProjectPlaybackContext::scheduleEventsFrom(SamplePos startSample)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    if (!isLoaded_)
        return;

    int numScheduled = 0;
    int numSkipped = 0;

    // Iterate all tracks
    for (size_t trackIdx = 0; trackIdx < model_.tracks.size(); ++trackIdx)
    {
        const auto& trackModel = model_.tracks[trackIdx];

        // Skip muted tracks (v0.1: clips not loaded, so nothing to schedule)
        if (trackModel.muted)
            continue;

        // Iterate all clips on track
        for (const auto& clipModel : trackModel.clips)
        {
            // Skip muted clips (v0.1: not loaded into track)
            if (clipModel.muted)
                continue;

            const SamplePos clipStart = clipModel.startSample;
            const SamplePos clipEnd = clipStart + clipModel.lengthSamples;

            // v0.1 limitation: No mid-clip offset support
            // Only schedule clips that haven't started yet
            if (clipStart < startSample)
            {
                // Clip already started - skip for v0.1
                // v0.2+: Calculate mid-clip offset and schedule with offset
                numSkipped++;
                continue;
            }

            // Schedule ClipStart event
            bool scheduled = engine_.scheduleClipStart(
                static_cast<int>(trackIdx),
                clipModel.id,
                clipStart
            );

            if (!scheduled)
            {
                DBG("WARNING: Failed to schedule clip start (queue full?)");
                continue;
            }

            // Schedule ClipStop event at clip end
            // (Optional for one-shot clips, but allows clean fade-out)
            engine_.scheduleClipStop(
                static_cast<int>(trackIdx),
                clipModel.id,
                clipEnd
            );

            numScheduled++;
        }
    }

    DBG("Scheduled " << numScheduled << " clips from sample " << startSample
        << " (skipped " << numSkipped << " already-started clips)");
#else
    juce::ignoreUnused(startSample);
#endif
}

bool ProjectPlaybackContext::loadClipIntoTrack(int trackIndex,
                                               const ClipModel& clipModel,
                                               const juce::File& projectRoot)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    // Resolve file path (project-relative)
    juce::File clipFile = projectRoot.getChildFile(juce::String(clipModel.filePath));
    if (!clipFile.existsAsFile())
    {
        DBG("ERROR: Clip file not found: " << clipFile.getFullPathName());
        return false;
    }

    // Load audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats(); // WAV, AIFF, FLAC, MP3, OGG

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(clipFile)
    );

    if (!reader)
    {
        DBG("ERROR: Failed to create audio reader for: " << clipFile.getFullPathName());
        return false;
    }

    // Decode PCM into buffer
    const int numChannels = static_cast<int>(reader->numChannels);
    const int64_t numSamples = reader->lengthInSamples;

    auto pcmBuffer = std::make_shared<juce::AudioBuffer<float>>(
        numChannels,
        static_cast<int>(numSamples)
    );

    if (!reader->read(pcmBuffer.get(), 0, static_cast<int>(numSamples), 0, true, true))
    {
        DBG("ERROR: Failed to read audio samples from: " << clipFile.getFullPathName());
        return false;
    }

    // Create Clip with metadata
    Clip clip;
    clip.pcm = pcmBuffer;
    clip.startSample = clipModel.startSample;
    clip.lengthSamples = clipModel.lengthSamples;
    clip.srcOffset = clipModel.srcOffset;
    clip.gain = clipModel.gain;
    clip.fadeInSamples = clipModel.fadeInSamples;
    clip.fadeOutSamples = clipModel.fadeOutSamples;
    clip.loop = clipModel.loopEnabled;

    // Validate clip
    if (!clip.isValid())
    {
        DBG("ERROR: Invalid clip after loading: " << clipFile.getFullPathName());
        return false;
    }

    // Create ClipDef with unique ID
    ClipDef clipDef(clipModel.id, clip);

    // Add to track
    auto& mixer = engine_.getMixer();
    auto* track = mixer.getTrack(trackIndex);
    if (!track)
    {
        DBG("ERROR: Invalid track index: " << trackIndex);
        return false;
    }

    track->addClipDefinition(clipDef);

    DBG("Loaded clip " << clipModel.id << ": " << clipFile.getFileName()
        << " (" << numSamples << " samples, " << numChannels << " ch)");

    return true;
#else
    juce::ignoreUnused(trackIndex, clipModel, projectRoot);
    return false;
#endif
}

} // namespace zenith
