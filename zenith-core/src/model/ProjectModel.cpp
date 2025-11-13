/**
 * @file ProjectModel.cpp
 * @brief Implementation of project model conversion functions
 */

#include "../../include/model/ProjectModel.h"
#include "../../include/model/ProjectIDs.h"

namespace zenith
{
    //==========================================================================
    // Identifier definitions
    //==========================================================================

    namespace ids
    {
        // Node types
        const juce::Identifier project       { "Project" };
        const juce::Identifier track         { "Track" };
        const juce::Identifier audioClip     { "AudioClip" };

        // Project properties
        const juce::Identifier projName      { "name" };
        const juce::Identifier projSampleRate{ "sampleRate" };

        // Track properties
        const juce::Identifier trackId       { "trackId" };
        const juce::Identifier trackName     { "name" };
        const juce::Identifier trackGain     { "gain" };
        const juce::Identifier trackPan      { "pan" };
        const juce::Identifier trackMuted    { "muted" };

        // Clip properties
        const juce::Identifier clipId             { "clipId" };
        const juce::Identifier clipName           { "name" };
        const juce::Identifier clipFilePath       { "filePath" };
        const juce::Identifier clipStartSample    { "startSample" };
        const juce::Identifier clipLengthSamples  { "lengthSamples" };
        const juce::Identifier clipSrcOffset      { "srcOffset" };
        const juce::Identifier clipGain           { "gain" };
        const juce::Identifier clipFadeInSamples  { "fadeInSamples" };
        const juce::Identifier clipFadeOutSamples { "fadeOutSamples" };
        const juce::Identifier clipMuted          { "muted" };
    }

    //==========================================================================
    // Helper: makeEmptyProject
    //==========================================================================

    ProjectModel makeEmptyProject(double sampleRate)
    {
        ProjectModel model;
        model.name = "Untitled";
        model.sampleRate = sampleRate;
        model.tracks.clear();  // Explicitly empty

        return model;
    }

    //==========================================================================
    // Conversion: ProjectModel → ValueTree
    //==========================================================================

    juce::ValueTree projectToValueTree(const ProjectModel& model)
    {
        // Create root project node
        juce::ValueTree root(ids::project);

        // Set project properties
        root.setProperty(ids::projName, model.name, nullptr);
        root.setProperty(ids::projSampleRate, model.sampleRate, nullptr);

        // Add tracks
        for (const auto& tm : model.tracks)
        {
            juce::ValueTree trackVT(ids::track);

            // Set track properties
            trackVT.setProperty(ids::trackId, tm.id, nullptr);
            trackVT.setProperty(ids::trackName, tm.name, nullptr);
            trackVT.setProperty(ids::trackGain, tm.gain, nullptr);
            trackVT.setProperty(ids::trackPan, tm.pan, nullptr);
            trackVT.setProperty(ids::trackMuted, tm.muted, nullptr);

            // Add clips to track
            for (const auto& cm : tm.clips)
            {
                juce::ValueTree clipVT(ids::audioClip);

                // Set clip properties
                clipVT.setProperty(ids::clipId, cm.id, nullptr);
                clipVT.setProperty(ids::clipName, cm.name, nullptr);
                clipVT.setProperty(ids::clipFilePath, cm.file.getFullPathName(), nullptr);
                clipVT.setProperty(ids::clipStartSample, cm.startSample, nullptr);
                clipVT.setProperty(ids::clipLengthSamples, cm.lengthSamples, nullptr);
                clipVT.setProperty(ids::clipSrcOffset, cm.srcOffset, nullptr);
                clipVT.setProperty(ids::clipGain, cm.gain, nullptr);
                clipVT.setProperty(ids::clipFadeInSamples, cm.fadeInSamples, nullptr);
                clipVT.setProperty(ids::clipFadeOutSamples, cm.fadeOutSamples, nullptr);
                clipVT.setProperty(ids::clipMuted, cm.muted, nullptr);

                trackVT.addChild(clipVT, -1, nullptr);
            }

            root.addChild(trackVT, -1, nullptr);
        }

        return root;
    }

    //==========================================================================
    // Conversion: ValueTree → ProjectModel
    //==========================================================================

    ProjectModel projectFromValueTree(const juce::ValueTree& rootVT)
    {
        // Validate root node
        if (!rootVT.isValid() || rootVT.getType() != ids::project)
        {
            // Invalid tree, return empty project
            return makeEmptyProject(48000.0);
        }

        ProjectModel model;

        // Read project properties with defaults
        model.name = rootVT.getProperty(ids::projName, "Untitled").toString();
        model.sampleRate = static_cast<double>(rootVT.getProperty(ids::projSampleRate, 48000.0));

        // Parse tracks
        for (int i = 0; i < rootVT.getNumChildren(); ++i)
        {
            juce::ValueTree trackVT = rootVT.getChild(i);

            // Skip non-track children
            if (trackVT.getType() != ids::track)
                continue;

            TrackModel tm;

            // Read track properties with defaults
            tm.id = static_cast<int>(trackVT.getProperty(ids::trackId, 0));
            tm.name = trackVT.getProperty(ids::trackName, "Track").toString();
            tm.gain = static_cast<float>(trackVT.getProperty(ids::trackGain, 1.0f));
            tm.pan = static_cast<float>(trackVT.getProperty(ids::trackPan, 0.0f));
            tm.muted = static_cast<bool>(trackVT.getProperty(ids::trackMuted, false));

            // Parse clips in this track
            for (int j = 0; j < trackVT.getNumChildren(); ++j)
            {
                juce::ValueTree clipVT = trackVT.getChild(j);

                // Skip non-clip children
                if (clipVT.getType() != ids::audioClip)
                    continue;

                ClipModel cm;

                // Read clip properties with defaults
                cm.id = static_cast<int>(clipVT.getProperty(ids::clipId, 0));
                cm.name = clipVT.getProperty(ids::clipName, "Clip").toString();

                // Parse file path
                juce::String filePath = clipVT.getProperty(ids::clipFilePath, "").toString();
                cm.file = juce::File(filePath);

                cm.startSample = static_cast<SamplePos>(
                    clipVT.getProperty(ids::clipStartSample, (juce::int64)0));
                cm.lengthSamples = static_cast<SamplePos>(
                    clipVT.getProperty(ids::clipLengthSamples, (juce::int64)0));
                cm.srcOffset = static_cast<SamplePos>(
                    clipVT.getProperty(ids::clipSrcOffset, (juce::int64)0));

                cm.gain = static_cast<float>(clipVT.getProperty(ids::clipGain, 1.0f));
                cm.fadeInSamples = static_cast<int>(
                    clipVT.getProperty(ids::clipFadeInSamples, 0));
                cm.fadeOutSamples = static_cast<int>(
                    clipVT.getProperty(ids::clipFadeOutSamples, 0));
                cm.muted = static_cast<bool>(clipVT.getProperty(ids::clipMuted, false));

                tm.clips.push_back(std::move(cm));
            }

            model.tracks.push_back(std::move(tm));
        }

        return model;
    }
}
