/**
 * @file ArrangementPlaybackController.cpp
 * @brief Implementation of arrangement playback controller
 */

#include "../../include/playback/ArrangementPlaybackController.h"
#include "../../include/Engine.h"
#include "../../include/Track.h"

namespace zenith
{
    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    ArrangementPlaybackController::ArrangementPlaybackController(Engine& engine)
        : engine_(engine)
    {
        DBG("ArrangementPlaybackController: Constructor");
    }

    ArrangementPlaybackController::~ArrangementPlaybackController()
    {
        DBG("ArrangementPlaybackController: Destructor");
    }

    //==========================================================================
    // Project Management
    //==========================================================================

    void ArrangementPlaybackController::setProject(const ProjectModel& project)
    {
        // MESSAGE THREAD ONLY
        DBG("ArrangementPlaybackController: Loading project '" + project.name + "'");

        // Store project (make local copy)
        project_ = project;

        // Apply to engine
        applyProjectToEngine_();
    }

    void ArrangementPlaybackController::applyProjectToEngine_()
    {
        // MESSAGE THREAD ONLY

        // 1) Stop engine and reset transport
        DBG("ArrangementPlaybackController: Stopping engine and resetting transport");
        engine_.stop();
        engine_.seekSamples(0);

        // 2) Get project sample rate for time conversion
        const double sampleRate = project_.sampleRate;
        if (sampleRate <= 0.0)
        {
            DBG("ArrangementPlaybackController: Invalid sample rate " +
                juce::String(sampleRate) + ", aborting");
            return;
        }

        // 3) Resize engine tracks to match model
        const int numTracks = static_cast<int>(project_.tracks.size());
        DBG("ArrangementPlaybackController: Creating " + juce::String(numTracks) + " tracks");
        engine_.setNumTracks(numTracks);

        // 4) For each track in the model, rebuild clips in the engine
        for (int t = 0; t < numTracks; ++t)
        {
            const auto& trackModel = project_.tracks[static_cast<size_t>(t)];

            auto* track = engine_.getTrack(t);
            if (track == nullptr)
            {
                DBG("ArrangementPlaybackController: ERROR - Track " +
                    juce::String(t) + " is null!");
                continue;
            }

            // Set track name
            track->setName(trackModel.name);

            // Clear all existing clips for this track
            track->clearClips();

            // Apply track-level settings
            track->setVolume(trackModel.gain);
            track->setPan(trackModel.pan);
            track->setMute(trackModel.muted);

            DBG("ArrangementPlaybackController: Track " + juce::String(t) +
                " '" + trackModel.name + "' - " +
                juce::String(trackModel.clips.size()) + " clips");

            // Skip clip loading if track is muted (optional optimization)
            if (trackModel.muted)
            {
                DBG("  Track is muted, skipping clip loading");
                continue;
            }

            // Build clips from ClipModel entries
            for (const auto& clipModel : trackModel.clips)
            {
                // Skip muted clips
                if (clipModel.muted)
                {
                    DBG("  Clip '" + clipModel.name + "' is muted, skipping");
                    continue;
                }

                // Check if file exists
                const juce::File file = clipModel.file;
                if (!file.existsAsFile())
                {
                    DBG("  ERROR: Clip file does not exist: " +
                        file.getFullPathName());
                    continue;
                }

                // Convert sample positions to time (seconds)
                // NOTE: This assumes the audio file has the same sample rate
                // as the project. Sample rate conversion is TODO for later.
                const double startTime = static_cast<double>(clipModel.startSample) / sampleRate;

                // If lengthSamples is 0, Track::Clip will use the entire file
                // Otherwise, use the specified length
                const double length = (clipModel.lengthSamples > 0)
                    ? static_cast<double>(clipModel.lengthSamples) / sampleRate
                    : 0.0;  // 0.0 means "use entire file"

                DBG("  Loading clip '" + clipModel.name + "' at " +
                    juce::String(startTime, 2) + "s" +
                    " srcOffset=" + juce::String(clipModel.srcOffset) +
                    " gain=" + juce::String(clipModel.gain, 2) +
                    " fadeIn=" + juce::String(clipModel.fadeInSamples) +
                    " fadeOut=" + juce::String(clipModel.fadeOutSamples));

                // Create clip in track with all properties
                auto* clip = track->addClip(
                    file,
                    startTime,
                    length,
                    clipModel.srcOffset,        // srcOffsetSamples
                    clipModel.gain,             // clip gain
                    clipModel.fadeInSamples,    // fade in
                    clipModel.fadeOutSamples    // fade out
                );

                if (clip == nullptr)
                {
                    DBG("    ERROR: Failed to create clip!");
                    continue;
                }
            }
        }

        DBG("ArrangementPlaybackController: Project loaded successfully");
    }

    //==========================================================================
    // Transport Control
    //==========================================================================

    void ArrangementPlaybackController::playFromStart()
    {
        DBG("ArrangementPlaybackController: Play from start");
        engine_.stop();
        engine_.seekSamples(0);
        engine_.play();
    }

    void ArrangementPlaybackController::playFromSamples(SamplePos pos)
    {
        if (pos < 0)
            pos = 0;

        DBG("ArrangementPlaybackController: Play from sample " + juce::String(pos));
        engine_.stop();
        engine_.seekSamples(pos);
        engine_.play();
    }

    void ArrangementPlaybackController::pause()
    {
        DBG("ArrangementPlaybackController: Pause");
        engine_.pause();
    }

    void ArrangementPlaybackController::stop()
    {
        DBG("ArrangementPlaybackController: Stop");
        engine_.stop();
        engine_.seekSamples(0);
    }

    SamplePos ArrangementPlaybackController::getTransportSamples() const
    {
        return engine_.getTransportSamples();
    }
}
