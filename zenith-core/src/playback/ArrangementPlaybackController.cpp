/**
 * @file ArrangementPlaybackController.cpp
 * @brief Implementation of ArrangementPlaybackController
 */

#include <playback/ArrangementPlaybackController.h>
#include <Engine.h>
#include <AudioTrack.h>

namespace zenith
{

//==============================================================================
// Constructor / Destructor
//==============================================================================

ArrangementPlaybackController::ArrangementPlaybackController(Engine& engine)
    : engine_(engine)
{
    // Register standard audio formats (WAV, AIFF, FLAC, etc.)
    formatManager_.registerBasicFormats();
}

//==============================================================================
// Project Management
//==============================================================================

void ArrangementPlaybackController::setProject(const ProjectModel& project)
{
    // MESSAGE THREAD ONLY
    project_ = project;  // copy

    applyProjectToEngine_();
}

//==============================================================================
// Transport Controls
//==============================================================================

void ArrangementPlaybackController::playFromStart()
{
    // MESSAGE THREAD ONLY
    engine_.stop();
    engine_.seekSamples(0);
    engine_.play();
}

void ArrangementPlaybackController::playFromSamples(SamplePos startSample)
{
    // MESSAGE THREAD ONLY
    if (startSample < 0)
        startSample = 0;

    engine_.stop();
    engine_.seekSamples(startSample);
    engine_.play();

    // v0.1 note: This will work for clips that overlap the start position
    // because AudioTrack::process() checks if playhead is within clip bounds.
    // Mid-clip start offsets should work automatically via the existing
    // clip rendering logic (W13).
}

void ArrangementPlaybackController::stop()
{
    // MESSAGE THREAD ONLY
    engine_.stop();  // stops and resets to 0
}

void ArrangementPlaybackController::pause()
{
    // MESSAGE THREAD ONLY
    engine_.pause();  // stops but keeps position
}

ArrangementPlaybackController::SamplePos
ArrangementPlaybackController::getTransportSamples() const
{
    return engine_.getTransportSamples();
}

bool ArrangementPlaybackController::isPlaying() const
{
    return engine_.isPlaying();
}

void ArrangementPlaybackController::seekSamples(SamplePos sample)
{
    // MESSAGE THREAD ONLY
    if (sample < 0)
        sample = 0;

    engine_.seekSamples(sample);
}

//==============================================================================
// Core Logic: Project → Engine Conversion
//==============================================================================

void ArrangementPlaybackController::applyProjectToEngine_()
{
    // MESSAGE THREAD ONLY

    DBG("ArrangementPlaybackController: Applying project to engine...");
    DBG("  Project: " + project_.name);
    DBG("  Sample Rate: " + juce::String(project_.sampleRate));
    DBG("  Tracks: " + juce::String(project_.tracks.size()));

    // 1) Stop transport & reset
    engine_.stop();
    engine_.seekSamples(0);

    // 2) Clear old clip audio data
    clipAudioData_.clear();

    // 3) Configure number of tracks
    const int numTracks = static_cast<int>(project_.tracks.size());
    engine_.setNumTracks(numTracks);

    // 4) For each track in the model:
    for (int t = 0; t < numTracks; ++t)
    {
        const auto& trackModel = project_.tracks[static_cast<size_t>(t)];

        auto* track = engine_.getTrack(t);
        if (track == nullptr)
        {
            DBG("ArrangementPlaybackController: ERROR - Track " + juce::String(t) + " is null!");
            jassertfalse;
            continue;
        }

        DBG("  Track " + juce::String(t) + ": " + trackModel.name +
            " (" + juce::String(trackModel.clips.size()) + " clips)");

        // Clear any existing clips (message thread only API)
        track->clearClips();

        // 5) For each clip on this track:
        for (const auto& clipModel : trackModel.clips)
        {
            if (clipModel.muted)
            {
                DBG("    Clip " + juce::String(clipModel.id) + ": MUTED (skipped)");
                continue;  // v0.1: mute = "don't even add"
            }

            // Resolve file path
            const juce::File file = clipModel.file;

            if (!file.existsAsFile())
            {
                DBG("    Clip " + juce::String(clipModel.id) + ": MISSING FILE - " + file.getFullPathName());
                continue;  // v0.1: skip missing files
            }

            DBG("    Clip " + juce::String(clipModel.id) + ": " + file.getFileName());

            // Decode entire file into a buffer
            auto clipBuffer = std::make_unique<juce::AudioBuffer<float>>();

            if (!loadAudioFile_(file, *clipBuffer))
            {
                DBG("      Failed to load audio file!");
                continue;
            }

            DBG("      Loaded: " + juce::String(clipBuffer->getNumChannels()) + " ch, " +
                juce::String(clipBuffer->getNumSamples()) + " samples");

            // Determine clip length
            // If model specifies length > 0, use it; otherwise use full buffer
            const juce::int64 clipLength = clipModel.lengthSamples > 0
                                         ? clipModel.lengthSamples
                                         : clipBuffer->getNumSamples();

            // Create AudioTrack::AudioClip
            AudioTrack::AudioClip clip;
            clip.id = juce::String(clipModel.id);
            clip.startPosition = clipModel.startSample;
            clip.length = clipLength;
            clip.gain = clipModel.gain;
            clip.fadeInSamples = clipModel.fadeInSamples;
            clip.fadeOutSamples = clipModel.fadeOutSamples;

            // Store decoded audio in our map (we own this memory)
            const juce::int64 clipId = clipModel.id;
            clipAudioData_[clipId] = std::move(clipBuffer);

            // Copy audio data to clip's buffer
            // AudioTrack expects clip.audioData to own the audio
            clip.audioData = *clipAudioData_[clipId];

            // Add to track (message thread only)
            track->addClip(clip);

            DBG("      Added to track: start=" + juce::String(clip.startPosition) +
                ", len=" + juce::String(clip.length) +
                ", gain=" + juce::String(clip.gain));
        }
    }

    DBG("ArrangementPlaybackController: Project applied successfully!");
}

//==============================================================================
// Audio File Loading
//==============================================================================

bool ArrangementPlaybackController::loadAudioFile_(const juce::File& file,
                                                    juce::AudioBuffer<float>& outBuffer)
{
    // MESSAGE THREAD ONLY

    if (!file.existsAsFile())
        return false;

    // Create reader for the file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));

    if (reader == nullptr)
    {
        DBG("ArrangementPlaybackController: Failed to create reader for " + file.getFullPathName());
        return false;
    }

    // Get file info
    const int numChannels = static_cast<int>(reader->numChannels);
    const juce::int64 numSamples = reader->lengthInSamples;
    const double fileSampleRate = reader->sampleRate;

    DBG("      File info: " + juce::String(numChannels) + " ch, " +
        juce::String(numSamples) + " samples @ " +
        juce::String(fileSampleRate) + " Hz");

    // v0.1: Simple approach - load entire file at native sample rate
    // TODO: Handle sample rate conversion if file SR != project SR
    // For now, just warn if they don't match
    if (std::abs(fileSampleRate - project_.sampleRate) > 0.1)
    {
        DBG("      WARNING: File sample rate (" + juce::String(fileSampleRate) +
            ") doesn't match project (" + juce::String(project_.sampleRate) + ")");
        DBG("      Playback pitch will be incorrect!");
        // v0.1: We still load it, but playback will be pitch-shifted
        // TODO: Add sample rate conversion in future
    }

    // Allocate buffer for entire file
    outBuffer.setSize(numChannels, static_cast<int>(numSamples));

    // Read entire file into buffer
    if (!reader->read(&outBuffer, 0, static_cast<int>(numSamples), 0, true, true))
    {
        DBG("      ERROR: Failed to read audio data from file!");
        return false;
    }

    return true;
}

} // namespace zenith
